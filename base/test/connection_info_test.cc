//
// Copyright (c) 2014 Juniper Networks, Inc. All rights reserved.
// Copyright Contributors to the OpenSDN project
//

#include "testing/gunit.h"

#include <boost/system/error_code.hpp>
#include "base/connection_info.h"
#include "io/event_manager.h"

using process::ConnectionStateManager;
using process::ConnectionState;
using process::ConnectionInfo;
using process::FlagManager;
using process::FlagConfigManager;
using process::Flag;
using process::FlagConfig;
using process::FlagState;
using process::FlagContext;
using process::ContextVec;
using process::FlagVec;
using process::FlagConfigVec;
using process::ProcessState;
using process::ConnectionStatus;
using process::ConnectionType;
using process::g_process_info_constants;
using process::GetProcessStateCb;
using process::ConnectionTypeName;

class ConnectionInfoTest : public ::testing::Test {
 protected:
    static void SetUpTestCase() {
        std::vector<ConnectionTypeName> expected_connections = boost::assign::list_of
         (ConnectionTypeName("Test", "Test1"))
         (ConnectionTypeName("Test", "Test2"));
        ConnectionStateManager::
            GetInstance()->Init(*evm_.io_service(), "Test",
            "ConnectionInfoTest", "0", boost::bind(
            &process::GetProcessStateCb, _1, _2, _3, expected_connections), "ObjectTest");
        string build = "{\"build-info\":                                           \
                           [{                                                      \
                             \"build-time\": \"2020-01-29 01:13:56.160282\",       \
                             \"build-hostname\": \"ubuntu-build03\",               \
                             \"build-user\": \"maheshskumar\",                     \
                             \"build-version\": \"1910\"                           \
                           }]                                                      \
                        }";
        flag_config_mgr_ = FlagConfigManager::GetInstance();
        flag_config_mgr_->Initialize(build);
        flag_mgr_ = FlagManager::GetInstance();
    }
    static void TearDownTestCase() {
        ConnectionStateManager::
            GetInstance()->Shutdown();
    }
    void PopulateConnInfo(ConnectionInfo *cinfo, const std::string &name,
        ConnectionStatus::type status, const std::string &description) {
        cinfo->set_type(g_process_info_constants.ConnectionTypeNames.find(
            ConnectionType::TEST)->second);
        cinfo->set_name(name);
        std::string eps("127.0.0.1:0");
        std::vector<std::string> epsv(1, eps);
        cinfo->set_server_addrs(epsv);
        cinfo->set_status(g_process_info_constants.ConnectionStatusNames.find(
            status)->second);
        cinfo->set_description(description);
    }
    void UpdateConnInfo(const std::string &name, ConnectionStatus::type status,
        const std::string &description, std::vector<ConnectionInfo> *vcinfo) {
        ConnectionInfo cinfo;
        PopulateConnInfo(&cinfo, name, status, description);
        vcinfo->push_back(cinfo);
    }
    void UpdateConnState(const std::string &name, ConnectionStatus::type status,
        const std::string &description,
        const std::vector<ConnectionInfo> &vcinfo) {
        boost::system::error_code ec;
        boost::asio::ip::address addr(boost::asio::ip::address::from_string(
            "127.0.0.1", ec));
        ASSERT_EQ(0, ec.value());
        process::Endpoint ep(addr, 0);
        // Set callback
        ConnectionStateManager::
            GetInstance()->SetProcessStateCb(boost::bind(
                &ConnectionInfoTest::VerifyProcessStateCb, this, _1, _2, _3,
                vcinfo));
        // Update
        ConnectionState::GetInstance()->Update(ConnectionType::TEST, name,
            status, ep, description);
    }
    void DeleteConnInfo(const std::string &name,
        std::vector<ConnectionInfo> *vcinfo) {
        const std::string ctype(
            g_process_info_constants.ConnectionTypeNames.find(
                ConnectionType::TEST)->second);
        for (size_t i = 0; i < vcinfo->size(); i++) {
            ConnectionInfo &tinfo(vcinfo->at(i));
            if (tinfo.get_type() == ctype &&
                tinfo.get_name() == name) {
                vcinfo->erase(vcinfo->begin() + i);
            }
        }
    }
    void DeleteConnState(const std::string &name,
        const std::vector<ConnectionInfo> &vcinfo) {
        // Set callback
        ConnectionStateManager::
            GetInstance()->SetProcessStateCb(boost::bind(
                &ConnectionInfoTest::VerifyProcessStateCb, this, _1, _2, _3,
                vcinfo));
        // Delete
        ConnectionState::GetInstance()->Delete(ConnectionType::TEST, name);
    }
    void CheckFlag(const string &name,
                   const ContextVec &c_vec,
                   bool enabled,
                   bool dflt = false) {
        bool result = flag_mgr_->IsFlagEnabled(name, dflt, c_vec);
        EXPECT_EQ(result, enabled);
    }
    void CheckFlag(Flag *flag, bool res) {
        EXPECT_EQ(flag->enabled(), res);
    }
    void ConfigureFlag(const std::string &name, const std::string version,
                       bool enabled, ContextVec &c_vec) {
        flag_config_mgr_->Set(name, version, enabled, FlagState::EXPERIMENTAL,
                              c_vec);
    }
    void UnconfigureFlag(const string& name) {
        flag_config_mgr_->Unset(name);
    }
    void RegisterFlag(Flag *&flag, const std::string &name,
        const std::string &desc, ContextVec &c_vec, bool dflt = false) {
        flag = new Flag(flag_mgr_, name, desc, dflt, c_vec);
        bool interest = flag_mgr_->IsRegistered(flag);
        EXPECT_EQ(interest, true);
    }
    void UnregisterFlag(Flag *&flag) {
        delete(flag);
    }
    void VerifyFlagMapSize(int count) {
        int size = flag_mgr_->GetFlagMapCount();
        EXPECT_EQ(size, count);
    }
    void VerifyFlagInfoCount(int count) {
        FlagConfigVec flag_vec = flag_mgr_->GetFlagInfos();
        EXPECT_EQ(flag_vec.size(), count);
    }
    void VerifyIntMapSize(int count) {
        int size = flag_mgr_->GetIntMapCount();
        EXPECT_EQ(size, count);
    }
    void VerifyProcessStateCb(const std::vector<ConnectionInfo> &cinfos,
        ProcessState::type &state, std::string &message,
        const std::vector<ConnectionInfo> &ecinfos) {
        state = ProcessState::FUNCTIONAL;
        EXPECT_EQ(ecinfos, cinfos);
    }

    static EventManager evm_;
    static FlagManager *flag_mgr_;
    static FlagConfigManager *flag_config_mgr_;
};

EventManager ConnectionInfoTest::evm_;
FlagManager *ConnectionInfoTest::flag_mgr_;
FlagConfigManager *ConnectionInfoTest::flag_config_mgr_;

TEST_F(ConnectionInfoTest, FlagTest) {
    ContextVec c_vec, c_vec1, c_vec2;

    // Add module interest for "Feature Ten" with default enabled
    // Since there is no user configuration, library should return
    // the default state in the module.
    Flag *flag10 = NULL;
    RegisterFlag(flag10, "Feature Ten", "Feature Ten Description", c_vec, true);
    VerifyIntMapSize(1);
    CheckFlag("Feature Ten", c_vec, true, true);
    CheckFlag(flag10, true);

    // Remove module interest for "Feature Three"
    UnregisterFlag(flag10);
    VerifyIntMapSize(0);

    // ------------------------------------------------------------------

    // -- Feature One --
    // Configure and enable a flag with no context info
    ConfigureFlag("Feature One", "1910", true, c_vec);
    VerifyFlagInfoCount(0);
    VerifyFlagMapSize(1);

    // Add module interest for "Feature One"
    Flag *flag1a = NULL;
    RegisterFlag(flag1a, "Feature One", "Feature One Description", c_vec);
    VerifyFlagInfoCount(1);
    VerifyIntMapSize(1);
    CheckFlag(flag1a, true);

    // Add module interest for "Feature One" with different state
    Flag *flag1b = NULL;
    RegisterFlag(flag1b, "Feature One", "Feature Another Description", c_vec);
    VerifyFlagInfoCount(1);
    VerifyIntMapSize(2);
    CheckFlag(flag1b, true);

    // Remove module interest for "Feature One" in EXPERIMENTAL state
    UnregisterFlag(flag1a);
    VerifyFlagInfoCount(1);
    VerifyIntMapSize(1);

    // Remove module interest for "Feature One" in ALPHA state
    UnregisterFlag(flag1b);
    VerifyFlagInfoCount(0);
    VerifyIntMapSize(0);

    // ------------------------------------------------------------------

    // -- Feature Two --
    // Configure and disable a flag with no context info
    ConfigureFlag("Feature Two", "1910", false, c_vec);
    VerifyFlagInfoCount(0);
    VerifyFlagMapSize(2);
    CheckFlag("Feature Two", c_vec, false);

    // Update "Feature Two" flag and change it to enabled
    ConfigureFlag("Feature Two", "1910", true, c_vec);
    VerifyFlagInfoCount(0);
    VerifyFlagMapSize(2);
    CheckFlag("Feature Two", c_vec, true);

    // Add module interest for "Feature Two"
    Flag *flag2 = NULL;
    RegisterFlag(flag2, "Feature Two", "Feature Two Description", c_vec);
    VerifyFlagInfoCount(1);
    VerifyIntMapSize(1);
    CheckFlag(flag2, true);

    // Remove module interest for "Feature Two"
    UnregisterFlag(flag2);
    VerifyFlagInfoCount(0);
    VerifyIntMapSize(0);

    // ------------------------------------------------------------------

    // -- Feature Three --
    // Add context info
    FlagContext c_info1("interface", "one");
    c_vec.push_back(c_info1);

    // Configure and disable a flag with context info
    ConfigureFlag("Feature Three", "1910",  false, c_vec);

    // Check if flag is enabled for null context.
    CheckFlag("Feature Three", c_vec1, false);
    VerifyFlagInfoCount(0);
    VerifyFlagMapSize(3);

    // Update context info for "Feature Three"
    FlagContext c_info2("interface", "two");
    c_vec.push_back(c_info2);
    c_vec1.push_back(c_info2);
    ConfigureFlag("Feature Three", "1910", false, c_vec);

    // Check if "Feature Three" is enabled on "interface two"
    VerifyFlagInfoCount(0);
    VerifyFlagMapSize(3);
    CheckFlag("Feature Three", c_vec1, false);

    // Add module interest for "Feature Three" with default state enabled
    // Since there is user configuration with flag in disabled state it
    // should override the default "enabled" state
    Flag *flag3;
    RegisterFlag(flag3, "Feature Three", "Feature Three Description",
                 c_vec, true);
    VerifyFlagInfoCount(1);
    VerifyIntMapSize(1);
    CheckFlag(flag3, false);

    // Remove module interest for "Feature Three"
    UnregisterFlag(flag3);
    VerifyFlagInfoCount(0);
    VerifyIntMapSize(0);

    // ------------------------------------------------------------------

    // -- Feature Four --
    // Configure and disable a flag with context info
    ConfigureFlag("Feature Four", "1910", false, c_vec);
    VerifyFlagInfoCount(0);
    VerifyFlagMapSize(4);
    CheckFlag("Feature Four", c_vec, false);

    // ------------------------------------------------------------------

    // -- Feature Five --
    // Add context info
    FlagContext c_info3("compute", "three");
    c_vec.push_back(c_info3);
    c_vec1.push_back(c_info3);
    FlagContext c_info4("compute", "four");
    c_vec2.push_back(c_info4);

    // Configure and enable a flag with context info
    ConfigureFlag("Feature Five", "1910", true, c_vec);
    VerifyFlagInfoCount(0);
    VerifyFlagMapSize(5);
    CheckFlag("Feature Five", c_vec1, true);
    CheckFlag("Feature Five", c_vec2, false);

    // ------------------------------------------------------------------

    // -- Feature Six --
    // Configure and disable a flag with context info
    ConfigureFlag("Feature Six", "1910", false, c_vec);
    VerifyFlagInfoCount(0);
    VerifyFlagMapSize(6);
    CheckFlag("Feature Six", c_vec, false);

    // Configure the same flag with different version and make sure it
    // is ignored by the flag library
    ConfigureFlag("Feature Six", "1911", false, c_vec);
    VerifyFlagInfoCount(0);
    VerifyFlagMapSize(6);

    // ------------------------------------------------------------------

    UnconfigureFlag("Feature Six");
    VerifyFlagMapSize(5);
    UnconfigureFlag("Feature Five");
    VerifyFlagMapSize(4);
    UnconfigureFlag("Feature Four");
    VerifyFlagMapSize(3);
    UnconfigureFlag("Feature Three");
    VerifyFlagMapSize(2);
    UnconfigureFlag("Feature Two");
    VerifyFlagMapSize(1);
    UnconfigureFlag("Feature One");
    VerifyFlagMapSize(0);
}

TEST_F(ConnectionInfoTest, Basic) {
    std::vector<ConnectionInfo> vcinfo;
    // Verify update
    UpdateConnInfo("Test1", ConnectionStatus::UP, "Test1 UP", &vcinfo);
    UpdateConnState("Test1", ConnectionStatus::UP, "Test1 UP", vcinfo);
    UpdateConnInfo("Test2", ConnectionStatus::DOWN, "Test2 DOWN", &vcinfo);
    UpdateConnState("Test2", ConnectionStatus::DOWN, "Test2 DOWN", vcinfo);
    // Verify delete
    DeleteConnInfo("Test1", &vcinfo);
    DeleteConnState("Test1", vcinfo);
}

TEST_F(ConnectionInfoTest, Callback) {
    std::vector<ConnectionInfo> vcinfo;
    UpdateConnInfo("Test1", ConnectionStatus::UP, "Test1 UP", &vcinfo);
    ProcessState::type pstate;
    std::string message1;
    std::vector<ConnectionTypeName> expected_connections = boost::assign::list_of
         (ConnectionTypeName("Test", "Test1"));
    // Expected connection and conn_info are same
    GetProcessStateCb(vcinfo, pstate, message1, expected_connections);
    EXPECT_EQ(ProcessState::FUNCTIONAL, pstate);
    EXPECT_TRUE(message1.empty());
    std::string message2;
    expected_connections.push_back(ConnectionTypeName("Test","Test2"));
    // Expected connection more than conn_info
    GetProcessStateCb(vcinfo, pstate, message2, expected_connections);
    EXPECT_EQ(ProcessState::NON_FUNCTIONAL, pstate);
    EXPECT_EQ("Number of connections:1, Expected:2 Missing: Test:Test2", message2);
    // 2 expected connections are more than conn_info
    expected_connections.push_back(ConnectionTypeName("Test","Test3"));
    std::string message3;
    GetProcessStateCb(vcinfo, pstate, message3, expected_connections);
    EXPECT_EQ(ProcessState::NON_FUNCTIONAL, pstate);
    EXPECT_EQ("Number of connections:1, Expected:3 Missing: Test:Test2,Test:Test3", message3);
    expected_connections.pop_back();
    UpdateConnInfo("Test2", ConnectionStatus::DOWN, "Test2 DOWN", &vcinfo);
    std::string message4;
    GetProcessStateCb(vcinfo, pstate, message4, expected_connections);
    EXPECT_EQ(ProcessState::NON_FUNCTIONAL, pstate);
    EXPECT_EQ("Test:Test2 connection down", message4);
    UpdateConnInfo("Test3", ConnectionStatus::DOWN, "Test3 DOWN", &vcinfo);
    std::string message5;
    // More connection in conn_info than expected_connections
    GetProcessStateCb(vcinfo, pstate, message5, expected_connections);
    EXPECT_EQ(ProcessState::NON_FUNCTIONAL, pstate);
    EXPECT_EQ("Number of connections:3, Expected:2 Extra: Test:Test3", message5);
    std::string message6;
    expected_connections.push_back(ConnectionTypeName("Test","Test3"));
    GetProcessStateCb(vcinfo, pstate, message6, expected_connections);
    EXPECT_EQ(ProcessState::NON_FUNCTIONAL, pstate);
    EXPECT_EQ("Test:Test2, Test:Test3 connection down", message6);
}

int main(int argc, char *argv[]) {
    LoggingInit();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
