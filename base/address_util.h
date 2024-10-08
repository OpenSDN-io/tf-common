/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */
#ifndef BASE_ADDRESS_UTIL_H__
#define BASE_ADDRESS_UTIL_H__

#include <string>
#include <vector>

#include <base/address.h>
#include <boost/asio/io_service.hpp>

/*
 * Returns true if the given IPv4 address is member of the IPv4 subnet
 * indicated by IPv4 address and prefix length. Otherwise returns false.
 */
bool IsIp4SubnetMember(const Ip4Address &ip, const Ip4Address &prefix_ip,
                       uint16_t plen);
/*
 * Returns true if the given IPv6 address is member of the IPv6 subnet
 * indicated by IPv6 address and prefix length. Otherwise returns false.
 */
bool IsIp6SubnetMember(const Ip6Address &ip, const Ip6Address &subnet,
                       uint8_t plen);
/*
 * Returns canonical hostname
 */

std::string ResolveCanonicalName();

/*
 * Returns canonical hostname by IPv4
 */

std::string ResolveCanonicalName(const std::string& ipv4);

/*
 * Returns canonical  hostname by IPv6
 */

std::string ResolveCanonicalNameIPv6(const std::string& ipv6);

/*
 * Returns boost::asio::ip::address if given string is either valid
 * IPv4, IPv6 or a resolvable FQDN
*/

boost::asio::ip::address AddressFromString(
                         const std::string &ip_address_str,
                         boost::system::error_code *ec);

Ip4Address GetIp4SubnetBroadcastAddress(const Ip4Address &ip_prefix,
                                        uint16_t plen);

bool ValidateIPAddressString(std::string ip_address_str,
                             std::string *error_msg);

IpAddress PrefixToIpNetmask(uint32_t prefix_len);
uint32_t NetmaskToPrefix(uint32_t netmask);

IpAddress PrefixToIp6Netmask(uint32_t plen);

void IpToU64(const IpAddress &sip, const IpAddress &dip,
             uint64_t *sip_u, uint64_t *sip_l,
             uint64_t *dip_u, uint64_t *dip_l);
void U64ToIpv6(uint64_t upper, uint64_t lower, IpAddress *ip);
void U64ToIp(uint64_t sip_u, uint64_t sip_l, uint64_t dip_u, uint64_t dip_l,
             int family, IpAddress *sip, IpAddress *dip);
void Ip6AddressToU64Array(const Ip6Address &addr, uint64_t *arr, int size);

bool ValidateServerEndpoints(std::vector<std::string> list,
                             std::string *error_msg);

std::string VectorIpv6ToString(const std::vector<signed char> &ipv6);

/*
 * Return IP address string for a host if it is resolvable, empty string
 * otherwise.
 */
std::string GetHostIp(boost::asio::io_context *io_service,
                      const std::string &hostname);

/*
 * Get VN name from routing instance
 */
std::string GetVNFromRoutingInstance(const std::string &vn);

#endif // BASE_ADDRESS_UTIL_H__
