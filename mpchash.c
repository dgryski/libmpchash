/* mpchash.c */
/* This library implements multi-probe consistent hashing */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "mpchash.h"

uint64_t siphash(uint64_t k0, uint64_t k1, const unsigned char *m,
		 size_t len);

struct bucket_t {
    uint64_t point;
    size_t node_idx;
} bucket_t;

struct mpchash_t {
    struct bucket_t *blist;
    size_t nbuckets;
    char **node_names;
    size_t *name_lens;
    size_t num_names;
    size_t probes;
    uint64_t seeds[2];
} mpchash_t;

static int cmpbucket(const void *a, const void *b)
{
    struct bucket_t *b1 = (struct bucket_t *) a;
    struct bucket_t *b2 = (struct bucket_t *) b;

    if (b1->point < b2->point) {
	return -1;
    }

    if (b1->point > b2->point) {
	return 1;
    }

    return 0;
}

struct mpchash_t *mpchash_create(const char **node_names,
				 size_t * name_lens, size_t num_names,
				 size_t probes, uint64_t seed1,
				 uint64_t seed2)
{
    struct mpchash_t *mpchash;

    struct bucket_t *blist =
	(struct bucket_t *) malloc(sizeof(bucket_t) * num_names);
    char **nlist = (char **) malloc(sizeof(char *) * num_names);
    size_t *lens = (size_t *) malloc(sizeof(size_t) * num_names);
    size_t n, bidx = 0;

    for (n = 0; n < num_names; n++) {
	nlist[n] = (char *) malloc(sizeof(char) * name_lens[n]);
	lens[n] = name_lens[n];
	memcpy(nlist[n], node_names[n], lens[n]);
	blist[bidx].node_idx = n;
	blist[bidx].point =
	    siphash(0, 0, (const unsigned char *) nlist[n], lens[n]);
	bidx++;
    }

    qsort(blist, bidx, sizeof(struct bucket_t), cmpbucket);

    mpchash = malloc(sizeof(mpchash_t));
    mpchash->blist = blist;
    mpchash->nbuckets = bidx;
    mpchash->node_names = nlist;
    mpchash->name_lens = lens;
    mpchash->num_names = num_names;
    mpchash->probes = probes;
    mpchash->seeds[0] = seed1;
    mpchash->seeds[1] = seed2;

    return mpchash;
}

void mpchash_lookup(struct mpchash_t *mpchash, const char *key, size_t len,
		    const char **node_name, size_t * name_len)
{
    size_t k;
    uint64_t distance;
    uint64_t min_distance = 0xffffffffffffffff;
    size_t min_node;
    uint64_t h1;
    uint64_t h2;

    h1 = siphash(mpchash->seeds[0], 0, (const unsigned char *) key, len);
    h2 = siphash(mpchash->seeds[1], 0, (const unsigned char *) key, len);

    for (k = 0; k < mpchash->probes; k++) {
	uint64_t point = h1 + k * h2;
	uint32_t low = 0, high = mpchash->nbuckets;

	/* binary search through blist */
	while (low < high) {
	    uint32_t mid = low + (high - low) / 2;
	    if (mpchash->blist[mid].point > point) {
		high = mid;
	    } else {
		low = mid + 1;
	    }
	}

	if (low >= mpchash->nbuckets) {
	    low = 0;
	}

	distance = mpchash->blist[low].point - point;
	if (distance < min_distance) {
	    min_distance = distance;
	    min_node = mpchash->blist[low].node_idx;
	}
    }

    *node_name = mpchash->node_names[min_node];
    *name_len = mpchash->name_lens[min_node];
}

void mpchash_free(struct mpchash_t *mpchash)
{
    size_t i;

    for (i = 0; i < mpchash->num_names; i++) {
	free(mpchash->node_names[i]);
    }
    free(mpchash->node_names);
    free(mpchash->name_lens);
    free(mpchash->blist);
    free(mpchash);
}

/* Floodyberry's public-domain siphash: https://github.com/floodyberry/siphash */
static uint64_t U8TO64_LE(const unsigned char *p)
{
    return *(const uint64_t *) p;
}

#define ROTL64(a,b) (((a)<<(b))|((a)>>(64-b)))

uint64_t siphash(uint64_t k0, uint64_t k1, const unsigned char *m,
		 size_t len)
{
    uint64_t v0, v1, v2, v3;
    uint64_t mi;
    uint64_t last7;
    size_t i, blocks;

    v0 = k0 ^ 0x736f6d6570736575ull;
    v1 = k1 ^ 0x646f72616e646f6dull;
    v2 = k0 ^ 0x6c7967656e657261ull;
    v3 = k1 ^ 0x7465646279746573ull;

    last7 = (uint64_t) (len & 0xff) << 56;

#define sipcompress() \
	v0 += v1; v2 += v3; \
	v1 = ROTL64(v1,13);	v3 = ROTL64(v3,16); \
	v1 ^= v0; v3 ^= v2; \
	v0 = ROTL64(v0,32); \
	v2 += v1; v0 += v3; \
	v1 = ROTL64(v1,17); v3 = ROTL64(v3,21); \
	v1 ^= v2; v3 ^= v0; \
	v2 = ROTL64(v2,32);

    for (i = 0, blocks = (len & ~7); i < blocks; i += 8) {
	mi = U8TO64_LE(m + i);
	v3 ^= mi;
	sipcompress()
	    sipcompress()
	    v0 ^= mi;
    }

    switch (len - blocks) {
    case 7:
	last7 |= (uint64_t) m[i + 6] << 48;
    case 6:
	last7 |= (uint64_t) m[i + 5] << 40;
    case 5:
	last7 |= (uint64_t) m[i + 4] << 32;
    case 4:
	last7 |= (uint64_t) m[i + 3] << 24;
    case 3:
	last7 |= (uint64_t) m[i + 2] << 16;
    case 2:
	last7 |= (uint64_t) m[i + 1] << 8;
    case 1:
	last7 |= (uint64_t) m[i + 0];
    case 0:
    default:;
    };
    v3 ^= last7;
    sipcompress()
	sipcompress()
	v0 ^= last7;
    v2 ^= 0xff;
    sipcompress()
	sipcompress()
	sipcompress()
	sipcompress()
	return v0 ^ v1 ^ v2 ^ v3;
}
