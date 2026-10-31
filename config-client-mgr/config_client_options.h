/*
 * Copyright (c) 2016 Juniper Networks, Inc. All rights reserved.
 * Copyright Contributors to the OpenSDN project
 */

#ifndef __CONFIG_CLIENT_OPTIONS_H__
#define __CONFIG_CLIENT_OPTIONS_H__

#include <string>
#include <vector>

struct ConfigClientOptions {
    ConfigClientOptions() :
        stale_entries_cleanup_timeout(0),
        end_of_rib_timeout(0),
        peer_response_wait_time(0),
        config_db_use_ssl(false),
        rabbitmq_use_ssl(false),
        config_db_use_etcd(false) {
    }

    ConfigClientOptions(const std::string &in_server,
            const std::string &in_password, const std::string &in_user,
            const std::string &in_certs_store, int in_sect_time,
            int in_eort_time, int in_prwt_time)
        : server_url(in_server),
          password(in_password), user(in_user),
          certs_store(in_certs_store),
          stale_entries_cleanup_timeout(in_sect_time),
          end_of_rib_timeout(in_eort_time),
          peer_response_wait_time(in_prwt_time),
          config_db_use_ssl(false),
          rabbitmq_use_ssl(false),
          config_db_use_etcd(false) {
    }

    ConfigClientOptions(const std::string &in_server,
            const std::string &in_password, const std::string &in_user,
            const std::string &in_certs_store, int in_sect_time,
            int in_eort_time, int in_prwt_time,
            const std::string &cfg_db_user, const std::string &cfg_db_password,
            std::vector<std::string> &cfg_db_server_list)
        : server_url(in_server),
          password(in_password),
          user(in_user),
          certs_store(in_certs_store),
          stale_entries_cleanup_timeout(in_sect_time),
          end_of_rib_timeout(in_eort_time),
          peer_response_wait_time(in_prwt_time),
          config_db_username(cfg_db_user),
          config_db_password(cfg_db_password),
          config_db_server_list(cfg_db_server_list),
          config_db_use_ssl(false),
          rabbitmq_use_ssl(false),
          config_db_use_etcd(false) {
    }

    std::string server_url;
    std::string password;
    std::string user;
    std::string certs_store;
    int stale_entries_cleanup_timeout;  // in seconds
    int end_of_rib_timeout;  // in seconds
    int peer_response_wait_time;  // in seconds
    std::string config_db_username;
    std::string config_db_password;
    std::vector<std::string> config_db_server_list;
    bool config_db_use_ssl;
    std::string config_db_ca_certs;
    std::vector<std::string> rabbitmq_server_list;
    std::string rabbitmq_user;
    std::string rabbitmq_password;
    std::string rabbitmq_vhost;
    bool rabbitmq_use_ssl;
    std::string rabbitmq_ssl_version;
    std::string rabbitmq_ssl_keyfile;
    std::string rabbitmq_ssl_certfile;
    std::string rabbitmq_ssl_ca_certs;
    bool config_db_use_etcd;
};

#endif /* defined(__CONFIG_CLIENT_OPTIONS_H__) */
