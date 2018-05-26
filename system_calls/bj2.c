#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

void sigchld_handler(int s)
{
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

int ActivateSignal()
{
    struct sigaction sa;
    sa.sa_handler = sigchld_handler; // collect all zombie processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    return sigaction(SIGCHLD, &sa, NULL);
}

struct addrinfo GetAddrinfo(const char* url, const char* port)
{
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // любой протокол (AF_INET/AF_INET6)
    hints.ai_socktype = SOCK_STREAM; // поток байтов (SOCK_STREAM/SOCK_DGRAM)
    hints.ai_flags = AI_PASSIVE; // любой IP текущего хоста (либо конкретный)

    addrinfo* response;

    const int status = getaddrinfo(url, port, &hints, &response);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(1);
    }

    addrinfo result = *response;
    freeaddrinfo((addrinfo*) response);
    return result;
}

void PrintIPAddress(struct sockaddr_storage& their_addr)
{
    char ip[INET6_ADDRSTRLEN] = {0};
    const void* addr = ((struct sockaddr*)&their_addr)->sa_family == AF_INET
        ? (void*)&(((struct sockaddr_in*)(struct sockaddr*)&their_addr)->sin_addr)
        : (void*)&(((struct sockaddr_in6*)(struct sockaddr*)&their_addr)->sin6_addr);

    inet_ntop(their_addr.ss_family, addr, ip, sizeof(ip));
    printf("Connection from: %s\n", ip);
}

int Accept(const int sockfd)
{
    struct sockaddr_storage their_addr;
    socklen_t addr_size = sizeof(their_addr);

    const int newfd = accept(sockfd, (struct sockaddr*)&their_addr, &addr_size);
    if (newfd == -1) {
        perror("accept");
        return -1;
    }

    PrintIPAddress(their_addr);

    return newfd;
}

void HandleConnection(const int newfd)
{
    const char* msg = "Beej was here!";
    const int msglen = strlen(msg);
    const int bytes_sent = send(newfd, msg, msglen, 0);
    if (bytes_sent == -1) {
        perror("send");
    }

    shutdown(newfd, 2);
    close(newfd);
}

int main(int argc, char** argv)
{
    const addrinfo addr = GetAddrinfo(NULL, "3490" /*telnet port*/);

    const int sockfd = socket(addr.ai_family, addr.ai_socktype, addr.ai_protocol);
    if (sockfd == -1) {
        perror("socket");
        return 2;
    }

    const int bind_ret = bind(sockfd, addr.ai_addr, addr.ai_addrlen);
    if (bind_ret == -1) {
        perror("bind");
        return 3;
    }

    const int listen_ret = listen(sockfd, 10);
    if (listen_ret == -1) {
        perror("listen");
        return 4;
    }

    int yes = 1;
    const int sockopt_ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if (sockopt_ret == -1) {
        perror("setsockopt");
        return 5;
    }

    const int sig_ret = ActivateSignal();
    if (sig_ret == -1) {
        perror("sigaction");
        return -1;
    }

    while (true) {
        const int newfd = Accept(sockfd);
        if (newfd == -1) {
            continue;
        }

        const int fork_ret = fork();
        if (fork_ret == 0) {
            close(sockfd);
            HandleConnection(newfd);
            exit(0);
        }
    }

    return 0;
}
