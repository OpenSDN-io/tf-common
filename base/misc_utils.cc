/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <base/misc_utils.h>
#include <base/logging.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "base/sandesh/version_types.h"
#include "base/logging.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using namespace std;

const map<MiscUtils::BuildModule, string> MiscUtils::BuildModuleNames =
    MiscUtils::MapInit();
time_t MiscUtils::startup_time_secs_ = MiscUtils::set_startup_time_secs();

SandeshTraceBufferPtr VersionTraceBuf(SandeshTraceBufferCreate(
                                       VERSION_TRACE_BUF, 500));

string MiscUtils::BaseName(string filename) {
    size_t pos = filename.find_last_of('/');
    if (pos != string::npos) {
        return filename.substr((pos+1));
    }
    return filename;
}

void MiscUtils::LogVersionInfo(const string build_info, Category::type categ) {
    VERSION_TRACE(VersionInfoTrace, build_info);
    if (!LoggingDisabled()) {
        VERSION_LOG(VersionInfoLog, categ, build_info);
    }
}

time_t MiscUtils::GetUpTimeSeconds() {
    return (UTCTimestamp() - startup_time_secs_);
}

time_t MiscUtils::set_startup_time_secs() {
    return (startup_time_secs_ = UTCTimestamp());
}
