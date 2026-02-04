/*
 * Copyright (c) 2016 Juniper Networks, Inc. All rights reserved.
 */

#ifndef config_db_client_h
#define config_db_client_h

#include <string>
#include <vector>
#include <atomic>
#include <tbb/spin_rw_mutex.h>

#include "base/regex.h"
#include "base/timer.h"
#include "base/task_annotations.h"
#include "config_client_manager.h"

struct ConfigClientOptions;
struct ConfigDBConnInfo;
struct ConfigDBUUIDCacheEntry;
struct ConfigDBFQNameCacheEntry;

class ObjectProcessReq {
 public:
    ObjectProcessReq(std::string oper,
                     std::string uuid_str,
                     std::string value) : oper_(oper),
    uuid_str_(uuid_str), value_(value) {
    }

    std::string oper_;
    std::string uuid_str_;
    std::string value_; // obj_type for Cassandra/json_value for ETCD

 private:
    DISALLOW_COPY_AND_ASSIGN(ObjectProcessReq);
};

/*
 * This is the base class for interactions with a database that stores the user
 * configuration.
 */
class ConfigDbClient {
public:
    // wait time before retrying in seconds
    static const uint64_t kInitRetryTimeUSec = 5000000;

    // Number of requests to handle in one config reader task execution
    static const int kMaxRequestsToYield = 512;

    // Number of config entries to read in one read request
    static const int kNumEntriesToRead = 4096;

    ConfigDbClient(ConfigClientManager *mgr,
                   EventManager *evm,
                   const ConfigClientOptions &options);
    virtual ~ConfigDbClient();

    typedef std::pair<std::string, std::string> ObjTypeFQNPair;

    const std::string& config_db_user() const;
    const std::string& config_db_password() const;
    const std::vector<std::string>& config_db_ips() const;
    int GetFirstConfigDbPort() const;
    virtual void PostShutdown() = 0;
    virtual void InitDatabase() = 0;
    virtual void EnqueueUUIDRequest(std::string uuid_str, std::string obj_type,
                                    std::string oper) = 0;

    virtual bool UUIDToObjCacheShow(
        const std::string &search_string, int inst_num,
        const std::string &last_uuid, uint32_t num_entries,
        std::vector<ConfigDBUUIDCacheEntry> *entries) const = 0;

    virtual bool IsListOrMapPropEmpty(const std::string &uuid_key,
                                   const std::string &lookup_key) = 0;

     // FQ Name Cache
    virtual void AddFQNameCache(const std::string &uuid,
                   const std::string &obj_type, const std::string &fq_name);
    virtual std::string FindFQName(const std::string &uuid) const;
    virtual void InvalidateFQNameCache(const std::string &uuid);
    virtual void PurgeFQNameCache(const std::string &uuid);
    virtual void ClearFQNameCache() {
        fq_name_cache_.clear();
    }
    ObjTypeFQNPair UUIDToFQName(const std::string &uuid_str,
                             bool deleted_ok = true) const;

    virtual bool UUIDToFQNameShow(
        const std::string &search_string, const std::string &last_uuid,
        uint32_t num_entries,
        std::vector<ConfigDBFQNameCacheEntry> *entries) const;

    virtual std::string uuid_str(const std::string &uuid);
    virtual std::string GetUUID(const std::string &key) const {
        return key;
    }

    virtual void InitConnectionInfo();
    virtual void UpdateConnectionInfo(bool success,
                                      bool force);
    virtual void GetConnectionInfo(ConfigDBConnInfo &status) const;

    virtual bool IsTaskTriggered() const;
    virtual void StartWatcher();

    ConfigClientManager *mgr() { return mgr_; }
    const ConfigClientManager *mgr() const { return mgr_; }

protected:
   // UUID to FQName mapping
    struct FQNameCacheType {
        FQNameCacheType(std::string in_obj_type, std::string in_fq_name)
            : obj_type(in_obj_type), fq_name(in_fq_name), deleted(false) {
        }
        std::string obj_type;
        std::string fq_name;
        bool deleted;
    };
    typedef std::map<std::string, FQNameCacheType> FQNameCacheMap;

    virtual void FillFQNameCacheInfo(
                          const std::string &uuid,
                          FQNameCacheMap::const_iterator it,
                          ConfigDBFQNameCacheEntry *entry) const;

    virtual const int GetMaxRequestsToYield() const {
        return kMaxRequestsToYield;
    }
    virtual const uint64_t GetInitRetryTimeUSec() const {
        return kInitRetryTimeUSec;
    }

    virtual uint32_t GetNumReadRequestToBunch() const;
    EventManager *event_manager() { return  evm_; }

private:
    ConfigClientManager *mgr_;
    EventManager *evm_;
    std::string config_db_user_;
    std::string config_db_password_;
    std::vector<std::string> config_db_ips_;
    std::vector<int> config_db_ports_;
    FQNameCacheMap fq_name_cache_;
    mutable tbb::spin_rw_mutex rw_mutex_;
    std::atomic<bool> client_connection_up_;
    std::atomic<uint64_t> connection_status_change_at_;
};

class ObjectCacheEntry {
 public:
    ObjectCacheEntry(uint64_t last_read_tstamp)
        : last_read_tstamp_(last_read_tstamp) {
    }

    ~ObjectCacheEntry() {};

    virtual void SetLastReadTimeStamp(uint64_t ts) {
        last_read_tstamp_ = ts;
    }
    virtual uint64_t GetLastReadTimeStamp() const {
        return last_read_tstamp_;
    }

    virtual void SetFQName(std::string fq_name) {
        fq_name_ = fq_name;
    }
    virtual const std::string &GetFQName() const {
        return fq_name_;
    }

    virtual void SetObjType(std::string obj_type) {
        obj_type_ = obj_type;
    }
    virtual const std::string &GetObjType() const {
        return obj_type_;
    }

 private:
    std::string obj_type_;
    std::string fq_name_;
    uint64_t last_read_tstamp_;
};
#endif  // config_db_client_h
