/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

#ifndef __BASE_COMPILER_H__
#define __BASE_COMPILER_H__

#ifdef __GNUC__

#ifdef __linux__
#include <features.h>
#define __GCC_HAS_PRAGMA            __GNUC_PREREQ(4, 4)
#define __GCC_HAS_DIAGNOSTIC_PUSH   __GNUC_PREREQ(4, 6)
#endif // OS

#endif // __GNUC__

#endif // __BASE_COMPILER_H__
