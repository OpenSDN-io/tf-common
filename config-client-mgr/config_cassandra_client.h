/*
 * Copyright (c) 2016 Juniper Networks, Inc. All rights reserved.
 */

#ifndef config_cass_client_h
#define config_cass_client_h

#include <boost/ptr_container/ptr_map.hpp>
#include <boost/shared_ptr.hpp>

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
#include "config_cassandra_client.h"
#include "database/gendb_if.h"
#include "json_adapter_data.h"

class EventManager;
class ConfigClientManager;
struct ConfigDBConnInfo;
class TaskTrigger;
class ConfigCassandraClient;
struct ConfigCassandraParseContext;
class ConfigDBFQNameCacheEntry;
class ConfigDBUUIDCacheEntry;

class ConfigCassandraPartition {
 public:
    ConfigCassandraPartition(ConfigCassandraClient *client, size_t idx);
    virtual ~ConfigCassandraPartition();

    typedef boost::shared_ptr<WorkQueue<ObjectProcessReq *> >
        ObjProcessWorkQType;

    struct FieldTimeStampInfo {
        uint64_t time_stamp;
        bool refreshed;
    };

    struct cmp_json_key {
        bool operator()(const JsonAdapterDataType &k1,
                         const JsonAdapterDataType &k2) const {
            return k1.key < k2.key;
        }
    };
    typedef std::map<JsonAdapterDataType, FieldTimeStampInfo, cmp_json_key>
                                                             FieldDetailMap;
    class ObjCacheEntry : public ObjectCacheEntry {
     public:
        ObjCacheEntry(ConfigCassandraPartition *parent,
                uint64_t last_read_tstamp)
            : ObjectCacheEntry(last_read_tstamp),
              retry_count_(0), retry_timer_(NULL),
              parent_(parent) {
        }

        virtual ~ObjCacheEntry();

        void EnableCassandraReadRetry(const std::string uuid);
        void DisableCassandraReadRetry(const std::string uuid);
        FieldDetailMap &GetFieldDetailMap() { return field_detail_map_; }
        const FieldDetailMap &GetFieldDetailMap() const {
            return field_detail_map_;
        }
        uint32_t GetRetryCount() const { return retry_count_; }
        bool IsRetryTimerCreated() const { return (retry_timer_ != NULL); }
        bool IsRetryTimerRunning() const;
        Timer *GetRetryTimer() { return retry_timer_; }

     private:
        friend class ConfigCassandraPartitionTest;
        friend class ConfigCassandraPartitionTest2;
        friend class ConfigCassandraClientPartitionTest;

        bool CassReadRetryTimerExpired(const std::string uuid);
        void CassReadRetryTimerErrorHandler();
        uint32_t retry_count_;
        Timer *retry_timer_;
        FieldDetailMap field_detail_map_;
        ConfigCassandraPartition *parent_;
    };

    static const uint32_t kMaxUUIDRetryTimePowOfTwo = 20;
    static const uint32_t kMinUUIDRetryTimeMSec = 100;
    typedef boost::ptr_map<std::string, ObjCacheEntry> ObjectCacheMap;

    ObjProcessWorkQType obj_process_queue() {
        return obj_process_queue_;
    }

    virtual int UUIDRetryTimeInMSec(const ObjCacheEntry *obj) const;
    ObjCacheEntry *GetObjCacheEntry(const std::string &uuid);
    const ObjCacheEntry *GetObjCacheEntry(const std::string &uuid) const;
    bool StoreKeyIfUpdated(const std::string &uuid,
                           JsonAdapterDataType *adapter,
                           uint64_t timestamp,
                           ConfigCassandraParseContext &context);
    void ListMapPropReviseUpdateList(const std::string &uuid,
                                     ConfigCassandraParseContext &context);
    ObjCacheEntry *MarkCacheDirty(const std::string &uuid);
    void DeleteCacheMap(const std::string &uuid) {
        object_cache_map_.erase(uuid);
    }
    void Enqueue(ObjectProcessReq *req);


    bool UUIDToObjCacheShow(
        const std::string &search_string, const std::string &last_uuid,
        uint32_t num_entries,
        std::vector<ConfigDBUUIDCacheEntry> *entries) const;
    int GetInstanceId() const { return worker_id_; }

    boost::asio::io_context *ioservice();

    bool IsListOrMapPropEmpty(const string &uuid_key, const string &lookup_key);
    bool IsTaskTriggered() const;
protected:
    virtual bool ReadObjUUIDTable(const std::set<std::string> &uuid_list);
    bool ProcessObjUUIDTableEntry(const std::string &uuid_key,
                                  const GenDb::ColList &col_list);
    virtual void ParseObjUUIDTableEntry(const std::string &uuid,
        const GenDb::ColList &col_list, CassColumnKVVec *cass_data_vec,
        ConfigCassandraParseContext &context);
    void ParseObjUUIDTableEachColumnBuildContext(const std::string &uuid,
           const std::string &key, const std::string &value,
           uint64_t timestamp, CassColumnKVVec *cass_data_vec,
           ConfigCassandraParseContext &context);
    virtual void HandleObjectDelete(const string &uuid, bool add_change);
    ConfigCassandraClient *client() { return config_client_; }

private:
    friend class ConfigCassandraClient;

    struct ObjectProcessRequestType {
        ObjectProcessRequestType(const std::string &in_oper,
                                 const std::string &in_obj_type,
                                 const std::string &in_uuid)
            : oper(in_oper), obj_type(in_obj_type), uuid(in_uuid) {
        }
        std::string oper;
        std::string obj_type;
        std::string uuid;
    };

    typedef std::map<std::string, ObjectProcessRequestType *> UUIDProcessSet;

    bool RequestHandler(ObjectProcessReq *req);
    void AddUUIDToRequestList(const std::string &oper,
                      const std::string &obj_type, const std::string &uuid_str);
    bool ConfigReader();
    void RemoveObjReqEntries(std::set<std::string> &req_list);
    void RemoveObjReqEntry(std::string &uuid);
    virtual void GenerateAndPushJson(
            const string &uuid_key, const string &obj_type,
            const CassColumnKVVec &cass_data_vec, bool add_change);

    void FillUUIDToObjCacheInfo(const std::string &uuid,
                                ObjectCacheMap::const_iterator uuid_iter,
                                ConfigDBUUIDCacheEntry *entry) const;

    ObjProcessWorkQType obj_process_queue_;
    UUIDProcessSet uuid_read_set_;
    ObjectCacheMap object_cache_map_;
    boost::shared_ptr<TaskTrigger> config_reader_;
    ConfigCassandraClient *config_client_;
    int worker_id_;
};

/*
 * This class has the functionality to interact with the cassandra servers that
 * store the user configuration.
 */
class ConfigCassandraClient : public ConfigDbClient {
 public:
    // Cassandra table names
    static const std::string kUuidTableName;
    static const std::string kFqnTableName;

    // Task names
    static const std::string kCassClientTaskId;
    static const std::string kObjectProcessTaskId;

    // Number of UUIDs to read in one read request
    static const int kMaxNumUUIDToRead = 64;

    // Number of FQName entries to read in one read request
    static const int kNumFQNameEntriesToRead = 4096;

    typedef boost::scoped_ptr<GenDb::GenDbIf> GenDbIfPtr;
    typedef std::vector<ConfigCassandraPartition *> PartitionList;

    ConfigCassandraClient(ConfigClientManager *mgr, EventManager *evm,
                          const ConfigClientOptions &options,
                          int num_workers);
    virtual ~ConfigCassandraClient();

    virtual void InitDatabase();
    void BulkSyncDone();
    ConfigCassandraPartition *GetPartition(const std::string &uuid);
    const ConfigCassandraPartition *GetPartition(const std::string &uuid) const;
    const ConfigCassandraPartition *GetPartition(int worker_id) const;

    void EnqueueUUIDRequest(std::string oper, std::string obj_type,
                                    std::string uuid_str);

    virtual bool UUIDToObjCacheShow(
        const std::string &search_string, int inst_num,
        const std::string &last_uuid, uint32_t num_entries,
        std::vector<ConfigDBUUIDCacheEntry> *entries) const;
    virtual bool IsListOrMapPropEmpty(const string &uuid_key,
                                      const string &lookup_key);
    virtual bool IsTaskTriggered() const;
protected:
    typedef std::pair<std::string, std::string> ObjTypeUUIDType;
    typedef std::list<ObjTypeUUIDType> ObjTypeUUIDList;

    void UpdateFQNameCache(const std::string &key, const std::string &obj_type,
                           ObjTypeUUIDList &uuid_list);
    virtual bool BulkDataSync();
    bool EnqueueDBSyncRequest(const ObjTypeUUIDList &uuid_list);
    virtual std::string FetchUUIDFromFQNameEntry(const std::string &key) const;

    virtual int HashUUID(const std::string &uuid_str) const;
    virtual bool SkipTimeStampCheckForTypeAndFQName() const { return true; }
    virtual uint32_t GetFQNameEntriesToRead() const {
        return kNumFQNameEntriesToRead;
    }
    int num_workers() const { return num_workers_; }
    PartitionList &partitions() { return partitions_; }
    virtual void PostShutdown();

 private:
    friend class ConfigCassandraPartition;

    bool InitRetry();

    bool FQNameReader();
    bool ParseFQNameRowGetUUIDList(const std::string &obj_type,
               const GenDb::ColList &col_list, ObjTypeUUIDList &uuid_list,
               std::string *last_column);

    void HandleCassandraConnectionStatus(bool success,
                                         bool force_update = false);

    GenDbIfPtr dbif_;
    int num_workers_;
    PartitionList partitions_;
    boost::scoped_ptr<TaskTrigger> fq_name_reader_;
    tbb::atomic<long> bulk_sync_status_;
};

#endif  // config_cass_client_h
