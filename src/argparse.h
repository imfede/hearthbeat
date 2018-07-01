
#ifndef _ARGPARSE
#define _ARGPARSE

/* callback type for storing the value clientside */
typedef void (*args_callback_t)(char *value);

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

/* call this to start the reaction (it will free all the resources) */
void argparse_read_properties(const char *path);

#endif