#ifndef HashCalc_HPP
#define HashCalc_HPP

#define  __STDC_CONSTANT_MACROS
#include <stdlib.h>
#include <stdint.h>
#include <string>

namespace loss
{


uint32_t CalcKeyHash(const char *key,unsigned int key_length);  // hash_fnv1a_64

uint32_t hash_fnv1_64(const char *key, size_t key_length);

uint32_t hash_fnv1a_64(const char *key, size_t key_length);

uint32_t murmur3_32(const char *key, size_t key_length, uint32_t seed);

}

#endif
