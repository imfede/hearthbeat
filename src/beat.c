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
bool stopping = false;

void removepidfile() {
    if (unlink(pidfilename) < 0) {
        fprintf(stderr, "Unable to remove pidfile, error: %d", errno);
    }
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

        struct sigaction action;
        action.sa_handler = signal_handler;
        action.sa_flags = 0;
        sigemptyset(&action.sa_mask);
        sigaction(SIGINT, &action, NULL);
        sigaction(SIGTERM, &action, NULL);

        struct sockaddr_in server_address;
        memset(&server_address, 0, sizeof(server_address));
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(0xBEA7);
        server_address.sin_addr.s_addr = htonl(INADDR_ANY);

        int server_socket = socket(PF_INET, SOCK_STREAM, 0);
        if (server_socket < 0) {
            fprintf(stderr, "Error creating socket\n");
            removepidfile();
            exit(-1);
        }

        if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
            fprintf(stderr, "Cannot bind socket, error: %d\n", errno);
            removepidfile();
            exit(-1);
        }

        if (listen(server_socket, 16) < 0) {
            fprintf(stderr, "Cannot listen to socket, error: %d\n", errno);
            removepidfile();
            exit(-1);
        }

        while (1) {
            int sock = accept(server_socket, NULL, NULL);
            if (stopping) {
                printf("Stopping\n");
                close(sock);
                close(server_socket);
                removepidfile();
                exit(0);
            }
            if (sock < 0) {
                fprintf(stderr, "Cannot accept socket, error: %d\n", errno);
            } else {
                if (send(sock, message, strlen(message), 0) < 0) {
                    fprintf(stderr, "Cannot send message to socket, error: %d\n", errno);
                }

                if (close(sock) < 0) {
                    fprintf(stderr, "Cannot close socket, error: %d\n", errno);
                }
            }
        }
    } else {
        // parent
        FILE *pid_file = fopen(pidfilename, "w");
        if (pid_file == NULL) {
            fprintf(stderr, "Error creating file: %d", errno);
            exit(errno);
        }
        int print_ret = fprintf(pid_file, "%d", fork_pid);
        if (print_ret < 0) {
            fprintf(stderr, "Error trying to write pid to file");
            exit(-1);
        }
        printf("Spawned child with pid %d, parent now dying\n", fork_pid);
    }
}