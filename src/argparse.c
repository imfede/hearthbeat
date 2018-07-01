#include "argparse.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

enum type { INT = 0, STR = 1, LST = 2 };

struct registered_pointer {
    char key[256];
    enum type t;
    void *ptr;
};

struct registered_pointer *registered_pointers = NULL;
size_t registered_pointers_length = 0;

struct registered_pointer *get_registered_pointer(char *key) {
    for (int i = 0; i < registered_pointers_length; i++) {
        if (strcmp(key, registered_pointers[i].key) == 0) {
            return &registered_pointers[i];
        }
    }
    return NULL;
}

void generic_callback(char *key, char *value) {
    struct registered_pointer *pointer = get_registered_pointer(key);
    if (pointer != NULL) {
        switch (pointer->t) {
        case INT:
            *((int *)(pointer->ptr)) = atoi(value);
            break;
        case STR:
            *((char **)pointer->ptr) = malloc(strlen(value) + 1);
            strcpy(*((char **)pointer->ptr), value);
            break;
        case LST:
        default:
            fprintf(stderr, "Not yet implemented!\n");
        }
    }
}

void register_pointer(char *key, enum type t, void *ptr) {
    registered_pointers = realloc(registered_pointers, (registered_pointers_length + 1) * sizeof(struct registered_pointer));
    if (registered_pointers == NULL) {
        fprintf(stderr, "Error reallocating register_pointer struct\n");
    }
    registered_pointers_length += 1;
    struct registered_pointer *new_ptr = &registered_pointers[registered_pointers_length - 1];
    strcpy(new_ptr->key, key);
    new_ptr->t = t;
    new_ptr->ptr = ptr;

    argparse_register_argument(key, &generic_callback);
}

void argparse_register_argument_int(char *key, int *ptr) { register_pointer(key, INT, (void *)ptr); }

void argparse_register_argument_str(char *key, char **ptr) { register_pointer(key, STR, (void *)ptr); }

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
                if (cb != NULL)
                    cb->callback(key, value);
            }
        }
    }
    fclose(f);
    free(callbacks);
    callbacks_length = 0;
    free(registered_pointers);
    registered_pointers_length = 0;
}
