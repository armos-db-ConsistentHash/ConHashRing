// ******************************************************************************
// * Description   : Consistent Hash Ring
// * Date          : 2014-11-05
// * Author        : dj
// ******************************************************************************

#include "ConHashRing.h"

#include <sys/time.h>
#include <cstdlib>
#include <iostream>
#include <iomanip>

class time_measure
{
    public:
        time_measure()
        {
            struct timeval tv;
            ::gettimeofday(&tv, NULL);
            m_uStart = ((uint64_t)tv.tv_sec) * 1000 * 1000 + tv.tv_usec;
        }
        ~time_measure()
        {
            struct timeval tv;
            ::gettimeofday(&tv, NULL);
            m_uStart = ((uint64_t)tv.tv_sec) * 1000 * 1000 + tv.tv_usec - m_uStart;
            std::cout << "---tick usec:" << m_uStart << std::endl;
        }
    private:
        uint64_t m_uStart;
};

template<typename T>
void lookupNode(ConHashRing<T> & intRing, int count)
{
    if (intRing.isEmpty()) {
        return ;
    }

    std::cout << "=== start ===" << std::endl;
    std::cout << "  ===hash Node" << std::endl;
    T uNode = -1;
    std::map<T, uint32_t> mapNode2Count;
    for (int i = 0; i < count - 1; ++i) {
        uint32_t uKey = ::rand();
        intRing.lookupNode(uKey, uNode);
        mapNode2Count[uNode]++;
    }
    char buf[32];
    size_t len = ::snprintf(buf, sizeof(buf), "%u%u", (uint32_t)::time(NULL), ::rand());
    intRing.lookupNode(buf, len, uNode);
    mapNode2Count[uNode]++;

    for (typename std::map<T, uint32_t>::iterator it = mapNode2Count.begin();
            it != mapNode2Count.end(); ++it) {
        std::cout << "  node:" << std::hex << it->first << std::dec
            << " count:" << it->second << std::endl;
    }
    std::cout << std::endl;

    mapNode2Count.clear();
    // round robin
    std::cout << "  ===round robin Node" << std::endl;
    for (int i = 0; i < count; ++i) {
        intRing.roundRobinNode(uNode);
        mapNode2Count[uNode]++;
    }
    for (typename std::map<T, uint32_t>::iterator it = mapNode2Count.begin();
            it != mapNode2Count.end(); ++it) {
        std::cout << "  node:" << std::hex << it->first << std::dec
            << " count:" << it->second << std::endl;
    }
    //std::cout << std::endl;
    //for (int i = 0; i < 10; ++i) {
    //    T master = -1, backup = -1;
    //    intRing.roundRobinNode(master, backup);
    //    std::cout << "master:" << std::hex << master << " backup:" << backup << std::endl;
    //}
    std::cout << "=== end ===" << std::endl;
}

struct McNode
{
    char name[64];
    uint32_t weight;
};

int main(int sz, char ** args)
{
    ::srand(::time(NULL));
    int count = 1000000;
    if (sz > 1) {
        count = ::atoi(args[1]);
    }
    std::cout << "total lookup count:" << count << std::endl;


    time_measure tm;

    // trivial type Node
    ConHashRing<uint64_t> intRing;
    uint64_t uTempNode = -1;
    if (!intRing.lookupNode(::rand(), uTempNode)) {
        std::cout << "lookup node faild! hash ring is empty:" << intRing.isEmpty() << std::endl;
    }

    intRing.addHashNode(0x123456789LL, 1);
    intRing.addHashNode(0x224466880LL, 3);
    intRing.addHashNode(0x6789abcdeLL, 2);
    intRing.addHashNode(0x888888888LL, 2);
    intRing.addHashNode(0xfedcba987LL, 2);
    std::cout << "intRing.count:" << intRing.getNodeCount() << std::endl;

    const char* fruits[] = {"apple", "pear", "banana", "orange", "cherry", "apricot"};
    for (uint32_t f = 0; f < sizeof(fruits) / sizeof(fruits[0]); f++) {
        uint64_t master = -1, backup = -1;
        intRing.lookupNode(fruits[f], strlen(fruits[f]), master, backup);
        std::cout << "fruit:" << std::setw(10) << std::left << fruits[f]
            << std::hex << " master:" << master << " backup:" << backup << std::endl;
    }

    lookupNode(intRing, count);

    intRing.removeHashNode(0x888888888LL, 2);
    intRing.removeHashNode(0xfedcba987LL, 2);
    intRing.addHashNode(0xfedcba987LL, 4);
    std::cout << "intRing.count:" << intRing.getNodeCount() << std::endl;

    lookupNode(intRing, count);
    
    // non-trivail type Node
    McNode mcs[3];
    ::strcpy(mcs[0].name, "9.8.7.6:11211");
    ::strcpy(mcs[1].name, "5.6.7.8:11311");
    ::strcpy(mcs[2].name, "6.5.4.3:11511");
    mcs[0].weight = 2;
    mcs[1].weight = 3;
    mcs[2].weight = 5;

    ConHashRing<McNode *> mcRing;
    mcRing.addHashNode(&mcs[0], mcs[0].name, strlen(mcs[0].name), mcs[0].weight);
    mcRing.addHashNode(&mcs[1], mcs[1].name, strlen(mcs[1].name), mcs[1].weight);
    mcRing.addHashNode(&mcs[2], mcs[2].name, strlen(mcs[2].name), mcs[2].weight);
    std::cout << "mcRing.count:" << mcRing.getNodeCount() << std::endl;

    char keybuf[64];
    std::map<McNode *, uint32_t> mapNode2Count;
    for (int i = 0; i < count; ++i) {
        struct timeval stv;
        gettimeofday(&stv, NULL);
        uint64_t usec = (uint64_t)stv.tv_sec * 1000000 + stv.tv_usec + ::rand();

        size_t len = ::snprintf(keybuf, 64, "%u-%ju", i, usec);
        McNode * uNode = NULL;
        mcRing.lookupNode(keybuf, len, uNode);
        mapNode2Count[uNode]++;
    }

    for (std::map<McNode *, uint32_t>::iterator it = mapNode2Count.begin();
            it != mapNode2Count.end(); ++it) {
        std::cout << "node:" << it->first->name
            << " count:" << it->second << std::endl;
    }

    mcRing.removeHashNode(mcs[1].name, strlen(mcs[1].name), mcs[1].weight);
    std::cout << "mcRing.count:" << mcRing.getNodeCount() << std::endl;

    return 0;
}

