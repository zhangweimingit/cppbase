#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include "../../../../inc/base/utils/networks_utils.hpp"

#define TEST_TIMES 1000
#define HOST_TO_RESOLVE "www.baidu.com"
using namespace cppbase;

void TEST_resolve_with_dnsserver() {
    dns_res_t res;
    if (dns_resolve(HOST_TO_RESOLVE, "8.8.8.8", &res) < 0) {
        printf("dns_resolve failed\n");
        exit(-1);
    }

    int i;
    printf("---------specify dns--\n");
    for (i = 0; i < res.size; ++i) {
        char *ip = inet_ntoa(res.ip[i]);
        printf("ip: %s\n", ip);
    }
    printf("-------------------\n");
}

void TEST_resolve_with_default() {
    dns_res_t res;
    if (dns_resolve(HOST_TO_RESOLVE, NULL, &res) < 0) {
        printf("dns_resolve failed\n");
        exit(-1);
    }

    int i;
    printf("--------default----\n");
    for (i = 0; i < res.size; ++i) {
        char *ip = inet_ntoa(res.ip[i]);
        printf("ip: %s\n", ip);
    }
    printf("-------------------\n");
}

int main() {
    int i = 0;
    for (i = 0; i < TEST_TIMES; ++i) {
        TEST_resolve_with_default();
        TEST_resolve_with_dnsserver();
    }

    return 0;
}
