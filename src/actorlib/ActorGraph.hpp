#ifndef ACTORGRAPH_HPP
#define ACTORGRAPH_HPP

#pragma once

#include <GASPI.h>
#include <GASPI_Ext.h>

#include <stdlib.h>
#include <utility>
#include <string>
#include <vector>
#include <queue>
#include <cstdint>
#include <stdexcept>

#include "LocalChannel.hpp"
#include "RemoteChannel.hpp"
#include "Channel.hpp"
#include "InPort.hpp"
#include "OutPort.hpp"
#include "Actor.hpp"
#include "DummyRemoteActor.hpp"
#include "utils/connection_type_util.hpp"

class ActorGraph
{
public:

	gaspi_rank_t threadRank, totNoThreads;
	std::vector<Actor* > localActorRefList;
	std::vector<Actor* > remoteActorRefList;
	std::vector<uint64_t> localActorIDList;
	std::vector<uint64_t> remoteActorIDList;
	std::vector<std::string> remoteActorNameList;
	std::unordered_map<uint64_t, Actor* > localActorRefMap;
	std::queue<uint64_t> localChannelTriggers;

	std::vector<uint64_t> noOfOutgoingChannels, noOfIncomingChannels;
	std::vector<uint64_t> offsetVals;
	std::vector<uint64_t> dataBlockSize;
	int dataQueueLen;

	std::vector<AbstractChannel *> remoteChannelList;
	std::vector<AbstractChannel *> localChannelList;
	std::unordered_map<uint64_t, AbstractChannel *> remoteChannelMap;
	uint64_t localChannelCt;
	
	uint64_t noLocalRemoteChannels, noRemoteLocalChannels;

	gaspi_segment_id_t segmentIDRemoteLookup;
	gaspi_segment_id_t segmentIDLocalClear;
	gaspi_segment_id_t segmentIDLocalTrigger;
	gaspi_segment_id_t segmentIDDatabank;
	gaspi_segment_id_t segmentIDLocalCache;
	gaspi_segment_id_t segmentIDRemoteVecSize;

	gaspi_pointer_t gasptrRemoteLookup;
	gaspi_pointer_t gasptrLocalClear;
	gaspi_pointer_t gasptrLocalTrigger;
	gaspi_pointer_t gasptrDatabank;
	gaspi_pointer_t gasptrLocalCache;
	gaspi_pointer_t gasptrRemoteVecSize;

	uint64_t minBlockSize;
	uint64_t maxBlocksNeeded;
	uint64_t fullSizeOfSpace;
	uint64_t noBlocks;
	uint64_t maxIncomingBlockSize;
	
	Actor* getLocalActor(uint64_t globID);
	Actor* getLocalActor(const std::string actName);
	Actor* getRemoteActor(uint64_t globID);
	Actor* getRemoteActor(const std::string actName);
	Actor& getActor(const std::string actName);

	bool isLocalActor(uint64_t globID);
	bool isLocalActor(std::string actName);
	bool isRemoteActor(uint64_t globID);
	bool isRegisteredActor(uint64_t globID);


	ActorConnectionType getActorConnectionType(uint64_t globIDSrcActor, uint64_t globIDDestActor);
	ActorConnectionType getActorConnectionType(std::pair<uint64_t, uint64_t> curPair);
	template <typename T, int capacity> void connectPorts (uint64_t srcGlobID, const std::string srcPortLabel, uint64_t destGlobID, const std::string destPortLabel);
	template <typename T, int capacity> void connectPorts (Actor &srcActor, const std::string srcPortLabel, Actor &dstActor, const std::string destPortLabel);
	void sortConnections();
	void makeConnections();

	void genOffsets();
	std::string getOffsetString();

	double run();
	void clearDataAreas();

	ActorGraph();
	ActorGraph(int dataQueueLen);
	ActorGraph(int dataBlockSize, int dataQueueLen);
	~ActorGraph();
	ActorGraph(ActorGraph &other) = delete;
	ActorGraph & operator=(ActorGraph &other) = delete;

	void addActor(Actor* newActor);
	void syncActors();
	void synchronizeActors();
	void printActors();

	void printLookupSegment();
	void printCacheSegment();
	void printTriggerSegment();
	void printLocalLookup();

	void finalizeInitialization();
	bool finished;
};

// have to define templated method in header file, or provide all possible instantiations in cpp
template <typename T, int capacity> void ActorGraph :: connectPorts(Actor &scrActor, const std::string srcPortLabel, Actor &dstActor, const std::string destPortLabel)
{
	uint64_t srcGlobID, dstGlobID;
	srcGlobID = scrActor.actorGlobID;
	dstGlobID = dstActor.actorGlobID;
	connectPorts<T, capacity>(srcGlobID, srcPortLabel, dstGlobID, destPortLabel);
}
template <typename T, int capacity> void ActorGraph :: connectPorts(uint64_t srcGlobID, const std::string srcPortLabel, uint64_t destGlobID, const std::string destPortLabel)
{
	ActorConnectionType currentConnectionType = getActorConnectionType(srcGlobID, destGlobID);
	dataQueueLen = dataQueueLen > capacity ? dataQueueLen : capacity;
	switch (currentConnectionType)
	{
		case ActorConnectionType::LOCAL_LOCAL:
		{
			//get actors
			Actor* ac1 = getLocalActor(srcGlobID);
			Actor* ac2 = getLocalActor(destGlobID);
			//establish channel
			LocalChannel<T, capacity> *channel = new LocalChannel<T, capacity>(srcGlobID, destGlobID, localChannelCt);
			channel->name = std::to_string(srcGlobID) + " " + srcPortLabel + " " + std::to_string(destGlobID) + " " + destPortLabel;
			channel->triggerQueue = &localChannelTriggers;
			localChannelList.push_back(channel);
			localChannelCt++;
			//make ports
			Port* inPort = new InPort<T, capacity>(channel, destPortLabel);
			Port* outPort = new OutPort<T, capacity>(channel, srcPortLabel);

			ac1->addOutPort(outPort);
			ac2->addInPort(inPort);
			break;
		}
		case ActorConnectionType::LOCAL_REMOTE:
		case ActorConnectionType::REMOTE_LOCAL:
		case ActorConnectionType::REMOTE_REMOTE:
		{	
			uint64_t srcRank = Actor::decodeGlobID(srcGlobID).first;
			dataBlockSize[srcRank] = dataBlockSize[srcRank] > (sizeof(T) * capacity) ? dataBlockSize[srcRank] : (sizeof(T) * capacity);

			if(currentConnectionType == ActorConnectionType::LOCAL_REMOTE)
			{
				//get actor
				Actor* ac1 = getLocalActor(srcGlobID);
				//create outgoing channel
				RemoteChannel<T, capacity> *channel = new RemoteChannel<T, capacity> (currentConnectionType, srcGlobID, destGlobID);
				//add channel to channel list
				channel->name = std::to_string(srcGlobID) + srcPortLabel + std::to_string(destGlobID) + destPortLabel;
				remoteChannelList.push_back(channel);
				//create port
				Port* outPort = new OutPort<T, capacity>(channel, srcPortLabel);
				//link up
				ac1->addOutPort(outPort);

				noLocalRemoteChannels++;
			}
			else if(currentConnectionType == ActorConnectionType::REMOTE_LOCAL)
			{
				//get actor
				Actor* ac1 = getLocalActor(destGlobID);
				//create outgoing channel
				RemoteChannel<T, capacity> *channel = new RemoteChannel<T, capacity> (currentConnectionType, srcGlobID, destGlobID);
				channel->name = std::to_string(srcGlobID) + srcPortLabel + std::to_string(destGlobID) + destPortLabel;
				//add channel to channel list
				remoteChannelList.push_back(channel);
				//create port
				Port* inPort = new InPort<T, capacity>(channel, destPortLabel);
				//link up
				ac1->addInPort(inPort);

				noRemoteLocalChannels++;
			}
			else //current connection type is remote remote
			{
				RemoteChannel<T, capacity> *channel = new RemoteChannel<T, capacity> (currentConnectionType, srcGlobID, destGlobID);
				channel->name = std::to_string(srcGlobID) + srcPortLabel + std::to_string(destGlobID) + destPortLabel;
				//add channel to channel list
				remoteChannelList.push_back(channel);
			}
		}
	}
}

#endif