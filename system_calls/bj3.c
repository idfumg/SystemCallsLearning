#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

struct addrinfo GetAddrinfo(const char* url, const char* port)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // любой протокол (AF_INET/AF_INET6)
    hints.ai_socktype = SOCK_STREAM; // поток байтов (SOCK_STREAM/SOCK_DGRAM)

    struct addrinfo* response;

    const int status = getaddrinfo(url, port, &hints, &response);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(1);
    }

    struct addrinfo result = *response;
    freeaddrinfo((struct addrinfo*) response);
    return result;
}

void PrintConnecting(const struct addrinfo& their_addr)
{
    char ip[INET6_ADDRSTRLEN] = {0};
    const void* addr = ((struct sockaddr*)&their_addr)->sa_family == AF_INET
        ? (void*)&(((struct sockaddr_in*)(struct sockaddr*)&their_addr)->sin_addr)
        : (void*)&(((struct sockaddr_in6*)(struct sockaddr*)&their_addr)->sin6_addr);

    inet_ntop(their_addr.ai_family, addr, ip, sizeof(ip));
    printf("Connection to: %s\n", ip);
}

int main(int argc, char** argv)
{
    const struct addrinfo addr = GetAddrinfo("localhost", "3490" /*telnet*/);

    const int sockfd = socket(addr.ai_family, addr.ai_socktype, addr.ai_protocol);
    if (sockfd == -1) {
        perror("socket");
        return 2;
    }

    PrintConnecting(addr);

    const int connect_ret = connect(sockfd, addr.ai_addr, addr.ai_addrlen);
    if (connect_ret == -1) {
        perror("connect");
        return 3;
    }

    char buff[BUFSIZ] = {0};
    const int recvn = recv(sockfd, buff, sizeof(buff), 0);
    if (recvn == -1) {
        perror("recv");
        return 4;
    }

    buff[recvn] = '\0';
    printf("Received: `%s`\n", buff);

    shutdown(sockfd, 2);
    close(sockfd);

    return 0;
}
