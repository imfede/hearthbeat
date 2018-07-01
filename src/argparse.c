#include "argparse.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int callbacks_length = 0;
struct callback *callbacks = NULL;

void argparse_register_argument(char *key, args_callback_t function) {
    callbacks_length += 1;
    callbacks = realloc(callbacks, callbacks_length * sizeof(struct callback));
    if (callbacks == NULL) {
        fprintf(stderr, "Error reallocating callbacks struct\n");
    }
    struct callback *new_cb = &callbacks[callbacks_length - 1];
    strcpy(new_cb->key, key);
    new_cb->callback = function;
}

struct callback *get_callback(char *key) {
    for (int i = 0; i < callbacks_length; i++) {
        if (strcmp(key, callbacks[i].key) == 0) {
            return &callbacks[i];
        }
    }
    return NULL;
}

void argparse_read_properties(const char *path) {
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        fprintf(stderr, "Cannot open config file, error: %d\n", errno);
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), f) != NULL) {
        if (line[0] != '\n' && line[0] != '#') {
            char key[256];
            char value[256];
            memset(key, '\0', sizeof(key));
            memset(value, '\0', sizeof(value));
            char *eq = strchr(line, '=');
            char *nl = strchr(line, '\n');
            if (eq != NULL && nl != NULL) {
                strncpy(key, line, eq - line);
                strncpy(value, eq + 1, nl - eq - 1);

                struct callback *cb = get_callback(key);
                cb->callback(value);
            }
        }
    }
    fclose(f);
    free(callbacks);
    callbacks_length = 0;
}
