/*
 * Copyright (c) 2018 Juniper Networks, Inc. All rights reserved.
 */

#ifndef config_etcd_client_h
#define config_etcd_client_h

#include <boost/ptr_container/ptr_map.hpp>
#include <boost/shared_ptr.hpp>

#include "database/etcd/eql_if.h"

#include <atomic>
#include <list>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/queue_task.h"
#include "base/timer.h"

#include "config_db_client.h"
#include "config_json_parser_base.h"
#include "json_adapter_data.h"

using namespace std;
using etcd::etcdql::EtcdIf;
using etcd::etcdql::EtcdResponse;
using contrail_rapidjson::Document;
using contrail_rapidjson::Value;

class EventManager;
class ConfigClientManager;
struct ConfigDBConnInfo;
class TaskTrigger;
class ConfigEtcdClient;
class ConfigDBUUIDCacheEntry;

class ConfigEtcdPartition {
 public:
    ConfigEtcdPartition(ConfigEtcdClient *client, size_t idx);
    virtual ~ConfigEtcdPartition();

    typedef boost::shared_ptr<WorkQueue<ObjectProcessReq *> >
             UUIDProcessWorkQType;

    class UUIDCacheEntry : public ObjectCacheEntry {
     public:
        UUIDCacheEntry(ConfigEtcdPartition *parent,
                       const string &value_str,
                       uint64_t last_read_tstamp)
                : ObjectCacheEntry(last_read_tstamp),
                  retry_count_(0),
                  retry_timer_(NULL),
                  json_str_(value_str),
                  parent_(parent) {
        }

        ~UUIDCacheEntry();

        void EnableEtcdReadRetry(const string uuid,
                                 const string value);
        void DisableEtcdReadRetry(const string uuid);

        const string &GetJsonString() const { return json_str_; }
        void SetJsonString(const string &value_str) {
            json_str_ = value_str;
        }

        void SetListOrMapPropEmpty(const string &prop, bool empty) {
            list_map_set_.insert(make_pair(prop.c_str(), empty));
        }
        bool ListOrMapPropEmpty(const string &prop) const;

        uint32_t GetRetryCount() const {
            return retry_count_;
        }
        bool IsRetryTimerCreated() const {
            return (retry_timer_ != NULL);
        }
        bool IsRetryTimerRunning() const;
        Timer *GetRetryTimer() { return retry_timer_; }

     private:
        friend class ConfigEtcdPartitionTest;
        bool EtcdReadRetryTimerExpired(const string uuid,
                                       const string value);
        void EtcdReadRetryTimerErrorHandler();
        typedef map<string, bool> ListMapSet;
        ListMapSet list_map_set_;
        uint32_t retry_count_;
        Timer *retry_timer_;
        string json_str_;
        ConfigEtcdPartition *parent_;
    };

    static const uint32_t kMaxUUIDRetryTimePowOfTwo = 20;
    static const uint32_t kMinUUIDRetryTimeMSec = 100;

    typedef boost::ptr_map<string, UUIDCacheEntry> UUIDCacheMap;

    UUIDCacheEntry *GetUUIDCacheEntry(const string &uuid);
    const UUIDCacheEntry *GetUUIDCacheEntry(const string &uuid) const;
    UUIDCacheEntry *GetUUIDCacheEntry(const string &uuid,
                                      const string &value_str,
                                      bool &is_new);
    const UUIDCacheEntry *GetUUIDCacheEntry(const string &uuid,
                                            const string &value_str,
                                            bool &is_new) const;
    void DeleteCacheMap(const string &uuid) {
        uuid_cache_map_.erase(uuid);
    }
    virtual int UUIDRetryTimeInMSec(const UUIDCacheEntry *obj) const;

    void FillUUIDToObjCacheInfo(const string &uuid,
                                UUIDCacheMap::const_iterator uuid_iter,
                                ConfigDBUUIDCacheEntry *entry) const;
    bool UUIDToObjCacheShow(
        const string &search_string, const string &last_uuid,
        uint32_t num_entries,
        vector<ConfigDBUUIDCacheEntry> *entries) const;

    int GetInstanceId() const { return worker_id_; }

    UUIDProcessWorkQType obj_process_queue() {
        return obj_process_queue_;
    }

    void Enqueue(ObjectProcessReq *req);
    bool IsListOrMapPropEmpty(const string &uuid_key,
                              const string &lookup_key);
    virtual bool IsTaskTriggered() const;

protected:
    ConfigEtcdClient *client() {
        return config_client_;
    }

private:
    friend class ConfigEtcdClient;

    struct UUIDProcessRequestType {
        UUIDProcessRequestType(const string &in_oper,
                                 const string &in_uuid,
                                 const string &in_value)
            : oper(in_oper), uuid(in_uuid), value(in_value) {
        }
        string oper;
        string uuid;
        string value;
    };

    typedef map<string, UUIDProcessRequestType *> UUIDProcessSet;

    bool RequestHandler(ObjectProcessReq *req);
    void AddUUIDToProcessList(const string &oper,
                              const string &uuid_key,
                              const string &value_str);
    bool ConfigReader();
    void ProcessUUIDUpdate(const string &uuid_key,
                           const string &value_str);
    void ProcessUUIDDelete(const string &uuid_key);
    virtual bool GenerateAndPushJson(
            const string &uuid_key,
            Document &doc,
            bool add_change,
            UUIDCacheEntry *cache);
    void RemoveObjReqEntry(string &uuid);

    UUIDProcessWorkQType obj_process_queue_;
    UUIDProcessSet uuid_process_set_;
    UUIDCacheMap uuid_cache_map_;
    boost::shared_ptr<TaskTrigger> config_reader_;
    ConfigEtcdClient *config_client_;
    int worker_id_;
};

/*
 * This class has the functionality to interact with the cassandra servers that
 * store the user configuration.
 */
class ConfigEtcdClient : public ConfigDbClient {
 public:
    typedef vector<ConfigEtcdPartition *> PartitionList;

    ConfigEtcdClient(ConfigClientManager *mgr, EventManager *evm,
                          const ConfigClientOptions &options,
                          int num_workers);
    virtual ~ConfigEtcdClient();

    virtual void InitDatabase();
    void BulkSyncDone();
    void EnqueueUUIDRequest(string oper, string obj_type,
                                    string uuid_str);

    ConfigEtcdPartition *GetPartition(const string &uuid);
    const ConfigEtcdPartition *GetPartition(const string &uuid) const;
    const ConfigEtcdPartition *GetPartition(int worker_id) const;

    // Start ETCD watch for config updates
    void StartWatcher();

    // UUID Cache
    virtual bool UUIDToObjCacheShow(
        const string &search_string, int inst_num,
        const string &last_uuid, uint32_t num_entries,
        vector<ConfigDBUUIDCacheEntry> *entries) const;

    virtual bool IsListOrMapPropEmpty(const string &uuid_key,
                                      const string &lookup_key);

    bool IsTaskTriggered() const;
    virtual void ProcessResponse(EtcdResponse resp);

    // For testing
    static void set_watch_disable(bool disable) {
        disable_watch_ = disable;
    }
protected:
    typedef pair<string, string> UUIDValueType;
    typedef list<UUIDValueType> UUIDValueList;

    virtual bool BulkDataSync();
    void EnqueueDBSyncRequest(const UUIDValueList &uuid_list);

    virtual int HashUUID(const std::string &uuid_str) const;
    int num_workers() const { return num_workers_; }
    PartitionList &partitions() { return partitions_; }
    virtual void PostShutdown();

private:
    friend class ConfigEtcdPartition;

    // A Job for watching changes to config stored in etcd
    class EtcdWatcher;

    bool InitRetry();
    bool UUIDReader();
    void HandleEtcdConnectionStatus(bool success,
                                    bool force_update = false);

    // For testing
    static bool disable_watch_;

    boost::scoped_ptr<EtcdIf> eqlif_;
    int num_workers_;
    PartitionList partitions_;
    boost::scoped_ptr<TaskTrigger> uuid_reader_;
    std::atomic<long> bulk_sync_status_;
};

#endif  // config_etcd_client_h
