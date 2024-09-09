/*
 * Copyright (c) 2017 Juniper Networks, Inc. All rights reserved.
 */
#ifndef __CONFIG__CONFIG_FACTORY_H__
#define __CONFIG__CONFIG_FACTORY_H__

#include <string>
#include <vector>

#include <boost/function.hpp>
#include "base/factory.h"

namespace cass { namespace cql { class CqlIf; } }
using cass::cql::CqlIf;

namespace etcd { namespace etcdql { class EtcdIf; } }
using etcd::etcdql::EtcdIf;

class ConfigAmqpChannel;
class ConfigCassandraClient;
class ConfigCassandraPartition;
class ConfigEtcdClient;
class ConfigEtcdPartition;
class ConfigClientManager;
class ConfigJsonParserBase;
struct ConfigClientOptions;
class EventManager;

struct ConfigStaticObjectFactory : public StaticObjectFactory {

// The overload for ConfigCassandraClient (because of references)
template <class Base, class T1, class T2, class T3, class T4>
static typename FactoryTypes<Base, T1*, T2*, const T3&, T4>::BasePointer
CreateRef(T1* arg1, T2* arg2, const T3& arg3, T4 arg4) {
    return FactoryRecord<Base, T1*, T2*, const T3&, T4>::
        create_func_(arg1, arg2, arg3, arg4);
}

// The overload for CqlIf (because of references)
template <class Base, class T1, class T2, class T3>
static typename FactoryTypes<Base, T1*, const T2&, int, const T3&, const T3&, bool, const T3&, bool>::BasePointer
CreateRef(T1 *arg1, const T2& arg2, int arg3, const T3& arg4, const T3& arg5, bool arg6, const T3&arg7, bool arg8) {
    return FactoryRecord<Base, T1*, const T2&, int, const T3&, const T3&, bool, const T3&, bool>::
        create_func_(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
}

// The overload for CqlIf (because of references)
template <class Base, class T1, class T2, class T3>
static typename FactoryTypes<Base, T1*, const T2&, int, const T3&, const T3&, bool, const T3&>::BasePointer
CreateRef(T1 *arg1, const T2& arg2, int arg3, const T3& arg4, const T3& arg5, bool arg6, const T3&arg7) {
    return FactoryRecord<Base, T1*, const T2&, int, const T3&, const T3&, bool, const T3&>::
        create_func_(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
}

};

using ConfigJsonParserRec =
    ConfigStaticObjectFactory::FactoryRecord<ConfigJsonParserBase>;

using ConfigAmqpChannelRec =
    ConfigStaticObjectFactory::FactoryRecord<ConfigAmqpChannel>;

using ConfigCassandraClientRec =
    ConfigStaticObjectFactory::FactoryRecord<ConfigCassandraClient,
        ConfigClientManager *,
        EventManager *,
        const ConfigClientOptions &,
        int>;

using ConfigCassandraPartitionRec =
    ConfigStaticObjectFactory::FactoryRecord<ConfigCassandraPartition,
        ConfigCassandraClient *,
        size_t>;

using CqlIfRec1 =
    ConfigStaticObjectFactory::FactoryRecord<CqlIf,
        EventManager *,
        const std::vector<std::string, std::allocator<std::string>>&,
        int,
        const std::string &,
        const std::string &,
        bool,
        const std::string &,
        bool>;

using CqlIfRec2 =
    ConfigStaticObjectFactory::FactoryRecord<CqlIf,
        EventManager *,
        const std::vector<std::string, std::allocator<std::string>>&,
        int,
        const std::string &,
        const std::string &,
        bool,
        const std::string &>;

//template<> struct ConfigStaticObjectFactory::CastTo<ConfigAmqpChannel> {using BaseType = ConfigAmqpChannel;};

//To be defined in a corresponding module
//template<> struct ConfigStaticObjectFactory::CastTo<ConfigJsonParser> {using BaseType = ConfigJsonParserBase;};

//template<> struct ConfigStaticObjectFactory::CastTo<ConfigCassandraClient> {using BaseType = ConfigCassandraClient;};

//template<> struct ConfigStaticObjectFactory::CastTo<ConfigCassandraPartition> {using BaseType = ConfigCassandraPartition;};

//template<> struct ConfigStaticObjectFactory::CastTo<CqlIf> {using BaseType = CqlIf;};

/*
 Etcd client was disabled in SConscript
template<> struct ConfigStaticObjectFactory::CastTo<ConfigEtcdClient> {using BaseType = ConfigEtcdClient;};

template<> struct ConfigStaticObjectFactory::CastTo<ConfigEtcdPartition> {using BaseType = ConfigEtcdPartition;};

template<> struct ConfigStaticObjectFactory::CastTo<EtcdIf> {using BaseType = EtcdIf;};
*/

#endif  // __CONFIG__CONFIG_FACTORY_H__
