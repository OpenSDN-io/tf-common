/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

#include "boost/asio/detail/recycling_allocator.hpp"

#include "io/tcp_message_write.h"

#include "base/util.h"
#include "base/logging.h"
#include "io/tcp_session.h"
#include "io/io_log.h"

using boost::asio::buffer;
using boost::asio::buffer_cast;
using boost::asio::mutable_buffer;
using boost::system::error_code;
using tbb::mutex;
using std::min;

const int TcpMessageWriter::kDefaultWriteBufferSize;
const int TcpMessageWriter::kMaxPendingBufferSize;
const int TcpMessageWriter::kMinPendingBufferSize;

TcpMessageWriter::TcpMessageWriter(TcpSession *session,
                                   size_t buffer_send_size) :
    offset_(0), last_write_(0), buffer_send_size_(buffer_send_size),
    session_(session) {
}

TcpMessageWriter::~TcpMessageWriter() {
    for (BufferQueue::iterator iter = buffer_queue_.begin();
         iter != buffer_queue_.end(); ++iter) {
        DeleteBuffer(*iter);
    }
    buffer_queue_.clear();
}

int TcpMessageWriter::AsyncSend(const uint8_t *data, size_t len, error_code *ec) {

    int write = len;

    if (buffer_queue_.empty()) {
        BufferAppend(data, len);
        if (session_->io_strand_) {
            boost::asio::detail::recycling_allocator<void> allocator;
            session_->io_strand_->post(bind(&TcpSession::AsyncWriteInternal,
                                       session_, TcpSessionPtr(session_)), allocator);
        }
    } else {
        BufferAppend(data, len);
    }

    if ((GetBufferQueueSize() - offset_) > TcpMessageWriter::kMaxPendingBufferSize) {
        if (!session_->write_blocked_) {
            /* throttle the sender */
            session_->stats_.write_blocked++;
            session_->server_->stats_.write_blocked++;
            session_->stats_.write_block_start_time = UTCTimestampUsec();
            session_->write_blocked_ = true;
        }
        write = 0;
    }

    return write;
}

void TcpMessageWriter::TriggerAsyncWrite() {

    /* assert if there is an async write in progress */
    assert(last_write_ == 0);
    assert(!buffer_queue_.empty());

    boost::asio::mutable_buffer head = buffer_queue_.front();
    size_t remaining = buffer_size(head) - offset_;
    last_write_  = min(buffer_send_size_, remaining);

    // Update socket write call statistics.
    session_->stats_.write_calls++;
    session_->server_->stats_.write_calls++;

    const uint8_t *data = buffer_cast<const uint8_t *>(head) + offset_;
    session_->AsyncWrite(data, last_write_);
}

bool TcpMessageWriter::UpdateBufferQueue(size_t wrote, bool *send_ready) {

    assert(last_write_ == wrote);
    assert(!buffer_queue_.empty());

    bool more_write = true;
    last_write_ = 0;
    *send_ready = false;

    boost::asio::mutable_buffer head = buffer_queue_.front();
    if ((offset_ + wrote) == buffer_size(head)) {
        offset_ = 0;
        DeleteBuffer(head);
        buffer_queue_.pop_front();
    } else {
        offset_ += wrote;
    }

    if (session_->write_blocked_ && ((GetBufferQueueSize() - offset_)  <
                                     TcpMessageWriter::kMinPendingBufferSize)) {
        uint64_t blocked_usecs =  UTCTimestampUsec() -
                session_->stats_.write_block_start_time;
        session_->stats_.write_blocked_duration_usecs += blocked_usecs;
        session_->server_->stats_.write_blocked_duration_usecs += blocked_usecs;
        session_->write_blocked_ = false;
        *send_ready = true;
    }

    if (buffer_queue_.empty()) {
        buffer_queue_.clear();
        more_write = false;
    }

    return more_write;
}

void TcpMessageWriter::BufferAppend(const uint8_t *src, int bytes) {
    uint8_t *data = new uint8_t[bytes];
    memcpy(data, src, bytes);
    mutable_buffer buffer = mutable_buffer(data, bytes);
    buffer_queue_.push_back(buffer);
}

void TcpMessageWriter::DeleteBuffer(mutable_buffer buffer) {
    const uint8_t *data = buffer_cast<const uint8_t *>(buffer);
    delete[] data;
    return;
}

