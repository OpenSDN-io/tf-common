//
// Copyright (c) 2018 Juniper Networks, Inc. All rights reserved.
// Copyright Contributors to the OpenSDN project
//

#include <iostream>
#include <sandesh/sandesh.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/foreach.hpp>
#include <boost/unordered_map.hpp>
#include <boost/system/error_code.hpp>
#include <eql_types.h>
#include "proto/kv.pb.h"
#include "schema/vnc_cfg_types.h"
#include "eql_if.h"
#include "eql_log.h"
#include "base/address_util.h"

using namespace std;
using namespace etcd::etcdql;

using grpc::Channel;
using etcdserverpb::RangeRequest;
using etcdserverpb::PutRequest;
using etcdserverpb::DeleteRangeRequest;
using etcdserverpb::WatchCreateRequest;

SandeshTraceBufferPtr EqlTraceBuf(SandeshTraceBufferCreate(
     EQL_TRACE_BUF, 10000));

EtcdIf::EtcdIf(const std::vector<std::string> &etcd_hosts,
               const int port, bool useSsl)
    : port_(port),
      useSsl_(useSsl) {

    BOOST_FOREACH(const std::string &etcd_host, etcd_hosts) {
        hosts_.push_back(etcd_host);
        boost::system::error_code ec;
        boost::asio::ip::address etcd_addr;
        etcd_addr = AddressFromString(etcd_host, &ec);
        if (ec) {
            EQL_DEBUG(EtcdClientDebug, "Invalid IP address");
        }
        Endpoint endpoint(etcd_addr, port);
        endpoints_.push_back(endpoint);
    }

    watch_call_.reset(new EtcdAsyncWatchCall);
}

EtcdIf::~EtcdIf () {
}

bool EtcdIf::Connect() {
    ostringstream url;

    BOOST_FOREACH(const std::string &etcd_host, hosts_) {
        url << etcd_host << ":" << port_;

        shared_ptr<Channel> chan;

        if (useSsl_) {
            auto channel_creds = grpc::SslCredentials(
                                     grpc::SslCredentialsOptions());
            chan = grpc::CreateChannel(url.str(), channel_creds);
        } else {
            chan = grpc::CreateChannel(
                                     url.str(),
                                     grpc::InsecureChannelCredentials());
        }

        //if (chan->GetState(false) != GRPC_CHANNEL_READY) continue;

        kv_stub_ = KV::NewStub(chan);
        watch_stub_ = Watch::NewStub(chan);
        return true;
    }

    return false;
}

EtcdResponse EtcdIf::Get(string const& key,
                         string const& range_end,
                         int limit) {
    ostringstream os;

    EQL_DEBUG(EtcdClientDebug, os << "Get Request - key: " << key
                                  << " range_end: " << range_end
                                  << " limit: " << limit);

    /**
     * Set up GET request.
     */
    RangeRequest req;
    req.set_key(key);
    req.set_range_end(range_end);
    req.set_sort_target(RangeRequest::SortTarget::RangeRequest_SortTarget_KEY);
    req.set_sort_order(RangeRequest::SortOrder::RangeRequest_SortOrder_ASCEND);
    req.set_limit(limit);

    /**
     * Create a GET stream reader and invoke async read.
     */
    get_call_.reset(new EtcdAsyncGetCall);
    get_call_->get_reader_ = kv_stub_->AsyncRange(&get_call_->ctx_,
                                                  req,
                                                  &get_call_->cq_);
    get_call_->get_reader_->Finish(&get_call_->get_resp_,
                                   &get_call_->status_,
                                   (static_cast<void *>(&get_call_->gtag_)));

    /**
     * Parse the GET response.
     */
    EtcdResponse resp = get_call_->ParseGetResponse();

    return resp;
}

EtcdResponse EtcdIf::EtcdAsyncGetCall::ParseGetResponse() {
    EtcdResponse resp;
    void* got_tag;
    bool ok = false;
    ostringstream os;

    // Block until the next result is available in the completion queue "cq".
    while (cq_.Next(&got_tag, &ok)) {

        /*
         * The tag is the memory location of the call's tag object
         */
        /**
         * Verify that the request was completed successfully. Note that "ok"
         * corresponds solely to the request for updates introduced by Finish().
         */
        if (!status_.ok()) {
            resp.set_err_code(status_.error_code());
            resp.set_err_msg(status_.error_message());
            EQL_TRACE(EtcdClientErrorTrace, "Get Response: Error",
                      resp.err_code(), resp.err_msg());
            break;
        }

        if (got_tag == (static_cast<void *>(&gtag_))) {
            /*
             * Read the Get response.
             */
            resp.set_revision((get_resp_.header()).revision());
            if(get_resp_.kvs_size() == 0) {
                resp.set_err_code(100);
                resp.set_err_msg("Prefix/Key not found");
                EQL_TRACE(EtcdClientErrorTrace,
                          "Get Response: Prefix Not Found",
                          resp.err_code(),
                          resp.err_msg());
                break;
            } else {
                multimap<string, string> kvs;
                for(int i = 0; i < get_resp_.kvs_size(); i++) {
                    kvs.insert(pair<string, string>
                               (get_resp_.kvs(i).key(),
                                get_resp_.kvs(i).value()));
                }
                resp.set_kv_map(kvs);
           }
        }

        os.str("");
        EQL_DEBUG(EtcdClientDebug, os << "Get Response: Success"
                                      << " revision: "
                                      << resp.revision()
                                      << " KEY-VALUE LIST: ");

        for(int i = 0; i < get_resp_.kvs_size(); i++) {

            os.str("");
            EQL_DEBUG(EtcdClientDebug, os << " Index: " << i
                                          << " Key: "
                                          << get_resp_.kvs(i).key()
                                          << " Value: "
                                          << get_resp_.kvs(i).value());
        }

        break;
    }

    return (resp);
}

void EtcdIf::Set (const string& key, const string& value) {
    void *got_tag;
    bool ok = false;
    ostringstream os;

    EQL_DEBUG(EtcdClientDebug, os << "Set Request - Key: "
                                  << key
                                  << " Value: "
                                  << value);

    /**
     * Set up SET request.
     */
    PutRequest req;
    req.set_key(key);
    req.set_value(value);
    req.set_prev_kv(true);

    /**
     * Create a SET stream reader and invoke async set.
     */
    set_call_.reset(new EtcdAsyncSetCall);
    set_call_->set_reader_ = kv_stub_->AsyncPut(&set_call_->ctx_,
                                                req,
                                                &set_call_->cq_);
    set_call_->set_reader_->Finish(&set_call_->set_resp_,
                                   &set_call_->status_,
                                   (void*)this);

    /**
     * Parse the SET response.
     */
    while (set_call_->cq_.Next(&got_tag, &ok)) {

        if (!set_call_->status_.ok()) {
            EQL_TRACE(EtcdClientErrorTrace,
                      "Set Response: Error",
                      set_call_->status_.error_code(),
                      set_call_->status_.error_message());
        }

        if (got_tag == (void *)this) {
            os.str("");
            EQL_DEBUG(EtcdClientDebug, os << "Set Response: Success"
                           << " PrevKey: "
                           << (set_call_->set_resp_.prev_kv()).key()
                           << " PrevValue: "
                           << (set_call_->set_resp_.prev_kv()).value());
        }
        break;
    }
}

void EtcdIf::Delete (const string& key, string const& range_end) {
    void *got_tag;
    bool ok = false;
    ostringstream os;

    EQL_DEBUG(EtcdClientDebug, os << "Delete Request - Key: "
                                  << key
                                  << " Range End: "
                                  << range_end);

    /**
     * Set up DELETE request.
     */
    DeleteRangeRequest req;
    req.set_key(key);
    req.set_range_end(range_end);
    req.set_prev_kv(true);

    /**
     * Create a SET stream reader and invoke async set.
     */
    delete_call_.reset(new EtcdAsyncDeleteCall);
    delete_call_->delete_reader_ = kv_stub_->AsyncDeleteRange(
                                                &delete_call_->ctx_,
                                                req,
                                                &delete_call_->cq_);
    delete_call_->delete_reader_->Finish(&delete_call_->delete_resp_,
                                         &delete_call_->status_,
                                         (void*)this);

    /**
     * Parse the DELETE response.
     */
    while (delete_call_->cq_.Next(&got_tag, &ok)) {
        if (!delete_call_->status_.ok()) {
            EQL_TRACE(EtcdClientErrorTrace,
                      "Delete Response: Error",
                      delete_call_->status_.error_code(),
                      delete_call_->status_.error_message());
        }
        if (got_tag == (void *)this) {

            os.str("");
            EQL_DEBUG(EtcdClientDebug, os << "Delete Response: Success"
                               << " # Keys Deleted: "
                               << delete_call_->delete_resp_.deleted());

            for (int i = 0; i < delete_call_->delete_resp_.deleted(); i++) {

                os.str("");
                EQL_DEBUG(EtcdClientDebug, os << " Index: " << i
                   << " PrevKey: "
                   << (delete_call_->delete_resp_.prev_kvs(i)).key()
                   << " PrevVal: "
                   << (delete_call_->delete_resp_.prev_kvs(i)).value());
            }
        }
        break;
    }
}

void EtcdIf::Watch (const string& key, WatchCb cb) {
    WatchRequest req;
    WatchCreateRequest create_req;
    void* got_tag;
    bool ok = false;
    ostringstream os;

    EQL_DEBUG(EtcdClientDebug, os << "Watch Request - Key: " << key);

    /**
     * Create and start the Async reader-writer stream.
     */
    watch_call_->watch_reader_ = watch_stub_->AsyncWatch(&watch_call_->ctx_,
                                                  &watch_call_->cq_,
                                                  (void *)this);

    create_req.set_key(key);
    create_req.set_prev_kv(true);

    string range_end(key);
    range_end.back() = ((int)range_end[range_end.length()-1])+1;

    create_req.set_range_end(range_end);

    req.mutable_create_request()->CopyFrom(create_req);

    /**
     * When we start a stream, that is creating the reader/writer
     * object with a tag, we need to use CompletionQueue::Next to
     * wait for the tag to come back before calling Write.
     * Write the watch request to the stream and set up the first
     * read with the tag as the call object.
     */
    while (watch_call_->cq_.Next(&got_tag, &ok)) {
        if (got_tag == (void *)this) {
            watch_call_->watch_reader_->Write(req, (void *)"write");
            watch_call_->watch_reader_->Read(&watch_call_->watch_resp_,
                                             (void *)(&watch_call_->wtag));
            break;
        }
    }

    /* Set bool indicating that watch has started */
    watch_call_->watch_active_ = true;

    /* Wait for changes to happen and process them */
    watch_call_->WaitForWatchResponse(cb);
}

void EtcdIf::EtcdAsyncWatchCall::WaitForWatchResponse(WatchCb cb) {
    void* got_tag;
    bool ok = false;
    EtcdResponse resp;
    ostringstream os;

    /**
     * Waiting here for the notification of the next change
     */
    while(cq_.Next(&got_tag, &ok)) {

        /**
         * The tag is the memory location of the call object
         */

        /**
         * Verify that the request was completed successfully. Note that "ok"
         * corresponds solely to the request for updates introduced by Finish().
         * Also, stop watching if watch has been cancelled.
         */
        if (!ok || !watch_active_) {
            resp.set_err_code(10);
            resp.set_err_msg("Watch RPC failed");

            EQL_TRACE(EtcdClientErrorTrace,
                      "Watch Response: Error",
                      resp.err_code(),
                      resp.err_msg());
            break;
        }

        if (got_tag == (void*)&wtag) {
            /**
             * Read the Watch response. First time around, we won't have
             * anything to read.
             */
            if (watch_resp_.events_size()) {

                /**
                 * Got a change. Process it, populate the EtcdResponse
                 * and invoke the callback.
                 */
                resp.set_revision(watch_resp_.header().revision());
                for (int i = 0; i < watch_resp_.events_size(); i++) {
                    auto event = watch_resp_.events(i);

                    switch (event.type()) {
                    case mvccpb::Event::EventType::Event_EventType_PUT: {
                        if (event.kv().version() == 0) {
                            resp.set_action(CREATE);
                        } else {
                            resp.set_action(UPDATE);
                        }
                        break;
                    }
                    case mvccpb::Event::EventType::Event_EventType_DELETE: {
                        resp.set_action(DELETE);
                        break;
                    }
                    default:
                        break;
                    }
                    resp.set_key(event.kv().key());
                    resp.set_val(event.kv().value());

                    if (event.has_prev_kv()) {
                        resp.set_prev_key(event.prev_kv().key());
                        resp.set_prev_val(event.prev_kv().value());
                    }

                    EQL_DEBUG(EtcdClientDebug, os << "Watch Response: "
                                                  << "Success"
                                                  << " revision: "
                                                  << resp.revision()
                                                  << " action: "
                                                  << resp.action()
                                                  << " Key: "
                                                  << resp.key()
                                                  << " Value: "
                                                  << resp.value()
                                                  << " PrevKey: "
                                                  << resp.prev_key()
                                                  << " PrevValue: "
                                                  << resp.prev_value());

                    /**
                     * Invoke config client cb with watch response.
                     */
                    cb(resp);
                }
            }

            /**
             * Continue watching for subsequent changes.
             */
            watch_reader_->Read(&watch_resp_, (void*)&wtag);
        }
    } // while
}

void EtcdIf::StopWatch() {
    if (watch_call_->watch_active_) {
        watch_call_->watch_active_ = false;
        watch_call_->watch_reader_->WritesDone((void *)"Stop Watch");
    }
}
