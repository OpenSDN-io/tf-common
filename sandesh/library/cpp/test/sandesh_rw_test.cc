/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 * Copyright Contributors to the OpenSDN project
 */

//
// sandesh_rw_test.cc
//
// Sandesh Read-Write Test
//

#include <map>
#include <arpa/inet.h>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/range/iterator_range.hpp>

#include "testing/gunit.h"

#include "base/logging.h"
#include "base/util.h"

#include <sandesh/protocol/TXMLProtocol.h>
#include <sandesh/protocol/TJSONProtocol.h>
#include <sandesh/protocol/TBinaryProtocol.h>
#include <sandesh/transport/TBufferTransports.h>

#include <sandesh/sandesh_types.h>
#include <sandesh/sandesh.h>
#include "sandesh_rw_test_types.h"

using namespace boost::asio::ip;
using namespace contrail::sandesh::protocol;
using namespace contrail::sandesh::transport;

static const int32_t test_i32 = 0xdeadbeef;
static const uint32_t test_list_size = 5;

class SandeshReadWriteUnitTest : public ::testing::Test {
protected:
    SandeshReadWriteUnitTest() {
    }

    void SandeshReadWriteProcess(boost::shared_ptr<TMemoryBuffer> btrans,
            boost::shared_ptr<TProtocol> prot) {
        uint8_t *buffer;
        uint32_t offset, wxfer, rxfer;
        // Initialize the struct
        wstruct_test_.set_i32Test(test_i32);
        // Create a vector of SandeshStructTest
        std::vector<SandeshListTestElement> test_list;
        for (uint32_t i = 0; i < test_list_size; i++) {
            SandeshListTestElement tmp;
            tmp.set_i32Elem(i);
            test_list.push_back(tmp); // 0, 1, 2, 3, 4
        }
        wstruct_test_.set_listTest(test_list);
        // Create a vector of int32_t
        std::vector<int32_t> test_basic_type_list;
        for (uint32_t i = 0; i < test_list_size; i++) {
            test_basic_type_list.push_back(i); // 0, 1, 2, 3, 4
        }
        wstruct_test_.set_basicTypeListTest(test_basic_type_list);
        // Create a vector of uuid
        std::vector<boost::uuids::uuid> test_uuid_list;
        for (uint32_t i = 0; i < test_list_size; i++) {
            boost::uuids::uuid uuid_temp =
                {0x00+i,0x00+i,0x01+i,0x01+i,0x02+i,0x02+i,0x03+i,0x03+i,
                 0x04+i,0x04+i,0x05+i,0x05+i,0x06+i,0x06+i,0x07+i,0x07+i};
            test_uuid_list.push_back(uuid_temp);
        }
        wstruct_test_.set_uuidListTest(test_uuid_list);
        // Create a map of <int32_t, string>
        std::map<int32_t, std::string> test_basic_type_map;
        for (uint32_t i = 0; i < test_list_size; i++) {
            // (0, "0"), (1, "1"), (2, "2"), (3, "3"), (4, "4")
            test_basic_type_map.insert(std::pair<int32_t, std::string>
                                       (i, "a"+integerToString(i)));
        }
        wstruct_test_.set_basicTypeMapTest(test_basic_type_map);
        // Create a map of <int32_t, SandeshListTestElement>
        std::map<int32_t, SandeshListTestElement> test_complex_type_map;
        for (uint32_t i = 0; i < test_list_size; i++) {
            SandeshListTestElement tmp;
            tmp.set_i32Elem(i);
            test_complex_type_map.insert(std::pair<int32_t, SandeshListTestElement>
                                         (i, tmp));
        }
        wstruct_test_.set_complexTypeMapTest(test_complex_type_map);
        wstruct_test_.set_u16Test(65535);
        wstruct_test_.set_u32Test(4294967295u);
        wstruct_test_.set_u64Test(18446744073709551615ull);
        wstruct_test_.set_ipv4Test(4294967295u);
        boost::uuids::uuid uuid_test =
             {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
              0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
        wstruct_test_.set_uuidTest(uuid_test);
        wstruct_test_.set_xmlTest("<abc>");
        wstruct_test_.set_xmlTest1("abc");
        wstruct_test_.set_xmlTest2("ab]");
        wstruct_test_.set_xmlTest3("abc]]");
        boost::system::error_code ec;
        wstruct_test_.set_ipaddrv4Test(address::from_string("10.10.1.1", ec));
        wstruct_test_.set_ipaddrv6Test(address::from_string("2001:db8::2:1", ec));
        std::vector<address> ipaddrs;
        ipaddrs.push_back(address::from_string("192.168.1.10", ec));
        ipaddrs.push_back(address::from_string("2001:dc8::1:2", ec));
        wstruct_test_.set_ipaddrListTest(ipaddrs);
        // Write the struct
        wxfer = wstruct_test_.write(prot);
        // Verify Default values
        EXPECT_EQ(true, wstruct_test_.get_uuidDefaultTest() ==
             boost::uuids::string_generator()
             ("00010203-0405-0607-0423-023434265323"));
        // Get the buffer
        btrans->getBuffer(&buffer, &offset);
        EXPECT_EQ(wxfer, offset);
        // Read the struct
        rxfer = rstruct_test_.read(prot);
        // Now compare
        EXPECT_EQ(rxfer, wxfer);
        EXPECT_EQ(wstruct_test_, rstruct_test_);
    }

    void SandeshJsonReadWriteProcess(boost::shared_ptr<TMemoryBuffer> btrans,
            boost::shared_ptr<TProtocol> prot) {
        uint8_t *buffer;
        uint32_t offset, wxfer, rxfer;
        wjson_struct_test_.set_name("abc");
        // Initialize the struct
        wjson_struct_test_.set_i32Test(test_i32);
        // Create a vector of SandeshStructTest
        std::vector<SandeshListTestElement> test_list;
        for (uint32_t i = 0; i < test_list_size; i++) {
            SandeshListTestElement tmp;
            tmp.set_i32Elem(i);
            test_list.push_back(tmp); // 0, 1, 2, 3, 4
        }
        wjson_struct_test_.set_listTest(test_list);
        // Create a vector of int32_t
        std::vector<int32_t> test_basic_type_list;
        for (uint32_t i = 0; i < test_list_size; i++) {
            test_basic_type_list.push_back(i); // 0, 1, 2, 3, 4
        }
        wjson_struct_test_.set_basicTypeListTest(test_basic_type_list);
        // Create a vector of uuid
        std::vector<boost::uuids::uuid> test_uuid_list;
        for (uint32_t i = 0; i < test_list_size; i++) {
            boost::uuids::uuid uuid_temp =
                {0x00+i,0x00+i,0x01+i,0x01+i,0x02+i,0x02+i,0x03+i,0x03+i,
                 0x04+i,0x04+i,0x05+i,0x05+i,0x06+i,0x06+i,0x07+i,0x07+i};
            test_uuid_list.push_back(uuid_temp);
        }
        wjson_struct_test_.set_uuidListTest(test_uuid_list);
        // Create a map of <int32_t, string>
        std::map<int32_t, std::string> test_basic_type_map;
        for (uint32_t i = 0; i < test_list_size; i++) {
            // (0, "0"), (1, "1"), (2, "2"), (3, "3"), (4, "4")
            test_basic_type_map.insert(std::pair<int32_t, std::string>
                                       (i, "a"+integerToString(i)));
        }
        wjson_struct_test_.set_basicTypeMapTest(test_basic_type_map);
        // Create a map of <int32_t, SandeshListTestElement>
        std::map<int32_t, SandeshListTestElement> test_complex_type_map;
        for (uint32_t i = 0; i < test_list_size; i++) {
            SandeshListTestElement tmp;
            tmp.set_i32Elem(i);
            test_complex_type_map.insert(std::pair<int32_t, SandeshListTestElement>
                                         (i, tmp));
        }
        wjson_struct_test_.set_complexTypeMapTest(test_complex_type_map);
        wjson_struct_test_.set_u16Test(65535);
        wjson_struct_test_.set_u32Test(4294967295u);
        wjson_struct_test_.set_u64Test(18446744073709551615ull);
        wjson_struct_test_.set_ipv4Test(4294967295u);
        boost::uuids::uuid uuid_test =
             {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
              0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
        wjson_struct_test_.set_uuidTest(uuid_test);
        boost::system::error_code ec;
        wjson_struct_test_.set_ipaddrv4Test(address::from_string("10.10.1.1", ec));
        wjson_struct_test_.set_ipaddrv6Test(address::from_string("2001:db8::2:1", ec));
        std::vector<address> ipaddrs;
        ipaddrs.push_back(address::from_string("192.168.1.10", ec));
        ipaddrs.push_back(address::from_string("2001:dc8::1:2", ec));
        wjson_struct_test_.set_ipaddrListTest(ipaddrs);
        SandeshInnerStructElement innerstruct;
        innerstruct.stringInnerTest = "InnerStructElement";
        innerstruct.u16InnerTest = 16;
        wjson_struct_test_.set_innerStructTest(innerstruct);
        vector<vector<int> >  v(2, vector<int>(2, 1));
        wjson_struct_test_.set_innserListOfListTest(v);
        wjson_struct_test_.set_boolTest(true);
        std::set<int> set_test;
        set_test.insert(1);
        set_test.insert(2);
        set_test.insert(3);
        wjson_struct_test_.set_setBasic(set_test);
        // Write the struct
        wxfer = wjson_struct_test_.write(prot);
        // Verify Default values
        EXPECT_EQ(true, wjson_struct_test_.get_uuidDefaultTest() ==
             boost::uuids::string_generator()
             ("00010203-0405-0607-0423-023434265323"));
        // Get the buffer
        btrans->getBuffer(&buffer, &offset);
        uint8_t stream[4096];
        uint8_t *data = stream;
        std::string data_s;
        data_s.append((char *)buffer, offset);
    	std::cout << " buffer len " << offset << std::endl;
    	std::cout << " buffer val " << data_s << std::endl;
        std::string expected_json="{\"VAL\":{\"name\":{\"TYPE\":\"string\",\"ANNOTATION\":{\"key\":\"ObjectCollectorInfo\"},\"VAL\":\"abc\"},\"i32Test\":{\"TYPE\":\"i32\",\"VAL\":-559038737},\"listTest\":{\"TYPE\":\"list\",\"ANNOTATION\":{\"tags\":\".i32Elem\"},\"VAL\":{\"VAL\":[{\"VAL\":{\"i32Elem\":{\"TYPE\":\"i32\",\"VAL\":0}}},{\"VAL\":{\"i32Elem\":{\"TYPE\":\"i32\",\"VAL\":1}}},{\"VAL\":{\"i32Elem\":{\"TYPE\":\"i32\",\"VAL\":2}}},{\"VAL\":{\"i32Elem\":{\"TYPE\":\"i32\",\"VAL\":3}}},{\"VAL\":{\"i32Elem\":{\"TYPE\":\"i32\",\"VAL\":4}}}]}},\"basicTypeListTest\":{\"TYPE\":\"list\",\"ANNOTATION\":{\"tags\":\"\"},\"VAL\":{\"VAL\":[0,1,2,3,4]}},\"basicTypeMapTest\":{\"TYPE\":\"map\",\"ANNOTATION\":{\"tags\":\"\"},\"VAL\":{\"KEY\":\"i32\",\"VALUE\":\"string\",\"VAL\":{\"0\":\"a0\",\"1\":\"a1\",\"2\":\"a2\",\"3\":\"a3\",\"4\":\"a4\"}}},\"complexTypeMapTest\":{\"TYPE\":\"map\",\"VAL\":{\"KEY\":\"i32\",\"VALUE\":\"struct\",\"VAL\":{\"0\":{\"VAL\":{\"i32Elem\":{\"TYPE\":\"i32\",\"VAL\":0}}},\"1\":{\"VAL\":{\"i32Elem\":{\"TYPE\":\"i32\",\"VAL\":1}}},\"2\":{\"VAL\":{\"i32Elem\":{\"TYPE\":\"i32\",\"VAL\":2}}},\"3\":{\"VAL\":{\"i32Elem\":{\"TYPE\":\"i32\",\"VAL\":3}}},\"4\":{\"VAL\":{\"i32Elem\":{\"TYPE\":\"i32\",\"VAL\":4}}}}}},\"u16Test\":{\"TYPE\":\"u16\",\"VAL\":65535},\"u32Test\":{\"TYPE\":\"u32\",\"VAL\":4294967295},\"u64Test\":{\"TYPE\":\"u64\",\"VAL\":18446744073709551615},\"ipv4Test\":{\"TYPE\":\"ipv4\",\"VAL\":4294967295},\"uuidTest\":{\"TYPE\":\"uuid_t\",\"VAL\":\"00010203-0405-0607-0809-0a0b0c0d0e0f\"},\"uuidListTest\":{\"TYPE\":\"list\",\"VAL\":{\"VAL\":[\"00000101-0202-0303-0404-050506060707\",\"01010202-0303-0404-0505-060607070808\",\"02020303-0404-0505-0606-070708080909\",\"03030404-0505-0606-0707-080809090a0a\",\"04040505-0606-0707-0808-09090a0a0b0b\"]}},\"uuidDefaultTest\":{\"TYPE\":\"uuid_t\",\"VAL\":\"00010203-0405-0607-0423-023434265323\"},\"ipaddrv4Test\":{\"TYPE\":\"ipaddr\",\"VAL\":\"10.10.1.1\"},\"ipaddrv6Test\":{\"TYPE\":\"ipaddr\",\"VAL\":\"2001:db8::2:1\"},\"ipaddrListTest\":{\"TYPE\":\"list\",\"ANNOTATION\":{\"tags\":\"\"},\"VAL\":{\"VAL\":[\"192.168.1.10\",\"2001:dc8::1:2\"]}},\"innerStructTest\":{\"TYPE\":\"struct\",\"VAL\":{\"VAL\":{\"stringInnerTest\":{\"TYPE\":\"string\",\"VAL\":\"InnerStructElement\"},\"u16InnerTest\":{\"TYPE\":\"u16\",\"VAL\":16}}}},\"innserListOfListTest\":{\"TYPE\":\"list\",\"VAL\":{\"VAL\":[{\"VAL\":[\"1\",\"1\"]},{\"VAL\":[\"1\",\"1\"]}]}},\"boolTest\":{\"TYPE\":\"bool\",\"VAL\":true},\"setBasic\":{\"TYPE\":\"set\",\"VAL\":{\"VAL\":[1,2,3]}}}}";
        EXPECT_EQ(wxfer, offset);
        EXPECT_EQ(data_s, expected_json);
    }

    SandeshStructTest wstruct_test_;
    SandeshStructTest rstruct_test_;
    SandeshStructJsonTest wjson_struct_test_;
};


TEST_F(SandeshReadWriteUnitTest, StructXMLReadWrite) {
    boost::shared_ptr<TMemoryBuffer> btrans =
            boost::shared_ptr<TMemoryBuffer>(
                    new TMemoryBuffer(4096));
    boost::shared_ptr<TXMLProtocol> prot =
            boost::shared_ptr<TXMLProtocol>(
                    new TXMLProtocol(btrans));
    SandeshReadWriteProcess(btrans, prot);
}

TEST_F(SandeshReadWriteUnitTest, StructBinaryReadWrite) {
    boost::shared_ptr<TMemoryBuffer> btrans =
            boost::shared_ptr<TMemoryBuffer>(
                    new TMemoryBuffer(4096));
    boost::shared_ptr<TBinaryProtocol> prot =
            boost::shared_ptr<TBinaryProtocol>(
                    new TBinaryProtocol(btrans));
    SandeshReadWriteProcess(btrans, prot);
}


TEST_F(SandeshReadWriteUnitTest, StructJSONReadWrite) {
    boost::shared_ptr<TMemoryBuffer> btrans =
            boost::shared_ptr<TMemoryBuffer>(
                    new TMemoryBuffer(4096));
    boost::shared_ptr<TJSONProtocol> prot =
            boost::shared_ptr<TJSONProtocol>(
                    new TJSONProtocol(btrans));
    prot->setSandeshEnd(false);
    SandeshJsonReadWriteProcess(btrans, prot);
}

class SandeshLogUnitTest : public ::testing::Test {
protected:
    SandeshLogUnitTest() {
    }

    SandeshLogTest ltest_;
};

TEST_F(SandeshLogUnitTest, Basic) {
    ltest_.set_byteTest(255);
    ltest_.set_byteTest1(120);
    ltest_.set_boolTest(true);
    std::string buf = ltest_.log();
    EXPECT_STREQ("byteTest = -1 byteTest1 = 120 boolTest = 1", buf.c_str());
}

int main(int argc, char **argv) {
    LoggingInit();
    ::testing::InitGoogleTest(&argc, argv);
    bool success = RUN_ALL_TESTS();
    TaskScheduler::GetInstance()->Terminate();
    return success;
}
