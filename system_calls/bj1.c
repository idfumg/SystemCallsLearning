#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

void* GetSockaddr(const struct addrinfo* p)
{
    if (p->ai_family == AF_INET) {
        struct sockaddr_in* ipv4 = (struct sockaddr_in*) p->ai_addr;
        return &(ipv4->sin_addr);
    }
    else {
        struct sockaddr_in6* ipv6 = (struct sockaddr_in6*) p->ai_addr;
        return &(ipv6->sin6_addr);
    }
}

struct addrinfo* GetAddrinfo(const char* name)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* response;
    const int status = getaddrinfo(name, NULL, &hints, &response);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(2);
    }

    return response;
}

void ShowIPs(const char* name)
{
    printf("IP addresses for %s:\n\n", name);

    const struct addrinfo* response = GetAddrinfo(name);

    for (const struct addrinfo* p = response; p; p = p->ai_next) {
        const char *ipver = p->ai_family == AF_INET ? "IPv4" : "IPv6";
        const void *addr = GetSockaddr(p);

        char ip[INET6_ADDRSTRLEN];
        inet_ntop(p->ai_family, addr, ip, sizeof(ip));

        printf("%s: %s\n", ipver, ip);
    }

    freeaddrinfo((struct addrinfo*) response);
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        fprintf(stderr, "hostname missed\n");
        return 1;
    }

    ShowIPs(argv[1]);

    return 0;
}
