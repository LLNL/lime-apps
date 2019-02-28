
#ifndef _SHORT_H
#define _SHORT_H

#ifdef __cplusplus
extern "C" {
#endif

extern void short128_hash(const void *key, int len, int seed, void *out);
extern void short64_hash(const void *key, int len, int seed, void *out);
extern void short32_hash(const void *key, int len, int seed, void *out);

#ifdef __cplusplus
}
#endif

#endif /* _SHORT_H */
