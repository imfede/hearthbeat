#include "telegram.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    telegram_init();
    telegram_send_message("this is a test");
}