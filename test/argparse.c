#include "../src/argparse.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int test_list() {
    struct list_cell *list = NULL;

    argparse_register_argument_strlst("testlst", &list);
    argparse_read_properties("test/test_list.txt");

    struct list_cell *it = list;

    int i = 0;
    while(it != NULL) {
        printf("list element %d: %s\n", i, it->value);
        it = it->next;
        i += 1;
    }

    free_list_cell(list);
}

int test_str() {
    char *str;
    argparse_register_argument_str("str", &str);
    argparse_read_properties("test/test_list.txt");

    printf("str: %s\n", str);
    free(str);
}

int test_int() {
    int i;
    argparse_register_argument_int("int", &i);
    argparse_read_properties("test/test_list.txt");

    printf("int: %d\n", i);
}

int main() {
    test_list();
    test_str();
    test_int();
}