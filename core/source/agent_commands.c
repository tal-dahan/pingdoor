#include "../headers/agent_commands.h"
#include "../headers/network_helper.h"



void signal_agent(struct addrinfo *agent_info, int cmd) {
    ping(agent_info, cmd);
}
