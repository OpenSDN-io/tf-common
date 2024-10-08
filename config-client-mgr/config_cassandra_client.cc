/*
 * Copyright (c) 2016 Juniper Networks, Inc. All rights reserved.
 */

#include "config-client-mgr/config_cassandra_client.h"

#include <sandesh/request_pipeline.h>

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
#include <vector>

#include "base/connection_info.h"
#include "base/logging.h"
#include "base/regex.h"
#include "base/task.h"
#include "base/task_annotations.h"
#include "base/task_trigger.h"
#include "config_cass2json_adapter.h"
#include "io/event_manager.h"
#include "database/cassandra/cql/cql_if.h"
#include "config_factory.h"
#include "config_client_log.h"
#include "config_client_log_types.h"
#include "config_client_show_types.h"
#include "sandesh/common/vns_constants.h"

using contrail::regex;
using contrail::regex_match;
using contrail::regex_search;
using std::unique_ptr;
using std::multimap;
using std::set;
using std::string;

const string ConfigCassandraClient::kUuidTableName = "obj_uuid_table";
const string ConfigCassandraClient::kFqnTableName = "obj_fq_name_table";
const string ConfigCassandraClient::kCassClientTaskId = "config_client::Reader";
const string ConfigCassandraClient::kObjectProcessTaskId =
                                               "config_client::ObjectProcessor";

ConfigCassandraClient::ConfigCassandraClient(ConfigClientManager *mgr,
                         EventManager *evm, const ConfigClientOptions &options,
                         int num_workers)
        : ConfigDbClient(mgr, evm, options), num_workers_(num_workers) {
    dbif_.reset(ConfigStaticObjectFactory::CreateRef<cass::cql::CqlIf>(
             evm, config_db_ips(),
             GetFirstConfigDbPort(), config_db_user(),
             config_db_password(),
             static_cast<bool>(options.config_db_use_ssl), options.config_db_ca_certs));

    // Initialized the casssadra connection status;
    InitConnectionInfo();
    bulk_sync_status_ = 0;

    for (int i = 0; i < num_workers_; i++) {
        partitions_.push_back(
                ConfigStaticObjectFactory::Create<ConfigCassandraPartition>
                    (this, static_cast<size_t>(i)));
    }

    fq_name_reader_.reset(new
       TaskTrigger(boost::bind(&ConfigCassandraClient::FQNameReader, this),
       TaskScheduler::GetInstance()->GetTaskId("config_client::DBReader"),
       0));
}

ConfigCassandraClient::~ConfigCassandraClient() {
    if (dbif_) {
        // dbif_->Db_Uninit(....);
    }

    STLDeleteValues(&partitions_);
}

void ConfigCassandraClient::InitDatabase() {
    HandleCassandraConnectionStatus(false, true);
    while (true) {
        CONFIG_CLIENT_DEBUG(ConfigClientMgrDebug, "Cassandra SM: Db Init");
        if (!dbif_->Db_Init()) {
            CONFIG_CLIENT_DEBUG(ConfigCassInitErrorMessage,
                                     "Database initialization failed");
            if (!InitRetry()) return;
            continue;
        }
        CONFIG_CLIENT_DEBUG(ConfigClientMgrDebug,
                            "Cassandra SM: Db SetTableSpace");
        if (!dbif_->Db_SetTablespace(
                g_vns_constants.API_SERVER_KEYSPACE_NAME)) {
            CONFIG_CLIENT_DEBUG(ConfigCassInitErrorMessage,
                                     "Setting database keyspace failed");
            if (!InitRetry()) return;
            continue;
        }
        CONFIG_CLIENT_DEBUG(ConfigClientMgrDebug,
                            "Cassandra SM: Db UseColumnFamily uuidTableName");
        if (!dbif_->Db_UseColumnfamily(kUuidTableName)) {
            if (!InitRetry()) return;
            continue;
        }
        CONFIG_CLIENT_DEBUG(ConfigClientMgrDebug,
                            "Cassandra SM: Db UseColumnFamily fqnTableName");
        if (!dbif_->Db_UseColumnfamily(kFqnTableName)) {
            if (!InitRetry()) return;
            continue;
        }
        break;
    }
    HandleCassandraConnectionStatus(true);
    BulkDataSync();
}

bool ConfigCassandraClient::InitRetry() {
    CONFIG_CLIENT_DEBUG(ConfigClientMgrDebug, "Cassandra SM: DB uninit");
    dbif_->Db_Uninit();
    // If reinit is triggered, return false to abort connection attempt
    if (mgr()->is_reinit_triggered()) return false;
    usleep(GetInitRetryTimeUSec());
    return true;
}

ConfigCassandraPartition *
ConfigCassandraClient::GetPartition(const string &uuid) {
    int worker_id = HashUUID(uuid);
    return partitions_[worker_id];
}

const ConfigCassandraPartition *
ConfigCassandraClient::GetPartition(const string &uuid) const {
    int worker_id = HashUUID(uuid);
    return partitions_[worker_id];
}

const ConfigCassandraPartition *
ConfigCassandraClient::GetPartition(int worker_id) const {
    assert(worker_id < num_workers_);
    return partitions_[worker_id];
}

int ConfigCassandraClient::HashUUID(const string &uuid_str) const {
    boost::hash<string> string_hash;
    return string_hash(uuid_str) % num_workers_;
}

bool ConfigCassandraPartition::ReadObjUUIDTable(const set<string> &req_list) {
    GenDb::ColListVec col_list_vec;

    set<string> uuid_list = req_list;
    vector<GenDb::DbDataValueVec> keys;
    for (set<string>::const_iterator it = uuid_list.begin();
         it != uuid_list.end(); it++) {
        GenDb::DbDataValueVec key;
        key.push_back(GenDb::Blob(reinterpret_cast<const uint8_t *>
                                  (it->c_str()), it->size()));
        keys.push_back(key);
    }

    GenDb::Blob col_filter(reinterpret_cast<const uint8_t *>("d"), 1);
    GenDb::ColumnNameRange crange;
    crange.start_ =
      boost::assign::list_of(GenDb::DbDataValue(col_filter)).convert_to_container<GenDb::DbDataValueVec>();

    GenDb::FieldNamesToReadVec field_vec;
    field_vec.push_back(boost::make_tuple("key", true, false, false));
    field_vec.push_back(boost::make_tuple("column1", false, true, false));
    field_vec.push_back(boost::make_tuple("value", false, false, true));

    if (client()->dbif_->Db_GetMultiRow(&col_list_vec,
                              ConfigCassandraClient::kUuidTableName, keys,
                              crange, field_vec,
                              GenDb::DbConsistency::QUORUM)) {
        client()->HandleCassandraConnectionStatus(true);
        BOOST_FOREACH(const GenDb::ColList &col_list, col_list_vec) {
            assert(col_list.rowkey_.size() == 1);
            assert(col_list.rowkey_[0].which() == GenDb::DB_VALUE_BLOB);
            if (col_list.columns_.size()) {
                GenDb::Blob uuid(boost::get<GenDb::Blob>(col_list.rowkey_[0]));
                string uuid_str(reinterpret_cast<const char *>(uuid.data()),
                                uuid.size());
                ProcessObjUUIDTableEntry(uuid_str, col_list);
                uuid_list.erase(uuid_str);
            }
        }
    } else {
        // Failure is returned due to connectivity issue or consistency
        // issues in reading from cassandra
        client()->HandleCassandraConnectionStatus(false);
        CONFIG_CLIENT_WARN(ConfigClientGetRowError,
                     "GetMultiRow failed for table",
                     ConfigCassandraClient::kUuidTableName, "");
        //
        // Task is rescheduled to read the request queue
        // Due to a bug CQL driver from datastax, connection status is
        // not notified asynchronously. Because of this, polling is the only
        // choice to determine the cql connection status.
        // Since there are dedicated threads to read config,
        // and it is ok to retry by rescheduling the reader task
        // TODO: Sleep or No Sleep?
        //
        return false;
    }

    // Delete all stale entries from the data base.
    BOOST_FOREACH(string uuid_key, uuid_list) {
        CONFIG_CLIENT_WARN(ConfigClientGetRowError, "Missing row in the table",
                            ConfigCassandraClient::kUuidTableName, uuid_key);
        HandleObjectDelete(uuid_key, false);
    }

    // Clear the uuid list.
    uuid_list.clear();
    return true;
}

// Notes on list map property processing:
// Separate entries per list/map keys are stored in the ObjUuidCache partition
// based on the uuid.
// The cache map entries contain a refreshed bit and timestamp in addition to
// the values etc.
// A set containing list/map property names (updated_list_map_properties) that
// have key/value pairs with a new timestamp, a second set
// (candidate_list_map_properties) also containing list/map property names that
// may require an update given some key/value pairs have been deleted, and a
// multimap (list_map_properties) for all the  list/map key value pairs in the
// new configuration, are build and held in the context(temporary).
// These lists are used to determine which list/map properties need to be pushed
// to the backend, they are built as columns are parsed. Once all columns are
// parsed,
// in ListMapPropReviseUpdateList, for each property name in
// candidate_list_map_properties we check it is already in the
// updated_list_map_property list, if not we proceed to find at least one stale
// list/map key value pair with the property name in the ObJUuidCache, if one is
// found that requires an update, the property name is added to
// updated_list_map_properties.
// Once updated_list_map_properties is revised, we iterate through each property
// name in it and push all matching key/value pairs in list_map_properties.
//  ConfigCass2JsonAdapter groups the key value pairs belonging to the same
//  property so that a single DB request is sent to the
//  backend.
//  Deletes are handled by FormDeleteRequestList. Note that deletes are sent
//  only when all key/value pairs for a given list/map property are removed.
//  Additionally, the resulting DB request only resets the property_set bit, it
//  does not clear the entries in the backend.
//
//  parent_or_ref_fq_name_unknown indicates that at least one parent or
//  ref cannot be found in the FQNameCache, this can happen if the parent or
//  referred object is not yet read.
struct ConfigCassandraParseContext {
    ConfigCassandraParseContext() : obj_type(""), fq_name_present(false),
        ignore_object(false), parent_or_ref_fq_name_unknown(false) {
    }
    std::multimap<string, JsonAdapterDataType> list_map_properties;
    set<string> updated_list_map_properties;
    set<string> candidate_list_map_properties;
    string obj_type;
    string fq_name;
    bool fq_name_present;
    bool ignore_object;
    bool parent_or_ref_fq_name_unknown;

private:
    DISALLOW_COPY_AND_ASSIGN(ConfigCassandraParseContext);
};

bool ConfigCassandraPartition::ProcessObjUUIDTableEntry(const string &uuid_key,
                                           const GenDb::ColList &col_list) {
    CassColumnKVVec cass_data_vec;

    ConfigCassandraParseContext context;

    ConfigCassandraPartition::ObjCacheEntry *obj = MarkCacheDirty(uuid_key);

    ParseObjUUIDTableEntry(uuid_key, col_list, &cass_data_vec, context);
    // Ignore draft objects.
    if (context.ignore_object) {
        client()->PurgeFQNameCache(uuid_key);
        DeleteCacheMap(uuid_key);
        return false;
    }
    // If type or fq-name is not present in the db object, ignore the object
    // and trigger delete of the object.
    if (context.obj_type.empty() || !context.fq_name_present) {
        // Handle as delete
        CONFIG_CLIENT_WARN(ConfigClientGetRowError,
             "Parsing row response for type/fq_name failed for table",
             ConfigCassandraClient::kUuidTableName, uuid_key);
        obj->DisableCassandraReadRetry(uuid_key);
        HandleObjectDelete(uuid_key, false);
        return false;
    }

    obj->SetFQName(context.fq_name);
    obj->SetObjType(context.obj_type);

    if (context.parent_or_ref_fq_name_unknown) {
        obj->EnableCassandraReadRetry(uuid_key);
    } else {
        obj->DisableCassandraReadRetry(uuid_key);
    }

    ListMapPropReviseUpdateList(uuid_key, context);

    // Read the context for map and list properties
    if (context.updated_list_map_properties.size()) {
        for (set<string>::iterator it =
             context.updated_list_map_properties.begin();
             it != context.updated_list_map_properties.end(); it++) {
            pair<multimap<string, JsonAdapterDataType>::iterator,
                multimap<string, JsonAdapterDataType>::iterator> ret =
                context.list_map_properties.equal_range(*it);
            for (multimap<string, JsonAdapterDataType>::iterator mit =
                 ret.first; mit != ret.second; mit++) {
                cass_data_vec.push_back(mit->second);
            }
        }
    }
    GenerateAndPushJson(uuid_key, context.obj_type, cass_data_vec, true);
    HandleObjectDelete(uuid_key, true);
    return true;
}

void ConfigCassandraPartition::ParseObjUUIDTableEntry(const string &uuid,
        const GenDb::ColList &col_list, CassColumnKVVec *cass_data_vec,
        ConfigCassandraParseContext &context) {
    BOOST_FOREACH(const GenDb::NewCol &ncol, col_list.columns_) {
        assert(ncol.name->size() == 1);
        assert(ncol.value->size() == 1);
        assert(ncol.timestamp->size() == 1);

        const GenDb::DbDataValue &dname(ncol.name->at(0));
        assert(dname.which() == GenDb::DB_VALUE_BLOB);
        GenDb::Blob dname_blob(boost::get<GenDb::Blob>(dname));
        string key(reinterpret_cast<const char *>(dname_blob.data()),
                   dname_blob.size());

        const GenDb::DbDataValue &dvalue(ncol.value->at(0));
        assert(dvalue.which() == GenDb::DB_VALUE_STRING);
        string value(boost::get<string>(dvalue));

        const GenDb::DbDataValue &dtimestamp(ncol.timestamp->at(0));
        assert(dtimestamp.which() == GenDb::DB_VALUE_UINT64);
        uint64_t timestamp = boost::get<uint64_t>(dtimestamp);
        ParseObjUUIDTableEachColumnBuildContext(uuid, key, value, timestamp,
                                             cass_data_vec, context);
    }
}

void ConfigCassandraPartition::ParseObjUUIDTableEachColumnBuildContext(
                     const string &uuid, const string &key, const string &value,
                     uint64_t timestamp, CassColumnKVVec *cass_data_vec,
                     ConfigCassandraParseContext &context) {
    // Check whether there was an update to property of ref
    JsonAdapterDataType adapter(key, value);
    if (StoreKeyIfUpdated(uuid, &adapter, timestamp, context)) {
        // Field is updated.. enqueue to parsing
        cass_data_vec->push_back(adapter);
    }
}

void ConfigCassandraPartition::GenerateAndPushJson(
    const string &uuid_key, const string &obj_type,
    const CassColumnKVVec &cass_data_vec, bool add_change) {

    ConfigCass2JsonAdapter ccja(uuid_key, client(), obj_type,
                                cass_data_vec);
    client()->mgr()->config_json_parser()->Receive(ccja, add_change);
}

// Post shutdown during reinit, cleanup all previous states and connections
// 1. Disconnect from cassandra cluster
// 2. Clean FQ Name cache
// 3. Delete partitions which inturn will clear up the object cache and
// previously enqueued uuid read requests
void ConfigCassandraClient::PostShutdown() {
    CONFIG_CLIENT_DEBUG(ConfigClientMgrDebug,
                        "Cassandra SM: Post shutdown during re init");
    CONFIG_CLIENT_DEBUG(ConfigClientMgrDebug, "Cassandra SM: Db Uninit");
    dbif_->Db_Uninit();
    STLDeleteValues(&partitions_);
    ClearFQNameCache();
}

bool ConfigCassandraClient::BulkDataSync() {
    CONFIG_CLIENT_DEBUG(
        ConfigClientMgrDebug, "Cassandra SM: BulkDataSync Started");
    bulk_sync_status_ = num_workers_;
    fq_name_reader_->Set();
    return true;
}

bool ConfigCassandraClient::IsTaskTriggered() const {
    // If FQNameReader task has been triggered return true.
    if (fq_name_reader_->IsSet()) {
        return true;
    }

    /**
      * Walk the partitions and check if ConfigReader task has
      * been triggered in any of them. If so, return true.
      */
    BOOST_FOREACH(ConfigCassandraPartition *partition, partitions_) {
        if (partition->IsTaskTriggered()) {
            return true;
        }
    }
    return false;
}

bool ConfigCassandraClient::FQNameReader() {
    for (ConfigClientManager::ObjectTypeList::const_iterator it =
         mgr()->config_json_parser()->ObjectTypeListToRead().begin();
         it != mgr()->config_json_parser()->ObjectTypeListToRead().end();
         it++) {
        string column_name;
        while (true) {
            // Ensure that FQName reader task aborts on reinit trigger.
            if (mgr()->is_reinit_triggered()) {
                CONFIG_CLIENT_DEBUG(ConfigClientMgrDebug,
                        "Cassandra SM: Abort FQName reader on reinit trigger");
                return true;
            }

            // Rowkey is obj-type
            GenDb::DbDataValueVec key;
            key.push_back(GenDb::Blob(reinterpret_cast<const uint8_t *>
                                      (it->c_str()), it->size()));
            GenDb::ColumnNameRange crange;
            if (!column_name.empty()) {
                GenDb::Blob col_filter(reinterpret_cast<const uint8_t *>
                                   (column_name.c_str()), column_name.size());
                // Start reading the next set of entries from where we ended in
                // last read
                crange.start_ =
                    boost::assign::list_of(
                       GenDb::DbDataValue(col_filter)).convert_to_container
                             <GenDb::DbDataValueVec>();
                crange.start_op_ = GenDb::Op::GT;
            }

            // In large scale scenarios, each object type may have a large
            // number of uuid entries. Read a fixed number of entries at a time
            // to avoid cpu hogging by this thread.
            crange.count_ = GetFQNameEntriesToRead();

            GenDb::FieldNamesToReadVec field_vec;
            field_vec.push_back(boost::make_tuple("key", true, false, false));
            field_vec.push_back(boost::make_tuple("column1", false, true,
                                                  false));

            GenDb::ColList col_list;
            if (dbif_->Db_GetRow(&col_list, kFqnTableName, key,
                     GenDb::DbConsistency::QUORUM, crange, field_vec)) {
                HandleCassandraConnectionStatus(true);

                // No entries for this obj-type
                if (!col_list.columns_.size())
                    break;

                ObjTypeUUIDList uuid_list;
                ParseFQNameRowGetUUIDList(*it, col_list, uuid_list,
                                          &column_name);
                EnqueueDBSyncRequest(uuid_list);

                // If we read less than what we sought, it means there are
                // no more entries for current obj-type. We move to next
                // obj-type.
                if (col_list.columns_.size() < GetFQNameEntriesToRead())
                    break;
            } else {
                HandleCassandraConnectionStatus(false);
                CONFIG_CLIENT_WARN(ConfigClientGetRowError,
                        "GetRow failed for table", kFqnTableName, *it);
                usleep(GetInitRetryTimeUSec());
            }
        }
    }
    // At the end of task trigger
    BOOST_FOREACH(ConfigCassandraPartition *partition, partitions_) {
        ObjectProcessReq *req = new ObjectProcessReq("EndOfConfig", "", "");
        partition->Enqueue(req);
    }

    return true;
}

bool ConfigCassandraClient::ParseFQNameRowGetUUIDList(const string &obj_type,
                  const GenDb::ColList &col_list, ObjTypeUUIDList &uuid_list,
                  string *last_column) {
    string column_name;
    BOOST_FOREACH(const GenDb::NewCol &ncol, col_list.columns_) {
        assert(ncol.name->size() == 1);
        const GenDb::DbDataValue &dname(ncol.name->at(0));
        assert(dname.which() == GenDb::DB_VALUE_BLOB);
        GenDb::Blob dname_blob(boost::get<GenDb::Blob>(dname));
        column_name = string(reinterpret_cast<const char *>(dname_blob.data()),
                   dname_blob.size());
        UpdateFQNameCache(column_name, obj_type, uuid_list);
    }

    *last_column = column_name;
    return true;
}

void ConfigCassandraClient::UpdateFQNameCache(const string &key,
        const string &obj_type, ObjTypeUUIDList &uuid_list) {
    string uuid_str = FetchUUIDFromFQNameEntry(key);
    if (uuid_str.empty())
        return;
    uuid_list.push_back(make_pair(obj_type, uuid_str));
    AddFQNameCache(uuid_str, obj_type, key.substr(0, key.rfind(':')));
}

string ConfigCassandraClient::FetchUUIDFromFQNameEntry(
        const string &key) const {
    size_t temp = key.rfind(':');
    return (temp == string::npos) ? "" : key.substr(temp+1);
}

bool ConfigCassandraClient::EnqueueDBSyncRequest(
        const ObjTypeUUIDList &uuid_list) {
    for (ObjTypeUUIDList::const_iterator it = uuid_list.begin();
         it != uuid_list.end(); it++) {
        EnqueueUUIDRequest("CREATE", it->first, it->second);
    }
    return true;
}

bool ConfigCassandraClient::UUIDToObjCacheShow(
    const string &search_string, int inst_num, const string &last_uuid,
    uint32_t num_entries, vector<ConfigDBUUIDCacheEntry> *entries) const {
    return GetPartition(inst_num)->UUIDToObjCacheShow(search_string, last_uuid,
                                                      num_entries, entries);
}

void ConfigCassandraClient::EnqueueUUIDRequest(string oper, string obj_type,
                                               string uuid_str) {
    ObjectProcessReq *req = new ObjectProcessReq(oper, uuid_str, obj_type);
    GetPartition(uuid_str)->Enqueue(req);
}

void ConfigCassandraClient::BulkSyncDone() {
    long num_config_readers_still_processing =
        bulk_sync_status_.fetch_and_decrement();
    if (num_config_readers_still_processing == 1) {
        CONFIG_CLIENT_DEBUG(ConfigClientMgrDebug,
                            "Cassandra SM: BulkSyncDone by all readers");
        mgr()->EndOfConfig();
    } else {
        CONFIG_CLIENT_DEBUG(ConfigClientMgrDebug,
                            "Cassandra SM: One reader finished BulkSync");
    }
}

void ConfigCassandraClient::HandleCassandraConnectionStatus(bool success,
                                                            bool force_update) {
    UpdateConnectionInfo(success, force_update);

    if (success) {
        // Update connection info
        process::ConnectionState::GetInstance()->Update(
            process::ConnectionType::DATABASE, "Cassandra",
            process::ConnectionStatus::UP,
            dbif_->Db_GetEndpoints(), "Established Cassandra connection");
        CONFIG_CLIENT_DEBUG(ConfigClientMgrDebug,
                            "Cassandra SM: Established Cassandra connection");
    } else {
        process::ConnectionState::GetInstance()->Update(
            process::ConnectionType::DATABASE, "Cassandra",
            process::ConnectionStatus::DOWN,
            dbif_->Db_GetEndpoints(), "Lost Cassandra connection");
        CONFIG_CLIENT_DEBUG(ConfigClientMgrDebug,
                            "Cassandra SM: Lost Cassandra connection");
    }
}

bool ConfigCassandraClient::IsListOrMapPropEmpty(const string &uuid_key,
      const string &lookup_key) {
    return GetPartition(uuid_key)->IsListOrMapPropEmpty(uuid_key, lookup_key);
}

ConfigCassandraPartition::ConfigCassandraPartition(
                   ConfigCassandraClient *client, size_t idx)
    : config_client_(client), worker_id_(idx) {
    int task_id = TaskScheduler::GetInstance()->GetTaskId("config_client::Reader");
    config_reader_.reset(new
     TaskTrigger(boost::bind(&ConfigCassandraPartition::ConfigReader, this),
     task_id, idx));
    task_id =
        TaskScheduler::GetInstance()->GetTaskId("config_client::ObjectProcessor");
    obj_process_queue_.reset(new WorkQueue<ObjectProcessReq *>(
        task_id, idx, bind(&ConfigCassandraPartition::RequestHandler, this, _1),
        WorkQueue<ObjectProcessReq *>::kMaxSize, 512));
}

ConfigCassandraPartition::~ConfigCassandraPartition() {
    obj_process_queue_->Shutdown();
}

void ConfigCassandraPartition::Enqueue(ObjectProcessReq *req) {
    obj_process_queue_->Enqueue(req);
}

bool ConfigCassandraPartition::RequestHandler(ObjectProcessReq *req) {
    AddUUIDToRequestList(req->oper_, req->value_, req->uuid_str_);
    delete req;
    return true;
}

void ConfigCassandraPartition::AddUUIDToRequestList(const string &oper,
                                                 const string &obj_type,
                                                 const string &uuid_str) {
    pair<UUIDProcessSet::iterator, bool> ret;
    bool trigger = uuid_read_set_.empty();
    ObjectProcessRequestType *req =
        new ObjectProcessRequestType(oper, obj_type, uuid_str);
    ret = uuid_read_set_.insert(make_pair(client()->GetUUID(uuid_str), req));
    if (ret.second) {
        if (trigger) {
            config_reader_->Set();
        }
    } else {
        delete req;
        ret.first->second->oper = oper;
        ret.first->second->uuid = uuid_str;
    }
}

void ConfigCassandraPartition::HandleObjectDelete(
                        const string &uuid, bool add_change) {
    if (!add_change) {
        ConfigCassandraClient::ObjTypeFQNPair obj_type_fq_name_pair =
            client()->UUIDToFQName(uuid, true);
        if (obj_type_fq_name_pair.second == "ERROR") {
            return;
        }
    }

    bool needNotify = false;
    std::string obj_type("");
    ObjectCacheMap::iterator uuid_iter = object_cache_map_.find(uuid);
    if (uuid_iter == object_cache_map_.end()) {
        assert(!add_change);
        return;
    }

    CassColumnKVVec cass_data_vec;
    for (FieldDetailMap::iterator it =
         uuid_iter->second->GetFieldDetailMap().begin(), itnext;
         it != uuid_iter->second->GetFieldDetailMap().end();
         it = itnext) {
        itnext = it;
        ++itnext;
        if (it->first.key == "type") {
            obj_type = it->first.value;
            obj_type.erase(remove(obj_type.begin(),
                      obj_type.end(), '\"'), obj_type.end());
            cass_data_vec.push_back(it->first);
        }

        if (it->first.key == "fq_name") {
            cass_data_vec.push_back(it->first);
        }

        if (!add_change || !it->second.refreshed) {
            if (it->first.key == "type" || it->first.key == "fq_name") {
                continue;
            }

            needNotify = true;
            cass_data_vec.push_back(it->first);
            if (add_change) {
                uuid_iter->second->GetFieldDetailMap().erase(it);
            }
        }
    }

    if (add_change != true) {
        object_cache_map_.erase(uuid_iter);
    }
    if (needNotify) {
        GenerateAndPushJson(uuid, obj_type, cass_data_vec, false);
    }
    if (!add_change) {
        client()->PurgeFQNameCache(uuid);
    }
}

bool ConfigCassandraPartition::IsListOrMapPropEmpty(const string &uuid_key,
      const string &lookup_key) {
    string key;
    ObjectCacheMap::iterator uuid_iter = object_cache_map_.find(uuid_key);
    if (uuid_iter == object_cache_map_.end()) {
        return true;
    }

    key = "propm:" + lookup_key;
    FieldDetailMap::iterator lower_bound_it =
        uuid_iter->second->GetFieldDetailMap().lower_bound(
                                          JsonAdapterDataType(key, ""));
    if (lower_bound_it != uuid_iter->second->GetFieldDetailMap().end() &&
        boost::starts_with(lower_bound_it->first.key, key)) {
        return false;
    }
    key = "propl:" + lookup_key;
    lower_bound_it =
        uuid_iter->second->GetFieldDetailMap().lower_bound(
                                          JsonAdapterDataType(key, ""));
    if (lower_bound_it != uuid_iter->second->GetFieldDetailMap().end() &&
        boost::starts_with(lower_bound_it->first.key, key)) {
        return false;
    }
    return true;
}

bool ConfigCassandraPartition::IsTaskTriggered() const {
    return (config_reader_->IsSet());
}

bool ConfigCassandraPartition::ConfigReader() {
    CHECK_CONCURRENCY("config_client::Reader");

    set<string> bunch_req_list;
    int num_req_handled = 0;
    // Config reader task should stop on reinit trigger
    for (UUIDProcessSet::iterator it = uuid_read_set_.begin(), itnext;
         it != uuid_read_set_.end() && !client()->mgr()->is_reinit_triggered();
         it = itnext) {
        itnext = it;
        ++itnext;
        ObjectProcessRequestType *obj_req = it->second;

        if (obj_req->oper == "CREATE" || obj_req->oper == "UPDATE" ||
                obj_req->oper == "UPDATE-IMPLICIT") {
            bunch_req_list.insert(obj_req->uuid);
            bool is_last = (itnext == uuid_read_set_.end());
            if (is_last ||
                bunch_req_list.size() == client()->GetNumReadRequestToBunch()) {
                if (!ReadObjUUIDTable(bunch_req_list)) {
                    return false;
                }
                num_req_handled += bunch_req_list.size();
                RemoveObjReqEntries(bunch_req_list);
                if (num_req_handled >= client()->GetMaxRequestsToYield()) {
                    return false;
                }
            }
            continue;
        } else if (obj_req->oper == "DELETE") {
            HandleObjectDelete(obj_req->uuid, false);
        } else if (obj_req->oper == "EndOfConfig") {
            client()->BulkSyncDone();
        }
        RemoveObjReqEntry(obj_req->uuid);
        if (++num_req_handled == client()->GetMaxRequestsToYield()) {
            return false;
        }
    }

    // No need to read the object uuid table if reinit is triggered
    if (!bunch_req_list.empty() && !client()->mgr()->is_reinit_triggered()) {
        if (!ReadObjUUIDTable(bunch_req_list))
            return false;
        RemoveObjReqEntries(bunch_req_list);
    }
    // Clear the UUID read set if we are currently processing reinit request
    if (client()->mgr()->is_reinit_triggered()) {
        CONFIG_CLIENT_DEBUG(ConfigClientMgrDebug,
            "Cassandra SM: Clear UUID read set due to reinit");
        uuid_read_set_.clear();
    }
    assert(uuid_read_set_.empty());
    return true;
}

void ConfigCassandraPartition::RemoveObjReqEntries(set<string> &req_list) {
    BOOST_FOREACH(string uuid, req_list) {
        RemoveObjReqEntry(uuid);
    }
    req_list.clear();
}

void ConfigCassandraPartition::RemoveObjReqEntry(string &uuid) {
    UUIDProcessSet::iterator req_it =
        uuid_read_set_.find(client()->GetUUID(uuid));
    delete req_it->second;
    uuid_read_set_.erase(req_it);
}


boost::asio::io_context *ConfigCassandraPartition::ioservice() {
    return client()->event_manager()->io_service();
}

ConfigCassandraPartition::ObjCacheEntry *
ConfigCassandraPartition::GetObjCacheEntry(const string &uuid) {
    ObjectCacheMap::iterator uuid_iter = object_cache_map_.find(uuid);
    if (uuid_iter == object_cache_map_.end())
        return NULL;
    return uuid_iter->second;
}

const ConfigCassandraPartition::ObjCacheEntry *
ConfigCassandraPartition::GetObjCacheEntry(const string &uuid) const {
    ObjectCacheMap::const_iterator uuid_iter = object_cache_map_.find(uuid);
    if (uuid_iter == object_cache_map_.end())
        return NULL;
    return uuid_iter->second;
}

int ConfigCassandraPartition::UUIDRetryTimeInMSec(
        const ObjCacheEntry *obj) const {
    uint32_t retry_time_pow_of_two =
        obj->GetRetryCount() > kMaxUUIDRetryTimePowOfTwo ?
        kMaxUUIDRetryTimePowOfTwo : obj->GetRetryCount();
    return ((1 << retry_time_pow_of_two) * kMinUUIDRetryTimeMSec);
}

ConfigCassandraPartition::ObjCacheEntry::~ObjCacheEntry() {
    if (retry_timer_) {
        TimerManager::DeleteTimer(retry_timer_);
    }
}

void ConfigCassandraPartition::ObjCacheEntry::EnableCassandraReadRetry(
        const string uuid) {
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
                &ConfigCassandraPartition::ObjCacheEntry::CassReadRetryTimerExpired,
                this, uuid),
            boost::bind(
                &ConfigCassandraPartition::ObjCacheEntry::CassReadRetryTimerErrorHandler,
                this));
    CONFIG_CLIENT_DEBUG(ConfigClientReadRetry,
            "Start/restart UUID Read Retry timer due to configuration", uuid);
}

void ConfigCassandraPartition::ObjCacheEntry::DisableCassandraReadRetry(
        const string uuid) {
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

bool ConfigCassandraPartition::ObjCacheEntry::IsRetryTimerRunning() const {
    if (retry_timer_)
        return (retry_timer_->running());
    return false;
}

bool ConfigCassandraPartition::ObjCacheEntry::CassReadRetryTimerExpired(
        const string uuid) {
    parent_->client()->mgr()->EnqueueUUIDRequest(
            "UPDATE", GetObjType(), parent_->client()->uuid_str(uuid));
    retry_count_++;
    CONFIG_CLIENT_DEBUG(ConfigClientReadRetry, "timer expired ", uuid);
    return false;
}

void
ConfigCassandraPartition::ObjCacheEntry::CassReadRetryTimerErrorHandler() {
     std::string message = "Timer";
     CONFIG_CLIENT_WARN(ConfigClientGetRowError,
            "UUID Read Retry Timer error ", message, message);
}

void ConfigCassandraPartition::ListMapPropReviseUpdateList(
    const string &uuid, ConfigCassandraParseContext &context) {
    for (set<string>::iterator it =
            context.candidate_list_map_properties.begin();
            it != context.candidate_list_map_properties.end(); it++) {
        if (context.updated_list_map_properties.find(*it) !=
                context.updated_list_map_properties.end()) {
            continue;
        }
        ObjectCacheMap::iterator uuid_iter = object_cache_map_.find(uuid);
        assert(uuid_iter != object_cache_map_.end());
        FieldDetailMap::iterator field_iter =
            uuid_iter->second->GetFieldDetailMap().lower_bound(
                                          JsonAdapterDataType(*it, ""));
        assert(field_iter !=  uuid_iter->second->GetFieldDetailMap().end());
        assert(it->compare(0, it->size() - 1, field_iter->first.key,
                    0, it->size() - 1) == 0);
        while (it->compare(0, it->size() - 1, field_iter->first.key,
                    0, it->size() - 1) == 0) {
            if (field_iter->second.refreshed == false) {
                context.updated_list_map_properties.insert(*it);
                break;
            }
            field_iter++;
        }
    }
}

bool ConfigCassandraPartition::StoreKeyIfUpdated(const string &uuid,
                  JsonAdapterDataType *adapter, uint64_t timestamp,
                  ConfigCassandraParseContext &context) {
    ObjectCacheMap::iterator uuid_iter = object_cache_map_.find(uuid);
    assert(uuid_iter != object_cache_map_.end());
    size_t from_front_pos = adapter->key.find(':');
    size_t from_back_pos = adapter->key.rfind(':');
    string type_field = adapter->key.substr(0, from_front_pos+1);
    bool is_ref = (type_field == ConfigCass2JsonAdapter::ref_prefix);
    bool is_parent = (type_field == ConfigCass2JsonAdapter::parent_prefix);
    bool is_propl = (type_field == ConfigCass2JsonAdapter::list_prop_prefix);
    bool is_propm = (type_field == ConfigCass2JsonAdapter::map_prop_prefix);
    bool is_prop = (type_field == ConfigCass2JsonAdapter::prop_prefix);
    if (is_prop) {
        string prop_name  = adapter->key.substr(from_front_pos+1);
        //
        // properties like perms2 has no importance to control-node/dns
        // This property is present on each config object. Hence skipping such
        // properties gives performance improvement
        //
        if (ConfigClientManager::skip_properties.find(prop_name) !=
            ConfigClientManager::skip_properties.end()) {
            if ((prop_name.compare("draft_mode_state") == 0) &&
               !adapter->value.empty()) {
                context.ignore_object = true;
            }
            return false;
        }
    }

    string prop_name = "";
    if (is_ref || is_parent) {
        string ref_uuid = adapter->key.substr(from_back_pos+1);

        string ref_name = client()->UUIDToFQName(ref_uuid).second;
        if (ref_name == "ERROR") {
            context.parent_or_ref_fq_name_unknown = true;
            CONFIG_CLIENT_DEBUG(ConfigClientReadRetry,
                    "Out of order parent or ref", uuid + ":" + adapter->key);
            return false;
        }
        if (is_ref) {
            adapter->ref_fq_name = ref_name;
        }
    } else if (is_propl || is_propm) {
        prop_name = adapter->key.substr(0, from_back_pos);

        context.list_map_properties.insert(make_pair(prop_name, *adapter));
    }

    if (adapter->key.compare("type") == 0) {
        if (context.obj_type.empty()) {
            context.obj_type = adapter->value;
            context.obj_type.erase(remove(context.obj_type.begin(),
                      context.obj_type.end(), '\"'), context.obj_type.end());
        }
    } else if (adapter->key.compare("fq_name") == 0) {
        context.fq_name_present = true;
        if (context.fq_name.empty()) {
            context.fq_name = adapter->value.substr(1, adapter->value.size()-2);
            context.fq_name.erase(remove(context.fq_name.begin(),
                        context.fq_name.end(), '\"'), context.fq_name.end());
            context.fq_name.erase(remove(context.fq_name.begin(),
                        context.fq_name.end(), ' '), context.fq_name.end());
            replace(context.fq_name.begin(), context.fq_name.end(), ',', ':');
        }
    }
    FieldDetailMap::iterator field_iter =
    uuid_iter->second->GetFieldDetailMap().find(*adapter);
    if (field_iter == uuid_iter->second->GetFieldDetailMap().end()) {
        // seeing field for first time
        FieldTimeStampInfo field_ts_info;
        field_ts_info.refreshed = true;
        field_ts_info.time_stamp = timestamp;
        uuid_iter->second->GetFieldDetailMap().insert(make_pair
                                        (*adapter, field_ts_info));
    } else {
        field_iter->second.refreshed = true;
        if (client()->SkipTimeStampCheckForTypeAndFQName() &&
                ((adapter->key.compare("type") == 0) ||
                 (adapter->key.compare("fq_name") == 0))) {
            return true;
        }
        if (timestamp && field_iter->second.time_stamp == timestamp) {
            if (is_propl || is_propm) {
                context.candidate_list_map_properties.insert(prop_name);
            }
            return false;
        }
        field_iter->second.time_stamp = timestamp;
    }
    if (is_propl || is_propm) {
        context.updated_list_map_properties.insert(prop_name);
        return false;
    } else {
        return true;
    }
}

ConfigCassandraPartition::ObjCacheEntry *
ConfigCassandraPartition::MarkCacheDirty(const string &uuid) {
    ObjectCacheMap::iterator uuid_iter = object_cache_map_.find(uuid);
    if (uuid_iter == object_cache_map_.end()) {
        ObjCacheEntry *obj;
        string tmp_uuid = uuid;
        obj = new ObjCacheEntry(this, UTCTimestampUsec());
        pair<ObjectCacheMap::iterator, bool> ret_uuid =
            object_cache_map_.insert(tmp_uuid, obj);
        assert(ret_uuid.second);
        uuid_iter = ret_uuid.first;
    } else {
        uuid_iter->second->SetLastReadTimeStamp(UTCTimestampUsec());
    }
    for (FieldDetailMap::iterator it =
         uuid_iter->second->GetFieldDetailMap().begin();
         it != uuid_iter->second->GetFieldDetailMap().end(); it++) {
        it->second.refreshed = false;
    }
    return uuid_iter->second;
}

void ConfigCassandraPartition::FillUUIDToObjCacheInfo(const string &uuid,
                                      ObjectCacheMap::const_iterator uuid_iter,
                                      ConfigDBUUIDCacheEntry *entry) const {
    entry->set_uuid(uuid);
    entry->set_timestamp(
            UTCUsecToString(uuid_iter->second->GetLastReadTimeStamp()));
    entry->set_retry_count(uuid_iter->second->GetRetryCount());
    entry->set_fq_name(uuid_iter->second->GetFQName());
    entry->set_obj_type(uuid_iter->second->GetObjType());
    entry->set_timer_running(uuid_iter->second->IsRetryTimerRunning());
    entry->set_timer_created(uuid_iter->second->IsRetryTimerCreated());
    vector<ConfigDBUUIDCacheData> fields;
    for (FieldDetailMap::const_iterator it =
         uuid_iter->second->GetFieldDetailMap().begin();
         it != uuid_iter->second->GetFieldDetailMap().end(); it++) {
        ConfigDBUUIDCacheData each_field;
        each_field.set_refresh(it->second.refreshed);
        each_field.set_field_name(it->first.key);
        each_field.set_timestamp(UTCUsecToString(it->second.time_stamp));
        fields.push_back(each_field);
    }
    entry->set_field_list(fields);
}

bool ConfigCassandraPartition::UUIDToObjCacheShow(
    const string &search_string, const string &last_uuid, uint32_t num_entries,
    vector<ConfigDBUUIDCacheEntry> *entries) const {
    uint32_t count = 0;
    regex search_expr(search_string);
    for (ObjectCacheMap::const_iterator it =
        object_cache_map_.upper_bound(last_uuid);
        count < num_entries && it != object_cache_map_.end(); it++) {
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
