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

void execute_simple_query(char *query) {
    sqlite3_stmt *stmt;
    sqlite3_prepare(handle, query, strlen(query), &stmt, NULL);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

int get_target_id(char *name) {
    sqlite3_stmt *stmt;
    char *query = "SELECT id FROM targets WHERE name = ?;";
    sqlite3_prepare(handle, query, strlen(query), &stmt, NULL);
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    int res = sqlite3_step(stmt);
    int id = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    return res == SQLITE_DONE ? -1 : id;
}

void execute_insert_query(char *name, struct timespec *time) {
    int id = get_target_id(name);
    char *query;
    sqlite3_stmt *stmt;

    if (id == -1) {
        query = "INSERT INTO targets (name) VALUES (?);";
        sqlite3_prepare(handle, query, strlen(query), &stmt, NULL);
        sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        id = get_target_id(name);
    }

    query = "INSERT INTO clocks (target, s, ns) VALUES (?, ?, ?);";
    sqlite3_prepare(handle, query, strlen(query), &stmt, NULL);
    sqlite3_bind_int(stmt, 1, id);
    sqlite3_bind_int(stmt, 2, time->tv_sec);
    sqlite3_bind_int(stmt, 3, time->tv_nsec);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void logtime_init() {
    sqlite3_open(logtime_conf, &handle);

    execute_simple_query("CREATE TABLE IF NOT EXISTS targets ("
                         "  id INTEGER PRIMARY KEY,"
                         "  name TEXT);");

    execute_simple_query("CREATE TABLE IF NOT EXISTS clocks ("
                         "  target INTEGER,"
                         "  time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
                         "  s INTEGER,"
                         "  ns INTEGER,"
                         "  FOREIGN KEY(target) REFERENCES targets(id));");
}

void logtime_close() { sqlite3_close(handle); }

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

        struct timespec interval;
        timespec_diff(&nclock->departure, &arrival_time, &interval);

        execute_insert_query(name, &interval);
    }
}