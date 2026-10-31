/*
 * Copyright 2019, Juniper Networks Inc.
 * Copyright Contributors to the OpenSDN project
 */

#ifndef _THRIFT_XML_PROTOCOL_H
#define _THRIFT_XML_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __KERNEL__

#define TRUE   1
#define FALSE  0

#define MAX_XML_BUFFER_SIZE   256
#define MAX_XML_TAG_SIZE      128
#define MAX_TAG_STACK_SIZE    100

typedef enum {
    TAG_SANDESH,
    TAG_FIELD,
    TAG_LIST,
    TAG_MAX,
} tag_type_t;

typedef struct {
    char stack[MAX_TAG_STACK_SIZE][MAX_XML_TAG_SIZE];
    int tos;
} tag_stack_t;

typedef struct {
    /* Must be the first element */
    ThriftProtocol proto;
    /* Private data */
    tag_stack_t tag_stacks[TAG_MAX];
} ThriftXMLProtocol;

void thrift_xml_protocol_init (ThriftProtocol *protocol);

#endif

#ifdef __cplusplus
}
#endif

#endif /* _THRIFT_XML_PROTOCOL_H */
