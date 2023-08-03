#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "../headers/network_helper.h"


int start_listen(int port) {
    int status;
    char service[5];
    sprintf(service, "%d", port);

    struct addrinfo *listen_info;

    // set all structs for getaddrinfo
    struct addrinfo hints; // holds relevant details about the service

    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC; // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me

    if ((status = getaddrinfo(NULL, service, &hints, &listen_info)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    }

    int sockfd = socket(listen_info->ai_family, listen_info->ai_socktype, listen_info->ai_protocol);
    bind(sockfd, listen_info->ai_addr, listen_info->ai_addrlen);
    listen(sockfd, BACKLOG);

    return sockfd;
}


void getaddrinfo_by_ip(char *ip, struct addrinfo **ping_info) {
    int status;
    
    // set all structs for getaddrinfo
    struct addrinfo hints; // holds relevant details about the service

    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC; // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_RAW; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me
    hints.ai_protocol = 0; // any service, since we are using raw socket for ping

    if ((status = getaddrinfo(ip, NULL, &hints, ping_info)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    }
}


char *convert_addrinfo_to_ip(struct addrinfo * servinfo) {

    char *ip = malloc(INET6_ADDRSTRLEN * sizeof(char));
    void *addr;

    if (servinfo->ai_family == AF_INET) { // IPv4
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)servinfo->ai_addr;
        addr = &(ipv4->sin_addr);
    } else { // IPv6
        struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)servinfo->ai_addr;
        addr = &(ipv6->sin6_addr);
    }

    inet_ntop(servinfo->ai_family, addr, ip, INET6_ADDRSTRLEN * sizeof(char));

    return ip;
}


int _exec_ping(struct addrinfo *servinfo, int payload_size) {
    const char *PING_PATH = "/usr/bin/ping";
    const char *COMMAND_ARG = "/usr/bin/ping";
    const char *COUNT_FLAG = "-c";
    const char *COUNT_FLAG_VALUE = "1";
    const char *SIZE_FLAG = "-s";
    char *SIZE_FLAG_VALUE;
    sprintf(SIZE_FLAG_VALUE, "%d", payload_size);
    const char *IP_ADDR_ARG = convert_addrinfo_to_ip(servinfo);

    int r = execl(PING_PATH,
                  COMMAND_ARG,
                  COUNT_FLAG, COUNT_FLAG_VALUE,
                  SIZE_FLAG, SIZE_FLAG_VALUE,
                  IP_ADDR_ARG,
                  (char*)NULL);    
}


int ping(struct addrinfo *servinfo, int payload_size) {
    int pfds[2];
    pipe(pfds);
    int rfd = pfds[0];
    int wfd = pfds[1];

    // child - exec ping
    if (!fork()) {
        dup2(wfd, STDOUT_FILENO);
        _exec_ping(servinfo, payload_size);
    }

    // parent - check ping status
    else {
        wait(NULL);
        char ping_result[256];
        read(rfd, ping_result, sizeof(ping_result));
        char *r = strstr(ping_result, "1 received");

        if (r == NULL) {
            return -1;
        } else {
            return 1;
        }
    }
}