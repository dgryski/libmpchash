/* mpmpchash.h */
#ifndef _MPmpchash_H_
#define _MPmpchash_H_

struct mpchash_t;

struct mpchash_t *mpchash_create(const char **node_names, size_t * name_lens, size_t num_names, size_t k, uint64_t seed1, uint64_t seed2);

void mpchash_lookup(struct mpchash_t *mpchash, const char *key, size_t len, const char **node_name, size_t * name_len);

void mpchash_free(struct mpchash_t *mpchash);

#endif /* _mpchash_H_ */
