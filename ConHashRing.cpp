// ******************************************************************************
// * Description   : Consistent Hash Ring
// * Date          : 2014-11-05
// * Author        : dj
// ******************************************************************************

#include "ConHashRing.h"

uint32_t hash_fnv(const char *pKey, size_t ulen)
{
    uint32_t uMagic = 16777619;
    uint32_t uHash = 0x811C9DC5;

    while (ulen--)
    {
        uHash = (uHash ^ (*(unsigned char *)pKey)) * uMagic;
        pKey++;
    }

    uHash += uHash << 13;
    uHash ^= uHash >> 7;
    uHash += uHash << 3;
    uHash ^= uHash >> 17;
    uHash += uHash << 5;

    return uHash;    
}

#undef get16bits
#if (defined(__GNUC__) && defined(__i386__))
#define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
                              +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif
uint32_t hash_hsieh(const char * key, size_t len)
{
    uint32_t hash = 0, tmp;
    int rem;

    if (len <= 0 || key == NULL) {
        return 0;
    }

    rem = len & 3;
    len >>= 2;

    for (; len > 0; len--) {
        hash += get16bits (key);
        tmp = (get16bits (key+2) << 11) ^ hash;
        hash = (hash << 16) ^ tmp;
        key += 2*sizeof (uint16_t);
        hash += hash >> 11;
    }

    switch (rem) {
        case 3:
            hash += get16bits (key);
            hash ^= hash << 16;
            hash ^= (uint32_t)key[sizeof (uint16_t)] << 18;
            hash += hash >> 11;
            break;

        case 2:
            hash += get16bits (key);
            hash ^= hash << 11;
            hash += hash >> 17;
            break;

        case 1:
            hash += (unsigned char)(*key);
            hash ^= hash << 10;
            hash += hash >> 1;

        default:
            break;
    }

    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}
#undef get16bits

uint32_t hash_murmur(const char * key, size_t length)
{
    /*
     * 'm' and 'r' are mixing constants generated offline.  They're not
     * really 'magic', they just happen to work well.
     */

    const unsigned int m = 0x5bd1e995;
    const uint32_t seed = (0xdeadbeef * (uint32_t)length);
    const int r = 24;


    /* Initialize the hash to a 'random' value */

    uint32_t h = seed ^ (uint32_t)length;

    /* Mix 4 bytes at a time into the hash */

    const unsigned char * data = (const unsigned char *)key;

    while (length >= 4) {
        unsigned int k = *(unsigned int *)data;

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        length -= 4;
    }

    /* Handle the last few bytes of the input array */

    switch(length) {
        case 3:
            h ^= ((uint32_t)data[2]) << 16;

        case 2:
            h ^= ((uint32_t)data[1]) << 8;

        case 1:
            h ^= data[0];
            h *= m;

        default:
            break;
    };

    /*
     * Do a few final mixes of the hash to ensure the last few bytes are
     * well-incorporated.
     */

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
}

