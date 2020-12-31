#include "Actor.hpp"
#include "InPort.hpp"
#include "OutPort.hpp"
#include "utils/gpi_utils.hpp"

#include <GASPI.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <utility>
#include <cstdint>
#include <stdexcept>
#include <sstream>

#include <iostream>

#ifndef ASSERT
#define ASSERT(ec) gpi_util::success_or_exit(__FILE__,__LINE__,ec)
#endif

uint64_t Actor::localRank = -10;
uint64_t Actor::localSrNo = 0;
std::set<uint64_t> Actor::takenSrNos;

std::ostream &operator<<(std::ostream &os, Actor const &actor) {
  return os << "[ Rank: " << actor.getRank() << ""
            << ", Name: " << actor.getName() << " ]";
}

Actor::Actor(std::string name, uint64_t threadRank, uint64_t actorSrNo)
{
	objectInit(name, threadRank, actorSrNo);
}
Actor::Actor(uint64_t threadRank, uint64_t actorSrNo) 
{
	objectInit("A-"+std::to_string(threadRank)+"-"+std::to_string(actorSrNo), threadRank, actorSrNo);
}
Actor::Actor(uint64_t actorSrNo)
{
	if(Actor::localRank == -10)
		Actor::localRank = gpi_util::get_local_rank();
	objectInit("A-"+std::to_string(Actor::localRank)+"-"+std::to_string(actorSrNo), Actor::localRank, actorSrNo);
}
Actor::Actor(std::string name)
{
	if(Actor::localRank == -10)
		Actor::localRank = gpi_util::get_local_rank();
	objectInit(name, Actor::localRank, Actor::localSrNo);
}
Actor::Actor()
{
	if(Actor::localRank == -10)
		Actor::localRank = gpi_util::get_local_rank();
	objectInit("A-"+std::to_string(Actor::localRank)+"-"+std::to_string(Actor::localSrNo), Actor::localRank, Actor::localSrNo);
}

void Actor::objectInit(std::string name, uint64_t threadRank, uint64_t actorSrNo)
{
	Actor::takenSrNos.insert(actorSrNo);
	this->name = name;
	this->threadRank = threadRank;
	this->actorSrNo = actorSrNo;
	this->actorGlobID = Actor::encodeGlobID(threadRank, actorSrNo);
	this->triggers = 0;
	finished = false;
	Actor::setNextSrNo();
}

void Actor::setNextSrNo() //static
{
	while(Actor::takenSrNos.count(Actor::localSrNo) != 0) //while number exists in set
		Actor::localSrNo++;
}

Actor::~Actor()
{
	for(auto &ip : inPortList)
		delete ip.second;
	for(auto &op : outPortList)
		delete op.second;
}

void Actor::addInPort(Port* inPort)
{
	inPortList[inPort->name] = inPort;
}
void Actor::addOutPort(Port* outPort)
{
	outPortList[outPort->name] = outPort;
}

uint64_t Actor::trigger()
{
	triggers++;
	return triggers;
}
uint64_t Actor::detrigger()
{
	triggers--;
	return triggers;
}
bool Actor::isTriggered()
{
	return (triggers > 0);
}

uint64_t Actor::encodeGlobID(uint64_t procNo, uint64_t actNo) //static
{
	return (procNo << 20) | actNo;
}

std::pair<uint64_t,uint64_t> Actor::decodeGlobID(uint64_t inpGlobId) //static
{
	uint64_t procNo = inpGlobId >> 20;
	uint64_t actNo = inpGlobId & ((1 << 20) - 1);
	return (std::make_pair(procNo, actNo));
}