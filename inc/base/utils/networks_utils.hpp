/*
 * =====================================================================================
 *
 *       Filename:  networks_utils.hpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/05/16 12:16:39
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */
#ifndef NETWORKS_UTILS_HPP_
#define NETWORKS_UTILS_HPP_

#include <string>

#include <arpa/inet.h>
#include <sys/socket.h>


namespace cppbase {

#ifndef IPQUAD_FMT
#define IPQUAD_FMT		"%u.%u.%u.%u"
#endif

#ifndef NIPQUAD
#define NIPQUAD(addr) \
		((unsigned char *)&addr)[0], \
		((unsigned char *)&addr)[1], \
		((unsigned char *)&addr)[2], \
		((unsigned char *)&addr)[3] 
#endif

#ifndef HIPQUAD
#define HIPQUAD(addr) \
	((unsigned char *)&addr)[3], \
	((unsigned char *)&addr)[2], \
	((unsigned char *)&addr)[1], \
	((unsigned char *)&addr)[0] 
#endif

#ifndef MACQUAD_FMT
#define MACQUAD_FMT					"%02x:%02x:%02x:%02x:%02x:%02x"
#endif

#ifndef MACQUAD
#define MACQUAD(mac)			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]
#endif

static inline uint32_t convert_str_to_ipv4(std::string ip_str)
{
	uint32_t ip;

	inet_pton(AF_INET, ip_str.c_str(), &ip);

	ip = ntohl(ip);

	return ip;
}

#define MAX_IP_COUNT (20)
#define DNS_PORT (53)

typedef struct {
    int size;
    struct in_addr ip[MAX_IP_COUNT];
} dns_res_t;

int dns_resolve(const char *host, const char *dnsserver, dns_res_t *res);

}
#endif

