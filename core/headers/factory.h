#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void (*maker)(int, char**);

struct item {
    char* key;
    maker callback;
};

struct factory
{
    struct item *items;
    int len;
};

void build_factory(struct factory **f);

maker make(struct factory *f, char *key);

void register_item(struct factory *f, char *key, maker callback);

char **keys(struct factory *f);