from kazoo.client import KazooClient
from kazoo.client import KazooState
from kazoo.exceptions import CancelledError
import gevent
import os
from gevent import Greenlet
from .consistent_hash import ConsistentHash
import logging
from pysandesh.connection_info import ConnectionState
from pysandesh.gen_py.process_info.ttypes import ConnectionStatus, \
    ConnectionType
from pysandesh.gen_py.sandesh.ttypes import SandeshLevel

""" Partition Library 
This library provides functionality to implement partition sharing between
cluster nodes
"""

class PartitionClient(object):
    """ Client Class for the Partition Library
    Example usage:
    ---------------------
    import libpartition
    from libpartition.libpartition import PartitionClient

    def own_change_cb(l):
            print "ownership change:" + str(l)

    c = PartitionClient("test", "s1", ["s1", "s2", "s3"], 32, 
            own_change_cb, "zookeeper_s1")

    ##do some real work now"
    if (c.own_partition(1)):
        ...... do something with partition #1 .....
        .........
    ...
    c.update_cluster_list(["s1", "s2"])
    ...
    ----------------------
    You should not call any partition library routine from within the 
    callback function

    Args:
        app_name(str): Name of the app for which partition cluster is used
        self_name(str): Name of the local cluster node (can be ip address)
        cluster_list(list): List of all the nodes in the cluster including 
            local node
        max_partition(int): Partition space always go from 0..max_partition-1
        partition_update_cb: Callback function invoked when partition
            ownership list is updated.x
        zk_server(str): <zookeeper server>:<zookeeper server port>
    """
    def __init__(
            self, app_name, self_name, cluster_list, max_partition,
            partition_update_cb, zk_server, logger = None):
       
        # Initialize local variables
        self._zk_server = zk_server
        self._cluster_list = set(cluster_list)
        self._max_partition = max_partition
        self._update_cb = partition_update_cb
        self._curr_part_ownership_list = []
        self._target_part_ownership_list = []
        self._con_hash = ConsistentHash(cluster_list)
        self._name = self_name

        # some sanity check
        if not(self._name in cluster_list):
            raise ValueError('cluster list is missing local server name')

        # initialize logging and other stuff
        if logger is None:
            logging.basicConfig()
            self._logger = logging
        else:
            self._logger = logger
        self._conn_state = None
        self._sandesh_connection_info_update(status='INIT', message='')

        # connect to zookeeper
        while True:
            self._logger.info("Libpartition zk start")
            self._zk = KazooClient(zk_server, timeout=60.0)
            self._zk.add_listener(self._zk_listen)
            try:
                self._zk.start()
                while self._conn_state != ConnectionStatus.UP:
                    gevent.sleep(1)
                break
            except Exception as e:
                # Update connection info
                self._sandesh_connection_info_update(status='DOWN',
                                                     message=str(e))
                self._zk.remove_listener(self._zk_listen)
                try:
                    self._zk.stop()
                    self._zk.close()
                except Exception as ex:
                    template = "Exception {0} in Libpartition zk stop/close. Args:\n{1!r}"
                    messag = template.format(type(ex).__name__, ex.args)
                    self._logger.error("%s : traceback %s for %s" % \
                        (messag, traceback.format_exc(), self._name))
                finally:
                    self._zk = None
                gevent.sleep(1)

        # create a lock array to contain locks for each partition
        self._part_locks = []
        for part in range(0, self._max_partition):
            lockpath = "/lockpath/"+ app_name + "/" + str(part)
            l = self._zk.Lock(lockpath, self._name)
            self._part_locks.append(l)

        # initialize partition # to lock acquire greenlet dictionary
        self._part_lock_task_dict = {}
       
        self._logger.info("initial servers:" + str(self._cluster_list))

        # update target partition ownership list
        for part in range(0, self._max_partition):
            if (self._con_hash.get_node(str(part)) == self._name):
                self._target_part_ownership_list.append(part)

        # update current ownership list
        self._acquire_partition_ownership()

    #end __init__

    def _sandesh_connection_info_update(self, status, message):
        new_conn_state = getattr(ConnectionStatus, status)
        ConnectionState.update(conn_type = ConnectionType.ZOOKEEPER,
                name = 'Zookeeper', status = new_conn_state,
                message = message,
                server_addrs = self._zk_server.split(','))

        if (self._conn_state and self._conn_state != ConnectionStatus.DOWN and
                new_conn_state == ConnectionStatus.DOWN):
            msg = 'Connection to Zookeeper down: %s' %(message)
            self._logger.error(msg)
        if (self._conn_state and self._conn_state != new_conn_state and
                new_conn_state == ConnectionStatus.UP):
            msg = 'Connection to Zookeeper ESTABLISHED'
            self._logger.info(msg)

        self._conn_state = new_conn_state
    # end _sandesh_connection_info_update

    def _zk_listen(self, state):
        self._logger.info("Libpartition listen %s" % str(state))
        if state == KazooState.CONNECTED:
            # Update connection info
            self._sandesh_connection_info_update(status='UP', message='')
        elif state == KazooState.LOST:
            self._logger.error("Libpartition connection LOST")
            # Lost the session with ZooKeeper Server
            # Best of option we have is to exit the process and restart all 
            # over again
            self._sandesh_connection_info_update(status='DOWN',
                                      message='Connection to Zookeeper lost')
            os._exit(2)
        elif state == KazooState.SUSPENDED:
            self._logger.error("Libpartition connection SUSPENDED")
            # Update connection info
            self._sandesh_connection_info_update(status='INIT',
                message = 'Connection to zookeeper lost. Retrying')

    # following routine is the greenlet task function to acquire the lock
    # for a partition
    def _acquire_lock(self, part):
        # lock for the partition
        l = self._part_locks[part]

        # go in an infinite loop waiting to acquire the lock
        try:
            while True:
                ret = l.acquire(blocking=False)
                if ret == True:
                    self._logger.info("Acquired lock for:" + str(part))
                    self._curr_part_ownership_list.append(part)
                    self._update_cb(self._curr_part_ownership_list)
                    return True
                else:
                    gevent.sleep(1)
        except CancelledError:
            self._logger.error("Lock acquire cancelled for:" + str(part))
            return False
        except Exception as ex:
            # TODO: If we have a non-KazooException, the lock object
            #       may get stuck in the "cancelled" state
            self._logger.error("Lock acquire unexpected error!: " + str(ex))
            # This exception should get propogated to main thread
            raise SystemExit(1)
            return False
    #end _acquire_lock

    # get rid of finished spawned tasks from datastructures
    def _cleanup_greenlets(self):
        for part in list(self._part_lock_task_dict.keys()):
            if (self._part_lock_task_dict[part].ready()):
                del self._part_lock_task_dict[part]
    #end _cleanup_greenlets 

    # following routine launches tasks to acquire partition locks
    def _acquire_partition_ownership(self):
        # cleanup any finished greenlets
        self._cleanup_greenlets()

        # this variable will help us decide if we need to call callback
        updated_curr_ownership = False 

        # list of partitions for which locks have to be released
        release_lock_list = []

        self._logger.info("known servers: %s" % self._con_hash.get_all_nodes())

        for part in range(0, self._max_partition):
            if (part in self._target_part_ownership_list):
                if (part in self._curr_part_ownership_list):
                    # do nothing, I already have ownership of this partition
                    self._logger.info("No need to acquire ownership of:" +
                            str(part))
                else:
                    # I need to acquire lock for this partition before I own
                    if (part in list(self._part_lock_task_dict.keys())):
                        try:
                            self._part_lock_task_dict[part].get(block=False)
                        except:
                            # do nothing there is already a greenlet running to
                            # acquire the lock
                            self._logger.error("Already a greenlet running to" 
                                    " acquire:" + str(part))
                            continue

                        # Greenlet died without getting ownership. Cleanup
                        self._logger.error("Cleanup stale greenlet running to" 
                                " acquire:" + str(part))
                        del self._part_lock_task_dict[part]

                    self._logger.info("Starting greenlet running to" 
                            " acquire:" + str(part))
                    # launch the greenlet to acquire the loc, k
                    g = Greenlet.spawn(self._acquire_lock, part)
                    self._part_lock_task_dict[part] = g

            else:
                # give up ownership of the partition

                # cancel any lock acquisition which is ongoing 
                if (part in list(self._part_lock_task_dict.keys())):
                    try:
                        self._part_lock_task_dict[part].get(block=False)
                    except:
                        
                        self._logger.error("canceling lock acquisition going on \
                            for:" + str(part))
                        # Cancelling the lock should result in killing the gevent
                        self._part_locks[part].cancel()
                        self._part_lock_task_dict[part].get(block=True)
                        
                    del self._part_lock_task_dict[part]
                        
                if (part in self._curr_part_ownership_list):
                    release_lock_list.append(part)
                    self._curr_part_ownership_list.remove(part)
                    updated_curr_ownership = True
                    self._logger.error("giving up ownership of:" + str(part))

        if (updated_curr_ownership is True):
            # current partition membership was updated call the callback 
            self._update_cb(self._curr_part_ownership_list)

        if (len(release_lock_list) != 0):
            # release locks which were acquired
            for part in release_lock_list:
                self._logger.error("release the lock which was acquired:" + \
                        str(part))
                try:
                    self._part_locks[part].release()
                    self._logger.error("fully gave up ownership of:" + str(part))
                except:
                    pass
    #end _acquire_partition_ownership

    def update_cluster_list(self, cluster_list):
        """ Updates the cluster node list
        Args:
            cluster_list(list): New list of names of the nodes in 
                the cluster
        Returns:
            None
        """
        # some sanity check
        if not(self._name in cluster_list):
            raise ValueError('cluster list is missing local server name')

        new_cluster_list = set(cluster_list)
        new_servers = list(new_cluster_list.difference(
            self._cluster_list))
        deleted_servers = list(set(self._cluster_list).difference(
            new_cluster_list)) 
        self._cluster_list = set(cluster_list)

        # update the hash structure
        if new_servers:
            self._logger.info("new servers:" + str(new_servers))
            self._con_hash.add_nodes(new_servers)
        if deleted_servers:
            self._logger.info("deleted servers:" + str(deleted_servers))
            self._con_hash.del_nodes(deleted_servers)

        # update target partition ownership list
        self._target_part_ownership_list = []
        for part in range(0, self._max_partition):
            if (self._con_hash.get_node(str(part)) == self._name):
                if not (part in self._target_part_ownership_list):
                    self._target_part_ownership_list.append(part)

        # update current ownership list
        self._acquire_partition_ownership()

    #end update_cluster_list

    def own_partition(self, part_no):
        """ Returns ownership information of a partition
        Args:
            part_no(int) : Partition no 
        Returns:
            True if partition is owned by the local node
            False if partition is not owned by the local node
        """
        return part_no in self._curr_part_ownership_list 
    #end own_partition

    def close(self):
        """ Closes any connections and frees up any data structures
        Args:
        Returns:
            None
        """
        # clean up greenlets
        for part in list(self._part_lock_task_dict.keys()):
            try:
                self._logger.info("libpartition greenlet cleanup %s" % str(part))
                self._part_lock_task_dict[part].kill()
            except:
                pass

        self._zk.remove_listener(self._zk_listen)
        gevent.sleep(1)
        self._logger.info("Stopping libpartition")
        # close zookeeper
        try:
            self._zk.stop()
        except:
            self._logger.error("Stopping libpartition failed")
        else:
            self._logger.info("Stopping libpartition successful")

        self._logger.info("Closing libpartition")
        try:
            self._zk.close()
        except:
            self._logger.error("Closing libpartition failed")
        else:
            self._logger.info("Closing libpartition successful")

    #end close
