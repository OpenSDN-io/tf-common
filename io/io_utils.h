//
// Copyright (c) 2014 Juniper Networks, Inc. All rights reserved.
//

#ifndef SRC_IO_IO_UTILS_H_
#define SRC_IO_IO_UTILS_H_

#include <atomic>

class SocketIOStats;

namespace io {

struct SocketStats {
    SocketStats();

    void GetRxStats(SocketIOStats *socket_stats) const;
    void GetTxStats(SocketIOStats *socket_stats) const;

    std::atomic<uint64_t> read_calls;
    std::atomic<uint64_t> read_bytes;
    std::atomic<uint64_t> read_errors;
    std::atomic<uint64_t> write_calls;
    std::atomic<uint64_t> write_bytes;
    std::atomic<uint64_t> write_errors;
    std::atomic<uint64_t> write_block_start_time;
    std::atomic<uint64_t> write_blocked;
    std::atomic<uint64_t> write_blocked_duration_usecs;
    std::atomic<uint64_t> read_block_start_time;
    std::atomic<uint64_t> read_blocked;
    std::atomic<uint64_t> read_blocked_duration_usecs;
};

}  // namespace io

#endif  // SRC_IO_IO_UTILS_H_
