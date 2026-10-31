//
// Copyright (c) 2019 Juniper Networks, Inc. All rights reserved.
// Copyright Contributors to the OpenSDN project
//

#include <sys/wait.h>
#include <boost/system/error_code.hpp>

#include <io/process_signal.h>

namespace process {

boost::system::error_code Signal::InitializeSigChild() {
    boost::system::error_code ec;

    if (!sigchld_callbacks_.empty() || always_handle_sigchild_) {
        ec = AddSignal(SIGCHLD);
    }

    return ec;
}

void Signal::RegisterHandler(SignalChildHandler handler) {
    if (sigchld_callbacks_.empty()) {
        // Add signal first
        AddSignal(SIGCHLD);
    }
    sigchld_callbacks_.push_back(handler);
}

bool Signal::HandleSigOsSpecific(const boost::system::error_code& error,
                                 int sig) {
    if (sig != SIGCHLD) {
        return false;
    }

    int status = 0;
    pid_t pid = 0;

    while ((pid = WaitPid(-1, &status, WNOHANG)) > 0) {
        NotifySigChld(error, sig, pid, status);
    }

    return true;
}

} // namespace process
