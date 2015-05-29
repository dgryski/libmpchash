#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "mpchash.h"

int main()
{
    struct mpchash_t *mpchash;
    unsigned int fails = 0;
    int i;

    char *node_names[6000];
    size_t lens[6000];

    FILE *f;
    char line[80];

    for (i = 1; i <= 6000; i++) {
        lens[i-1] = asprintf(&node_names[i-1], "shard-%d", i);
    }

    mpchash = mpchash_create((const char **)node_names, lens, 6000, 21, 1, 2);

    f = fopen("testdata/compat.out", "r");
    if (f == NULL) {
        printf("unable to open compat.out: %s", strerror(errno));
    }

    i = 0;
    while(fgets(line, sizeof(line), f)) {
        size_t linelen = strlen(line);
        const char *node;
        size_t len;
        linelen--;
        line[linelen]='\0';
        mpchash_lookup(mpchash, node_names[i], lens[i], &node, &len);
        if (linelen != len || memcmp(line, node, len) != 0) {
                fails++;
                printf("lookup(%s)=%s, want %s\n", node_names[i], node, line);
        }
        i++;
    }

    mpchash_free(mpchash);

    for(i=0; i < 6000; i++) {
        free(node_names[i]);
    }

    printf("fails=%d\n", fails);

    return (!!fails);
}
