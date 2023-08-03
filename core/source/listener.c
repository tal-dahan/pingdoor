#include "../headers/listener.h"


struct listener *build_listener(int port) {
    int sock_fd = start_listen(port);

    if (sock_fd == -1) {
        return NULL;
    } else {
        struct listener *l = malloc(sizeof(struct listener));
        l->port = port;
        l->sock_fd = sock_fd;

        return l;
    }
}


int wait_for_connection(struct listener *l) {
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    addr_size = sizeof their_addr;
    int connection_fd = accept(l->sock_fd, (struct sockaddr *)&their_addr, &addr_size);

    return connection_fd;
}  
