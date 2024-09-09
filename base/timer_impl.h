/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

#ifndef BASE_TIMER_IMPL_H_
#define BASE_TIMER_IMPL_H__

#include <boost/asio/steady_timer.hpp>
#include <boost/chrono.hpp>

class TimerImpl {
public:
    typedef boost::asio::steady_timer TimerType;

    TimerImpl(boost::asio::io_context &io_service)
            : timer_(io_service) {
    }

    void expires_from_now(uint64_t ms, boost::system::error_code &ec) {
#if __cplusplus >= 201103L 
        timer_.expires_from_now(std::chrono::milliseconds(ms), ec);
#else
        timer_.expires_from_now(boost::chrono::milliseconds(ms), ec);
#endif
    }

    TimerType::duration expires_from_now() {
        return timer_.expires_from_now();
    }

    template <typename WaitHandler>
    void async_wait(WaitHandler handler) { timer_.async_wait(handler); }
    void cancel(boost::system::error_code &ec) { timer_.cancel(ec); }

private:
    TimerType timer_;
};

#endif  // BASE_TIMER_IMPL_H__
