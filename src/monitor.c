#include "argparse.h"
#include "logtime.h"
#include "telegram.h"
#include <arpa/inet.h>
#include <errno.h>
#include <event.h>
#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

char *configfile = "/etc/hearthbeat/monitor";
int poll_interval;
int err_interval;
char *myname;

struct target {
    char *host;
    char *port;
    char *name;
    struct event *poll_ev;
    struct event *err_ev;
    bool isonline;
};
struct target *targets = NULL;
int targets_length = 0;

int udp_server_socket = 0;
char *message = "bip?";

struct event *listen_ev;
struct event_base *base;
struct timeval poll_tv;
struct timeval err_tv;

void die(int code, char *message, ...) {
    va_list fmtargs;
    fprintf(stderr, message, fmtargs);
    exit(code);
}

void handle_signal(int sig) {
    char buffer[256];
    snprintf(buffer, 256, "Stopping monitor on %s", myname);
    telegram_send_message(buffer);
    logtime_close();
    exit(0);
}

void init() {
    signal(SIGTERM, &handle_signal);
    signal(SIGINT, &handle_signal);

    struct list_cell *list = NULL;
    argparse_register_argument_strlst("target", &list);
    argparse_register_argument_str_def("myname", &myname, "myself");
    argparse_register_argument_int("poll_interval", &poll_interval);
    argparse_register_argument_int("err_interval", &err_interval);
    argparse_read_properties(configfile);

    struct list_cell *iterator = list;
    while (iterator != NULL) {
        targets_length += 1;
        targets = realloc(targets, targets_length * sizeof(struct target));
        if (targets == NULL) {
            die(errno, "Error reallocating targets\n");
        }
        struct target *newtarget = &targets[targets_length - 1];
        int totallength = strlen(iterator->value);
        char *firstcolon = strchr(iterator->value, ':');
        char *lastcolon = strrchr(iterator->value, ':');
        if (firstcolon == NULL || lastcolon == NULL) {
            die(-1, "Not enough ':' in target definition: %s\n", iterator->value);
        }

        newtarget->name = malloc(firstcolon - iterator->value + 1);
        memset(newtarget->name, 0x00, firstcolon - iterator->value + 1);
        strncpy(newtarget->name, iterator->value, firstcolon - iterator->value);

        newtarget->host = malloc(lastcolon - firstcolon + 1);
        memset(newtarget->host, 0x00, lastcolon - firstcolon + 1);
        strncpy(newtarget->host, firstcolon + 1, lastcolon - firstcolon - 1);

        newtarget->port = malloc(iterator->value + totallength - lastcolon + 1);
        memset(newtarget->port, 0x00, iterator->value + totallength - lastcolon + 1);
        strncpy(newtarget->port, lastcolon + 1, iterator->value + totallength - lastcolon);

        newtarget->poll_ev = NULL;
        newtarget->err_ev = NULL;
        newtarget->isonline = false;

        iterator = iterator->next;
    }
    free_list_cell(list);

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

struct target *get_target(struct sockaddr_in *client) {
    for (int i = 0; i < targets_length; i++) {
        struct target *iterator = &targets[i];
        struct sockaddr_in resolved = {0};
        lookup_target(iterator->host, iterator->port, &resolved);
        if (client->sin_addr.s_addr == resolved.sin_addr.s_addr && client->sin_port == resolved.sin_port) {
            return iterator;
        }
    }
    return NULL;
}

void send_bip(struct target *arg) {
    struct sockaddr_in target;
    if (lookup_target(arg->host, arg->port, &target) != 0) {
        fprintf(stderr, "Lookup error!\n");
        return;
    }

    if (sendto(udp_server_socket, message, strlen(message), 0, (struct sockaddr *)&target, sizeof(target)) < 0) {
        fprintf(stderr, "Error sending: %d\n", errno);
    } else {
        logtime_set_start(arg->name);
        printf("Sending to %s: %s\n", arg->name, message);
    }
}

void handle_poll_event(int fd, short event, void *arg) { send_bip((struct target *)arg); }

void handle_err_event(int fd, short event, void *arg) {
    struct target *target = (struct target *) arg;
    if (target->isonline) {
        target->isonline = false;
        printf("%s: Host %s is down!\n", myname, target->name);
        char buffer[256];
        snprintf(buffer, 256, "%s: Host %s is down!", myname, target->name);
        telegram_send_message(buffer);
    }
}

void reset_error_timer(struct target *target) {
    if (!target->isonline) {
        printf("%s: Host %s is up!\n", myname, target->name);
        char buffer[256];
        snprintf(buffer, 256, "%s: Host %s is up!", myname, target->name);
        telegram_send_message(buffer);
    }
    target->isonline = true;
    evtimer_del(target->err_ev);
    evtimer_add(target->err_ev, &err_tv);
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
        struct target *target = get_target(&client_addr);
        if (target != NULL) {
            reset_error_timer(target);
            logtime_set_record(target->name);
            printf("Received from %s: %s\n", target->name, buffer);
        }
    }
}

void handle_connection(int fd, short event, void *arg) { handle_answer(fd); }

int main() {
    init();
    logtime_init();
    telegram_init();

    base = event_base_new();

    for (int i = 0; i < targets_length; i++) {
        struct target *iterator = &targets[i];
        iterator->poll_ev = event_new(base, -1, EV_PERSIST, &handle_poll_event, iterator);
        iterator->err_ev = event_new(base, -1, EV_PERSIST, &handle_err_event, iterator);

        evtimer_add(iterator->poll_ev, &poll_tv);
        evtimer_add(iterator->err_ev, &err_tv);
    }

    listen_ev = event_new(base, udp_server_socket, EV_READ | EV_PERSIST, &handle_connection, NULL);
    event_add(listen_ev, NULL);

    char buffer[256];
    snprintf(buffer, 256, "Starting monitor on %s", myname);
    telegram_send_message(buffer);
    event_base_dispatch(base);
}