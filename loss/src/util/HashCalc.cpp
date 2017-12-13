/*******************************************************************************
* Project:  loss
* @file     HashCalc.cpp
* @brief 
* @author   
* @date:    2016年1月11日
* @note
* Modify history:
******************************************************************************/

#include "HashCalc.hpp"

#define ROT32(x, y) ((x << y) | (x >> (32 - y))) // avoid effort

namespace loss
{

const unsigned long FNV_64_INIT = 0x100000001b3;
const unsigned long FNV_64_PRIME = 0xcbf29ce484222325;

uint32_t CalcKeyHash(const char *key,unsigned int key_length)   // hash_fnv1a_64
{
    uint32_t hash = (uint32_t)FNV_64_INIT;
    unsigned int x;
    if(key==NULL)
        return hash;

    for (x = 0; x < key_length; x++) {
        uint32_t val = (uint32_t)key[x];
        hash ^= val;
        hash *= (uint32_t)FNV_64_PRIME;
    }
    return hash;
}

uint32_t CalcStringSum(const std::string& strKey)
{
    uint32_t uiHash = 0;
    int iValue = 0;
    for (size_t i = 0; i < strKey.size(); ++i)
    {
        iValue = strKey[i];
        iValue = (iValue > 0) ? iValue : iValue * -1;
        uiHash += iValue;
    }
    return(uiHash);
}

uint32_t hash_fnv1_64(const char *key, size_t key_length)
{
    uint64_t hash = FNV_64_INIT;
    size_t x;

    for (x = 0; x < key_length; x++) {
      hash *= FNV_64_PRIME;
      hash ^= (uint64_t)key[x];
    }

    return (uint32_t)hash;
}

uint32_t hash_fnv1a_64(const char *key, size_t key_length)
{
    uint32_t hash = (uint32_t) FNV_64_INIT;
    size_t x;

    for (x = 0; x < key_length; x++) {
      uint32_t val = (uint32_t)key[x];
      hash ^= val;
      hash *= (uint32_t) FNV_64_PRIME;
    }

    return hash;
}

uint32_t murmur3_32(const char *key, uint32_t len, uint32_t seed)
{
    static const uint32_t c1 = 0xcc9e2d51;
    static const uint32_t c2 = 0x1b873593;
    static const uint32_t r1 = 15;
    static const uint32_t r2 = 13;
    static const uint32_t m = 5;
    static const uint32_t n = 0xe6546b64;

    uint32_t hash = seed;

    const int nblocks = len / 4;
    const uint32_t *blocks = (const uint32_t *) key;
    int i;
    uint32_t k;
    for (i = 0; i < nblocks; i++)
    {
        k = blocks[i];
        k *= c1;
        k = ROT32(k, r1);
        k *= c2;

        hash ^= k;
        hash = ROT32(hash, r2) * m + n;
    }

    const uint8_t *tail = (const uint8_t *) (key + nblocks * 4);
    uint32_t k1 = 0;

    switch (len & 3)
    {
    case 3:
        k1 ^= tail[2] << 16;
    case 2:
        k1 ^= tail[1] << 8;
    case 1:
        k1 ^= tail[0];

        k1 *= c1;
        k1 = ROT32(k1, r1);
        k1 *= c2;
        hash ^= k1;
    }

    hash ^= len;
    hash ^= (hash >> 16);
    hash *= 0x85ebca6b;
    hash ^= (hash >> 13);
    hash *= 0xc2b2ae35;
    hash ^= (hash >> 16);

    return hash;
}

}

