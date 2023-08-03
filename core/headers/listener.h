#include "network_helper.h"

struct listener {
    int port;
    int sock_fd;
};

struct listener *build_listener(int port);

int wait_for_connection(struct listener *l);
