#include "logtime.h"
#include "../libs/sqlite3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

struct named_clock {
    char name[256];
    struct timespec departure;
};

struct named_clock *clocks = NULL;
int clocks_length = 0;

sqlite3 *handle;

struct named_clock *get_named_clock(char *name) {
    for (int i = 0; i < clocks_length; i++) {
        if (strcmp(name, clocks[i].name) == 0) {
            return &clocks[i];
        }
    }
    return NULL;
}

void timespec_diff(struct timespec *start, struct timespec *stop, struct timespec *result) {
    if ((stop->tv_nsec - start->tv_nsec) < 0) {
        result->tv_sec = stop->tv_sec - start->tv_sec - 1;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
    } else {
        result->tv_sec = stop->tv_sec - start->tv_sec;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec;
    }
}

void handle_interval(char *name, struct timespec *start, struct timespec *end) {
    struct timespec interval = {0};
    timespec_diff(start, end, &interval);
    printf("Clock %s: %lds %ldns\n", name, interval.tv_sec, interval.tv_nsec);
}

void logtime_set_start(char *name) {
    struct named_clock *nclock = get_named_clock(name);
    if (nclock == NULL) {
        clocks_length += 1;
        clocks = realloc(clocks, clocks_length * sizeof(struct named_clock));
        nclock = &clocks[clocks_length - 1];

        memset(nclock->name, 0x00, 256);
        strncpy(nclock->name, name, 256);
    }
    clock_gettime(CLOCK_REALTIME, &nclock->departure);
}

void logtime_set_record(char *name) {
    struct named_clock *nclock = get_named_clock(name);
    if (nclock == NULL) {
        fprintf(stderr, "No clock named %s started\n", name);
    } else {
        struct timespec arrival_time;
        clock_gettime(CLOCK_REALTIME, &arrival_time);
        handle_interval(name, &nclock->departure, &arrival_time);
    }
}

void logtime_init() {
    sqlite3_open("/var/hearthbeat/database.db", &handle);
    char *query = "CREATE TABLE test (id int);";
    sqlite3_stmt *stmt;
    sqlite3_prepare(handle, query, strlen(query), &stmt, NULL);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_close(handle);
}

void logtime_close() { sqlite3_close(handle); }