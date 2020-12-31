#ifndef INPORT_HPP
#define INPORT_HPP

#include "Channel.hpp"
#include "Port.hpp"

#include <string>
#include <vector>

#pragma once
template <typename T, int capacity> class InPort : public Port
{
public:
    Channel<T, capacity> *connChannel;
    //T data;

    InPort(Channel<T, capacity>* channel);
    InPort(Channel<T, capacity>* channel, std::string name);

    T read();     // read from channel, return
    uint64_t available(); // poll channel to see if data available
    T peek();     // get element without popping

    std::queue<T> dataStash;

    ~InPort();
	InPort(InPort &other) = delete;
	InPort & operator=(InPort &other) = delete;
};

template <typename T, int capacity> InPort <T, capacity> ::InPort(Channel<T, capacity>* channel, std::string name)
{
    connChannel = channel;
    this->name = name;
}

template <typename T, int capacity> InPort <T, capacity> ::InPort(Channel<T, capacity>* channel) : InPort(channel, "temp")  { }

template <typename T, int capacity> InPort <T, capacity> :: ~InPort()
{
    delete this->connChannel;
}

template <typename T, int capacity> T InPort <T, capacity> :: read()
{
    if(dataStash.empty())
        dataStash.push(connChannel->pullData());
    T temp = dataStash.front();
    dataStash.pop();
    return temp;
}

template <typename T, int capacity> uint64_t InPort <T, capacity> :: available()
{
    if(dataStash.empty())
        return (connChannel->isAvailableToPull());
    return (connChannel->isAvailableToPull() + 1);
}

template <typename T, int capacity> T InPort <T, capacity> :: peek()
{
    if(dataStash.empty())
        dataStash.push(connChannel->pullData());
    return dataStash.front();
}
#endif