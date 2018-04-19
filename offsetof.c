#include <stddef.h>

struct foo {
    int a;
    int b;
    char *c;
};

struct struct_desc {
    const char *name;
    int type;
    size_t off;
};

static const struct struct_desc foo_desc[] = {
        { "a", INT, offsetof(struct foo, a) },
        { "b", INT, offsetof(struct foo, b) },
        { "c", CHARPTR, offsetof(struct foo, c) },
};

int main()
{

    return EXIT_SUCCESS;
}