/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_client_sm.h
//
// Sandesh Client State Machine
//

#ifndef __SANDESH_CLIENT_SM_H__
#define __SANDESH_CLIENT_SM_H__

#include <atomic>
#include <string>
#include <mutex>
#include <boost/function.hpp>
#include <io/tcp_server.h>
#include <sandesh/sandesh_session.h>

class SandeshHeader;
class SandeshSession;


// This is the interface for the Sandesh Client State Machine.
//
// The user of the state machine instantiates it using
// the static function "CreateClientSM"
//
// The user must provide callbacks by implementing the
// SandeshClientSM::Mgr class, which has to be passed in
// at creation time.

class SandeshClientSM {
public:
    class Mgr {
        public:
            virtual bool ReceiveMsg(const std::string& msg,
                        const SandeshHeader &header, const std::string &sandesh_name,
                        const uint32_t header_offset) = 0;
            virtual void SendUVE(int count,
                        const std::string & stateName, const std::string & server,
                        const TcpServer::Endpoint & server_ip,
                        const std::vector<TcpServer::Endpoint> & collectors) = 0;
            virtual SandeshSession *CreateSMSession(
                        TcpSession::EventObserver eocb,
                        SandeshReceiveMsgCb rmcb,
                        TcpServer::Endpoint ep) = 0;
            virtual void InitializeSMSession(int connects) = 0;
            virtual void DeleteSMSession(SandeshSession * session) = 0;
            virtual StatsClient *stats_client() const = 0;
        protected:
            Mgr() {}
            virtual ~Mgr() {}
    };

    typedef enum {
        IDLE = 0,
        DISCONNECT = 1,
        CONNECT = 2,
        CLIENT_INIT = 3,
        ESTABLISHED = 4
    } State;
    static const int kTickInterval = 30000; // 30 sec .. specified in milliseconds
    static SandeshClientSM * CreateClientSM(EventManager *evm, Mgr *mgr,
            int sm_task_instance, int sm_task_id, bool periodicuve);
    State state() const { return state_; }
    virtual const std::string &StateName() const = 0;
    SandeshSession *session() {
        return session_;
    }
    TcpServer::Endpoint server() {
        std::scoped_lock l(mtex_); return server_;
    }

    // This function is used to start and stop the state machine
    virtual void SetAdminState(bool down) = 0;

    // This function should be called when there is a change in the Collector list
    virtual void SetCollectors(const std::vector<TcpServer::Endpoint> &collectors) = 0;

    // This function is used to send UVE sandesh's to the server
    virtual bool SendSandeshUVE(Sandesh* snh) = 0;

    // This function is used to send sandesh's to the server
    virtual bool SendSandesh(Sandesh* snh) = 0;

    virtual ~SandeshClientSM() {}

protected:
    SandeshClientSM(Mgr *mgr):  mgr_(mgr), session_(), server_() { state_ = IDLE; }

    virtual void EnqueDelSession(SandeshSession * session) = 0;

    void set_session(SandeshSession * session, bool enq) {
        session = session_.exchange(session);
        if (session != NULL) {
            session->set_observer(NULL);
            session->SetReceiveMsgCb(NULL);
            session->Close();
            session->Shutdown();
            if (enq)
                EnqueDelSession(session);
        }
    }

    bool send_session(Sandesh *snh) {
        return snh->Enqueue(session()->send_queue());
    }

    void set_server(TcpServer::Endpoint e) {
        std::scoped_lock l(mtex_); server_ = e;
    }

    Mgr * const mgr_;
    std::atomic<State> state_;

private:
    std::mutex mtex_;
    std::atomic<SandeshSession *> session_;
    TcpServer::Endpoint server_;

    friend class SandeshClientStateMachineTest;
};

#endif
