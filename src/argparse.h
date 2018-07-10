#ifndef _ARGPARSE
#define _ARGPARSE

/* callback type for storing the value clientside */
typedef void (*args_callback_t)(char *key, char *value);

/* internal struct used to store one argument */
struct callback {
    char key[256];
    args_callback_t callback;
};

/* arraylist */
static struct callback *callbacks;
static int callbacks_length;

/* call this function to register an argument in the arraylist. key wll be copied */
void argparse_register_argument(char *key, args_callback_t function);

/* or use those ready made helpers for common cases */
typedef struct list_cell {
    struct list_cell *next;
    char value[256];
} list_cell;
void free_list_cell(struct list_cell *list);

void argparse_register_argument_int(char *key, int *ptr);
void argparse_register_argument_int_def(char *key, int *ptr, int def);
void argparse_register_argument_str(char *key, char **ptr);
void argparse_register_argument_str_def(char *key, char **ptr, char *def);
void argparse_register_argument_strlst(char *key, struct list_cell **ptr);

/* call this to start the reaction (it will free all the resources) */
void argparse_read_properties(const char *path);

#endif