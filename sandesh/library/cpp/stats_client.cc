/*
 *  * Copyright (c) 2018 Juniper Networks, Inc. All rights reserved.
 *   */

//
// stats_client.cc
//


#include <boost/bind.hpp>
#include <boost/assign.hpp>
#include <sandesh/transport/TBufferTransports.h>
#include <sandesh/protocol/TJSONProtocol.h>
#include <stats_client.h>

#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
void StatsClientLocal::Initialize() {
    boost::system::error_code ec;
    stats_socket_->connect(stats_server_ep_, ec);
    if (ec) {
        SANDESH_LOG(ERROR, "LOCAL could not connect to socket: " << ec.message());
        is_connected_ = false;
        return;
    }
    is_connected_ = true;
}

size_t StatsClientLocal::SendBuf(uint8_t *data, size_t size) {
    if (!is_connected_) {
        Initialize();
    }
    boost::system::error_code ec;
    size_t ret = stats_socket_->send(boost::asio::buffer(data, size), 0, ec);
    if (ec) {
        SANDESH_LOG(ERROR, "LOCAL could not send to socket: " << ec.message());
        is_connected_ = false;
    }
    return ret;
}

bool StatsClientLocal::SendMsg(Sandesh *sandesh) {
    std::scoped_lock lock(send_mutex_);
    uint8_t *buffer;
    int32_t xfer = 0, ret = 0;
    uint32_t offset;
    namespace sandesh_prot = contrail::sandesh::protocol;
    namespace sandesh_trans = contrail::sandesh::transport;
    boost::shared_ptr<sandesh_trans::TMemoryBuffer> btrans(
                    new sandesh_trans::TMemoryBuffer(kEncodeBufferSize));
    boost::shared_ptr<sandesh_prot::TJSONProtocol> prot(
                    new sandesh_prot::TJSONProtocol(btrans));
    if ((ret = sandesh->Write(prot)) < 0) {
        SANDESH_LOG(ERROR, __func__ << ": Sandesh write FAILED: "<<
            sandesh->Name() << " : " << sandesh->source() << ":" <<
            sandesh->module() << ":" << sandesh->instance_id() <<
            " Sequence Number:" << sandesh->seqnum());
        Sandesh::UpdateTxMsgFailStats(sandesh->Name(), 0,
            SandeshTxDropReason::WriteFailed);
        return true;
    }
    xfer += ret;
    btrans->getBuffer(&buffer, &offset);
    SendBuf(buffer, offset);
    return true;
}
#endif

void StatsClientRemote::Initialize() {
    boost::system::error_code ec;
    stats_socket_->open(boost::asio::ip::udp::v4(), ec);
    if (ec) {
        SANDESH_LOG(ERROR, "REMOTE could not open socket: " << ec.message());
        is_connected_ = false;
        return;
    }
    stats_socket_->connect(stats_server_ep_, ec);
    if (ec) {
        SANDESH_LOG(ERROR, "REMOTE could not connect address: " << ec.message());
        is_connected_ = false;
        stats_socket_->close();
        return;
    }
    is_connected_ = true;
}

size_t StatsClientRemote::SendBuf(uint8_t *data, size_t size) {
    if (!is_connected_) {
        Initialize();
    }
    boost::system::error_code ec;
    size_t ret = stats_socket_->send(boost::asio::buffer(data, size), 0, ec);
    if (ec) {
        SANDESH_LOG(ERROR, "REMOTE could not send to socket: " << ec.message());
        is_connected_ = false;
    }
    return ret;
}

bool StatsClientRemote::SendMsg(Sandesh *sandesh) {
    std::scoped_lock lock(send_mutex_);
    uint8_t *buffer;
    int32_t xfer = 0, ret = 0;
    uint32_t offset;
    namespace sandesh_prot = contrail::sandesh::protocol;
    namespace sandesh_trans = contrail::sandesh::transport;
    boost::shared_ptr<sandesh_trans::TMemoryBuffer> btrans(
                    new sandesh_trans::TMemoryBuffer(kEncodeBufferSize));
    boost::shared_ptr<sandesh_prot::TJSONProtocol> prot(
                    new sandesh_prot::TJSONProtocol(btrans));
    if ((ret = sandesh->Write(prot)) < 0) {
        SANDESH_LOG(ERROR, __func__ << ": Sandesh write FAILED: "<<
            sandesh->Name() << " : " << sandesh->source() << ":" <<
            sandesh->module() << ":" << sandesh->instance_id() <<
            " Sequence Number:" << sandesh->seqnum());
        Sandesh::UpdateTxMsgFailStats(sandesh->Name(), 0,
            SandeshTxDropReason::WriteFailed);
        return true;
    }
    xfer += ret;
    btrans->getBuffer(&buffer, &offset);
    SendBuf(buffer, offset);
    return true;
}
