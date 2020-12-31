#ifndef LOCAL_CHANNEL_HPP
#define LOCAL_CHANNEL_HPP

#pragma once

#include "utils/connection_type_util.hpp"
#include "Channel.hpp"

#include <vector>
#include <queue>
#include <cstdint>
#include <iostream>

template <typename T, int capacity> class LocalChannel: public Channel<T, capacity>
{
public:
    std::queue <T> data;
    std::queue <uint64_t> *triggerQueue;

    void pushData(T ndata);
    T pullData();

    uint64_t isAvailableToPush();
    uint64_t isAvailableToPull();

    LocalChannel(uint64_t srcID, uint64_t dstID, uint64_t fixedOffset);
};

template <typename T, int capacity> class LocalChannel<std::vector <T> ,capacity>: public Channel<std::vector<T>, capacity>
{
public:
    std::queue <std::vector<T>> data;
    std::queue <uint64_t> *triggerQueue;

    void pushData(std::vector<T> &ndata);
    std::vector<T> pullData();

    uint64_t isAvailableToPush();
    uint64_t isAvailableToPull();

    LocalChannel(uint64_t srcID, uint64_t dstID, uint64_t fixedOffset);
};

template <typename T, int capacity> LocalChannel<T, capacity>::LocalChannel(uint64_t srcID, uint64_t dstID, uint64_t fixedOffset)
{
    this->currConnectionType = ActorConnectionType::LOCAL_LOCAL;
    this->maxQueueSize = capacity;
    this->curQueueSize = capacity;
    this->dstID = dstID;
    this->srcID = srcID;
    this->fixedDataOffset = fixedOffset;
    this->initialized = false;
}

template <typename T, int capacity> LocalChannel<std::vector<T>, capacity>::LocalChannel(uint64_t srcID, uint64_t dstID, uint64_t fixedOffset)
{
    this->currConnectionType = ActorConnectionType::LOCAL_LOCAL;
    this->maxQueueSize = capacity;
    this->curQueueSize = capacity;
    this->dstID = dstID;
    this->srcID = srcID;
    this->fixedDataOffset = fixedOffset;
    this->initialized = false;
}

template <typename T, int capacity> T LocalChannel <T, capacity> :: pullData()
{
    if(this->isAvailableToPull())
    {
        T toRet = data.front();
        data.pop();
        this->curQueueSize++;
        return toRet;
    }
    else
    {
        throw std::runtime_error("No data in channel");
    }
}
template <typename T, int capacity> void LocalChannel <T, capacity> :: pushData(T ndata)
{
    //std::cout << "in channel write" <<std::endl;
    if(this->isAvailableToPush())
    {
        T localCpy = ndata;
        data.push(localCpy);
        //std::cout <<"Pushed" <<std::endl;
        this->curQueueSize--;
        triggerQueue->push(this->dstID);
    }
    else
    {
        throw std::runtime_error("Too much data in channel");
    }
}

template <typename T, int capacity> std::vector<T> LocalChannel <std::vector<T>, capacity> :: pullData()
{
    if(this->isAvailableToPull())
    {
        std::vector<T> toRet = data.front();
        data.pop();
        this->curQueueSize++;
        return toRet;
    }
    else
    {
        throw std::runtime_error("No data in channel");
    }
}
template <typename T, int capacity> void LocalChannel <std::vector<T>, capacity> :: pushData(std::vector<T> &ndata)
{
    if(this->isAvailableToPush())
    {
        std::vector<T> localCpy(ndata);
        data.push(localCpy);

        this->curQueueSize--;
        triggerQueue->push(this->dstID);
    }
    else
    {
        throw std::runtime_error("Too much data in channel");
    }
}

template <typename T, int capacity> uint64_t LocalChannel <T, capacity> :: isAvailableToPull()
{
    return (this->maxQueueSize - this->curQueueSize);
}
template <typename T, int capacity> uint64_t LocalChannel <T, capacity> ::isAvailableToPush()
{
    return (this->curQueueSize * this->minBlockSize);
}

template <typename T, int capacity> uint64_t LocalChannel <std::vector<T>, capacity> :: isAvailableToPull()
{
    return (this->maxQueueSize - this->curQueueSize);
}
template <typename T, int capacity> uint64_t LocalChannel <std::vector<T>, capacity> ::isAvailableToPush()
{
    return (this->curQueueSize * this->minBlockSize);
}
#endif