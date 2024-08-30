/*
 * Copyright (c) 2017 Juniper Networks, Inc. All rights reserved.
 */

#include <vector>

#include "config_factory.h"

#include "config_json_parser_base.h"
template<> ConfigJsonParserRec::FunctionType
    ConfigJsonParserRec::create_func_ = nullptr;
// Do not define the default record for ConfigJsonParser,
// because it is purely abstract
//template<> ConfigJsonParserRec::DefaultLinkType
//    ConfigJsonParserRec::default_link_{};

#include "config_amqp_client.h"
template<> ConfigAmqpChannelRec::FunctionType
    ConfigAmqpChannelRec::create_func_ = nullptr;
template<> ConfigAmqpChannelRec::DefaultLinkType
    ConfigAmqpChannelRec::default_link_{};

#include "config_cassandra_client.h"
template<> ConfigCassandraClientRec::FunctionType
    ConfigCassandraClientRec::create_func_ = nullptr;
template<> ConfigCassandraClientRec::DefaultLinkType
    ConfigCassandraClientRec::default_link_{};

template<> ConfigCassandraPartitionRec::FunctionType
    ConfigCassandraPartitionRec::create_func_ = nullptr;
template<> ConfigCassandraPartitionRec::DefaultLinkType
    ConfigCassandraPartitionRec::default_link_{};

#include "database/cassandra/cql/cql_if.h"
template<> CqlIfRec1::FunctionType
    CqlIfRec1::create_func_ = nullptr;
template<> CqlIfRec1::DefaultLinkType
    CqlIfRec1::default_link_{};

template<> CqlIfRec2::FunctionType
    CqlIfRec2::create_func_ = nullptr;
template<> CqlIfRec2::DefaultLinkType
    CqlIfRec2::default_link_{};

