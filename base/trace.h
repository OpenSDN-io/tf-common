/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

#ifndef __TRACE_H__
#define __TRACE_H__

#include <atomic>
#include <mutex>
#include <map>
#include <vector>
#include <stdexcept>

#include <boost/function.hpp>
#include <boost/ptr_container/ptr_circular_buffer.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include "base/util.h"

/// Manages a trace buffer's memory. A trace buffer is circular buffer with
/// the given size (count of records) and associated with the given name.
/// The type of records is specified during compilation using the template
/// parameter TraceEntryT. Trace buffers are organized into a table.
template<typename TraceEntryT>
class TraceBuffer {
public:

    /// The type defines how a map (a table) of trace buffers is stored.
    typedef std::map<const std::string,
                     boost::weak_ptr<TraceBuffer<TraceEntryT> > > TraceBufMap;

    /// Creates a new trace buffer with the given name and size (enable
    /// by default).
    TraceBuffer(const std::string& buf_name, size_t size, bool trace_enable)
        : trace_buf_name_(buf_name),
          trace_buf_size_(size),
          trace_buf_(trace_buf_size_),
          write_index_(0),
          read_index_(0),
          wrap_(false) {
        seqno_ = 0;
        trace_enable_ = trace_enable;
    }

    /// Destroys a trace buffer.
    ~TraceBuffer() {
        read_context_map_.clear();
        trace_buf_.clear();
    }

    /// Returns the name of the trace buffer.
    std::string Name() {
        return trace_buf_name_;
    }

    /// Enables the trace buffer.
    void TraceOn() {
        trace_enable_ = true;
    }

    /// Disables the trace buffer.
    void TraceOff() {
        trace_enable_ = false;
    }

    /// Determines whether the trace buffer is enabled or not.
    bool IsTraceOn() {
        return trace_enable_;
    }

    /// Returns the length (the maximum number of records) in the circular 
    /// buffer.
    size_t TraceBufSizeGet() {
        return trace_buf_size_;
    }

    /// Returns the length (the maximum number of records) in the circular 
    /// buffer.
    size_t TraceBufCapacityGet() {
        return trace_buf_.capacity();
    }

    /// Resets the size of the circular buffer.
    void TraceBufCapacityReset(size_t size) {
        trace_buf_.rset_capacity(size);
        trace_buf_size_ = size;
    }

    /// Writes the provided data into the circular buffer.
    void TraceWrite(TraceEntryT *trace_entry) {
        std::scoped_lock lock(mutex_);

        // Add the trace
        trace_buf_.push_back(trace_entry);

        // Once the trace buffer is wrapped, increment the read index
        if (wrap_) {
            if (++read_index_ == trace_buf_size_) {
                read_index_ = 0;
            }
        }

        // Increment the write_index_ and reset upon reaching trace_buf_size_
        if (++write_index_ == trace_buf_size_) {
            write_index_ = 0;
            wrap_ = true;
        }

        // Trace messages could be read in batches instead of reading
        // the entire trace buffer in one shot. Therefore, trace messages
        // could be added between subsequent read requests. If the
        // read_index_ [points to the oldest message in the trace buffer]
        // becomes same as the read index [points to the position in the
        // trace buffer from where the next trace message should be read]
        // stored in the read context, then there is no need to remember the
        // read context.
        ReadContextMap::iterator it = read_context_map_.begin();
        ReadContextMap::iterator next = it;
        for (size_t i = 0, cnt = read_context_map_.size(); i < cnt;
             i++, it = next) {
            ++next;
            if (*it->second.get() == read_index_) {
                read_context_map_.erase(it);
            }
        }
    }

    /// Returns the next sequence number.
    uint32_t GetNextSeqNum() {
        uint32_t nseqno(seqno_.fetch_add(1));
        // Reset seqno_ if it reaches max value
        if (nseqno+1 >= kMaxSeqno) {
            seqno_ = kMinSeqno;
        }
        return nseqno;
    }

    /// Reads the specified number of records from the buffer. Each records
    /// is submitted into the specified callback function.
    void TraceRead(const std::string& context, const int count,
            boost::function<void (TraceEntryT *, bool)> cb) {
        std::scoped_lock lock(mutex_);
        if (trace_buf_.empty()) {
            // No message in the trace buffer
            return;
        }

        // if count = 0, then set the cnt equal to the size of trace_buf_
        size_t cnt = count ? count : trace_buf_.size();

        size_t *read_index_ptr;
        typename ContainerType::iterator it;
        ReadContextMap::iterator context_it =
            read_context_map_.find(context);
        if (context_it != read_context_map_.end()) {
            // If the read context is present, manipulate the position
            // from where we wanna start
            read_index_ptr = context_it->second.get();
            size_t offset = *read_index_ptr - read_index_;
            offset = offset > 0 ? offset : trace_buf_size_ + offset;
            it = trace_buf_.begin() + offset;
        } else {
            // Create read context
            boost::shared_ptr<size_t> read_context(new size_t(read_index_));
            read_index_ptr = read_context.get();
            read_context_map_.insert(std::make_pair(context, read_context));
            it = trace_buf_.begin();
        }

        size_t i;
        typename ContainerType::iterator next = it;
        for (i = 0; (it != trace_buf_.end()) && (i < cnt); i++, it = next) {
            ++next;
            cb(&(*it), next != trace_buf_.end());
        }

        // Update the read index in the read context
        size_t offset = *read_index_ptr + i;
        *read_index_ptr = offset >= trace_buf_size_ ?
            offset - trace_buf_size_ : offset;
    }

    /// The member function is called to complete the reading of the
    /// circular buffer data.
    void TraceReadDone(const std::string& context) {
        std::scoped_lock lock(mutex_);
        ReadContextMap::iterator context_it =
            read_context_map_.find(context);
        if (context_it != read_context_map_.end()) {
            read_context_map_.erase(context_it);
        }
    }

private:

    /// Specifies the data type for storing records of the trace buffer.
    typedef boost::ptr_circular_buffer<TraceEntryT> ContainerType;

    /// Specifies the read context for the trace buffer.
    typedef std::map<const std::string, boost::shared_ptr<size_t> >
        ReadContextMap;

    /// Stores the name of the trace buffer.
    std::string trace_buf_name_;

    /// Stores the size of the trace buffer.
    size_t trace_buf_size_;

    /// Stores the records of the trace buffer.
    ContainerType trace_buf_;

    /// A flag to determine whether the trace buffer is enabled (ready
    /// for reading and writing).
    std::atomic<bool> trace_enable_;

    /// Points to the position in the trace buffer
    /// where the next trace message would be added
    size_t write_index_;

    /// Points to the position of the oldest
    /// trace message in the trace buffer
    size_t read_index_;

    /// Indicates if the trace buffer is wrapped
    bool wrap_;

    /// Stores the read context
    ReadContextMap read_context_map_;

    /// Stores the current sequence number.
    std::atomic<uint32_t> seqno_;

    /// Used to restrict simulateneous access to the trace buffer data
    /// from 2 threads
    std::mutex mutex_;

    /// Reserves max(uint32_t)
    static const uint32_t kMaxSeqno = ((2 ^ 32) - 1) - 1;

    /// Reserves 0
    static const uint32_t kMinSeqno = 1;

    DISALLOW_COPY_AND_ASSIGN(TraceBuffer);
};

/// The class is responsible for the destruction of a trace buffer.
template<typename TraceEntryT>
class TraceBufferDeleter {
public:

    /// A link to the trace buffers table type.
    using TraceBufMap = typename TraceBuffer<TraceEntryT>::TraceBufMap;

    /// Creates a new instance of this class using the given trace buffer table
    /// and a mutex object.
    explicit TraceBufferDeleter(TraceBufMap &trace_buf_map, std::mutex &mutex) :
            trace_buf_map_(trace_buf_map),
            mutex_(mutex) {
    }

    /// Performs the deletion of the trace buffer from the given map.
    void operator()(TraceBuffer<TraceEntryT> *trace_buffer) const {
        std::scoped_lock lock(mutex_);
        for (typename TraceBufMap::iterator it = trace_buf_map_.begin();
             it != trace_buf_map_.end();
             it++) {
            if (it->second.lock() == NULL) {
                trace_buf_map_.erase(it->first);
                delete trace_buffer;
                break;
            }
        }
    }

private:

    /// A reference to the trace buffers table.
    TraceBufMap &trace_buf_map_;

    /// A reference to the mutex object.
    std::mutex &mutex_;
};

/// The table for managing trace buffers using a map between their names
/// and instances. The table is a singletone, the memory for its records
/// is managed by the user (only weak pointers are stored in the table).
template<typename TraceEntryT>
class Trace {
public:

    /// A link to the trace buffers table type.
    using TraceBufMap = typename TraceBuffer<TraceEntryT>::TraceBufMap;

    /// Returns a pointer to the trace buffers table instance.
    static Trace* GetInstance() {
        if (!trace_) {
            trace_ = new Trace;
        }
        return trace_;
    }

    /// Enables tracing for the table.
    void TraceOn() {
        trace_enable_ = true;
    }

    /// Disables tracing for the table.
    void TraceOff() {
        trace_enable_ = false;
    }

    /// Determines whether tracing is enabled for the table.
    bool IsTraceOn() {
        return trace_enable_;
    }

    /// Returns a pointer to the trace buffer associated with the given name.
    /// If there is no such a trace buffer, then an empty one is returned.
    boost::shared_ptr<TraceBuffer<TraceEntryT> > TraceBufGet(const std::string& buf_name) {
        std::scoped_lock lock(mutex_);
        typename TraceBufMap::iterator it = trace_buf_map_.find(buf_name);
        if (it != trace_buf_map_.end()) {
            return it->second.lock();
        }
        return boost::shared_ptr<TraceBuffer<TraceEntryT> >();
    }

    /// Adds a trace buffer with the given name and size and returns a
    /// reference to it. Returns a shared_ptr of the trace buffer for the
    /// memory management.
    boost::shared_ptr<TraceBuffer<TraceEntryT> > TraceBufAdd(const std::string& buf_name, size_t size,
                     bool trace_enable) {
        // should we have a default size for the buffer?
        if (!size) {
            return boost::shared_ptr<TraceBuffer<TraceEntryT> >();
        }
        std::scoped_lock lock(mutex_);
        typename TraceBufMap::iterator it = trace_buf_map_.find(buf_name);
        if (it == trace_buf_map_.end()) {
            boost::shared_ptr<TraceBuffer<TraceEntryT> > trace_buf(
                new TraceBuffer<TraceEntryT>(buf_name, size, trace_enable),
                TraceBufferDeleter<TraceEntryT>(trace_buf_map_, mutex_));
            trace_buf_map_.insert(std::make_pair(buf_name, trace_buf));
            return trace_buf;
        }
        return it->second.lock();
    }

    /// Requests the list of trace buffers names from the table.
    void TraceBufListGet(std::vector<std::string>& trace_buf_list) {
        std::scoped_lock lock(mutex_);
        typename TraceBufMap::iterator it;
        for (it = trace_buf_map_.begin(); it != trace_buf_map_.end(); ++it) {
            trace_buf_list.push_back(it->first);
        }
    }

    /// Returns the capacity of the trace buffer with the given name.
    size_t TraceBufCapacityGet(const std::string& buf_name) {
        std::scoped_lock lock(mutex_);
        typename TraceBufMap::iterator it = trace_buf_map_.find(buf_name);
        if (it != trace_buf_map_.end()) {
            boost::shared_ptr<TraceBuffer<TraceEntryT> > trace_buf =
                                                             it->second.lock();
            return trace_buf->TraceBufCapacityGet();
        } else {
            return 0;
        }
    }

    /// Sets a new size of the trace buffer with the given name. If the trace
    /// buffer with the specified with given name is not found, then an empty
    /// trace buffer is returned.
    boost::shared_ptr<TraceBuffer<TraceEntryT> > TraceBufCapacityReset(
                                    const std::string& buf_name, size_t size) {
        std::scoped_lock lock(mutex_);
        typename TraceBufMap::iterator it = trace_buf_map_.find(buf_name);
        if (it != trace_buf_map_.end()) {
            boost::shared_ptr<TraceBuffer<TraceEntryT> > trace_buf =
                                                             it->second.lock();
            trace_buf->TraceBufCapacityReset(size);
            return trace_buf;
        }
        return boost::shared_ptr<TraceBuffer<TraceEntryT> >();
    }

private:

    /// Forbids the default ctor.
    Trace() {
        trace_enable_ = true;
    }

    /// Destroys the table.
    ~Trace() {

        delete trace_;
    }

    /// A pointer to the table (singleton) used in this program.
    static Trace *trace_;

    /// Determines if the tracing is enabled for the table.
    std::atomic<bool> trace_enable_;

    /// Stores the table of trace buffers.
    TraceBufMap trace_buf_map_;

    /// A mutex to protect the table from data races.
    std::mutex mutex_;

    DISALLOW_COPY_AND_ASSIGN(Trace);
};

#endif // __TRACE_H__
