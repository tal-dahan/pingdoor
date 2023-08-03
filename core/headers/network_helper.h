#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

#define BACKLOG 0
#define REVERSE_SHELL_PORT 6789

void getaddrinfo_by_ip(char *ip, struct addrinfo **servinfo);

char *convert_addrinfo_to_ip(struct addrinfo *servinfo);

int ping(struct addrinfo *servinfo, int payload_size);

int start_listen(int port);