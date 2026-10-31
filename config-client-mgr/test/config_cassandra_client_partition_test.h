/*
 * Copyright (c) 2019 Juniper Networks, Inc. All rights reserved.
 * Copyright Contributors to the OpenSDN project
 */
#ifndef config_cassandra_client_partition_test_h
#define config_cassandra_client_partition_test_h

class ConfigCassandraClientPartitionTest : public ConfigCassandraPartition {
public:
    static const int kUUIDReadRetryTimeInMsec = 300000;
    ConfigCassandraClientPartitionTest(ConfigCassandraClient *client,
                                       size_t idx)
            : ConfigCassandraPartition(client, idx),
        retry_time_ms_(kUUIDReadRetryTimeInMsec) {
    }

    virtual int UUIDRetryTimeInMSec(const ObjCacheEntry *obj) const {
        return retry_time_ms_;
    }

    void SetRetryTimeInMSec(int time) {
        retry_time_ms_ = time;
    }

    uint32_t GetUUIDReadRetryCount(string uuid) const {
        const ObjCacheEntry *obj = GetObjCacheEntry(uuid);
        if (obj)
            return (obj->GetRetryCount());
        return 0;
    }

    void RestartTimer(ObjCacheEntry *obj, string uuid) {
        obj->GetRetryTimer()->Cancel();
        obj->GetRetryTimer()->Start(10,
                boost::bind(
                        &ConfigCassandraPartition::ObjCacheEntry::
                        CassReadRetryTimerExpired,
                        obj, uuid),
                boost::bind(
                        &ConfigCassandraPartition::ObjCacheEntry::
                        CassReadRetryTimerErrorHandler,
                        obj));
    }

    void FireUUIDReadRetryTimer(string uuid) {
        ObjCacheEntry *obj = GetObjCacheEntry(uuid);
        if (obj) {
            if (obj->IsRetryTimerCreated()) {
                RestartTimer(obj, uuid);
            }
        }
    }

    virtual void HandleObjectDelete(const std::string &uuid, bool add_change) {
        std::vector<std::string> tokens;
        boost::split(tokens, uuid, boost::is_any_of(":"));
        std::string u;
        if (tokens.size() == 2) {
            u = tokens[1];
        } else {
            u = uuid;
        }
        ConfigCassandraPartition::HandleObjectDelete(u, add_change);
    }

    bool ReadObjUUIDTable(const std::set<std::string> &uuid_list) {
        ConfigCassandraClientTest *test_client =
            dynamic_cast<ConfigCassandraClientTest *>(client());
        BOOST_FOREACH(const std::string &uuid_key, uuid_list) {
            vector<string> tokens;
            boost::split(tokens, uuid_key, boost::is_any_of(":"));
            int index = atoi(tokens[0].c_str());
            std::string u = tokens[1];
            assert((*test_client->events())[index].IsObject());
            int idx = test_client->HashUUID(u);
            test_client->curr_db_idx_ = index;
            test_client->db_index_[idx].insert(make_pair(u, index));
            ProcessObjUUIDTableEntry(u, GenDb::ColList());
        }
        return true;
    }

    void ParseObjUUIDTableEntry(const std::string &uuid,
                                const GenDb::ColList &col_list,
                                CassColumnKVVec *cass_data_vec,
                                ConfigCassandraParseContext &context) {
        // Retrieve event index prepended to uuid, to get to the correct db.
        ConfigCassandraClientTest *test_client =
            dynamic_cast<ConfigCassandraClientTest *>(client());
        int idx = test_client->HashUUID(uuid);
        ConfigCassandraClientTest::UUIDIndexMap::iterator it =
            test_client->db_index_[idx].find(uuid);
        int index = it->second;

        contrail_rapidjson::Document *events = test_client->events();
        if (!(*events)[contrail_rapidjson::SizeType(index)]["db"]
                .HasMember(uuid.c_str())) {
            return;
        }

        contrail_rapidjson::Value::ConstMemberIterator end =
            (*events)[contrail_rapidjson::SizeType(index)]["db"][uuid.c_str()]
                .MemberEnd();
        for (contrail_rapidjson::Value::ConstMemberIterator k =
             (*events)[contrail_rapidjson::SizeType(index)]["db"][
                uuid.c_str()].MemberBegin(); k != end; ++k) {
            const char *k1 = k->name.GetString();
            const char *v1;
            uint64_t  ts = 0;
            if (k->value.IsArray()) {
                v1 = k->value[contrail_rapidjson::SizeType(0)].GetString();
                if (k->value.Size() > 1) {
                    ts = k->value[contrail_rapidjson::SizeType(1)].GetUint64();
                }
            } else {
                v1 = k->value.GetString();
            }

            ParseObjUUIDTableEachColumnBuildContext(uuid, k1, v1, ts,
                                                    cass_data_vec, context);
        }
        test_client->db_index_[idx].erase(it);
    }

private:
    int retry_time_ms_;
};

#endif  // config_cassandra_client_partition_test_h
