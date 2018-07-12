#ifndef _LOGTIME
#define _LOGTIME

static char *logtime_conf = "/var/hearthbeat/database.db";

void logtime_set_start(char *name);
void logtime_set_record(char *name);
void logtime_init();
void logtime_close();

#endif