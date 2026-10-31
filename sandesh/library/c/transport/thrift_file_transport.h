/*
 * Copyright 2019, Juniper Networks Inc.
 * Copyright Contributors to the OpenSDN project
 */

#ifndef _THRIFT_FILE_TRANSPORT_H
#define _THRIFT_FILE_TRANSPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __KERNEL__

#include <stdio.h>

/*! \file thrift_file_transport.h
 *  \brief Implementation of a file based thrift transport
 */

#define MAX_FILE_NAME    512

/*!
 * ThriftFileTransport instance.
 */
struct _ThriftFileTransport
{
    /* Needs to be the first element */
    ThriftTransport tt;

    /* private */
    char filename[MAX_FILE_NAME];
    FILE *fp;
};
typedef struct _ThriftFileTransport ThriftFileTransport;

int thrift_file_transport_init (ThriftFileTransport *t, char *filename);
void thrift_file_transport_close (ThriftFileTransport *ft);

#endif /* __KERNEL__ */

#ifdef __cplusplus
}
#endif

#endif
