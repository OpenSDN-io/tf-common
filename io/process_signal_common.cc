//
// Copyright (c) 2017 Juniper Networks, Inc. All rights reserved.
//

#include <sys/wait.h>

#include <boost/bind/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/range/adaptor/map.hpp>

#include <base/logging.h>
#include <io/event_manager.h>
#include <io/process_signal.h>

using namespace boost::placeholders;

namespace process {

Signal::Signal(EventManager *evm,
    const SignalCallbackMap &sig_callback_map,
    const std::vector<SignalChildHandler> &sigchld_callbacks,
    bool always_handle_sigchild) :
    signal_(*(evm->io_service())),
    sig_callback_map_(sig_callback_map),
    sigchld_callbacks_(sigchld_callbacks),
    always_handle_sigchild_(always_handle_sigchild) {
    Initialize();
}

Signal::Signal(EventManager *evm,
    const SignalCallbackMap &sig_callback_map) :
    signal_(*(evm->io_service())),
    sig_callback_map_(sig_callback_map),
    sigchld_callbacks_(std::vector<SignalChildHandler>()),
    always_handle_sigchild_(false) {
    Initialize();
}

Signal::~Signal() {
}

boost::system::error_code Signal::AddSignal(int sig) {
    boost::system::error_code ec;
    signal_.add(sig, ec);
    if (ec) {
        std::string sigstr(strsignal(sig));
        LOG(ERROR, sigstr << " registration failed: " << ec);
    }
    return ec;
}

void Signal::RegisterHandler(int sig, SignalHandler handler) {
    SignalCallbackMap::iterator it = sig_callback_map_.find(sig);
    if (it == sig_callback_map_.end()) {
        // Add signal first
        AddSignal(sig);
        sig_callback_map_.insert(std::make_pair(sig,
            std::vector<SignalHandler>(1, handler)));
    } else {
        std::vector<SignalHandler> &sig_handlers(it->second);
        sig_handlers.push_back(handler);
    }
}

void Signal::NotifySigChld(const boost::system::error_code &error, int sig,
                                int pid, int status) {
    BOOST_FOREACH(const SignalChildHandler &sh, sigchld_callbacks_) {
        sh(error, sig, pid, status);
    }
}

void Signal::NotifySig(const boost::system::error_code &error,
                              int sig) {
    SignalCallbackMap::const_iterator it = sig_callback_map_.find(sig);
    if (it == sig_callback_map_.end()) {
        return;
    }
    const std::vector<SignalHandler> &callbacks(it->second);
    BOOST_FOREACH(const SignalHandler &sh, callbacks) {
        sh(error, sig);
    }
}

int Signal::WaitPid(int pid, int *status, int options) {
    return ::waitpid(pid, status, options);
}

void Signal::HandleSig(const boost::system::error_code &error, int sig) {
    if (!error) {
        if (!HandleSigOsSpecific(error, sig)) {
            NotifySig(error, sig);
        }
        RegisterSigHandler();
    }
}

void Signal::RegisterSigHandler() {
    signal_.async_wait(boost::bind(&Signal::HandleSig, this, _1, _2));
}

void Signal::Initialize() {
    boost::system::error_code ec;

    // Add signals
    BOOST_FOREACH(int sig, sig_callback_map_ | boost::adaptors::map_keys) {
        ec = AddSignal(sig);
    }

    ec = InitializeSigChild();

    RegisterSigHandler();
}

void Signal::Terminate() {
    boost::system::error_code ec;
    signal_.cancel(ec);
    signal_.clear(ec);
}

}  // namespace process
