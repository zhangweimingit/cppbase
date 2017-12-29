#include "base/utils/networks_utils.hpp"

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>

namespace cppbase {

static struct __res_state g_res;

  static int set_default_dns(const char *server) {
      if (!server) {
          return 0;
      }
  
      if (res_init() != 0) {
          perror("res_init failed");
          return -1;
      }
  
      memset(&g_res, 0, sizeof(g_res));
      memcpy(&g_res, &_res, sizeof(_res));
  
      struct sockaddr_in sa;
      if (inet_pton(AF_INET, server, &sa.sin_addr) != 1) {
          return -1;
      }
  
      sa.sin_family = AF_INET;
      sa.sin_port = htons(DNS_PORT);
      _res.nscount = 1; //modify the _res which is defined in res_libc.c
      memcpy(&(_res.nsaddr_list[0]), &sa, sizeof(sa));
  
      return 0;
  }
  
  // reset _res
  static void _res_reset() { memcpy(&_res, &g_res, sizeof(g_res)); }
  
  static int resolve_host(const char *host, dns_res_t *res) {
      struct addrinfo hints, *result;
  
      memset(&hints, 0, sizeof(hints));
      hints.ai_family = PF_INET;
      hints.ai_socktype = SOCK_STREAM;
  
      int error = getaddrinfo(host, NULL, &hints, &result);
      if (error != 0) {
          return error;
      }
      res->size = 0;
      struct addrinfo *iter;
      for (iter = result; iter; iter = iter->ai_next) {
          if (iter->ai_family != PF_INET) {
              continue;
          }
  
          struct sockaddr_in *addr_in = (struct sockaddr_in *)iter->ai_addr;
          if (INADDR_ANY == addr_in->sin_addr.s_addr ||
              INADDR_NONE == addr_in->sin_addr.s_addr) {
              continue;
          }
          struct in_addr addr;
          addr.s_addr = addr_in->sin_addr.s_addr;
          res->ip[res->size++] = addr;
      }
      freeaddrinfo(result);
  
      return 0;
  }
  
  int dns_resolve(const char *host, const char *dnsserver, dns_res_t *res) {
      if (host == NULL || res == NULL) {
          return -1;
      }
  
      if (set_default_dns(dnsserver) != 0) {
          perror("set_default_dns failed");
          return -1;
      }
  
      if (resolve_host(host, res) != 0) {
          perror("resolve_host failed");
          return -1;
      }
  
      if (dnsserver != NULL) {
          _res_reset();
      }
  
      return 0;
  }
}
