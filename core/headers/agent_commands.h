#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include "listener.h"


#define AGENT_STATUS_CMD 1
#define AGENT_PS_CMD 2
#define AGENT_UNAME_CMD 3
#define AGENT_SHELL_CMD 4
#define AGENT_IP_CMD 5


void signal_agent(struct addrinfo *agent_info, int cmd);