/*
 * Copyright (c) 2018 Juniper Networks, Inc. All rights reserved.
 */

#include "config-client-mgr/config_etcd_client.h"

#include <sandesh/request_pipeline.h>

#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "config_client_log.h"
#include "config_client_log_types.h"
#include "config_client_show_types.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/foreach.hpp>
#include <boost/functional/hash.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/uuid/uuid.hpp>
#include <map>
#include <set>
#include <string>
#include <utility>

#include "base/connection_info.h"
#include "base/logging.h"
#include "base/regex.h"
#include "base/task.h"
#include "base/task_annotations.h"
#include "base/task_trigger.h"
#include "config_cass2json_adapter.h"
#include "io/event_manager.h"
#include "config_factory.h"
#include "config_client_log.h"
#include "config_client_log_types.h"
#include "config_client_show_types.h"
#include "sandesh/common/vns_constants.h"

using contrail::regex;
using contrail::regex_match;
using contrail::regex_search;
using namespace std;
using etcd::etcdql::EtcdIf;
using etcd::etcdql::EtcdResponse;
using contrail_rapidjson::Value;
using contrail_rapidjson::Document;
using contrail_rapidjson::SizeType;
using contrail_rapidjson::StringBuffer;
using contrail_rapidjson::Writer;

bool ConfigEtcdClient::disable_watch_;

/**
  * ETCD Watcher class to enable watching for any changes
  * to config.
  * Invokes etcd::Watch() which watches the ETCD server for
  * any changes and invokes provided callback when a change
  * is detected.
  */
class ConfigEtcdClient::EtcdWatcher : public Task {
public:
    EtcdWatcher(ConfigEtcdClient *etcd_client) :
        Task(TaskScheduler::GetInstance()->GetTaskId("etcd::EtcdWatcher")),
        etcd_client_(etcd_client) {
    }

    virtual bool Run();

    ConfigEtcdClient *client() const {
        return etcd_client_;
    }
    string Description() const {
        return "ConfigEtcdClient::EtcdWatcher";
    }

private:
    ConfigEtcdClient *etcd_client_;
    void ProcessResponse(EtcdResponse resp);
};

ConfigEtcdClient::ConfigEtcdClient(ConfigClientManager *mgr,
                                   EventManager *evm,
                                   const ConfigClientOptions &options,
                                   int num_workers)
                 : ConfigDbClient(mgr, evm, options),
                   num_workers_(num_workers)
{
    eqlif_.reset(ConfigStaticObjectFactory::Create<EtcdIf>(config_db_ips(),
                                              GetFirstConfigDbPort(),
                                              false));

    InitConnectionInfo();
    bulk_sync_status_ = 0;

    for (int i = 0; i < num_workers_; i++) {
        partitions_.push_back(
            ConfigStaticObjectFactory::Create<ConfigEtcdPartition>(this, i));
    }

    uuid_reader_.reset(new
        TaskTrigger(boost::bind(&ConfigEtcdClient::UUIDReader, this),
        TaskScheduler::GetInstance()->GetTaskId("config_client::DBReader"),
        0));
}

ConfigEtcdClient::~ConfigEtcdClient() {
    STLDeleteValues(&partitions_);
}

void ConfigEtcdClient::StartWatcher() {
    if (disable_watch_)  {
        CONFIG_CLIENT_DEBUG(
            ConfigClientMgrDebug,
            "ETCD Watcher SM: StartWatcher: ETCD watch disabled");
        return;
    }

    /**
      * If reinit is triggerred, Don't start the ETCD watcher.
      */
    if (mgr()->is_reinit_triggered()) {
        CONFIG_CLIENT_DEBUG(
            ConfigClientMgrDebug,
            "ETCD Watcher SM: StartWatcher: re init triggered,"
            " don't enqueue ETCD Watcher Task.");
        return;
    }

    TaskScheduler *scheduler = TaskScheduler::GetInstance();
    Task *task = new EtcdWatcher(this);
    scheduler->Enqueue(task);
}

void ConfigEtcdClient::EtcdWatcher::ProcessResponse(
                                         EtcdResponse resp) {
    client()->ProcessResponse(resp);
}

bool ConfigEtcdClient::EtcdWatcher::Run() {
    /**
      * If reinit is triggerred, don't wait for end of config
      * trigger. Return from here to process reinit.
      */
    if (etcd_client_->mgr()->is_reinit_triggered()) {
        CONFIG_CLIENT_DEBUG(
            ConfigClientMgrDebug,
            "ETCD Watcher SM: Run: re init triggered,"
            " don't wait for end of config");
        return true;
    }

    /**
      * Invoke etcd client library to watch for changes.
      */
    client()->eqlif_->Watch("/contrail/",
            boost::bind(&ConfigEtcdClient::EtcdWatcher::ProcessResponse,
                        this, _1));

    return true;
}

void ConfigEtcdClient::ProcessResponse(EtcdResponse resp) {
    /**
      * If reinit is triggerred, don't consume the message.
      * Also, stop etcd watch.
      */
    if (mgr()->is_reinit_triggered()) {
        CONFIG_CLIENT_DEBUG(
            ConfigClientMgrDebug,
            "ETCD Watcher SM: ProcessResponse: re init triggered,"
            " stop watching");
        eqlif_->StopWatch();
        return;
    }

    /**
      * To stqrt consuming the message, we should have finished
      * bulk sync in case we started it.
      */
    mgr()->WaitForEndOfConfig();

    /**
      * Check for errors and enqueue UUID update/delete request.
      * Also update FQName cache.
      */
    assert(resp.err_code() == 0);

    if (resp.action() == 0) {
        EnqueueUUIDRequest("CREATE", resp.key(), resp.value());
    } else if (resp.action() == 1) {
        EnqueueUUIDRequest("UPDATE", resp.key(), resp.value());
    } else if (resp.action() == 2) {
        EnqueueUUIDRequest("DELETE", resp.key(), resp.value());
    }
}

void ConfigEtcdClient::InitDatabase() {
    HandleEtcdConnectionStatus(false, true);
    while (true) {
        CONFIG_CLIENT_DEBUG(ConfigClientMgrDebug, "ETCD SM: Db Init");
        if (!eqlif_->Connect()) {
            CONFIG_CLIENT_DEBUG(ConfigEtcdInitErrorMessage,
                                "Database initialization failed");
            if (!InitRetry()) return;
            continue;
        }
        break;
    }
    HandleEtcdConnectionStatus(true);
    BulkDataSync();
}

void ConfigEtcdClient::HandleEtcdConnectionStatus(bool success,
                                                  bool force_update) {
    UpdateConnectionInfo(success, force_update);

    if (success) {
        // Update connection info
        process::ConnectionState::GetInstance()->Update(
            process::ConnectionType::DATABASE, "Etcd",
            process::ConnectionStatus::UP,
            eqlif_->endpoints(), "Established ETCD connection");
       CONFIG_CLIENT_DEBUG(ConfigClientMgrDebug,
                           "ETCD SM: Established ETCD connection");
    } else {
        process::ConnectionState::GetInstance()->Update(
            process::ConnectionType::DATABASE, "Etcd",
            process::ConnectionStatus::DOWN,
            eqlif_->endpoints(), "Lost ETCD connection");
       CONFIG_CLIENT_DEBUG(ConfigClientMgrDebug,
                           "ETCD SM: Lost ETCD connection");
    }
}

bool ConfigEtcdClient::InitRetry() {
    CONFIG_CLIENT_DEBUG(ConfigClientMgrDebug, "ETCD SM: DB Init Retry");
    // If reinit is triggered, return false to abort connection attempt
    if (mgr()->is_reinit_triggered()) return false;
    usleep(GetInitRetryTimeUSec());
    return true;
}

bool ConfigEtcdClient::BulkDataSync() {
    CONFIG_CLIENT_DEBUG(ConfigClientMgrDebug,
                        "ETCD SM: BulkDataSync Started");
    bulk_sync_status_ = num_workers_;
    uuid_reader_->Set();
    return true;
}

void ConfigEtcdClient::BulkSyncDone() {
    long num_config_readers_still_processing =
        bulk_sync_status_.fetch_and_decrement();
    if (num_config_readers_still_processing == 1) {
        CONFIG_CLIENT_DEBUG(ConfigClientMgrDebug,
                            "Etcd SM: BulkSyncDone by all readers");
        mgr()->EndOfConfig();
    } else {
        CONFIG_CLIENT_DEBUG(ConfigClientMgrDebug,
                            "Etcd SM: One reader finished BulkSync");
    }
}

void ConfigEtcdClient::PostShutdown() {
    CONFIG_CLIENT_DEBUG(ConfigClientMgrDebug,
                        "ETCD SM: Post shutdown during re-init");
    STLDeleteValues(&partitions_);
    ClearFQNameCache();
}

ConfigEtcdPartition *
ConfigEtcdClient::GetPartition(const string &uuid) {
    int worker_id = HashUUID(uuid);
    return partitions_[worker_id];
}

const ConfigEtcdPartition *
ConfigEtcdClient::GetPartition(const string &uuid) const {
    int worker_id = HashUUID(uuid);
    return partitions_[worker_id];
}

const ConfigEtcdPartition *
ConfigEtcdClient::GetPartition(int worker_id) const {
    assert(worker_id < num_workers_);
    return partitions_[worker_id];
}

int ConfigEtcdClient::HashUUID(const string &uuid_str) const {
    boost::hash<string> string_hash;
    return string_hash(uuid_str) % num_workers_;
}

void ConfigEtcdClient::EnqueueUUIDRequest(string oper,
                                          string uuid,
                                          string value) {
    /**
      * uuid contains the entire config path
      * For instance /contrail/virtual_network/<uuid>
      * We form the request with the entire path so that
      * testing code can utilize it to form various
      * requests for the same uuid. However, when the
      * request is processed later by the appropriate
      * Partition, the path will be trimmed and only
      * the uuid string will be used. Same is the case
      * for the FQName cache. It will use the trimmed
      * uuid as the key since it is invoked from the
      * Partitions later.
      */

    // Get the trimmed uuid
    size_t front_pos = uuid.rfind('/');
    string uuid_key = uuid.substr(front_pos + 1);

    // Cache uses the trimmed uuid
    if (oper == "CREATE" || oper == "UPDATE") {

        Document d;
        d.Parse<0>(value.c_str());
        Document::AllocatorType &a = d.GetAllocator();

        // If non-object JSON is received, log a warning and return.
        if (!d.IsObject()) {
            CONFIG_CLIENT_WARN(ConfigClientMgrWarning, "ETCD SM: Received "
                  "non-object json. uuid: "
                  + uuid_key + " value: "
                  + value + " .Skipping");
            return;
        }

        // ETCD does not provide obj-type since it is encoded in the
        // UUID key. Since config_json_parser and IFMap need type to be
        // present in the document, fix up by adding obj-type.
        if (!d.HasMember("type")) {
            string type = uuid.substr(10, front_pos - 10);
            Value v;
            Value va;
            d.AddMember(v.SetString("type", a),
                        va.SetString(type.c_str(), a), a);
            StringBuffer sb;
            Writer<StringBuffer> writer(sb);
            d.Accept(writer);
            value = sb.GetString();
        }

        // Add to FQName cache if not present
        if (d.HasMember("type") &&
            d.HasMember("fq_name")) {
            string obj_type = d["type"].GetString();
            string fq_name;
            const Value &name = d["fq_name"];
            for (Value::ConstValueIterator name_itr = name.Begin();
                 name_itr != name.End(); ++name_itr) {
                fq_name += name_itr->GetString();
                fq_name += ":";
            }
            fq_name.erase(fq_name.end()-1);
            if (FindFQName(uuid_key) == "ERROR") {
                AddFQNameCache(uuid_key, obj_type, fq_name);
            }
        }
    } else if (oper == "DELETE") {
        // Invalidate cache
        InvalidateFQNameCache(uuid_key);
    }

    // Request has the uuid with entire path
    ObjectProcessReq *req = new ObjectProcessReq(oper, uuid, value);

    // GetPartition uses the trimmed uuid so that the same
    // partition is returned for different requests on the
    // same UUID
    GetPartition(uuid_key)->Enqueue(req);
}

void ConfigEtcdClient::EnqueueDBSyncRequest(
         const UUIDValueList &uuid_list) {
    for (UUIDValueList::const_iterator it = uuid_list.begin();
        it != uuid_list.end(); it++) {
        EnqueueUUIDRequest("CREATE", it->first, it->second);
    }
}

bool ConfigEtcdClient::UUIDReader() {

    string next_key;
    string prefix = "/contrail/";
    bool read_done = false;
    ostringstream os;

    for (ConfigClientManager::ObjectTypeList::const_iterator it =
             mgr()->config_json_parser()->ObjectTypeListToRead().begin();
         it != mgr()->config_json_parser()->ObjectTypeListToRead().end();
         it++) {

        /* Form the key for the object type to lookup */
        next_key = prefix + it->c_str();
        os.str("");
        os << next_key << 1;

        while (true) {
            unsigned int num_entries;

            /**
              * Ensure that UUIDReader task aborts on reinit trigger.
              */
            if (mgr()->is_reinit_triggered()) {
                CONFIG_CLIENT_DEBUG(ConfigClientMgrDebug,
                "ETCD SM: Abort UUID reader on reinit trigger");
                return true;
            }

            /**
              * Get number of UUIDs to read at a time
              */
            num_entries = GetNumReadRequestToBunch();

            /**
              * Read num_entries UUIDs at a time
              */
            EtcdResponse resp = eqlif_->Get(next_key,
                                            os.str(),
                                            num_entries);
            EtcdResponse::kv_map kvs = resp.kvmap();

            /**
              * Process read response
              */
            if (resp.err_code() == 0) {
                /**
                  * Got UUID data for given ObjType
                  */
                UUIDValueList uuid_list;

                for (multimap<string, string>::const_iterator iter = kvs.begin();
                     iter != kvs.end();
                     ++iter) {
                     /**
                       * Parse the json string to get uuid and value
                       */
                     next_key = iter->first;
                     if (!boost::starts_with(next_key, "/contrail/")) {
                         CONFIG_CLIENT_WARN(ConfigClientMgrWarning,
                              "ETCD SM: Non-contrail uuid: "
                              + next_key + " received");
                     } else {
                         uuid_list.push_back(make_pair(iter->first, iter->second));
                     }
                }

                /**
                  * Process the UUID response in parallel by assigning
                  * the UUIDs to partitions.
                  */
                EnqueueDBSyncRequest(uuid_list);

                /**
                  * Get the next key to read for the current ObjType
                  */
                next_key += "00";

                /**
                  * If we read less than what we sought, it means there are
                  * no more entries for current obj-type. We move to next
                  * obj-type.
                  */
                if (kvs.size() < num_entries) {
                    break;
                }
            } else if (resp.err_code() == 100)  {
                /**
                  * ObjType not found. Continue reading next ObjType
                  */
                break;
            } else if (resp.err_code() == -1) {
                /* Test ONLY */
                read_done = true;
                break;
            } else {
                /**
                  * RPC failure. Connection down.
                  * Retry after a while
                  */
                HandleEtcdConnectionStatus(false);
                usleep(GetInitRetryTimeUSec());
            }
        } //while
        if (read_done) {
            break;
        }
    } //for

    // At the end of task trigger
    BOOST_FOREACH(ConfigEtcdPartition *partition, partitions_) {
        ObjectProcessReq *req = new ObjectProcessReq("EndOfConfig", "", "");
        partition->Enqueue(req);
    }

    return true;
}

bool ConfigEtcdClient::IsTaskTriggered() const {
    // If UUIDReader task has been triggered return true.
    if (uuid_reader_->IsSet()) {
        return true;
    }

    /**
      * Walk the partitions and check if ConfigReader task has
      * been triggered in any of them. If so, return true.
      */
    BOOST_FOREACH(ConfigEtcdPartition *partition, partitions_) {
        if (partition->IsTaskTriggered()) {
            return true;
        }
    }
    return false;
}

bool ConfigEtcdClient::UUIDToObjCacheShow(
                           const string &search_string,
                           int inst_num,
                           const string &last_uuid,
                           uint32_t num_entries,
                           vector<ConfigDBUUIDCacheEntry> *entries) const {
   return GetPartition(inst_num)->UUIDToObjCacheShow(search_string, last_uuid,
                                                     num_entries, entries);
}

bool ConfigEtcdClient::IsListOrMapPropEmpty(const string &uuid_key,
                                            const string &lookup_key) {
    return GetPartition(uuid_key)->IsListOrMapPropEmpty(uuid_key, lookup_key);
}

ConfigEtcdPartition::ConfigEtcdPartition(
                   ConfigEtcdClient *client, size_t idx)
    : config_client_(client), worker_id_(idx) {
    int task_id = TaskScheduler::GetInstance()->GetTaskId("config_client::Reader");
    config_reader_.reset(new
     TaskTrigger(boost::bind(&ConfigEtcdPartition::ConfigReader, this),
     task_id, idx));
    task_id =
        TaskScheduler::GetInstance()->GetTaskId("config_client::ObjectProcessor");
    obj_process_queue_.reset(new WorkQueue<ObjectProcessReq *>(
        task_id, idx, bind(&ConfigEtcdPartition::RequestHandler, this, _1),
        WorkQueue<ObjectProcessReq *>::kMaxSize, 512));
}

ConfigEtcdPartition::~ConfigEtcdPartition() {
    obj_process_queue_->Shutdown();
}

void ConfigEtcdPartition::Enqueue(ObjectProcessReq *req) {
    obj_process_queue_->Enqueue(req);
}

bool ConfigEtcdPartition::RequestHandler(ObjectProcessReq *req) {
    AddUUIDToProcessList(req->oper_, req->uuid_str_, req->value_);
    delete req;
    return true;
}

/**
  * Add the UUID key/value pair to the process list.
  */
void ConfigEtcdPartition::AddUUIDToProcessList(const string &oper,
                                               const string &uuid_key,
                                               const string &value_str) {
    pair<UUIDProcessSet::iterator, bool> ret;
    bool trigger = uuid_process_set_.empty();

    /**
      * Get UUID from uuid_key which contains the entire path
      * and insert into uuid_process_set_
      */
    size_t front_pos = uuid_key.rfind('/');
    string uuid = uuid_key.substr(front_pos + 1);
    UUIDProcessRequestType *req =
        new UUIDProcessRequestType(oper, uuid, value_str);
    ret = uuid_process_set_.insert(make_pair(client()->GetUUID(uuid), req));
    if (ret.second) {
        /**
          * UUID not present in uuid_process_set_.
          * If uuid_process_set_ is empty, trigger config_reader_
          * to process the uuids.
          */
        if (trigger) {
            config_reader_->Set();
        }
    } else {
        /**
          * UUID already present in uuid_process_set_. If operation is
          * DELETE preceeded by CREATE, remove from uuid_process_set_.
          * For other cases (CREATE/UPDATE) replace the entry with the
          * new value and oper.
          */
        if ((oper == "DELETE") &&
            (ret.first->second->oper == "CREATE")) {
            uuid_process_set_.erase(ret.first);
            client()->PurgeFQNameCache(uuid);
        } else {
            delete req;
            ret.first->second->oper = oper;
            ret.first->second->uuid = uuid;
            ret.first->second->value = value_str;
        }
    }
}

ConfigEtcdPartition::UUIDCacheEntry::~UUIDCacheEntry() {
}

void ConfigEtcdPartition::FillUUIDToObjCacheInfo(const string &uuid,
                                  UUIDCacheMap::const_iterator uuid_iter,
                                  ConfigDBUUIDCacheEntry *entry) const {
    entry->set_uuid(uuid);
    entry->set_timestamp(
            UTCUsecToString(uuid_iter->second->GetLastReadTimeStamp()));
    entry->set_fq_name(uuid_iter->second->GetFQName());
    entry->set_obj_type(uuid_iter->second->GetObjType());
    entry->set_json_str(uuid_iter->second->GetJsonString());
}

bool ConfigEtcdPartition::UUIDToObjCacheShow(
                      const string &search_string,
                      const string &last_uuid,
                      uint32_t num_entries,
                      vector<ConfigDBUUIDCacheEntry> *entries) const {
    uint32_t count = 0;
    regex search_expr(search_string);
    for (UUIDCacheMap::const_iterator it =
        uuid_cache_map_.upper_bound(last_uuid);
        count < num_entries && it != uuid_cache_map_.end(); it++) {
        if (regex_search(it->first, search_expr) ||
                regex_search(it->second->GetObjType(), search_expr) ||
                regex_search(it->second->GetFQName(), search_expr)) {
            count++;
            ConfigDBUUIDCacheEntry entry;
            FillUUIDToObjCacheInfo(it->first, it, &entry);
            entries->push_back(entry);
        }
    }
    return true;
}

ConfigEtcdPartition::UUIDCacheEntry *
ConfigEtcdPartition::GetUUIDCacheEntry(const string &uuid) {
    UUIDCacheMap::iterator uuid_iter = uuid_cache_map_.find(uuid);
    if (uuid_iter == uuid_cache_map_.end()) {
        return NULL;
    }
    return uuid_iter->second;
}

ConfigEtcdPartition::UUIDCacheEntry *
ConfigEtcdPartition::GetUUIDCacheEntry(const string &uuid,
                                       const string &value,
                                       bool &is_new) {
    UUIDCacheMap::iterator uuid_iter = uuid_cache_map_.find(uuid);
    if (uuid_iter == uuid_cache_map_.end()) {
        /**
          * Cache entry not present. Create one.
          */
        UUIDCacheEntry *obj;
        obj = new UUIDCacheEntry(this,
                                 value,
                                 UTCTimestampUsec());
        /**
          * Insert cache entry into UUIDCacheMap.
          */
        string tmp_uuid = uuid;
        pair<UUIDCacheMap::iterator, bool> ret_uuid =
            uuid_cache_map_.insert(tmp_uuid, obj);
        assert(ret_uuid.second);
        uuid_iter = ret_uuid.first;

        /**
          * Indicate to calling function that cache has been
          * newly created.
          */
        is_new = true;
    } else {
        /**
          * Cache entry already present. Update LastReadTimestamp.
          */
        uuid_iter->second->SetLastReadTimeStamp(UTCTimestampUsec());
    }

    /**
      * Return the cache entry.
      */
    return uuid_iter->second;
}

int ConfigEtcdPartition::UUIDRetryTimeInMSec(
        const UUIDCacheEntry *obj) const {
    uint32_t retry_time_pow_of_two =
        obj->GetRetryCount() > kMaxUUIDRetryTimePowOfTwo ?
        kMaxUUIDRetryTimePowOfTwo : obj->GetRetryCount();
   return ((1 << retry_time_pow_of_two) * kMinUUIDRetryTimeMSec);
}

void ConfigEtcdPartition::UUIDCacheEntry::EnableEtcdReadRetry(
        const string uuid,
        const string value) {
    if (!retry_timer_) {
        retry_timer_ = TimerManager::CreateTimer(
                *parent_->client()->event_manager()->io_service(),
                "UUID retry timer for " + uuid,
                TaskScheduler::GetInstance()->GetTaskId(
                                "config_client::Reader"),
                parent_->worker_id_);
        CONFIG_CLIENT_DEBUG(ConfigClientReadRetry,
                "Created UUID read retry timer ", uuid);
    }
    retry_timer_->Cancel();
    retry_timer_->Start(parent_->UUIDRetryTimeInMSec(this),
            boost::bind(
                &ConfigEtcdPartition::UUIDCacheEntry::EtcdReadRetryTimerExpired,
                this, uuid, value),
            boost::bind(
                &ConfigEtcdPartition::UUIDCacheEntry::EtcdReadRetryTimerErrorHandler,
                this));
    CONFIG_CLIENT_DEBUG(ConfigClientReadRetry,
            "Start/restart UUID Read Retry timer due to configuration", uuid);
}

void ConfigEtcdPartition::UUIDCacheEntry::DisableEtcdReadRetry(
        const string uuid) {
    CHECK_CONCURRENCY("config_client::Reader");
    if (retry_timer_) {
        retry_timer_->Cancel();
        TimerManager::DeleteTimer(retry_timer_);
        retry_timer_ = NULL;
        retry_count_ = 0;
        CONFIG_CLIENT_DEBUG(ConfigClientReadRetry,
                "UUID Read retry timer - deleted timer due to configuration",
                uuid);
    }
}

bool ConfigEtcdPartition::UUIDCacheEntry::IsRetryTimerRunning() const {
    if (retry_timer_)
        return (retry_timer_->running());
    return false;
}

bool ConfigEtcdPartition::UUIDCacheEntry::EtcdReadRetryTimerExpired(
        const string uuid,
        const string value) {
    CHECK_CONCURRENCY("config_client::Reader");
    parent_->client()->EnqueueUUIDRequest(
            "UPDATE", parent_->client()->uuid_str(uuid), value);
    retry_count_++;
    CONFIG_CLIENT_DEBUG(ConfigClientReadRetry, "timer expired ", uuid);
    return false;
}

void
ConfigEtcdPartition::UUIDCacheEntry::EtcdReadRetryTimerErrorHandler() {
     std::string message = "Timer";
     CONFIG_CLIENT_WARN(ConfigClientGetRowError,
            "UUID Read Retry Timer error ", message, message);
}

bool ConfigEtcdPartition::UUIDCacheEntry::ListOrMapPropEmpty(
         const string &prop) const {
    ListMapSet::const_iterator it = list_map_set_.find(prop);
    if (it == list_map_set_.end()) {
        return true;
    }
    return (it->second == false);
}

bool ConfigEtcdPartition::GenerateAndPushJson(const string &uuid,
                                              Document &doc,
                                              bool add_change,
                                              UUIDCacheEntry *cache) {

    // Get obj_type from cache.
    const string &obj_type = cache->GetObjType();

    // string to get the type field.
    string type_str;

    // bool to indicate if an update to ifmap_server is
    // necessary.
    bool notify_update = false;

    // Walk the document, remove unwanted properties and do
    // needed fixup for the others.
    Value::ConstMemberIterator itr = doc.MemberBegin();
    Document::AllocatorType &a = doc.GetAllocator();

    while (itr != doc.MemberEnd()) {

        string key = itr->name.GetString();

        /*
         * Indicate need for update since document has
         * at least one field other than fq_name or
         * obj_type to be updated.
         */
        if (!notify_update &&
            key.compare("type") != 0 &&
            key.compare("fq_name") != 0) {
            notify_update = true;
        }

        /**
          *  Properties like perms2 has no importance to control-node/dns
          *  This property is present on each config object. Hence skipping
          *  such properties gives performance improvement.
          */
        if (ConfigClientManager::skip_properties.find(key) !=
            ConfigClientManager::skip_properties.end()) {
            itr = doc.EraseMember(itr);
            continue;
        }

        /**
          * Get the type string. This will be used as the key.
          * Also remove this field from the document.
          */
        if (key.compare("type") == 0) {
            type_str = itr->value.GetString();
            itr = doc.EraseMember(itr);
            continue;
        }

        string wrapper = client()->mgr()->config_json_parser()-> \
                           GetWrapperFieldName(obj_type, key.c_str());
        if (!wrapper.empty()) {

            /**
              * Handle prop_map and prop_list objects.
              * Need to indicate in cache if they are NULL.
              * To find if obj is prop_list or prop_map, we
              * use the WrapperFieldNames defined in schema.
              * Today they are present only for prop_list
              * and prop_map. If that changes, logic here has
              * to change to accommodate it.
              */

            // Get the propl/propm json Value
            Value &map_value = doc[key.c_str()];

            // Indicate in cache if propm/propl is empty
            cache->SetListOrMapPropEmpty(key,
                                         map_value.IsNull());

        } else if (key.compare("parent_type") == 0) {

            /**
              * Process parent_type. Need to change - to _.
              */

            string parent_type = doc[key.c_str()].GetString();
            replace(parent_type.begin(), parent_type.end(),
                   '-', '_');
            doc[key.c_str()].SetString(parent_type.c_str(), a);

        } else if (key.compare("parent_uuid") == 0) {

            /**
              * Process parent_uuid. For creates/updates, check if
              * parent fq_name is present. If not, enable retry.
              */

            if (add_change) {
                string parent_uuid = doc[key.c_str()].GetString();
                string parent_fq_name = client()->FindFQName(parent_uuid);
                if (parent_fq_name == "ERROR") {
                    CONFIG_CLIENT_DEBUG(ConfigClientReadRetry,
                        "Parent fq_name not available for ", uuid);
                    return false;
                }
            }

        } else if (key.compare("bgpaas_session_attributes") == 0) {

            /**
              * Process bgpaas_session_attributes property.
              * Value needs to be set to "".
              */

            doc[key.c_str()].SetString("", a);

        } else if (key.find("_refs") != string::npos && add_change) {

            /**
              * For _refs, if attr is NULL,
              * replace NULL with "".
              * Also add fq_name to each _ref.
              * Deletes do not need manipulation of _refs as previous
              * create/update would have already formatted them.
              */

            // Determine if NULL attr needs to be processed
            string ref_type = key.substr(0, key.length() - 5);
            bool link_with_attr =
                client()->mgr()->config_json_parser()-> \
                    IsLinkWithAttr(obj_type, ref_type);

            // Get a pointer to the _refs json Value
            Value *v = &doc[key.c_str()];

            assert(v->IsArray());
            for (SizeType i = 0; i < v->Size(); i++) {

                // Process NULL attr
                Value &va = (*v)[i];
                if (link_with_attr) {
                    if (va["attr"].IsNull()) {
                        (*v)[i].RemoveMember("attr");
                        Value vm;
                        (*v)[i].AddMember("attr", vm.SetObject(), a);
                    }
                }

                /**
                  * Add ref_fq_name to the _ref
                  * ETCD gives ref_fq_name as well but needs to be
                  * formatted as a string.
                  * Get ref_fq_name from FQNameCache if present.
                  * If not re-format the ref_fq_name present in the
                  * document as a string and insert it back.
                  */
                Value &uuidVal = va["uuid"];
                const string ref_uuid = uuidVal.GetString();
                string ref_fq_name = client()->FindFQName(ref_uuid);

                if (ref_fq_name == "ERROR") {
                    // ref_fq_name not in FQNameCache
                    // If we cannot find ref_fq_name in the doc
                    // as well, return false to enable retry.
                    if (!va.HasMember("to")) {
                        CONFIG_CLIENT_DEBUG(ConfigClientReadRetry,
                                   "Ref fq_name not available for ", uuid);
                        return false;
                    }

                    ref_fq_name.clear();
                    const Value &name = va["to"];
                    for (Value::ConstValueIterator itr = name.Begin();
                         itr != name.End(); ++itr) {
                        ref_fq_name += itr->GetString();
                        ref_fq_name += ":";
                    }
                    ref_fq_name.erase(ref_fq_name.end()-1);
                }

                // Remove ref_fq_name from doc and re-add the
                // string formatted fq_name.
                (*v)[i].RemoveMember("to");
                Value vs1(ref_fq_name.c_str(), a);
                (*v)[i].AddMember("to", vs1, a);
            }

            // For creates/updates, need to update cache json_str
            // with the new fixed ref_fq_names for _refs.
            // Remove existing reference in cache and create a new
            // ref with updated ref_fq_names.
            Document cacheDoc;
            string cache_json_str = cache->GetJsonString();
            cacheDoc.Parse<0>(cache_json_str.c_str());
            cacheDoc.RemoveMember(key.c_str());
            Value vr;
            Value vra;
            Value refVal;
            refVal.CopyFrom(*v, a);
            cacheDoc.AddMember(vr.SetString(key.c_str(), a),
                               refVal, a);
            StringBuffer sb;
            Writer<StringBuffer> writer(sb);
            cacheDoc.Accept(writer);
            string cache_str = sb.GetString();
            cache->SetJsonString(cache_str);
        }

        if (itr != doc.MemberEnd()) itr++;
    }

    if (!notify_update) {
        CONFIG_CLIENT_DEBUG(ConfigClientMgrDebug,
             "ETCD SM: Nothing to update");
        return true;
    }

    StringBuffer sb1;
    Writer<StringBuffer> writer1(sb1);
    Document refDoc;
    refDoc.CopyFrom(doc, refDoc.GetAllocator());
    refDoc.Accept(writer1);
    string refString = sb1.GetString();
    CONFIG_CLIENT_DEBUG(ConfigClientMgrDebug,
        "ETCD SM: JSON Doc fed to CJP: " + refString);

    ConfigCass2JsonAdapter ccja(uuid, type_str, doc);
    client()->mgr()->config_json_parser()->Receive(ccja, add_change);

    return true;
}

void ConfigEtcdPartition::ProcessUUIDDelete(
                                       const string &uuid_key) {

    /**
      * If FQName cache not present for the uuid, it is likely
      * a redundant delete since we remove it from the FQName
      * cache when a delete is processed. Ignore request.
      */
    if (client()->FindFQName(uuid_key) == "ERROR") {
        CONFIG_CLIENT_DEBUG(ConfigClientMgrDebug,
             "ETCD SM: Nothing to delete");
       return;
    }

    /**
      * Get the cache entry for the uuid.
      * Assert if uuid is not present in the cache.
      */
    UUIDCacheMap::iterator uuid_iter = uuid_cache_map_.find(uuid_key);
    if (uuid_iter == uuid_cache_map_.end()) {
        return;
    }
    UUIDCacheEntry *cache = uuid_iter->second;

    /**
      * If retry timer is running, the original create/update
      * for the UUID has not been processed. Stop the timer,
      * and purge the FQName cache entry.
      */
    if (cache->IsRetryTimerRunning()) {
        cache->DisableEtcdReadRetry(uuid_key);
        client()->PurgeFQNameCache(uuid_key);
        return;
    }

    /**
      * For CREATES, we could get here in erroneous
      * cases as well. For instance, fq_name or type field not
      * present in the update received. For such cases,
      * only delete cache entry.
      * Get the cached json_str and create a JSON Document.
      * This will be sent to lower layers as a delete.
      */
    const string cache_json_str = cache->GetJsonString();
    Document delDoc;
    delDoc.Parse<0>(cache_json_str.c_str());

    /**
      * Fixup the JSON document and push it down
      * to lower layers.
      */
    GenerateAndPushJson(uuid_key,
                        delDoc,
                        false,
                        cache);

    /**
      * Delete the UUIDCacheEntry.
      */
    uuid_cache_map_.erase(uuid_iter);

    /**
      * Purge FQName cache entry.
      */
    client()->PurgeFQNameCache(uuid_key);
}

void ConfigEtcdPartition::ProcessUUIDUpdate(const string &uuid_key,
                                            const string &value_str) {
    /**
      * Create UUIDCacheEntry if not present. This will create a
      * JSON document from the value_str and store in cache. The
      * cache entry is then inserted in to the UUIDCacheMap.
      * If cache entry already present, update the timestamp.
      */
    bool is_new = false;
    UUIDCacheEntry *cache = GetUUIDCacheEntry(uuid_key,
                                              value_str,
                                              is_new);

    /**
      * Get the cached json_str and create a JSON Document.
      * This will be used to compare with the updated
      * JSON value_str received.
      * As part of the update, fields in cacheDoc that are
      * present in the received update will be removed.
      * Finally, it will contain the fields that are to be
      * deleted.
      */
    string cache_json_str = cache->GetJsonString();
    if (cache_json_str.compare("retry") == 0) {
        // If we are retrying due to ref or parent
        // fq_name not available previously, cache
        // json_str would have been cleared and set
        // to retry. Process now like a new create.
        cache_json_str = value_str;
        is_new = true;
    }
    Document cacheDoc;
    cacheDoc.Parse<0>(cache_json_str.c_str());

    /**
      * Create a JSON Document from the received JSON value_str.
      * This will be updated to capture newly created and
      * updated fields. Properties that have not changed will be
      * removed from the document.
      */
    Document updDoc;
    updDoc.Parse<0>(value_str.c_str());
    string key;

    /**
      * If type or fq-name is not present in the db object, ignore
      * the object and trigger delete of the object.
      */
    if (!updDoc.HasMember("fq_name") ||
        !updDoc.HasMember("type")) {
        CONFIG_CLIENT_WARN(ConfigClientGetRowError,
             "fq_name or type not present for ",
             "obj_uuid_table with uuid: ", uuid_key);
        cache->DisableEtcdReadRetry(uuid_key);
        ProcessUUIDDelete(uuid_key);
        return;
    }

    /**
      * If cache is new, we can send the cacheDoc as is down to
      * the IFMAP server to send as update.
      * If not, we compare the cached version with the updated
      * values and potentially generate two documents.
      * 1. One that contains thew newly added fields and the
      *    fields that were updated.
      * 2. One that contains the fields that were removed in the
      *    received update.
      */
    Value::ConstMemberIterator itr = updDoc.MemberBegin();
    while (itr != updDoc.MemberEnd()) {

        key = itr->name.GetString();

        /**
          * For BOTH creates and updates.
          * Ignore if update is received in draft-mode state
          * and delete from cache.
          */
        if (key.compare("draft_mode_state") == 0) {
            string mode = itr->value.GetString();
            if (!mode.empty()) {
                client()->PurgeFQNameCache(uuid_key);
                DeleteCacheMap(uuid_key);
                return;
            }
            itr = updDoc.EraseMember(itr);
            continue;
        }

        /**
          * For new cache entries ONLY, populate fq_name and
          * obj_type fields in the ObjCache.
          * For updates/deletes, cache should already have
          * the fq_name and obj_type fields populated.
          */
        if (is_new) {
            if (key.compare("type") == 0) {
                string type = itr->value.GetString();
                cache->SetObjType(type);
            } else if (key.compare("fq_name") == 0) {
                string fq_name;
                const Value &name = updDoc[key.c_str()];
                for (Value::ConstValueIterator name_itr = name.Begin();
                     name_itr != name.End(); ++name_itr) {
                    fq_name += name_itr->GetString();
                    fq_name += ":";
                }
                fq_name.erase(fq_name.end()-1);
                cache->SetFQName(fq_name);
            }
        }

        /**
          * For updates ONLY.
          * If cache has the field and if it has not been updated
          * remove from updDoc. Also, remove the field
          * from cacheDoc. Skip fq_name and obj_type.

          */
        if (!is_new && cacheDoc.HasMember(key.c_str()) &&
            key.compare("type") != 0 &&
            key.compare("fq_name") != 0) {
            if (cacheDoc[key.c_str()] == updDoc[key.c_str()]) {
                itr = updDoc.EraseMember(itr);
            }
            assert(cacheDoc.RemoveMember(key.c_str()));
        } else {
            if (itr != updDoc.MemberEnd()) itr++;
        }
    }

    /**
      * Now that the updates are ready, replace the json_str in
      * the cache with the updated value_str.
      */
    cache->SetJsonString(value_str);

    /**
      * Fixup the JSON document and push it down
      * to lower layers.
      * Send the creates and updates followed
      * by the deleted fields.
      * Deletes are only sent for UPDATES as there
      * may be properties that were removed in the
      * update.
      * For CREATES, there is nothing to delete
      * as they are just newly added to cache.
      * When adding/updating properties, if there is
      * an error, retry after a while.
      */
    if (!GenerateAndPushJson(uuid_key,
                        updDoc,
                        true,
                        cache)) {
        cache->EnableEtcdReadRetry(uuid_key, value_str);
        cache->SetJsonString("retry");
    } else {
        cache->DisableEtcdReadRetry(uuid_key);
    }
    if (!is_new) {
        GenerateAndPushJson(uuid_key,
                            cacheDoc,
                            false,
                            cache);
    }
}

bool ConfigEtcdPartition::IsListOrMapPropEmpty(const string &uuid_key,
                                          const string &lookup_key) {
    UUIDCacheMap::iterator uuid_iter = uuid_cache_map_.find(uuid_key);
    if (uuid_iter == uuid_cache_map_.end()) {
        return true;
    }
    UUIDCacheEntry *cache = uuid_iter->second;

    return cache->ListOrMapPropEmpty(lookup_key);
}

bool ConfigEtcdPartition::IsTaskTriggered() const {
    return (config_reader_->IsSet());
}

bool ConfigEtcdPartition::ConfigReader() {
    CHECK_CONCURRENCY("config_client::Reader");

    int num_req_handled = 0;

    /**
      * Walk through the requests in uuid_process_set_ and process them.
      * uuid_process_set_ contains the response from ETCD with the
      * uuid key-value pairs.
      * Config reader task should stop on reinit trigger
      */
    for (UUIDProcessSet::iterator it = uuid_process_set_.begin(), itnext;
         it != uuid_process_set_.end() &&
                 !client()->mgr()->is_reinit_triggered();
         it = itnext) {

        itnext = it;
        ++itnext;

        UUIDProcessRequestType *obj_req = it->second;

        if (obj_req->oper == "CREATE" || obj_req->oper == "UPDATE") {
            ProcessUUIDUpdate(obj_req->uuid, obj_req->value);
        } else if (obj_req->oper == "DELETE") {
            ProcessUUIDDelete(obj_req->uuid);
        } else if (obj_req->oper == "EndOfConfig") {
            client()->BulkSyncDone();
        }
        RemoveObjReqEntry(obj_req->uuid);

        /**
          * Max. UUIDs to be processed in one config reader task
          * exectution is bound by kMaxRequestsToYield
          */
        if (++num_req_handled == client()->GetMaxRequestsToYield()) {
            return false;
        }
    }

    // Clear the UUID read set if we are currently processing reinit request
    if (client()->mgr()->is_reinit_triggered()) {
        CONFIG_CLIENT_DEBUG(ConfigClientMgrDebug,
            "ETCD SM: Clear UUID process set due to reinit");
        uuid_process_set_.clear();
    }
    assert(uuid_process_set_.empty());
    return true;
}

void ConfigEtcdPartition::RemoveObjReqEntry(string &uuid) {
    UUIDProcessSet::iterator req_it =
        uuid_process_set_.find(client()->GetUUID(uuid));
    delete req_it->second;
    uuid_process_set_.erase(req_it);
}
