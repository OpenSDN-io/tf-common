//
// Copyright (c) 2016 Juniper Networks, Inc. All rights reserved.
//

#ifndef ZOOKEEPER_ZOOKEEPER_CLIENT_H_
#define ZOOKEEPER_ZOOKEEPER_CLIENT_H_

#include <boost/function.hpp>

class ZookeeperClientTest;

namespace zookeeper {
namespace client {

// Forward declarations
namespace impl {
class ZookeeperClientImpl;
} // namespace impl

typedef enum Z_NODE_TYPE {
    Z_NODE_TYPE_PERSISTENT = 1,
    Z_NODE_TYPE_EPHEMERAL,
    Z_NODE_TYPE_SEQUENCE,
}Z_NODE_TYPE_E;

typedef boost::function<void (void)> ZooStateCallback;

//
// Blocking, synchronous, non-thread safe Zookeeper client
//
class ZookeeperClient {
 public:
    ZookeeperClient(const char *hostname, const char *servers);
    virtual ~ZookeeperClient();
    bool CreateNode(const char *path,
                    const char *data,
                    int type = Z_NODE_TYPE_PERSISTENT);
    bool CheckNodeExist(const char* path);
    bool DeleteNode(const char* path);
    void Shutdown();
    void AddListener(ZooStateCallback cb);
    ZooStateCallback cb;

 private:
    ZookeeperClient(impl::ZookeeperClientImpl *impl);

    friend class ZookeeperLock;
    friend class ::ZookeeperClientTest;

    std::unique_ptr<impl::ZookeeperClientImpl> impl_;
};

//
// Usage is to first create a ZookeeperClient, and then ZookeeperLock
// for distributed synchronization
//
class ZookeeperLock {
 public:
    ZookeeperLock(ZookeeperClient *client, const char *path);
    virtual ~ZookeeperLock();

    bool Lock();
    bool Release();

 private:
    class ZookeeperLockImpl;
    friend class ::ZookeeperClientTest;

    std::string Id() const;

    std::unique_ptr<ZookeeperLockImpl> impl_;
};

} // namespace client
} // namespace zookeeper

#endif // ZOOKEEPER_ZOOKEEPER_CLIENT_H_
