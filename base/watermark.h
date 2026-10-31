//
// Copyright (c) 2016 Juniper Networks, Inc. All rights reserved.
// Copyright Contributors to the OpenSDN project
//

// watermark.h
// class to handle multiple level high/low watermarks
// this is NOT thread-safe, caller needs take mutex.
//
#ifndef BASE_WATERMARK_H_
#define BASE_WATERMARK_H_

#include <set>
#include <boost/function.hpp>
#include <base/util.h>

// WaterMarkInfo
typedef boost::function<void (size_t)> WaterMarkCallback;

struct WaterMarkInfo {
    WaterMarkInfo(size_t count, WaterMarkCallback cb) :
        count_(count),
        cb_(cb) {
    }
    friend inline bool operator<(const WaterMarkInfo& lhs,
        const WaterMarkInfo& rhs);
    friend inline bool operator==(const WaterMarkInfo& lhs,
        const WaterMarkInfo& rhs);
    size_t count_;
    WaterMarkCallback cb_;
};

inline bool operator<(const WaterMarkInfo& lhs, const WaterMarkInfo& rhs) {
    return lhs.count_ < rhs.count_;
}

inline bool operator==(const WaterMarkInfo& lhs, const WaterMarkInfo& rhs) {
    return lhs.count_ == rhs.count_;
}

typedef std::set<WaterMarkInfo> WaterMarkInfos;

class WaterMarkTuple {
public:
    WaterMarkTuple();

    void SetHighWaterMark(const WaterMarkInfos &high_water);
    void SetHighWaterMark(const WaterMarkInfo& hwm_info);
    void ResetHighWaterMark();
    WaterMarkInfos GetHighWaterMark() const;
    void SetLowWaterMark(const WaterMarkInfos &low_water);
    void SetLowWaterMark(const WaterMarkInfo& lwm_info);
    void ResetLowWaterMark();
    WaterMarkInfos GetLowWaterMark() const;
    void ProcessWaterMarks(size_t in_count, size_t curr_count);
    bool AreWaterMarksSet() const;
    void ProcessHighWaterMarks(size_t count);
    void ProcessLowWaterMarks(size_t count);

private:
    WaterMarkInfos high_water_;
    WaterMarkInfos low_water_;
    size_t last_count_;

    DISALLOW_COPY_AND_ASSIGN(WaterMarkTuple);
};

#endif /* BASE_WATERMARK_H_ */
