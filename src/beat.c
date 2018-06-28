#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
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
pid_t fork_pid;
bool stopping = false;

void removepidfile() {
    if (fork_pid && unlink(pidfilename) < 0) {
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
        if (tcp_server_socket != 0)
            close(tcp_server_socket);
        if (tcp_sock != 0)
            close(tcp_sock);
        removepidfile();
        exit(0);
    }
}

int main(int argc, char **argv) {

    fork_pid = argc < 2 ? fork() : 0;

    if (fork_pid == 0) {
        // child

        if (argc < 2) {
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
        }

        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);

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
            struct sockaddr_in client_addr;
            socklen_t sockaddr_len;
            tcp_sock = accept(tcp_server_socket, (struct sockaddr *)&client_addr, &sockaddr_len);
            char out[64];
            int err = getnameinfo((struct sockaddr *)&client_addr, sockaddr_len, out, 64, NULL, 0, NI_NUMERICHOST);
            if (err != 0) {
                fprintf(stderr, "Error decoding: %d %d %d\n", err, errno, sockaddr_len);
                fprintf(stderr, "Error: %s\n", gai_strerror(err));
            } else {
                printf("Client connected: %s\n", out);
                fflush(stdout);
            }

            if (tcp_sock < 0) {
                fprintf(stderr, "Cannot accept socket, error: %d\n", errno);
            }

            if (send(tcp_sock, message, strlen(message), 0) < 0) {
                fprintf(stderr, "Cannot send message to socket, error: %d\n", errno);
                close(tcp_sock);
            }

            if (close(tcp_sock) < 0) {
                fprintf(stderr, "Cannot close socket, error: %d\n", errno);
                close(tcp_sock);
            }
        }
    } else {
        // parent
        printf("Spawned child with pid %d, parent now dying\n", fork_pid);
    }
}