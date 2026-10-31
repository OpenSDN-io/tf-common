//
// Copyright (c) 2017 Juniper Networks, Inc. All rights reserved.
// Copyright Contributors to the OpenSDN project
//

#ifndef LIBRARY_CPP_SANDESH_OPTIONS_H_
#define LIBRARY_CPP_SANDESH_OPTIONS_H_

#include <boost/program_options.hpp>

#include <sandesh/sandesh_constants.h>

struct SandeshConfig {
    SandeshConfig() :
        http_server_ip(),
        keyfile(),
        certfile(),
        server_keyfile(),
        server_certfile(),
        ca_cert(),
        stats_collector(),
        sandesh_ssl_enable(false),
        introspect_ssl_enable(false),
        introspect_ssl_insecure(false),
        disable_object_logs(false),
        tcp_keepalive_enable(true),
        tcp_keepalive_idle_time(7200),
        tcp_keepalive_probes(9),
        tcp_keepalive_interval(75),
        system_logs_rate_limit(
            g_sandesh_constants.DEFAULT_SANDESH_SEND_RATELIMIT) {
    }
    ~SandeshConfig() {
    }

    std::string http_server_ip;
    std::string keyfile;
    std::string certfile;
    std::string server_keyfile;
    std::string server_certfile;
    std::string ca_cert;
    std::string stats_collector;
    bool sandesh_ssl_enable;
    bool introspect_ssl_enable;
    bool introspect_ssl_insecure;
    bool disable_object_logs;
    bool tcp_keepalive_enable;
    int tcp_keepalive_idle_time;
    int tcp_keepalive_probes;
    int tcp_keepalive_interval;
    uint32_t system_logs_rate_limit;
};

namespace sandesh {
namespace options {

void AddOptions(boost::program_options::options_description *sandesh_options,
    SandeshConfig *sandesh_config);
void ProcessOptions(const boost::program_options::variables_map &var_map,
    SandeshConfig *sandesh_config);

}  // namespace options
}  // namespace sandesh

#endif  // LIBRARY_CPP_SANDESH_OPTIONS_H_
