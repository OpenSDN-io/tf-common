/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

/*
 * sandesh.h
 *
 * Sandesh C Library
 */

#ifndef __SANDESHC_H__
#define __SANDESHC_H__

#ifdef __cplusplus
extern "C" {
#endif

/* OS specific defines */
#ifdef __KERNEL__
#if defined(__linux__)
#include <linux/kernel.h>
#include <linux/slab.h>

#include <linux/types.h>
#include <linux/in.h>
#include <linux/in6.h>

#define OS_LOG_ERR KERN_ERR
#define OS_LOG_DEBUG KERN_DEBUG

#define os_malloc(size)                  kmalloc(size, GFP_KERNEL)
#define os_zalloc(size)                  kzalloc(size, GFP_KERNEL)
#define os_realloc(ptr, size)            krealloc(ptr, size, GFP_KERNEL)
#define os_free(ptr)                     kfree(ptr)
#define os_log(level, format, arg...)    printk(level format, ##arg)
#endif

extern int vrouter_dbg;
#else /* __KERNEL__ */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include <syslog.h>
#include <sys/errno.h>

#define OS_LOG_ERR LOG_ERR
#define OS_LOG_DEBUG LOG_DEBUG

#define os_malloc(size)                  malloc(size)
#define os_zalloc(size)                  calloc(1, size)
#define os_realloc(ptr, size)            realloc(ptr, size)
#define os_free(ptr)                     free(ptr)
#define os_log(level, format, arg...)    syslog(level, format, ##arg)


#endif /* __KERNEL__ */

typedef unsigned char ct_uuid_t[16];

typedef struct ipaddr_s {
    uint8_t iptype; // AF_INET or AF_INET6
    union {
        struct in_addr ipv4;
        struct in6_addr ipv6;
    };
} ipaddr_t;

static inline uint64_t os_get_value64(const uint8_t *data) {
    uint64_t value = 0;
    int i = 0;
    for (; i < 8; ++i) {
        if (i) value <<= 8;
        value += *data++;
    }
    return value;
}

static inline void os_put_value64(uint8_t *data, uint64_t value) {
    int i = 0, offset;
    for (; i < 8; i++) {
        offset = (8 - (i + 1)) * 8;
        *data++ = ((value >> offset) & 0xff);
    }
}

#include "thrift.h"
#include "transport/thrift_transport.h"
#include "transport/thrift_memory_buffer.h"
#include "transport/thrift_fake_transport.h"
#include "transport/thrift_file_transport.h"
#include "protocol/thrift_protocol.h"
#include "protocol/thrift_binary_protocol.h"
#include "protocol/thrift_xml_protocol.h"

typedef struct sandesh_info_s {
    const char *name;
    u_int32_t size;
    int32_t (*read) (void *, ThriftProtocol *, int *);
    int32_t (*read_binary_from_buffer) (void *, uint8_t *, const size_t, int *);
    int32_t (*write) (void *, ThriftProtocol *, int *);
    int32_t (*write_binary_to_buffer) (void *, uint8_t *, const size_t, int *);
    void (*process) (void *);
    void (*free) (void *);
} sandesh_info_t ;

typedef sandesh_info_t * (*sandesh_find_info_fn) (const char *name);

sandesh_info_t * sandesh_find_info(sandesh_info_t *infos, const char *name);
int32_t sandesh_decode(u_int8_t *buf, u_int32_t buf_len,
                       sandesh_find_info_fn sinfo_find_fn, int *error);
int32_t sandesh_encode(void *sandesh, const char *sname,
                       sandesh_find_info_fn sinfo_find_fn, u_int8_t *buf,
                       u_int32_t buf_len, int *error);
int32_t sandesh_get_encoded_length(void *sandesh, const char *sname,
                                   sandesh_find_info_fn sinfo_find_fn,
                                   int *error);

#ifdef __cplusplus
}
#endif

#endif /* __SANDESHC_H__ */
