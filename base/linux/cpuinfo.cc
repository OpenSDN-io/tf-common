/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 * Copyright Contributors to the OpenSDN project
 */

#include "sys/times.h"
#include <cstdlib>
#include <base/cpuinfo.h>

#include <fstream>
#include <iostream>

#include <boost/algorithm/string/find.hpp>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace boost;

uint32_t NumCpus() {
    static uint32_t count = 0;

    if (count != 0) {
        return count;
    }

    std::ifstream file("/proc/cpuinfo");
    std::string content((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
    // Create a find_iterator
    typedef find_iterator<std::string::iterator> string_find_iterator;

    for (string_find_iterator it =
        make_find_iterator(content, first_finder("model name", is_iequal()));
        it != string_find_iterator(); ++it, count++);
    return count;
}

void LoadAvg(CpuLoad &load) {
    double averages[3];
    uint32_t num_cpus = NumCpus();
    getloadavg(averages, 3);
    if (num_cpus > 0) {
        load.one_min_avg = averages[0]/num_cpus;
        load.five_min_avg = averages[1]/num_cpus;
        load.fifteen_min_avg = averages[2]/num_cpus;
    }
}



void ProcessMemInfo(ProcessMemInfo &info) {
    std::ifstream file("/proc/self/status");
    bool vmsize = false;
    bool peak = false;
    bool rss = false;
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("VmSize") != std::string::npos) {
            std::stringstream vm(line);
            std::string tmp; vm >> tmp; vm >> info.virt;
            vmsize = true;
        }   
        if (line.find("VmRSS") != std::string::npos) {
            std::stringstream vm(line);
            std::string tmp; vm >> tmp; vm >> info.res;
            rss = true;
        }   
        if (line.find("VmPeak") != std::string::npos) {
            std::stringstream vm(line);
            std::string tmp; vm >> tmp; vm >> info.peakvirt;
            peak = true;
        }   
        if (rss && vmsize && peak)
            break;
    }
}

void SystemMemInfo(SystemMemInfo &info) {
    std::ifstream file("/proc/meminfo");
    std::string tmp;
    // MemTotal:       132010576 kB
    file >> tmp; file >> info.total; file >> tmp; 
    // MemFree:        90333184 kB
    file >> tmp; file >> info.free; file >> tmp; 
    // Buffers:         1029924 kB
    file >> tmp; file >> info.buffers; file >> tmp;
    // Cached:         10290012 kB
    file >> tmp; file >> info.cached;
    // Used = Total - Free
    info.used = info.total - info.free;
}