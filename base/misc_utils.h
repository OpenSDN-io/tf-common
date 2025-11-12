/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

#ifndef ctrlplane_misc_utils_h
#define ctrlplane_misc_utils_h

#include <sandesh/sandesh_trace.h>
#include <sandesh/common/vns_types.h>
#include <sandesh/common/vns_constants.h>
#include <string>
#include <vector>
#include <map>
#include <ctime>

#define VERSION_TRACE_BUF "VersionTrace"

extern SandeshTraceBufferPtr VersionTraceBuf;

#define VERSION_TRACE(obj, ...) do {                                  \
    obj::TraceMsg(VersionTraceBuf, __FILE__, __LINE__, ##__VA_ARGS__); \
} while (false)

#define VERSION_LOG(obj, categ, ...)\
    do {\
            obj::Send(g_vns_constants.CategoryNames.find(categ)->second,\
                                  SandeshLevel::SYS_INFO, __FILE__, __LINE__, ##__VA_ARGS__);\
    } while (false)

#if (__cplusplus < 201103L)
   #define ARRAYBYTES_FILL(obj, val) (obj).assign((val));
   #define GENERIC_RAW_ARRAY(obj) obj.c_array()
#else
   #define ARRAYBYTES_FILL(obj, val) (obj).fill((val));
   #define GENERIC_RAW_ARRAY(obj) obj.data()
#endif


class MiscUtils {
public:
    enum BuildModule {
        Agent,
        Analytics,
        ControlNode,
        Dns,
        MaxModules
    };
    static std::map<BuildModule, std::string> MapInit() {
        std::map<BuildModule, std::string> m;
         m[Agent] = "contrail-vrouter-agent ";
         m[Analytics] = "contrail-analytics ";
         m[ControlNode] = "contrail-control ";
         m[Dns] = "contrail-dns ";
         return m;
    }
    static const std::map<BuildModule, std::string> BuildModuleNames;
    typedef std::multimap<std::time_t, std::string> FileMMap;
    static const std::string ContrailVersionCmd;
    static const std::string CoreFileDir;
    static const int MaxCoreFiles;

    static void GetHostIp(const std::string name, std::vector<std::string> &ip_list);
    static void LogVersionInfo(const std::string str, Category::type categ);
    static time_t GetUpTimeSeconds();
    static time_t set_startup_time_secs();

private:
    static std::string BaseName(std::string filename);
    static time_t startup_time_secs_;
};

#endif // ctrlplane_misc_utils_h
