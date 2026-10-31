/*
 * Copyright (c) 2017 Juniper Networks, Inc. All rights reserved.
 * Copyright Contributors to the OpenSDN project
 */

#include "base/index_map.h"
#include "base/logging.h"
#include "testing/gunit.h"
#include <boost/lexical_cast.hpp>


using namespace std;


class IndexMapTest : public ::testing::Test {
protected:
    typedef IndexMap<std::string, int> indexmaptype;
    indexmaptype indexmap;
};

// Insert and Remove bunch of entries
TEST_F(IndexMapTest, Basic) {
    int *value;
    std::string key;
    for (int pos = 0; pos <= 63; pos++) {
        key = "entry" + boost::lexical_cast<std::string>(pos);
        value = new int(pos);
        indexmap.Insert(key, value);
    }
    EXPECT_EQ(indexmap.count(), indexmap.size());
    for (int pos = 0; pos <= 63; pos++) {
        key = "entry" + boost::lexical_cast<std::string>(pos);
        indexmap.Remove(key, pos);
    }
}

// Insert and Remove bunch of entries without releasing index
TEST_F(IndexMapTest, RemoveWithoutRleasingIndex) {
    int *value;
    std::string key;
    for (int pos = 0; pos <= 63; pos++) {
        key = "entry" + boost::lexical_cast<std::string>(pos);
        value = new int(pos);
        indexmap.Insert(key, value);
    }
    EXPECT_EQ(indexmap.count(), indexmap.size());
    for (int pos = 0; pos <= 63; pos++) {
        key = "entry" + boost::lexical_cast<std::string>(pos);
        indexmap.Remove(key, pos, false);
    }
    EXPECT_EQ(64, indexmap.size());
    EXPECT_EQ(0, indexmap.count());
    for (int pos = 0; pos <= 63; pos++) {
        indexmap.ResetBit(pos);
    }
    EXPECT_EQ(0, indexmap.size());
}

// Remove 2 entries without releasing the index and then add one entry back
TEST_F(IndexMapTest, AddAfterDelete) {
    indexmap.ReserveBit(0);
    int *one = new int(1);
    indexmap.Insert("entry1", one);
    int *two = new int(2);
    indexmap.Insert("entry2", two);
    int *three = new int(3);
    indexmap.Insert("entry3", three);
    indexmap.Remove("entry2", 2, false);
    indexmap.Remove("entry3", 3, false);
    EXPECT_EQ(indexmap.count(), 1);
    EXPECT_EQ(4, indexmap.size());
    indexmap.ResetBit(3);
    EXPECT_EQ(3, indexmap.size());
    three = new int(3);
    indexmap.Insert("entry3", three);
    indexmap.Remove("entry3", 3);
    indexmap.ResetBit(2);
    indexmap.Remove("entry1", 1);
}

int main(int argc, char **argv) {
    LoggingInit();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
