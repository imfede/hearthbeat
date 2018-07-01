#include <stdlib.h>

#ifndef _TELEGRAM
#define _TELEGRAM

static char *telegram_token = NULL;
static int telegram_userid = 0;
static const char *telegram_url = "https://api.telegram.org/bot%s/%s";
static const char *telegram_conf = "/etc/hearthbeat/telegram";

/* call this once to read token/userid from config file and init them */
int telegram_init();

/* call this to send messages */
void telegram_send_message(char *message);

#endif