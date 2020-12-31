#ifndef ABSTRACTCHANNEL_HPP
#define ABSTRACTCHANNEL_HPP

#pragma once

#include "utils/connection_type_util.hpp"
#include <GASPI.h>
#include <GASPI_Ext.h>
#include <string>
#include <cstdint>
#include <vector>

class AbstractChannel
{
public:
    ActorConnectionType currConnectionType;
    int maxQueueSize, curQueueSize;
    bool initialized;
    uint64_t localDatablockSize;
    uint64_t srcID;
    uint64_t dstID;
    uint64_t fixedDataOffset, fixedTriggerOffset;
    uint64_t minBlockSize;
    uint64_t channelID;
    uint64_t remoteRank;
    uint64_t noOfDatablocksUsed;

    std::string name;

    gaspi_pointer_t fixedCommInitPtr;
    gaspi_pointer_t fixedDatabankInitPtr;
    gaspi_pointer_t cachePtr;
    gaspi_pointer_t fixedVecSizeInitPtr;

    //statics
    static std::vector<int64_t> lookupTable;
    static bool compareChannelNames(AbstractChannel* a, AbstractChannel* b);
    static uint64_t encodeGlobID(uint64_t procNo, uint64_t actNo);
	static std::pair<uint64_t,uint64_t> decodeGlobID(uint64_t inpGlobId);

    bool operator < (const AbstractChannel& other) const
    {
        return (name.compare(other.name) < 0);
    }
};


#endif