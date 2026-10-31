/*
 * Copyright (c) 2016 Juniper Networks, Inc. All rights reserved.
 * Copyright Contributors to the OpenSDN project
 */

//
// sandesh_util.h
//

#ifndef __SANDESH_UTIL_H__
#define __SANDESH_UTIL_H__

#include <io/tcp_server.h>
#include <io/udp_server.h>

bool MakeEndpoint(TcpServer::Endpoint* ep, const std::string& ep_str);
bool MakeEndpoint(UdpServer::Endpoint* ep, const std::string& ep_str);

#endif // __SANDESH_UTIL_H__
