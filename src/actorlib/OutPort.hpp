#ifndef OUTPORT_HPP
#define OUTPORT_HPP

#include "Channel.hpp"
#include "Port.hpp"

#include <string>
#include <vector>
#include <iostream>

#pragma once
template <typename T, int capacity> class OutPort : public Port
{
public:
    Channel<T, capacity> *connChannel;
    //T data;

    OutPort(Channel<T, capacity>* channel);
    OutPort(Channel<T, capacity>* channel, std::string name);

    uint64_t freeCapacity();         //poll channel to see if space available
    void write(T data);  //write to channel

    ~OutPort();
	OutPort(OutPort &other) = delete;
	OutPort & operator=(OutPort &other) = delete;
};

template <typename T, int capacity> OutPort <T, capacity> :: OutPort(Channel<T, capacity>* channel, std::string name)
{
    connChannel = channel;
    this->name = name;
}

template <typename T, int capacity> OutPort <T, capacity> :: OutPort(Channel<T, capacity>* channel) : OutPort(channel, "temp")  { }

template <typename T, int capacity> OutPort <T, capacity> :: ~OutPort()
{
    delete this->connChannel;
}

template <typename T, int capacity> void OutPort <T, capacity> :: write(T data)
{
    connChannel->pushData(data);
}

template <typename T, int capacity> uint64_t OutPort <T, capacity> :: freeCapacity()
{
    return (connChannel->isAvailableToPush());
}

#endif