#include "telegram.h"
#include "argparse.h"
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

char *configfile = "/etc/hearthbeat/monitor";
int poll_interval;
int err_interval;
char *host;
char *port;
char *name;

int udp_server_socket = 0;
char *message = "bip?";

bool isonline = false;
struct event *poll_ev;
struct event *err_ev;
struct event *listen_ev;
struct event_base *base;
struct timeval poll_tv;
struct timeval err_tv;

void die(int code, char *message, ...) {
    va_list fmtargs;
    fprintf(stderr, message, fmtargs);
    exit(code);
}

void init() {
    argparse_register_argument_str("host", &host);
    argparse_register_argument_str("port", &port);
    argparse_register_argument_str("name", &name);
    argparse_register_argument_int("poll_interval", &poll_interval);
    argparse_register_argument_int("err_interval", &err_interval);
    argparse_read_properties(configfile);

    poll_tv.tv_sec = poll_interval;
    poll_tv.tv_usec = 0;

    err_tv.tv_sec = err_interval;
    err_tv.tv_usec = 0;

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
}

int lookup_target(char *host, char *port, struct sockaddr_in *target) {
    struct addrinfo hints = {0};
    struct addrinfo *info;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    int result = getaddrinfo(host, port, &hints, &info);
    if (result != 0) {
        fprintf(stderr, "Error %d resolving host: %s\n", result, gai_strerror(result));
        return -1;
    }

    memcpy(target, info->ai_addr, sizeof(*target));

    freeaddrinfo(info);
    return 0;
}

void send_bip() {
    struct sockaddr_in target;
    if( lookup_target(host, port, &target) != 0 ) {
        fprintf(stderr, "Lookup error!\n");
        return;
    }

    if (sendto(udp_server_socket, message, strlen(message), 0, (struct sockaddr *)&target, sizeof(target)) < 0) {
        fprintf(stderr, "Error sending: %d\n", errno);
    } else {
        printf("Sent: %s\n", message);
    }
}

void handle_poll_event(int fd, short event, void *arg) {
    send_bip();
}

void handle_err_event(int fd, short event, void *arg) {
    if (isonline) {
        isonline = false;
        printf("Host [%s] is down!\n", name);
        char buffer[256];
        snprintf(buffer, 256, "Host [%s] is down!", name);
        telegram_send_message(buffer);
    }
}

void reset_error_timer() {
    if(!isonline) {
        printf("Host [%s] is up!\n", name);
        char buffer[256];
        snprintf(buffer, 256, "Host [%s] is up!", name);
        telegram_send_message(buffer);
    }
    isonline = true;
    evtimer_del(err_ev);
    evtimer_add(err_ev, &err_tv);
}

void handle_answer(int fd) {
    char buffer[128];
    struct sockaddr_in client_addr;
    socklen_t sockaddr_len = sizeof(client_addr);
    ssize_t count = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &sockaddr_len);
    buffer[count] = '\0';
    if (count == -1) {
        fprintf(stderr, "Cannot read UDP message: %d\n", errno);
    } else {
        reset_error_timer();
        printf("Received: %s\n", buffer);
    }
}

void handle_connection(int fd, short event, void *arg) {
    handle_answer(fd);
}

int main() {
    init();
    telegram_init();

    base = event_base_new();

    poll_ev = event_new(base, -1, EV_PERSIST, &handle_poll_event, NULL);
    err_ev = event_new(base, -1, EV_PERSIST, &handle_err_event, NULL);
    listen_ev = event_new(base, udp_server_socket, EV_READ | EV_PERSIST, &handle_connection, NULL);

    evtimer_add(poll_ev, &poll_tv);
    evtimer_add(err_ev, &err_tv);
    event_add(listen_ev, NULL);
    event_base_dispatch(base);
}