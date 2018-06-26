#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

const char *pidfilename = "/var/hearthbeat/pid";
const char *message = "bip";
int port = 0xBEA7;

int tcp_server_socket;
int tcp_sock;
bool stopping = false;

void removepidfile() {
    if (unlink(pidfilename) < 0) {
        fprintf(stderr, "Unable to remove pidfile, error: %d", errno);
    }
}

void die(int code, char *message, ...) {
    va_list fmtargs;
    fprintf(stderr, message, fmtargs);
    removepidfile();
    exit(code);
}

void signal_handler(int signo) {
    if (signo == SIGINT || signo == SIGTERM) {
        printf("Received stop signal\n");
        stopping = true;
    }
}

int main(int argc, char **argv) {

    pid_t fork_pid = fork();

    if (fork_pid == 0) {
        // child

        FILE *pid_file = fopen(pidfilename, "w");
        if (pid_file == NULL) {
            die(errno, "Error creating file: %d\n", errno);
        }
        if (fprintf(pid_file, "%d", getpid()) < 0) {
            die(-1, "Error trying to write pid to file\n");
        }
        if (fclose(pid_file) != 0) {
            die(errno, "Error closing pidfile: %d\n", errno);
        }

        struct sigaction action;
        action.sa_handler = signal_handler;
        action.sa_flags = 0;
        sigemptyset(&action.sa_mask);
        sigaction(SIGINT, &action, NULL);
        sigaction(SIGTERM, &action, NULL);

        struct sockaddr_in server_address;
        memset(&server_address, 0, sizeof(server_address));
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(argc > 1 ? atoi(argv[1]) : port);
        server_address.sin_addr.s_addr = htonl(INADDR_ANY);

        tcp_server_socket = socket(PF_INET, SOCK_STREAM, 0);
        if (tcp_server_socket < 0) {
            die(-1, "Error creating socket\n");
        }

        if (bind(tcp_server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
            die(errno, "Cannot bind socket, error: %d\n", errno);
        }

        if (listen(tcp_server_socket, 16) < 0) {
            die(errno, "Cannot listen to socket, error: %d\n", errno);
        }

        while (1) {
            tcp_sock = accept(tcp_server_socket, NULL, NULL);
            if (stopping) {
                printf("Stopping\n");
                close(tcp_sock);
                close(tcp_server_socket);
                removepidfile();
                exit(0);
            }

            if (tcp_sock < 0) {
                fprintf(stderr, "Cannot accept socket, error: %d\n", errno);
            }

            if (send(tcp_sock, message, strlen(message), 0) < 0) {
                fprintf(stderr, "Cannot send message to socket, error: %d\n", errno);
            }

            if (close(tcp_sock) < 0) {
                fprintf(stderr, "Cannot close socket, error: %d\n", errno);
            }
        }
    } else {
        // parent
        printf("Spawned child with pid %d, parent now dying\n", fork_pid);
    }
}