#include "logtime.h"
#include <stdio.h>
#include <sys/time.h>

struct timespec departure_time = {0};

void handle_interval(struct timespec *start, struct timespec *end) {
    printf("Recorded: %lds %ldns\n", end->tv_sec - start->tv_sec, end->tv_nsec - start->tv_nsec);
}

void logtime_set_start() { clock_gettime(CLOCK_REALTIME, &departure_time); }

void logtime_set_record() {
    struct timespec arrival_time;
    clock_gettime(CLOCK_REALTIME, &arrival_time);
    handle_interval(&departure_time, &arrival_time);
}