// ******************************************************************************
// * Description   : Consistent Hash Ring
// * Date          : 2014-11-05
// * Author        : dj
// ******************************************************************************

#ifndef __CON_HASH_RING_H__
#define __CON_HASH_RING_H__

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <map>

// hash algorithm
enum CON_HASH_FUNC
{
    CHASH_FUNC_FNV = 0,
    CHASH_FUNC_HSIEH,
    CHASH_FUNC_MURMUR,
};

uint32_t hash_fnv(const char * key, size_t len);
uint32_t hash_hsieh(const char * key, size_t len);
uint32_t hash_murmur(const char * key, size_t len);

template <typename Node>
class ConHashRing
{
private:
    typedef std::map<uint32_t, Node>   hash_circle_t;
    typedef uint32_t (*hashfunc)(const char * key, size_t len);

    hash_circle_t   m_mapHashCircle;
    hashfunc        m_pHashFunc;
    uint32_t        m_uHashReplicas;

    typename hash_circle_t::iterator    m_itRoundrobin;

    static const uint32_t MAX_WEIGHT = 10;

public:
    ConHashRing(CON_HASH_FUNC eFuncType = CHASH_FUNC_MURMUR, uint32_t uReplicas = 168)
        : m_uHashReplicas(uReplicas)
    {
        switch (eFuncType)
        {
        case CHASH_FUNC_FNV:    m_pHashFunc = hash_fnv;     break;
        case CHASH_FUNC_HSIEH:  m_pHashFunc = hash_hsieh;   break;
        case CHASH_FUNC_MURMUR: m_pHashFunc = hash_murmur;  break;
        default:                m_pHashFunc = hash_murmur;  break;
        }

        m_itRoundrobin = m_mapHashCircle.end();
    }

    uint32_t getNodeCount() { return m_mapHashCircle.size(); }
    bool isEmpty() { return m_mapHashCircle.empty(); }

    // trivial type Node (like int 8/16/32/64 bit)
    void addHashNode(Node val, uint32_t weight = 1);
    void removeHashNode(Node val, uint32_t weight = 1);

    template <typename Key>
        bool lookupNode(Key k, Node & val);
    template <typename Key>
        bool lookupNode(Key k, Node & val, Node & backup);

    bool lookupNode(const char * key, size_t len, Node & val);
    bool lookupNode(const char * key, size_t len, Node & val, Node & backup);

    bool roundRobinNode(Node & val);
    bool roundRobinNode(Node & val, Node & backup);
    
    // non-trivial type Node
    void addHashNode(const Node & val, const char * name, size_t len, uint32_t weight = 1);
    void removeHashNode(const char * name, size_t len, uint32_t weight = 1);
};

template <typename Node>
void ConHashRing<Node>::addHashNode(Node val, uint32_t weight)
{
    if (weight > MAX_WEIGHT) weight = MAX_WEIGHT;

    uint32_t uHash = 0;
    char buf[sizeof(Node) + 32];
    ::memcpy(buf, &val, sizeof(Node));

    char * pIndex = buf + sizeof(Node);
    size_t uLen = 0;

    for (uint32_t w = 0; w < weight; ++w) {
        for (uint32_t i = 0; i < m_uHashReplicas; ++i) {
            uLen = sizeof(Node) + ::snprintf(pIndex, 32, "%u%u", w, i);
            uHash = m_pHashFunc(buf, uLen);
            m_mapHashCircle[uHash] = val;
        }
    }
}
template <typename Node>
void ConHashRing<Node>::removeHashNode(Node val, uint32_t weight)
{
    if (weight > MAX_WEIGHT) weight = MAX_WEIGHT;

    uint32_t uHash = 0;
    char buf[sizeof(Node) + 32];
    ::memcpy(buf, &val, sizeof(Node));

    char * pIndex = buf + sizeof(Node);
    size_t uLen = 0;

    for (uint32_t w = 0; w < weight; ++w) {
        for (uint32_t i = 0; i < m_uHashReplicas; ++i) {
            uLen = sizeof(Node) + ::snprintf(pIndex, 32, "%u%u", w, i);
            uHash = m_pHashFunc(buf, uLen);

            typename hash_circle_t::iterator it = m_mapHashCircle.find(uHash);
            if (it != m_mapHashCircle.end()) {
                if (it == m_itRoundrobin) {
                    ++m_itRoundrobin;
                }

                m_mapHashCircle.erase(it);
            }
        }
    }
}
template <typename Node>
    template <typename Key>
bool ConHashRing<Node>::lookupNode(Key k, Node & val)
{
    if (m_mapHashCircle.empty()) {
        return false;
    }

    uint32_t uHash = m_pHashFunc((const char *)&k, sizeof(Key));
    typename hash_circle_t::iterator it = m_mapHashCircle.lower_bound(uHash);
    if (it == m_mapHashCircle.end()) {
        it = m_mapHashCircle.begin();
    }
    val = it->second;

    return true;
}
template <typename Node>
    template <typename Key>
bool ConHashRing<Node>::lookupNode(Key k, Node & val, Node & backup)
{
    if (m_mapHashCircle.empty()) {
        return false;
    }

    uint32_t uHash = m_pHashFunc((const char *)&k, sizeof(Key));
    typename hash_circle_t::iterator it = m_mapHashCircle.lower_bound(uHash);
    if (it == m_mapHashCircle.end()) {
        it = m_mapHashCircle.begin();
    }
    val = it->second;

    uint32_t step = 0;
    while ((it->second == val) && (step < m_uHashReplicas)) {
        if (++it == m_mapHashCircle.end()) {
            it = m_mapHashCircle.begin();
        }
        ++step;
    }
    backup = it->second;

    return true;
}
template <typename Node>
bool ConHashRing<Node>::lookupNode(const char * key, size_t len, Node & val)
{
    if (m_mapHashCircle.empty()) {
        return false;
    }

    uint32_t uHash = m_pHashFunc(key, len);
    typename hash_circle_t::iterator it = m_mapHashCircle.lower_bound(uHash);
    if (it == m_mapHashCircle.end()) {
        it = m_mapHashCircle.begin();
    }
    val = it->second;

    return true;
}
template <typename Node>
bool ConHashRing<Node>::lookupNode(const char * key, size_t len, Node & val, Node & backup)
{
    if (m_mapHashCircle.empty()) {
        return false;
    }
    uint32_t uHash = m_pHashFunc(key, len);
    typename hash_circle_t::iterator it = m_mapHashCircle.lower_bound(uHash);
    if (it == m_mapHashCircle.end()) {
        it = m_mapHashCircle.begin();
    }
    val = it->second;

    uint32_t step = 0;
    while ((it->second == val) && (step < m_uHashReplicas)) {
        if (++it == m_mapHashCircle.end()) {
            it = m_mapHashCircle.begin();
        }
        ++step;
    }
    backup = it->second;

    return true;
}

template <typename Node>
void ConHashRing<Node>::addHashNode(const Node & val, const char * name, size_t len, uint32_t weight)
{
    if (weight > MAX_WEIGHT) weight = MAX_WEIGHT;

    uint32_t uHash = 0;
    char buf[len + 32];
    ::memcpy(buf, name, len);

    char * pIndex = buf + len;
    size_t uLen = 0;

    for (uint32_t w = 0; w < weight; ++w) {
        for (uint32_t i = 0; i < m_uHashReplicas; ++i) {
            uLen = len + ::snprintf(pIndex, 32, "%u%u", w, i);
            uHash = m_pHashFunc(buf, uLen);
            m_mapHashCircle[uHash] = val;
        }
    }
}
template <typename Node>
void ConHashRing<Node>::removeHashNode(const char * name, size_t len, uint32_t weight)
{
    if (weight > MAX_WEIGHT) weight = MAX_WEIGHT;

    uint32_t uHash = 0;
    char buf[len + 32];
    ::memcpy(buf, name, len);

    char * pIndex = buf + len;
    size_t uLen = 0;

    for (uint32_t w = 0; w < weight; ++w) {
        for (uint32_t i = 0; i < m_uHashReplicas; ++i) {
            uLen = len + ::snprintf(pIndex, 32, "%u%u", w, i);
            uHash = m_pHashFunc(buf, uLen);

            typename hash_circle_t::iterator it = m_mapHashCircle.find(uHash);
            if (it != m_mapHashCircle.end()) {
                if (it == m_itRoundrobin) {
                    ++m_itRoundrobin;
                }

                m_mapHashCircle.erase(it);
            }
        }
    }
}

template <typename Node>
bool ConHashRing<Node>::roundRobinNode(Node & val)
{
    if (m_mapHashCircle.empty()) {
        return false;
    }

    if (m_itRoundrobin == m_mapHashCircle.end()) {
        m_itRoundrobin = m_mapHashCircle.begin();
    }

    val = m_itRoundrobin->second;

    uint32_t step = 0;
    const uint32_t MAX_STEP = m_mapHashCircle.size() / m_uHashReplicas;
    while ((m_itRoundrobin->second == val) && (step < MAX_STEP)) {
        if (++m_itRoundrobin == m_mapHashCircle.end()) {
            m_itRoundrobin = m_mapHashCircle.begin();
        }
        ++step;
    }

    return true;
}
template <typename Node>
bool ConHashRing<Node>::roundRobinNode(Node & val, Node & backup)
{
    if (m_mapHashCircle.empty()) {
        return false;
    }

    if (m_itRoundrobin == m_mapHashCircle.end()) {
        m_itRoundrobin = m_mapHashCircle.begin();
    }

    val = m_itRoundrobin->second;

    uint32_t step = 0;
    const uint32_t MAX_STEP = m_mapHashCircle.size() / m_uHashReplicas;
    while ((m_itRoundrobin->second == val) && (step < MAX_WEIGHT)) {
        if (++m_itRoundrobin == m_mapHashCircle.end()) {
            m_itRoundrobin = m_mapHashCircle.begin();
        }
        ++step;
    }
    backup = m_itRoundrobin->second;

    return true;
}

#endif
