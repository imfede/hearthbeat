#include <argp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
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

int tcp_server_socket;
int tcp_sock;
int udp_server_socket;
pid_t fork_pid;
bool stopping = false;

const char *argp_program_version = "v1.0.1";
const char *argp_program_bug_address = "federico.cergol@gmail.com ($ gpg --keyserver pgp.mit.edu --recv-key 0x2C831564)";
const char doc[] = "A simple heartbeat program";
const char arg_doc[] = "-p <port> --no-fork";
const struct argp_option options[] = {{"port", 'p', "port", 0, "Port on which the program will listen on", 0}, {"no-fork", 'n', NULL, 0, "Do not fork"}, {0}};
struct arguments {
    int port;
    bool fork;
};
error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;
    switch (key) {
    case 'p':
        arguments->port = atoi(arg);
        break;
    case 'n':
        arguments->fork = false;
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}
const struct argp argp = {options, parse_opt, arg_doc, doc};

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
        if (udp_server_socket != 0)
            close(udp_server_socket);
        removepidfile();
        exit(0);
    }
}

int main(int argc, char **argv) {

    struct arguments arguments;
    arguments.port = 0xBEA7;
    arguments.fork = true;

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    fork_pid = arguments.fork ? fork() : 0;

    if (fork_pid == 0) {
        // child

        if (arguments.fork) {
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

        // create and bind TCP
        struct sockaddr_in server_address;
        memset(&server_address, 0, sizeof(server_address));
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(arguments.port);
        server_address.sin_addr.s_addr = htonl(INADDR_ANY);

        tcp_server_socket = socket(PF_INET, SOCK_STREAM, 0);
        if (tcp_server_socket < 0) {
            die(-1, "Error creating TCP socket\n");
        }

        fcntl(tcp_server_socket, F_SETFL, O_NONBLOCK);

        if (bind(tcp_server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
            die(errno, "Cannot bind TCP socket, error: %d\n", errno);
        }

        if (listen(tcp_server_socket, 16) < 0) {
            die(errno, "Cannot listen to TCP socket, error: %d\n", errno);
        }

        // create and bind UDP
        udp_server_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (udp_server_socket < 0) {
            die(-1, "Error creating UDP socket\n");
        }

        if (bind(udp_server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
            die(errno, "Cannot bind UDP socket, error: %d\n", errno);
        }

        nfds_t nfds = 2;
        struct pollfd fds[nfds];
        memset(fds, 0, sizeof(fds));
        fds[0].fd = tcp_server_socket;
        fds[0].events = POLLIN;
        fds[1].fd = udp_server_socket;
        fds[1].events = POLLIN;

        printf("Listening now on port %d\n", arguments.port);
        while (true) {
            int ret = poll(fds, nfds, -1);

            if (fds[0].revents & POLLIN) {
                // someone contacted me over TCP
                struct sockaddr_in client_addr;
                memset(&client_addr, 0, sizeof(client_addr));
                socklen_t sockaddr_len;
                tcp_sock = accept(tcp_server_socket, (struct sockaddr *)&client_addr, &sockaddr_len);
                char out[64];
                int err = getnameinfo((struct sockaddr *)&client_addr, sockaddr_len, out, 64, NULL, 0, NI_NUMERICHOST);
                if (err != 0) {
                    fprintf(stderr, "Error decoding: %d %d %d\n", err, errno, sockaddr_len);
                    fprintf(stderr, "Error: %s\n", gai_strerror(err));
                } else {
                    printf("Sending TCP bip to: %s\n", out);
                    fflush(stdout);
                }

                if (tcp_sock < 0) {
                    fprintf(stderr, "Cannot accept TCP socket, error: %d\n", errno);
                }

                if (send(tcp_sock, message, strlen(message), 0) < 0) {
                    fprintf(stderr, "Cannot send message to TCP socket, error: %d\n", errno);
                    close(tcp_sock);
                }

                if (close(tcp_sock) < 0) {
                    fprintf(stderr, "Cannot close TCP socket, error: %d\n", errno);
                    close(tcp_sock);
                }
            } else if (fds[1].revents & POLLIN) {
                // someone contacted me over UDP
                char buffer[128];
                struct sockaddr_in client_addr;
                socklen_t sockaddr_len = sizeof(client_addr);
                ssize_t count = recvfrom(udp_server_socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &sockaddr_len);
                buffer[count] = '\0';
                if (count == -1) {
                    fprintf(stderr, "Cannot read UDP message: %d\n", errno);
                }
                char out[64];
                int err = getnameinfo((struct sockaddr *)&client_addr, sockaddr_len, out, 64, NULL, 0, NI_NUMERICHOST);
                if (err != 0) {
                    fprintf(stderr, "Error decoding: %d %d %d\n", err, errno, sockaddr_len);
                    fprintf(stderr, "Error: %s\n", gai_strerror(err));
                } else {
                    printf("Received UDP message from %s\n", out);
                    fflush(stdout);
                }
                if (sendto(udp_server_socket, message, strlen(message), 0, (struct sockaddr *)&client_addr, sockaddr_len) < 0) {
                    fprintf(stderr, "Error sending: %d\n", errno);
                }
            }
        }
    } else {
        // parent
        printf("Spawned child with pid %d, parent now dying\n", fork_pid);
    }
}