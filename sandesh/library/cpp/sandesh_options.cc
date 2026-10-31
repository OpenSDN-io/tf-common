//
// Copyright (c) 2017 Juniper Networks, Inc. All rights reserved.
// Copyright Contributors to the OpenSDN project
//

#include <cassert>
#include <base/options_util.h>
#include <sandesh/sandesh_options.h>

namespace opt = boost::program_options;
using namespace options::util;

namespace sandesh {
namespace options {

void AddOptions(opt::options_description *sandesh_options,
    SandeshConfig *sandesh_config) {
    // Command line and config file options.
    sandesh_options->add_options()
        ("SANDESH.sandesh_keyfile", opt::value<std::string>()->default_value(
         "/etc/contrail/ssl/private/server-privkey.pem"),
         "Sandesh SSL private key")
        ("SANDESH.sandesh_certfile", opt::value<std::string>()->default_value(
         "/etc/contrail/ssl/certs/server.pem"),
         "Sandesh SSL certificate")
        ("SANDESH.sandesh_server_keyfile", opt::value<std::string>()->default_value(
         "/etc/contrail/ssl/private/server-privkey.pem"),
         "Sandesh SSL Server private key")
        ("SANDESH.sandesh_server_certfile", opt::value<std::string>()->default_value(
         "/etc/contrail/ssl/certs/server.pem"),
         "Sandesh SSL Server certificate")
        ("SANDESH.sandesh_ca_cert", opt::value<std::string>()->default_value(
         "/etc/contrail/ssl/certs/ca-cert.pem"),
         "Sandesh CA SSL certificate")
        ("SANDESH.sandesh_ssl_enable",
         opt::bool_switch(&sandesh_config->sandesh_ssl_enable),
         "Enable SSL for sandesh connection")
        ("SANDESH.introspect_ssl_enable",
         opt::bool_switch(&sandesh_config->introspect_ssl_enable),
         "Enable SSL for introspect connection")
        ("SANDESH.introspect_ssl_insecure",
         opt::bool_switch(&sandesh_config->introspect_ssl_insecure),
         "Enable SSL insecure for introspect connection")
        ("SANDESH.disable_object_logs",
         opt::bool_switch(&sandesh_config->disable_object_logs),
         "Disable sending of object logs to collector")
        ("STATS.stats_collector", opt::value<std::string>()->default_value(
         ""),
         "External Stats Collector")
        ("DEFAULT.sandesh_send_rate_limit",
         opt::value<uint32_t>()->default_value(
         g_sandesh_constants.DEFAULT_SANDESH_SEND_RATELIMIT),
         "System logs send rate limit in messages per second per message type")
        ("DEFAULT.http_server_ip",
         opt::value<std::string>()->default_value(
         "0.0.0.0"),
         "Listen IP for the Introspect")
        ("SANDESH.tcp_keepalive_enable",
         opt::bool_switch(&sandesh_config->tcp_keepalive_enable)->default_value(true),
         "Enable Keepalive for tcp socket")
        ("SANDESH.tcp_keepalive_idle_time",
         opt::value<int>(&sandesh_config->tcp_keepalive_idle_time),
         "Keepalive idle time for tcp socket")
        ("SANDESH.tcp_keepalive_probes",
         opt::value<int>(&sandesh_config->tcp_keepalive_probes),
         "Keepalive probes for tcp socket")
        ("SANDESH.tcp_keepalive_interval",
         opt::value<int>(&sandesh_config->tcp_keepalive_interval),
         "Keepalive interval for tcp socket")
        ;
}

void ProcessOptions(const opt::variables_map &var_map,
    SandeshConfig *sandesh_config) {
    GetOptValue<std::string>(var_map, sandesh_config->keyfile,
                        "SANDESH.sandesh_keyfile");
    GetOptValue<std::string>(var_map, sandesh_config->certfile,
                        "SANDESH.sandesh_certfile");
    GetOptValue<std::string>(var_map, sandesh_config->server_keyfile,
                        "SANDESH.sandesh_server_keyfile");
    GetOptValue<std::string>(var_map, sandesh_config->server_certfile,
                        "SANDESH.sandesh_server_certfile");
    GetOptValue<std::string>(var_map, sandesh_config->ca_cert,
                        "SANDESH.sandesh_ca_cert");
    GetOptValue<bool>(var_map, sandesh_config->sandesh_ssl_enable,
                      "SANDESH.sandesh_ssl_enable");
    GetOptValue<bool>(var_map, sandesh_config->introspect_ssl_enable,
                      "SANDESH.introspect_ssl_enable");
    GetOptValue<bool>(var_map, sandesh_config->introspect_ssl_insecure,
                      "SANDESH.introspect_ssl_insecure");
    GetOptValue<bool>(var_map, sandesh_config->disable_object_logs,
                      "SANDESH.disable_object_logs");
    GetOptValue<std::string>(var_map, sandesh_config->stats_collector,
                        "STATS.stats_collector");
    GetOptValue<uint32_t>(var_map, sandesh_config->system_logs_rate_limit,
                          "DEFAULT.sandesh_send_rate_limit");
    GetOptValue<std::string>(var_map, sandesh_config->http_server_ip,
                        "DEFAULT.http_server_ip");
    GetOptValue<bool>(var_map, sandesh_config->tcp_keepalive_enable,
                       "SANDESH.tcp_keepalive_enable");
    GetOptValue<int>(var_map, sandesh_config->tcp_keepalive_idle_time,
                     "SANDESH.tcp_keepalive_idle_time");
    GetOptValue<int>(var_map, sandesh_config->tcp_keepalive_probes,
                     "SANDESH.tcp_keepalive_probes");
    GetOptValue<int>(var_map, sandesh_config->tcp_keepalive_interval,
                     "SANDESH.tcp_keepalive_interval");
}

}  // namespace options
}  // namespace sandesh
