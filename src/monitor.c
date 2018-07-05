#include "telegram.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <event.h>
#include <sys/time.h>
#include <pthread.h>

int udp_server_socket = 0;
struct sockaddr_in target_addr = {0};
char *message = "bip?";
int interval = 1;

void die(int code, char *message, ...) {
    va_list fmtargs;
    fprintf(stderr, message, fmtargs);
    exit(code);
}

void init() {
    struct addrinfo hints = {0};
    struct addrinfo *info;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    int result = getaddrinfo("home.imfede.net", "48807", &hints, &info);
    if (result != 0) {
        die(result, "%d: %s\n", result, gai_strerror(result));
    }

    memcpy(&target_addr, info->ai_addr, sizeof(target_addr));

    freeaddrinfo(info);
}

void send_bip() {
    if (sendto(udp_server_socket, message, strlen(message), 0, (struct sockaddr *)&target_addr, sizeof(target_addr)) < 0) {
        fprintf(stderr, "Error sending: %d\n", errno);
    } else {
        printf("Sent: %s\n", message);
    }
}

void handle_event(int fd, short event, void *arg) {
    send_bip();
}

void *handle_connection_thread(void *ign) {
    nfds_t nfds = 1;
    struct pollfd fds[nfds];
    memset(fds, 0, sizeof(fds));
    fds[0].fd = udp_server_socket;
    fds[0].events = POLLIN;

    while (true) {
        int ret = poll(fds, nfds, -1);

        if (fds[0].revents & POLLIN) {
            char buffer[128];
            struct sockaddr_in client_addr;
            socklen_t sockaddr_len = sizeof(client_addr);
            ssize_t count = recvfrom(udp_server_socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &sockaddr_len);
            buffer[count] = '\0';
            if (count == -1) {
                fprintf(stderr, "Cannot read UDP message: %d\n", errno);
            } else {
                printf("Received: %s\n", buffer);
            }
        }
    }
}

int main() {
    init();

    struct event_base *base = event_base_new();
    struct event *ev;
    struct timeval tv;

    tv.tv_sec = 1;
    tv.tv_usec = 0;

    ev = event_new(base, -1, EV_PERSIST, &handle_event, NULL);
    evtimer_add(ev, &tv);

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);

    // create and bind UDP
    udp_server_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_server_socket < 0) {
        die(-1, "Error creating UDP socket\n");
    }

    if (bind(udp_server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        die(errno, "Cannot bind UDP socket, error: %d\n", errno);
    }

    pthread_t listener;
    int thread_status = pthread_create(&listener, NULL, &handle_connection_thread, NULL);
    if(thread_status != 0) {
        die(thread_status, "Error threading: %d\n", thread_status);
    }

    event_base_dispatch(base);
}