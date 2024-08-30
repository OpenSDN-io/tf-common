/*
 *  * Copyright (c) 2018 Juniper Networks, Inc. All rights reserved.
 *   */

//
// stats_client.h
//

#ifndef __STATS_CLIENT_H__
#define __STATS_CLIENT_H__

#include <tbb/mutex.h>
#include <boost/asio.hpp>
#include <io/udp_server.h>
#include <sandesh/sandesh.h>
#include <sandesh/sandesh_util.h>

class StatsClient {
public:
    static const uint32_t kEncodeBufferSize = 2048;
    StatsClient() {};
    StatsClient(boost::asio::io_context& io_service, const std::string& endpoint);
    ~StatsClient() {}
    virtual void Initialize() = 0;
    virtual bool IsConnected() = 0;
    virtual bool SendMsg(Sandesh *sandesh) = 0;
    virtual size_t SendBuf(uint8_t *data, size_t size) = 0;
};

#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
class StatsClientLocal : public StatsClient {
public:
    StatsClientLocal(boost::asio::io_context& io_service, const std::string& stats_collector):
        stats_server_ep_(boost::asio::local::datagram_protocol::endpoint(stats_collector)),
        is_connected_(false) {
        stats_socket_.reset(new boost::asio::local::datagram_protocol::socket(io_service));
    }
    virtual ~StatsClientLocal() {stats_socket_->close();}
    virtual void Initialize();
    virtual bool IsConnected() {return is_connected_;}
    virtual bool SendMsg(Sandesh *sandesh);
    virtual size_t SendBuf(uint8_t *data, size_t size);
private:
    boost::asio::local::datagram_protocol::endpoint stats_server_ep_;
    boost::scoped_ptr<boost::asio::local::datagram_protocol::socket> stats_socket_;
    tbb::mutex send_mutex_;
    bool is_connected_;
};
#endif

class StatsClientRemote : public StatsClient {
public:
    StatsClientRemote(boost::asio::io_context& io_service, const std::string& stats_collector):
        is_connected_(false) {
        UdpServer::Endpoint stats_ep;
        MakeEndpoint(&stats_ep, stats_collector);
        stats_server_ep_ = stats_ep;
        stats_socket_.reset(new UdpServer::Socket(io_service));
    }
    virtual ~StatsClientRemote() {stats_socket_->close();}
    virtual void Initialize();
    virtual bool IsConnected() {return is_connected_;}
    virtual bool SendMsg(Sandesh *sandesh);
    virtual size_t SendBuf(uint8_t *data, size_t size);
private:
    UdpServer::Endpoint stats_server_ep_;
    boost::scoped_ptr<UdpServer::Socket> stats_socket_;
    tbb::mutex send_mutex_;
    bool is_connected_;
};

#endif // __STATS_CLIENT_H__
