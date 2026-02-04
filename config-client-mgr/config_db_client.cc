/*
 * Copyright (c) 2016 Juniper Networks, Inc. All rights reserved.
 */

#include "config_db_client.h"

#include <boost/tokenizer.hpp>

#include "base/string_util.h"
#include "config_client_options.h"
#include "config_client_log.h"
#include "config_client_log_types.h"
#include "config_client_show_types.h"

using contrail::regex;
using contrail::regex_match;
using contrail::regex_search;

using namespace std;

ConfigDbClient::ConfigDbClient(ConfigClientManager *mgr,
                               EventManager *evm,
                               const ConfigClientOptions &options)
    : mgr_(mgr), evm_(evm),
      config_db_user_(options.config_db_username),
      config_db_password_(options.config_db_password) {

    for (vector<string>::const_iterator iter =
                options.config_db_server_list.begin();
         iter != options.config_db_server_list.end(); iter++) {
        string server_info(*iter);
        typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
        boost::char_separator<char> sep(":");
        tokenizer tokens(server_info, sep);
        tokenizer::iterator tit = tokens.begin();
        string ip(*tit);
        config_db_ips_.push_back(ip);
        ++tit;
        string port_str(*tit);
        int port;
        stringToInteger(port_str, port);
        config_db_ports_.push_back(port);
    }
}

ConfigDbClient::~ConfigDbClient() {
}

const string& ConfigDbClient::config_db_user() const {
    return config_db_user_;
}

const string& ConfigDbClient::config_db_password() const {
    return config_db_password_;
}

const vector<string>& ConfigDbClient::config_db_ips() const {
    return config_db_ips_;
}

int ConfigDbClient::GetFirstConfigDbPort() const {
    return !config_db_ports_.empty() ? config_db_ports_[0] : 0;
}

string ConfigDbClient::uuid_str(const string &uuid) {
    return uuid;
}

uint32_t ConfigDbClient::GetNumReadRequestToBunch() const {
    static bool init_ = false;
    static uint32_t num_read_req_to_bunch = 0;

    if (!init_) {
        // XXX To be used for testing purposes only.
        char *count_str = getenv("CONFIG_NUM_DB_READ_REQ_TO_BUNCH");
        if (count_str) {
            num_read_req_to_bunch = strtol(count_str, NULL, 0);
        } else {
            num_read_req_to_bunch = kNumEntriesToRead;
        }
        init_ = true;
    }
    return num_read_req_to_bunch;
}

void ConfigDbClient::AddFQNameCache(const string &uuid,
                               const string &obj_type,
                               const string &fq_name) {
    tbb::spin_rw_mutex::scoped_lock write_lock(rw_mutex_, true);
    FQNameCacheType cache_obj(obj_type, fq_name);
    fq_name_cache_.insert(make_pair(uuid, cache_obj));
    return;
}

void ConfigDbClient::InvalidateFQNameCache(const string &uuid) {
    tbb::spin_rw_mutex::scoped_lock write_lock(rw_mutex_, true);
    FQNameCacheMap::iterator it = fq_name_cache_.find(uuid);
    if (it != fq_name_cache_.end()) {
        it->second.deleted = true;
    }
    return;
}

void ConfigDbClient::PurgeFQNameCache(const string &uuid) {
    tbb::spin_rw_mutex::scoped_lock write_lock(rw_mutex_, true);
    fq_name_cache_.erase(uuid);
}

string ConfigDbClient::FindFQName(const string &uuid) const {
    ObjTypeFQNPair obj_type_fq_name_pair = UUIDToFQName(uuid);
    return obj_type_fq_name_pair.second;
}

ConfigDbClient::ObjTypeFQNPair ConfigDbClient::UUIDToFQName(
                                  const string &uuid, bool deleted_ok) const {
    tbb::spin_rw_mutex::scoped_lock read_lock(rw_mutex_, false);
    FQNameCacheMap::const_iterator it = fq_name_cache_.find(uuid);
    if (it != fq_name_cache_.end()) {
        if (!it->second.deleted || (it->second.deleted && deleted_ok)) {
            return make_pair(it->second.obj_type, it->second.fq_name);
        }
    }
    return make_pair("ERROR", "ERROR");
}

void ConfigDbClient::FillFQNameCacheInfo(const string &uuid,
    FQNameCacheMap::const_iterator it, ConfigDBFQNameCacheEntry *entry) const {
    entry->set_uuid(it->first);
    entry->set_obj_type(it->second.obj_type);
    entry->set_fq_name(it->second.fq_name);
    entry->set_deleted(it->second.deleted);
}

bool ConfigDbClient::UUIDToFQNameShow(
    const string &search_string, const string &last_uuid,
    uint32_t num_entries,
    vector<ConfigDBFQNameCacheEntry> *entries) const {
    uint32_t count = 0;
    bool more = false;
    regex search_expr(search_string);
    tbb::spin_rw_mutex::scoped_lock read_lock(rw_mutex_, false);
    for (FQNameCacheMap::const_iterator it =
        fq_name_cache_.upper_bound(last_uuid);
        it != fq_name_cache_.end(); it++) {
        if (regex_search(it->first, search_expr) ||
            regex_search(it->second.obj_type, search_expr) ||
            regex_search(it->second.fq_name, search_expr)) {
            if (++count > num_entries) {
                more = true;
                break;
            }
            ConfigDBFQNameCacheEntry entry;
            FillFQNameCacheInfo(it->first, it, &entry);
            entries->push_back(entry);
        }
    }
    return more;
}

void ConfigDbClient::InitConnectionInfo() {
    client_connection_up_ = false;
    connection_status_change_at_ = UTCTimestampUsec();
}

void ConfigDbClient::UpdateConnectionInfo(bool success,
                                          bool force) {
    bool previous_status =
              client_connection_up_.exchange(success);
    if ((previous_status == success) && !force) {
        return;
    }

    connection_status_change_at_ = UTCTimestampUsec();
}

void ConfigDbClient::GetConnectionInfo(ConfigDBConnInfo &status) const {
    status.cluster = boost::algorithm::join(config_db_ips(), ", ");
    status.connection_status = client_connection_up_;
    status.connection_status_change_at =
                    UTCUsecToString(connection_status_change_at_);
    return;
}

bool ConfigDbClient::IsTaskTriggered() const {
    return false;
}

void ConfigDbClient::StartWatcher() {
}
