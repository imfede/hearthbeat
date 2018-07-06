#include "../src/argparse.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int test_list() {
    struct list_cell *list = NULL;

    argparse_register_argument_strlst("testlst", &list);
    argparse_read_properties("test/test_list.txt");

    struct list_cell *it = list;

    while(it != NULL) {
        printf("%s\n", it->value);
        it = it->next;
    }

    free_list_cell(list);
}

int main() {
    test_list();
}