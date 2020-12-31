#ifndef ACTOR_HPP
#define ACTOR_HPP

#pragma once

#include <stdlib.h>
#include <utility>
#include <unordered_map>
#include <string>
#include <cstdint>
#include <set>

#include "Port.hpp"
#include "InPort.hpp"
#include "OutPort.hpp"
#include "utils/gpi_utils.hpp"


class Actor
{

	friend std::ostream &operator<<(std::ostream &, Actor const &);

public:
	std::string name;
	uint64_t threadRank;
	uint64_t actorSrNo;
	uint64_t actorGlobID;
	uint64_t triggers;

	std::unordered_map<std::string, Port* > inPortList;
	std::unordered_map<std::string, Port* > outPortList;

	void addInPort(Port* inPort);
	void addOutPort(Port* outPort);
	uint64_t trigger();
	uint64_t detrigger();
	bool isTriggered();

    template<typename T, int capacity> InPort<T, capacity> * getInPort(const std::string &portName);
    template<typename T, int capacity> OutPort<T, capacity> * getOutPort(const std::string &portName);

	Actor(std::string name, uint64_t threadRank, uint64_t actorSrNo);
	Actor(uint64_t threadRank, uint64_t actorSrNo);
	Actor(uint64_t actorSrNo);
	Actor(std::string name);
	Actor();

	virtual ~Actor();
	Actor(Actor &other) = delete;
	Actor & operator=(Actor &other) = delete;
	
	virtual void act() = 0;
	//void act();
	bool finished;
	void finish() { finished = true; }
  	void stop() { finish(); }

	inline const auto &getName() const { return name; }
	inline auto getRank() const { return threadRank; }
	inline auto isLocal() const { return threadRank == gpi_util::get_local_rank(); }
	inline auto isRemote() const { return threadRank != gpi_util::get_local_rank(); }

	//statics
	static uint64_t encodeGlobID(uint64_t procNo, uint64_t actNo);
	static std::pair<uint64_t,uint64_t> decodeGlobID(uint64_t inpGlobId);

private:
	void objectInit(std::string name, uint64_t threadRank, uint64_t actorSrNo);
	static uint64_t localRank;
	static uint64_t localSrNo;
	static std::set<uint64_t> takenSrNos;
	static void setNextSrNo();
};

template <typename T, int capacity> InPort <T, capacity> * Actor :: getInPort(const std::string &portName)
{
    auto it = inPortList.find(portName);
    if (it == inPortList.end())
        throw std::runtime_error("Could not find InPort " + portName + " at actor ID "+std::to_string(this->actorGlobID));
    return static_cast <InPort <T, capacity> *> (it->second);
}
template <typename T, int capacity> OutPort <T, capacity> * Actor :: getOutPort(const std::string &portName)
{
    auto it = outPortList.find(portName);
    if (it == outPortList.end())
        throw std::runtime_error("Could not find OutPort " + portName);
    return static_cast <OutPort <T, capacity> *> (it->second);
}

#endif