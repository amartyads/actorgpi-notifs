#ifndef REMOTE_CHANNEL_HPP
#define REMOTE_CHANNEL_HPP

#pragma once

#include "utils/connection_type_util.hpp"
#include "utils/gpi_utils.hpp"
#include "AbstractChannel.hpp"
#include "Channel.hpp"

#include <GASPI.h>
#include <GASPI_Ext.h>
#include <vector>
#include <iostream>

#include <stdexcept>
#include <cstdint>
#include <sstream>

template <typename T, int capacity> class RemoteChannel: public Channel<T, capacity>
{
public:

    void pushData(T ndata);
    T pullData();

    uint64_t isAvailableToPush();
    uint64_t isAvailableToPull();

    int blockSize;
    int64_t pulledDataoffset;
    
    int queueLocation;
    gaspi_queue_id_t queue_id;
    RemoteChannel(ActorConnectionType currConnectionType, uint64_t srcID, uint64_t dstID);
};

template <typename T, int capacity> class RemoteChannel<std::vector <T> ,capacity>: public Channel<std::vector<T>, capacity>
{
public:

    void pushData(std::vector<T> &ndata);
    std::vector<T> pullData();

    uint64_t isAvailableToPush();
    uint64_t isAvailableToPull();

    int blockSize;
    int64_t pulledDataoffset;
    
    int queueLocation;
    gaspi_queue_id_t queue_id;
    RemoteChannel(ActorConnectionType currConnectionType, uint64_t srcID, uint64_t dstID);
};


#ifndef ASSERT
#define ASSERT(ec) gpi_util::success_or_exit(__FILE__,__LINE__,ec)
#endif


template <typename T, int capacity> RemoteChannel<T, capacity>::RemoteChannel(ActorConnectionType currConnectionType, uint64_t srcID, uint64_t dstID)
{
    this->currConnectionType = currConnectionType;
    this->maxQueueSize = capacity;
    this->curQueueSize = capacity;
    this->dstID = dstID;
    this->srcID = srcID;
    queueLocation = 0;
    queue_id = 0;
    pulledDataoffset = -1;
    this->initialized = false;
}

template <typename T, int capacity> RemoteChannel<std::vector<T>, capacity>::RemoteChannel(ActorConnectionType currConnectionType, uint64_t srcID, uint64_t dstID)
{
    this->currConnectionType = currConnectionType;
    this->maxQueueSize = capacity;
    this->curQueueSize = capacity;
    this->dstID = dstID;
    this->srcID = srcID;
    queueLocation = 0;
    queue_id = 0;
    pulledDataoffset = -1;
    this->initialized = false;
    //std::cout << "Max queue: " << this->maxQueueSize << " cur queue: " << this->curQueueSize << std::endl;
}

template<typename T, int capacity> std::vector<T> RemoteChannel <std::vector<T>, capacity> :: pullData()
{
    //data availability already checked
    //std::cout << "in pull Max queue: " << this->maxQueueSize << " cur queue: " << this->curQueueSize << std::endl;
    gpi_util::wait_for_queue_entries(&queue_id, 1);
    ASSERT (gaspi_read ( 4, 0
                        , this->remoteRank, 0, ((this->fixedDataOffset * this->maxQueueSize) + queueLocation) *sizeof(uint64_t)
                        , sizeof(uint64_t), queue_id, GASPI_BLOCK
                        )
            );
    ASSERT (gaspi_wait (queue_id, GASPI_BLOCK));
    pulledDataoffset = ((int64_t *)(this->cachePtr))[0];
    //std::cout << pulledDataoffset << std::endl;

    if(pulledDataoffset == -1)
        throw std::runtime_error("No data in channel");
    //gaspi read from fixedoffset * sizeof uint64_t for databank ofset (was in isavailabletopull)

    //read from no of datablock offset
    uint64_t bytesUsed;
    gpi_util::wait_for_queue_entries(&queue_id, 1);
    ASSERT (gaspi_read ( 4, 0
                        , this->remoteRank, 5, ((this->fixedDataOffset * this->maxQueueSize) + queueLocation) *sizeof(uint64_t)
                        , sizeof(uint64_t), queue_id, GASPI_BLOCK
                        )
            );
    ASSERT (gaspi_wait (queue_id, GASPI_BLOCK));
    bytesUsed = ((uint64_t *)(this->cachePtr))[0];

    //calc no of datablocks
    this->noOfDatablocksUsed = ((bytesUsed - 1) / this->minBlockSize) + 1;
    //loop thru consecutive datablocks (minblocksize is the same for all ranks)
    uint64_t bytesLeft = bytesUsed;
    uint64_t vecSize, elemsPerFullCache;
    std::vector<T> readData;
    vecSize = bytesUsed / sizeof(T);
    elemsPerFullCache = this->minBlockSize / sizeof(T);
    int64_t copyOfPulledDataOffset = this->pulledDataoffset;
    
    //gaspi read from databank offset, of amount bytesUsed, minBlocksize at a time
    for(uint64_t i = 0; i < this->noOfDatablocksUsed; i++)
    {
        gpi_util::wait_for_queue_entries(&queue_id, 1);
        ASSERT (gaspi_read ( 4, 0
                            , this->remoteRank, 3, copyOfPulledDataOffset
                            , this->minBlockSize, queue_id, GASPI_BLOCK
                            )
                );
        ASSERT (gaspi_wait (queue_id, GASPI_BLOCK));
        for(uint64_t j = 0; j < (vecSize < elemsPerFullCache ? vecSize : elemsPerFullCache); j++)
        {
            readData.push_back(((T *)(this->cachePtr))[j]);
        }
        
        vecSize -= elemsPerFullCache;
        copyOfPulledDataOffset += this->minBlockSize;
    }
    
    //gaspi write to localClear the databank offset, local actorgraph will divide by blocksize
    ((int64_t *)(this->cachePtr))[0] = pulledDataoffset;
    ((int64_t *)(this->cachePtr))[1] = -1;
    //gaspi write to localClear the databank offset, local actorgraph will divide by blocksize
    gpi_util::wait_for_queue_entries(&queue_id, 2);
    ASSERT (gaspi_write ( 4, 0
                        , this->remoteRank, 1, ((this->fixedDataOffset * this->maxQueueSize) + queueLocation) *sizeof(uint64_t)
                        , sizeof(uint64_t), queue_id, GASPI_BLOCK
                        )
            );
    //gaspi write to comm segment, free up the area
    ASSERT (gaspi_write ( 4, sizeof(int64_t)
                        , this->remoteRank, 0, ((this->fixedDataOffset * this->maxQueueSize) + queueLocation) *sizeof(int64_t)
                        , sizeof(int64_t), queue_id, GASPI_BLOCK
                        )
            );
    ASSERT (gaspi_wait (queue_id, GASPI_BLOCK));
    pulledDataoffset = -1;
    //else
    //{
    //}
    queueLocation++;
    queueLocation %= this->maxQueueSize;
    return readData;
}

template <typename T, int capacity> T RemoteChannel <T, capacity> :: pullData()
{
    //data availability already checked
    gpi_util::wait_for_queue_entries(&queue_id, 1);
    ASSERT (gaspi_read ( 4, 0
                        , this->remoteRank, 0, ((this->fixedDataOffset * this->maxQueueSize) + queueLocation) *sizeof(uint64_t)
                        , sizeof(uint64_t), queue_id, GASPI_BLOCK
                        )
            );
    ASSERT (gaspi_wait (queue_id, GASPI_BLOCK));
    pulledDataoffset = ((int64_t *)(this->cachePtr))[0];
    //std::cout << pulledDataoffset << std::endl;
    
    if(pulledDataoffset == -1)
        throw std::runtime_error("No data in channel");
    //gaspi read from fixedoffset * sizeof uint64_t for databank ofset (was in isavailabletopull)

    //read from no of datablock offset
    uint64_t bytesUsed;
    gpi_util::wait_for_queue_entries(&queue_id, 1);
    ASSERT (gaspi_read ( 4, 0
                        , this->remoteRank, 5, ((this->fixedDataOffset * this->maxQueueSize) + queueLocation) *sizeof(uint64_t)
                        , sizeof(uint64_t), queue_id, GASPI_BLOCK
                        )
            );
    ASSERT (gaspi_wait (queue_id, GASPI_BLOCK));
    bytesUsed = ((uint64_t *)(this->cachePtr))[0];

    //calc no of datablocks
    this->noOfDatablocksUsed = ((bytesUsed - 1) / this->minBlockSize) + 1;
    //loop thru consecutive datablocks (minblocksize is the same for all ranks)
    uint64_t bytesLeft = bytesUsed;
    uint64_t vecSize, elemsPerFullCache;
    //prepare reading vector
    std::vector<T> readData;
    vecSize = 1;
    elemsPerFullCache = this->minBlockSize / sizeof(T);
    
    //gaspi read from databank offset, of amount bytesUsed, minBlocksize at a time
    for(uint64_t i = 0; i < this->noOfDatablocksUsed; i++)
    {
        gpi_util::wait_for_queue_entries(&queue_id, 1);
        ASSERT (gaspi_read ( 4, 0
                            , this->remoteRank, 3, ((this->pulledDataoffset))
                            , this->minBlockSize, queue_id, GASPI_BLOCK
                            )
                );
        ASSERT (gaspi_wait (queue_id, GASPI_BLOCK));
        readData.push_back(((T *)(this->cachePtr))[0]);
    }
    ((int64_t *)(this->cachePtr))[0] = pulledDataoffset;
    ((int64_t *)(this->cachePtr))[1] = -1;
    //gaspi write to localClear the databank offset, local actorgraph will divide by blocksize
    gpi_util::wait_for_queue_entries(&queue_id, 2);
    ASSERT (gaspi_write ( 4, 0
                        , this->remoteRank, 1, ((this->fixedDataOffset * this->maxQueueSize) + queueLocation) *sizeof(uint64_t)
                        , sizeof(uint64_t), queue_id, GASPI_BLOCK
                        )
            );
    //gaspi write to comm segment, free up the area
    ASSERT (gaspi_write ( 4, sizeof(int64_t)
                        , this->remoteRank, 0, ((this->fixedDataOffset * this->maxQueueSize) + queueLocation) *sizeof(int64_t)
                        , sizeof(int64_t), queue_id, GASPI_BLOCK
                        )
            );
    ASSERT (gaspi_wait (queue_id, GASPI_BLOCK));
    
    pulledDataoffset = -1;
    //else
    //{
    //}
    queueLocation++;
    queueLocation %= this->maxQueueSize;
    return readData[0];
    
}

template <typename T, int capacity> void RemoteChannel <std::vector<T>, capacity> :: pushData (std::vector<T> &ndata)
{
    //std::cout << "in push Max queue: " << this->maxQueueSize << " cur queue: " << this->curQueueSize << std::endl;
    this->noOfDatablocksUsed = (((ndata.size() * sizeof(T) )- 1) / this->minBlockSize) + 1;
    uint64_t slot;
    bool notFound = true;
    for(slot = 0; slot < AbstractChannel::lookupTable.size(); slot++)
    {
        if(AbstractChannel::lookupTable[slot] == -1)
        {
            notFound = false;
            for(uint64_t i = 1; i < this->noOfDatablocksUsed; i++)
            {
                if(AbstractChannel::lookupTable[slot + i] != -1)
                {
                    slot += i-1;
                    notFound = true;
                    break;
                }
            }
            if(!notFound)
            {
                AbstractChannel::lookupTable[slot] = this->fixedDataOffset;
                for(uint64_t i = 1; i < this->noOfDatablocksUsed; i++)
                {
                    AbstractChannel::lookupTable[slot + i] = -2;
                }
                
                break;
            }
        }
    }
    if(notFound)
    {
        throw std::runtime_error("Too much data in channel");
    }
    //multiply index with minblocksize
    uint64_t localDatabankOffset = slot * this->minBlockSize; //blocksize includes sizeof
    //write to (localCommPtr) + fixedoffset * uint64 the databank offset (index*minblocksize)
    auto commPtr = (uint64_t *)(this->fixedCommInitPtr);
    commPtr[(this->fixedDataOffset * this->maxQueueSize) + queueLocation] = localDatabankOffset;
    //write to databank (localdataptr + index*minblock)

    //must split ndata into mindatablock sizes so as to not split data across edge
    if((this->minBlockSize % sizeof(T)) == 0 || this->noOfDatablocksUsed <= 1)
    {
        gaspi_pointer_t tempData = ((char*)(this->fixedDatabankInitPtr))  + localDatabankOffset;
        for(int64_t i = 0; i < ndata.size(); i++)
        {
            ((T *)(tempData))[i] = ndata[i];
        }
    }
    else
    {
        gaspi_pointer_t tempData[this->noOfDatablocksUsed];
        for(int i = 0; i < this->noOfDatablocksUsed; i++)
        {
            tempData[i] = ((char*)(this->fixedDatabankInitPtr)) + ((slot + i) * this->minBlockSize);
        }
        int64_t ndataIdx = 0, databankIdx = 0, databankDataIdx = 0, curDataCount = 0;
        while(ndataIdx < ndata.size() && databankIdx < this->noOfDatablocksUsed) //second condition should never be triggered, kept for sanity
        {
            ((T *)(tempData[databankIdx]))[databankDataIdx] = ndata[ndataIdx];
            curDataCount += sizeof(T);
            databankDataIdx++;
            ndataIdx++;
            if(this->minBlockSize - curDataCount < sizeof(T)) //remaining space in block not enough, move to next block
            {
                curDataCount = 0;
                databankIdx++;
                databankDataIdx = 0;
            }
        }
    }
    
    //write to vec size segment
    uint64_t bytesUsed;
    bytesUsed = ndata.size() * sizeof(T);
    auto sizePtr = (uint64_t *)(this->fixedVecSizeInitPtr);
    sizePtr[(this->fixedDataOffset * this->maxQueueSize) + queueLocation] = bytesUsed;

    //gaspi write trigger to remote actorgraph

   
    gpi_util::wait_for_queue_entries(&queue_id, 1);
    ((uint64_t *)(this->cachePtr))[0] = this->dstID;
    ASSERT (gaspi_write ( 4, 0
                        , this->remoteRank, 2, ((this->fixedTriggerOffset * this->maxQueueSize) + queueLocation) *sizeof(uint64_t)
                        , sizeof(uint64_t), queue_id, GASPI_BLOCK
                        )
            );

    ASSERT (gaspi_wait (queue_id, GASPI_BLOCK));
    queueLocation++;
    queueLocation %= this->maxQueueSize;
    this->curQueueSize--;
}

template <typename T, int capacity> void RemoteChannel <T, capacity> :: pushData(T ndata)
{
    //space already checked
    //calculate no of datablocks needed
    this->noOfDatablocksUsed = 1;
    
    //std::cout << this->fixedDataOffset << " " << queueLocation << std::endl;
    //reserve slots from static array
    uint64_t slot;
    bool notFound = true;
    for(slot = 0; slot < AbstractChannel::lookupTable.size(); slot++)
    {
        if(AbstractChannel::lookupTable[slot] == -1)
        {
            notFound = false;
            AbstractChannel::lookupTable[slot] = this->fixedDataOffset;
            break;
        }
    }
    if(notFound)
    {
        throw std::runtime_error("Too much data in channel");
    }
    //multiply index with minblocksize
    uint64_t localDatabankOffset = slot * this->minBlockSize; //blocksize includes sizeof
    //write to (localCommPtr) + fixedoffset * uint64 the databank offset (index*minblocksize)
    auto commPtr = (uint64_t *)(this->fixedCommInitPtr);
    commPtr[(this->fixedDataOffset * this->maxQueueSize) + queueLocation] = localDatabankOffset;
    //write to databank (localdataptr + index*minblock)
    gaspi_pointer_t tempData = ((char*)(this->fixedDatabankInitPtr))  + localDatabankOffset;

    ((T *)(tempData))[0] = ndata;

    //write to vec size segment
    gpi_util::wait_for_queue_entries(&queue_id, 2);
    uint64_t bytesUsed;

    bytesUsed = sizeof(T);

    auto sizePtr = (uint64_t *)(this->fixedVecSizeInitPtr);
    sizePtr[(this->fixedDataOffset * this->maxQueueSize) + queueLocation] = bytesUsed;

    //gaspi write trigger to remote actorgraph

   
    ((uint64_t *)(this->cachePtr))[0] = this->dstID;
    ASSERT (gaspi_write ( 4, 0
                        , this->remoteRank, 2, ((this->fixedTriggerOffset * this->maxQueueSize) + queueLocation) *sizeof(uint64_t)
                        , sizeof(uint64_t), queue_id, GASPI_BLOCK
                        )
            );

    ASSERT (gaspi_wait (queue_id, GASPI_BLOCK));
    queueLocation++;
    queueLocation %= this->maxQueueSize;
    this->curQueueSize--;
}

template <typename T, int capacity> uint64_t RemoteChannel <T, capacity> :: isAvailableToPull()
{
    //std::cout << this->fixedDataOffset << " " << queueLocation << " rem " << this->remoteRank << std::endl;
    //gaspi read databank offsets
    gpi_util::wait_for_queue_entries(&queue_id, 1);
    ASSERT (gaspi_read ( 4, 0
                        , this->remoteRank, 0, (this->fixedDataOffset * this->maxQueueSize) *sizeof(int64_t)
                        , this->maxQueueSize * sizeof(int64_t), queue_id, GASPI_BLOCK
                        )
            );
    ASSERT (gaspi_wait (queue_id, GASPI_BLOCK));
    
    //std::stringstream ss;
    int totElements = 0;
    //ss << "At dest " << this->dstID << ": ";
    int64_t* scan = (int64_t *)(this->cachePtr);
    for(int i = 0; i < this->maxQueueSize; i++)
    {
        if(scan[i] != -1)
            totElements++;
      //  ss << scan[i] << " ";
    }
    //std::cout << "At dest " << this->dstID << " readables: " << totElements << std::endl;
    //ss << std::endl;
    //std::cout << ss.str();
    return totElements;
    //return (this->curQueueSize < this->maxQueueSize);
}
template <typename T, int capacity> uint64_t RemoteChannel <T, capacity> :: isAvailableToPush()
{
    if (this->curQueueSize <= 0)
        return 0;
    
    //find longest datablock len in bytes
    uint64_t foundSlot = 0, temp = 0;
    for(uint64_t i = 0; i < AbstractChannel::lookupTable.size(); i++)
    {
        if(AbstractChannel::lookupTable[i] == -1)
        {
            temp++;
        }
        else
        {
            foundSlot = foundSlot > temp ? foundSlot:temp;
            temp = 0;
        }
    }
    foundSlot = foundSlot > temp ? foundSlot:temp;
    foundSlot *= this->minBlockSize;
    return foundSlot;
}

template <typename T, int capacity> uint64_t RemoteChannel <std::vector<T>, capacity> :: isAvailableToPull()
{
    //std::cout << this->fixedDataOffset << " " << queueLocation << " rem " << this->remoteRank << std::endl;
    //gaspi read databank offset
    gpi_util::wait_for_queue_entries(&queue_id, 1);
    ASSERT (gaspi_read ( 4, 0
                        , this->remoteRank, 0, (this->fixedDataOffset * this->maxQueueSize) *sizeof(int64_t)
                        , this->maxQueueSize * sizeof(int64_t), queue_id, GASPI_BLOCK
                        )
            );
    ASSERT (gaspi_wait (queue_id, GASPI_BLOCK));
    int totElements = 0;
    //std::stringstream ss;
    //ss << "At dest " << this->dstID << ":";
    int64_t* scan = (int64_t *)(this->cachePtr);
    for(int i = 0; i < this->maxQueueSize; i++)
    {
        if(scan[i] != -1)
            totElements++;
    //    ss << scan[i] << " ";
    }
    //std::cout << "At dest " << this->dstID << " readables: " << totElements << std::endl;
    //ss << std::endl;
    //std::cout << ss.str();
    return totElements;
    //return (this->curQueueSize < this->maxQueueSize);
}
template <typename T, int capacity> uint64_t RemoteChannel <std::vector<T>, capacity> :: isAvailableToPush()
{
    if (this->curQueueSize <= 0)
        return 0;
    
    //find longest datablock len in bytes
    uint64_t foundSlot = 0, temp = 0;
    for(uint64_t i = 0; i < AbstractChannel::lookupTable.size(); i++)
    {
        if(AbstractChannel::lookupTable[i] == -1)
        {
            temp++;
        }
        else
        {
            foundSlot = foundSlot > temp ? foundSlot:temp;
            temp = 0;
        }
    }
    foundSlot = foundSlot > temp ? foundSlot:temp;
    foundSlot *= this->minBlockSize;
    return foundSlot;
}

#endif