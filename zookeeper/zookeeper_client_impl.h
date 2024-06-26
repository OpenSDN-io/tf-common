//
// Copyright (c) 2016 Juniper Networks, Inc. All rights reserved.
//

#ifndef ZOOKEEPER_ZOOKEEPER_CLIENT_IMPL_H_
#define ZOOKEEPER_ZOOKEEPER_CLIENT_IMPL_H_

#include <zookeeper/zookeeper.h>

#include <zookeeper/zookeeper_interface.h>

namespace zookeeper {
namespace client {
namespace impl {

//
// Blocking, synchronous, non-thread safe Zookeeper client
//
class ZookeeperClientImpl {
 public:
    ZookeeperClientImpl(const char *hostname, const char *servers,
        zookeeper::interface::ZookeeperInterface *zki);
    virtual ~ZookeeperClientImpl();

    bool Connect();
    void Shutdown();
    bool Reconnect();
    bool IsConnected() const;
    bool CreateNode(const char *path, const char *value, int flag);
    bool DeleteNode(const char *path);
    int CreateNodeSync(const char *path, const char *value, int *err, int flag);
    int GetNodeDataSync(const char *path, char *buf, int *buf_len, int *err);
    int DeleteNodeSync(const char *path, int *err);
    bool CheckNodeExist(const char *path);
    void SetClient(void *client) {client_ = client;}
    void *GetClient() {return client_;}
    std::string Name() const;

 private:
    static const int kSessionTimeoutMSec_ = 60000;

    std::string hostname_;
    std::string servers_;
    zhandle_t *zk_handle_;
    bool connected_;
    void *client_;
    std::unique_ptr<zookeeper::interface::ZookeeperInterface> zki_;
};

} // namespace impl
} // namespace client
} // namespace zookeeper

#endif // ZOOKEEPER_ZOOKEEPER_CLIENT_IMPL_H_
