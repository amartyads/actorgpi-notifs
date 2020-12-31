#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#pragma once

#include "utils/connection_type_util.hpp"
#include "AbstractChannel.hpp"
#include <queue>
#include <vector>
#include <string>
template <typename T, int capacity> class Channel: public AbstractChannel
{
public:
    virtual void pushData(T data) = 0;
    virtual T pullData() = 0;
    virtual uint64_t isAvailableToPush() = 0;
    virtual uint64_t isAvailableToPull() = 0;
};

template <typename T, int capacity> class Channel <std::vector<T>, capacity>: public AbstractChannel
{
public:
    virtual void pushData(std::vector<T> &ndata) = 0;
    virtual std::vector<T> pullData() = 0;
    virtual uint64_t isAvailableToPush() = 0;
    virtual uint64_t isAvailableToPull() = 0;
};

#endif