#include "../headers/factory.h"

void build_factory(struct factory **f) {
    *f = malloc(sizeof(struct factory));
}


maker make(struct factory *f, char *key) {
    for (size_t cur_item_idx = 0; cur_item_idx < f->len; cur_item_idx++)
    {
        if (strcmp(f->items[cur_item_idx].key, key) == 0) {
            return f->items[cur_item_idx].callback;
        }
    } 

    return NULL;
}

void register_item(struct factory *f, char *key, maker callback) {
    f->items = realloc(f->items, (f->len + 1) * sizeof(struct item));

    struct item i = { key, callback };
    f->items[f->len] = i;
    f->len++;
}

char **keys(struct factory *f) {
    char **result = malloc(f->len * sizeof(char*));

    for (size_t cur_item_idx = 0; cur_item_idx < f->len; cur_item_idx++)
    {
        result[cur_item_idx] = f->items[cur_item_idx].key;
    }

    return result;
}