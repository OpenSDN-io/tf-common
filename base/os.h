/* System dependent (re)definitions */
#ifndef _agent_os_h_
#define _agent_os_h_

#if defined(__linux__)
# include <sys/types.h>
# include <net/ethernet.h>
# if !defined(_FEATURES_H)
#  define UNDEF__FEATURES_H
#  define _FEATURES_H
# endif
# if !defined(__USEMISC)
#  define UNDEF___USEMISC
#  define __USEMISC 1
# endif
# if !defined(__FAVOR_BSD)
#  define UNDEF___FAVOR_BSD
#  define __FAVOR_BSD
# endif
#endif

# include <netinet/tcp.h>
# include <netinet/udp.h>

#if defined(__linux__)
# if defined(UNDEF__FEATURES_H)
#  undef UNDEF__FATURES_H
#  undef _FEATURES_H
# endif
# if defined(UNDEF___USEMISC)
#  undef UNDEF___USEMISC
#  undef __USEMISC
# endif
# if defined(UNDEF___FAVOR_BSD)
#  undef UNDEF___FAVOR_BSD
#  undef __FAVOR_BSD
# endif
#endif

#endif /* ndef _agent_os_h_ */

