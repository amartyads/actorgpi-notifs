#include <iostream>
#include <inttypes.h> // print string formats
#include <cstdint>    // uint64
#include <algorithm>  // max_element
#include <cstdlib>
#include <chrono>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>

#include <GASPI.h>
#include <GASPI_Ext.h>
#include "utils/gpi_utils.hpp"
#include "utils/connection_type_util.hpp"
#include "Actor.hpp"
#include "ActorGraph.hpp"
#include "InPort.hpp"
#include "OutPort.hpp"
#include "Channel.hpp"
#include "LocalChannel.hpp"
#include "RemoteChannel.hpp"
#include "config.hpp"

#ifndef ASSERT
#define ASSERT(ec) gpi_util::success_or_exit(__FILE__,__LINE__,ec)
#endif

#ifndef MAX
#define MAX(a,b) (a>b?a:b)
#endif

ActorGraph::ActorGraph() : ActorGraph(0,1) { }

ActorGraph::ActorGraph(int dataQueueLen) : ActorGraph(0,dataQueueLen) { }

ActorGraph::ActorGraph(int dataBlockSize, int dataQueueLen)
{
	ASSERT( gaspi_proc_rank(&threadRank) );
	ASSERT( gaspi_proc_num(&totNoThreads) );
	this->dataBlockSize.assign(totNoThreads, dataBlockSize);
	this->dataQueueLen = dataQueueLen;
	noOfOutgoingChannels.assign(totNoThreads, 0);
	noOfIncomingChannels.assign(totNoThreads, 0);
	noLocalRemoteChannels = 0;
	noRemoteLocalChannels = 0;
	localChannelCt = 0;
	finished = false;
}

ActorGraph::~ActorGraph()
{
	for(auto &actor : localActorRefList)
		delete actor;
}

void ActorGraph::addActor(Actor* newActor)
{
	localActorRefList.push_back(newActor);
	localActorIDList.push_back(newActor->actorGlobID);
	localActorRefMap.insert({newActor->actorGlobID, newActor});
}

void ActorGraph::syncActors()
{
    int actorElemSize = sizeof(uint64_t);
    //declare segment IDs
    const gaspi_segment_id_t segment_id_loc_size = 0;
  	const gaspi_segment_id_t segment_id_rem_size = 1;
    const gaspi_segment_id_t segment_id_loc_array = 2;
	const gaspi_segment_id_t segment_id_rem_array = 3;
	const gaspi_segment_id_t segment_id_loc_names = 4;
	const gaspi_segment_id_t segment_id_rem_names = 5;

    //set up exchange for array sizes
    int* locSize = (int*) (gpi_util::create_segment_return_ptr(segment_id_loc_size, sizeof(int)));
	//gaspi_printf("Segment remote sizes : %" PRIu64 "\n", totNoThreads * sizeof(int));
	int* remoteNoActors = (int*) (gpi_util::create_segment_return_ptr(segment_id_rem_size, totNoThreads * sizeof(int)));

    *locSize = localActorRefList.size();

    gaspi_queue_id_t queue_id = 0;
    
	//find max name size
	int localMaxNameSize = 0, globalMaxNameSize = 0;
	for(int i = 0; i < localActorRefList.size(); i++)
		localMaxNameSize = std::max(localMaxNameSize, (int)localActorRefList[i]->name.size());
	gaspi_pointer_t loc_size_ptr = &localMaxNameSize;
	gaspi_pointer_t glob_max_size_ptr = &globalMaxNameSize;
	ASSERT( gaspi_allreduce(loc_size_ptr, glob_max_size_ptr, 1, GASPI_OP_MAX, GASPI_TYPE_INT, GASPI_GROUP_ALL, GASPI_BLOCK) );
	
	globalMaxNameSize += 2;

    //exchange array sizes
    // ASSERT (gaspi_barrier (GASPI_GROUP_ALL, GASPI_BLOCK));

    for(int i = 0; i < totNoThreads; i++)
	{
		if(i == threadRank)
			continue;
		//read no of actors
		gpi_util::wait_for_queue_entries(&queue_id, 1);
 		
 		const gaspi_offset_t offset_dst = i * sizeof (int);
      	ASSERT (gaspi_read ( segment_id_rem_size, offset_dst
                         , i, segment_id_loc_size, 0
                         , sizeof (int), queue_id, GASPI_BLOCK
                         )
             );
    }

    //overlap some computation before flushing queues
	//create segments
	//gaspi_printf("Segment local array : %" PRIu64 "\n", actorElemSize * localActorRefList.size());
    uint64_t *localArray = (uint64_t*) (gpi_util::create_segment_return_ptr(segment_id_loc_array, actorElemSize * localActorRefList.size()));
	//gaspi_printf("Segment local names : %" PRIu64 "\n", localActorRefList.size() * globalMaxNameSize * sizeof(char));
	char* localNameSegment = (char *)(gpi_util::create_segment_return_ptr(segment_id_loc_names, localActorRefList.size() * globalMaxNameSize * sizeof(char)));
	
	//copy local IDs
    for(int i = 0; i < *locSize; i++)
		localArray[i] = localActorIDList[i];
	
	//copy local names
	std::string tempName;
	for(int i = 0; i < localActorRefList.size(); i++)
	{
		tempName = localActorRefList[i]->name;
		tempName.copy(&localNameSegment[i * globalMaxNameSize], tempName.size(), 0);
	}

    //flush queues
	ASSERT (gaspi_barrier (GASPI_GROUP_ALL, GASPI_BLOCK));
	//gaspi_printf("FQ1\n");
    gpi_util::wait_for_flush_queues();

	//segsize for IDs
	int64_t segSize = 0;
	for(int i = 0; i < totNoThreads; i++)
	{
		if(i == threadRank)
			continue;
		segSize += remoteNoActors[i] * actorElemSize;
	}
	//segsize for names
	int64_t segSize2 = 0;
	for(int i = 0; i < totNoThreads; i++)
	{
		if(i == threadRank)
			continue;
		segSize2 += remoteNoActors[i] * globalMaxNameSize * sizeof(char);
	}

	//declare segs
	//gaspi_printf("Segment remote sizes store : %" PRIu64 "\n", segSize);
    uint64_t *remoteArray = (uint64_t*) (gpi_util::create_segment_return_ptr(segment_id_rem_array, segSize));
	//gaspi_printf("Segment remote names store : %" PRIu64 "\n", segSize2);
    char* remoteNameSegment = (char *)(gpi_util::create_segment_return_ptr(segment_id_rem_names, segSize2));

    int localOffset = 0;
	queue_id = 0;

	//read in remote actor IDs
	for(int i = 0; i < totNoThreads; i++)
	{
		if(i == threadRank)
			continue;

		const gaspi_size_t segment_size_cur_rem_arr = actorElemSize * remoteNoActors[i];
		const gaspi_offset_t loc_segment_offset = localOffset;

		gpi_util::wait_for_queue_entries(&queue_id, 1);
		ASSERT (gaspi_read ( segment_id_rem_array, loc_segment_offset
	                     , i, segment_id_loc_array, 0
	                     , segment_size_cur_rem_arr, queue_id, GASPI_BLOCK
	                     )
	         );
		localOffset += actorElemSize * remoteNoActors[i];
	}

	//read in names
	localOffset = 0;

	for(int i = 0; i < totNoThreads; i++)
	{
		if(i == threadRank)
			continue;

		const gaspi_size_t segment_size_cur_rem_arr = globalMaxNameSize * remoteNoActors[i] * sizeof(char);
		const gaspi_offset_t loc_segment_offset = localOffset;

		gpi_util::wait_for_queue_entries(&queue_id, 1);
		ASSERT (gaspi_read ( segment_id_rem_names, loc_segment_offset
	                     , i, segment_id_loc_names, 0
	                     , segment_size_cur_rem_arr, queue_id, GASPI_BLOCK
	                     )
	         );
		localOffset += globalMaxNameSize * remoteNoActors[i] * sizeof(char);
	}
	//gaspi_printf("FQ2\n");
	gpi_util::wait_for_flush_queues();
	//gaspi_printf("FQDoneeeeeeeeee\n");
	//use segment pointer and push back actors
	for(int j = 0; j < (segSize/actorElemSize); j++)
	{
		remoteActorIDList.push_back(remoteArray[j]);
	}
	for(int i = 0; i < segSize2/sizeof(char); i+= globalMaxNameSize)
	{
		std::string temp(&remoteNameSegment[i], globalMaxNameSize);
		temp.erase(std::find(temp.begin(), temp.end(), '\0'), temp.end());
		remoteActorNameList.push_back(temp);
	}
	
	for(int i = 0; i < remoteActorIDList.size(); i++)
	{
		std::pair<int, int> temp = Actor::decodeGlobID(remoteActorIDList[i]);
		remoteActorRefList.push_back(new DummyRemoteActor(remoteActorNameList[i], temp.first, temp.second));
	}

    //clean up
	ASSERT (gaspi_segment_delete(segment_id_loc_size) );
	ASSERT (gaspi_segment_delete(segment_id_rem_size) );
	ASSERT (gaspi_segment_delete(segment_id_loc_array ) );
	ASSERT (gaspi_segment_delete(segment_id_rem_array ) );
	ASSERT (gaspi_segment_delete(segment_id_loc_names ) );
	ASSERT (gaspi_segment_delete(segment_id_rem_names ) );

	ASSERT (gaspi_barrier (GASPI_GROUP_ALL, GASPI_BLOCK));
}

void ActorGraph::synchronizeActors()
{
	syncActors();
}

void ActorGraph::printActors()
{
	for(int i = 0; i <localActorRefList.size(); i++)
	{
		gaspi_printf("Local actor name %s of %" PRIu64 ", ID %" PRIu64 "\n", (*localActorRefList[i]).name.c_str(), localActorRefList[i]->threadRank, localActorRefList[i]->actorGlobID);
	}
	gaspi_printf("No of actors received: %d\n", remoteActorIDList.size());
	for(int i = 0; i <remoteActorIDList.size(); i++)
	{
		std::pair<int, int> temp = Actor::decodeGlobID(remoteActorIDList[i]);

		gaspi_printf("Non local actor ID %" PRIu64 " name %s no %" PRIu64 " of rank %" PRIu64 " \n", remoteActorIDList[i], remoteActorNameList[i].c_str(), temp.second, temp.first);
	}
}

Actor* ActorGraph::getLocalActor(uint64_t globID)
{
	for(int i = 0; i < localActorRefList.size(); i++)
	{
		if(localActorRefList[i]->actorGlobID == globID)
			return localActorRefList[i];
	}
	return nullptr;
}
Actor* ActorGraph::getLocalActor(std::string actName)
{
	for(int i = 0; i < localActorRefList.size(); i++)
	{
		if(localActorRefList[i]->name == actName)
			return localActorRefList[i];
	}
	return nullptr;
}
Actor* ActorGraph::getRemoteActor(uint64_t globID)
{
	for(int i = 0; i < remoteActorRefList.size(); i++)
	{
		if(remoteActorRefList[i]->actorGlobID == globID)
			return remoteActorRefList[i];
	}
	return nullptr;
}
Actor* ActorGraph::getRemoteActor(std::string actName)
{
	for(int i = 0; i < remoteActorRefList.size(); i++)
	{
		if(remoteActorRefList[i]->name == actName)
			return remoteActorRefList[i];
	}
	return nullptr;
}
Actor& ActorGraph::getActor(std::string actName)
{
	Actor* temp = getLocalActor(actName);
	if(temp != nullptr) return *temp;
	temp = getRemoteActor(actName);
	if(temp != nullptr) return *temp;
	throw std::runtime_error("Could not find actor " + actName + ".");
}
bool ActorGraph::isLocalActor(uint64_t globID)
{
	if(getLocalActor(globID) == nullptr)
		return false;
	return true;
}
bool ActorGraph::isLocalActor(std::string actName)
{
	if(getLocalActor(actName) == nullptr)
		return false;
	return true;
}
bool ActorGraph::isRemoteActor(uint64_t globID)
{
	for(int i = 0; i < remoteActorIDList.size(); i++)
	{
		if(globID == remoteActorIDList[i])
			return true;
	}
	return false;
}
bool ActorGraph::isRegisteredActor(uint64_t globID)
{
	return (isLocalActor(globID) || isRemoteActor(globID));
}

ActorConnectionType ActorGraph::getActorConnectionType(uint64_t globIDSrcActor, uint64_t globIDDestActor)
{
	if(!isRegisteredActor(globIDSrcActor))
		throw std::runtime_error("Could not find Actor ID " + globIDSrcActor);
    if(!isRegisteredActor(globIDDestActor))
		throw std::runtime_error("Could not find Actor ID " + globIDDestActor);
    
	bool srcLoc = isLocalActor(globIDSrcActor);
	bool destLoc = isLocalActor(globIDDestActor);
	if(srcLoc && destLoc)
		return ActorConnectionType::LOCAL_LOCAL;
	else if(srcLoc && !destLoc)
		return ActorConnectionType::LOCAL_REMOTE;
	else if(!srcLoc && destLoc)
		return ActorConnectionType::REMOTE_LOCAL;
	else
		return ActorConnectionType::REMOTE_REMOTE;
}

ActorConnectionType ActorGraph::getActorConnectionType(std::pair<uint64_t, uint64_t> curPair)
{
	return getActorConnectionType(curPair.first, curPair.second);
}

void ActorGraph::finalizeInitialization()
{
	//sort channels by name
	std::sort(remoteChannelList.begin(), remoteChannelList.end(), AbstractChannel::compareChannelNames);
	//set offsets according to sort
	int64_t dataOffset, triggerOffset;
	//std::cout << "rank " <<threadRank << " pre sizes calced" <<std::endl;
	for(int i = 0; i < remoteChannelList.size(); i++)
	{
		//std::cout << "rank " <<threadRank << " i  " <<i<<std::endl;
		dataOffset = -1;
		uint64_t srcRank = Actor::decodeGlobID(remoteChannelList[i]->srcID).first;
		uint64_t dstRank = Actor::decodeGlobID(remoteChannelList[i]->dstID).first;
		if(srcRank == dstRank)
			continue;
		//std::cout << "rank " <<threadRank << " src " <<srcRank << "dst" <<dstRank <<std::endl;
		dataOffset = noOfOutgoingChannels[srcRank];
		//std::cout << "rank " <<threadRank << " access1 " <<srcRank <<std::endl;
		triggerOffset = noOfIncomingChannels[dstRank];
		//std::cout << "rank " <<threadRank << " access2 " <<srcRank <<std::endl;
		noOfOutgoingChannels[srcRank]++;
		noOfIncomingChannels[dstRank]++;
		//std::cout << "rank " <<threadRank << " offed " <<srcRank <<std::endl;

		remoteChannelList[i]->fixedDataOffset = dataOffset;
		remoteChannelList[i]->fixedTriggerOffset = triggerOffset;
		remoteChannelList[i]->remoteRank = (dstRank == threadRank ? srcRank : dstRank);
		remoteChannelList[i]->channelID = AbstractChannel::encodeGlobID(srcRank, dataOffset);

		if(srcRank == threadRank)
			remoteChannelMap.insert({dataOffset, remoteChannelList[i]});
	}
	//set current rank datablocks
	//std::cout << "rank " <<threadRank << " remotes inited" <<std::endl;
	minBlockSize = dataBlockSize[threadRank];
	maxBlocksNeeded = noOfOutgoingChannels[threadRank];
	// initialize channel fixed comm array and segment

	//calculate no of blocks
	fullSizeOfSpace = minBlockSize * maxBlocksNeeded;

	//uint64_t hardlimit = sizeof(int8_t) * 1024 * 1024 * 2;  //2 MB
	//fullSizeOfSpace = (fullSizeOfSpace < hardlimit)? fullSizeOfSpace : hardlimit;
	//fullSizeOfSpace = hardlimit;
	//minBlockSize = sizeof(int8_t) * 1024 ; // 1 kB

	//noBlocks = fullSizeOfSpace / minBlockSize;

	minBlockSize = config::dataBlockSize;
	noBlocks = config::numBlocksInBank;
	fullSizeOfSpace = noBlocks * minBlockSize;

	// initialize block lookup array
	AbstractChannel::lookupTable.assign(noBlocks, -1);

	//calculate max incoming block size
	//maxIncomingBlockSize = *std::max_element(dataBlockSize.begin(), dataBlockSize.end());
	maxIncomingBlockSize = minBlockSize * config::numBlocksInCache;
	//std::cout << "rank " <<threadRank << " sizes calced" <<std::endl;
	/* Segment list:
		1: LOCAL_REMOTE: Store block number for reading data, local access (remoteLookup)
		2: LOCAL_REMOTE: Store block number after reading data, remote access (localClear)
		3: REMOTE_LOCAL: Write destID of actors with incoming inputs, remote access (localTrigger)
		4: Local data banks (dataBank)
		5: Temp bank for reading data (tempStorage)
		6: size of vector
		*/

	segmentIDRemoteLookup = 0;
	segmentIDLocalClear = 1;
	segmentIDLocalTrigger = 2;
	segmentIDDatabank = 3;
	segmentIDLocalCache = 4;
	segmentIDRemoteVecSize = 5;
	//gaspi_printf("Segment no 0 size %" PRIu64 "\n",MAX(noLocalRemoteChannels,1) * sizeof(uint64_t) * dataQueueLen);
	gasptrRemoteLookup = gpi_util::create_segment_return_ptr(segmentIDRemoteLookup, MAX(noLocalRemoteChannels,1) * sizeof(uint64_t) * dataQueueLen); // will have databank offset (minBlockSize * fixedOffset)
	//gaspi_printf("Segment no 1 size %" PRIu64 "\n",MAX(noLocalRemoteChannels,1) * sizeof(uint64_t) * dataQueueLen);
	gasptrLocalClear = gpi_util::create_segment_return_ptr(segmentIDLocalClear, MAX(noLocalRemoteChannels,1) * sizeof(uint64_t) * dataQueueLen); // will paste tha read databank offset
	//gaspi_printf("Segment no 2 size %" PRIu64 "\n", MAX(noRemoteLocalChannels,1) * sizeof(uint64_t) * dataQueueLen);
	gasptrLocalTrigger = gpi_util::create_segment_return_ptr(segmentIDLocalTrigger, MAX(noRemoteLocalChannels,1) * sizeof(uint64_t) * dataQueueLen);	//will paste dstID
	//gaspi_printf("Segment no 3 size %" PRIu64 "\n", fullSizeOfSpace);
	gasptrDatabank = gpi_util::create_segment_return_ptr(segmentIDDatabank, fullSizeOfSpace);
	//gaspi_printf("Segment no 4 size %" PRIu64 "\n", maxIncomingBlockSize);
	gasptrLocalCache = gpi_util::create_segment_return_ptr(segmentIDLocalCache, maxIncomingBlockSize);
	//gaspi_printf("Segment no 5 size %" PRIu64 "\n",MAX(noLocalRemoteChannels,1) * sizeof(uint64_t) * dataQueueLen);
	gasptrRemoteVecSize = gpi_util::create_segment_return_ptr(segmentIDRemoteVecSize, MAX(noLocalRemoteChannels,1) * sizeof(uint64_t) * dataQueueLen); // will have size of vector

	//std::cout << "rank " <<threadRank << " segs made" <<std::endl;
	//reset lookup table
	int64_t *lookupPtr = (int64_t *) gasptrRemoteLookup;
	for(uint64_t i = 0; i < noLocalRemoteChannels * dataQueueLen; i++)
	{
		lookupPtr[i] = -1;
	}
	//reset trigger table
	int64_t *triggerPtr = (int64_t *) gasptrLocalTrigger;
	for(uint64_t i = 0; i < noRemoteLocalChannels * dataQueueLen; i++)
	{
		triggerPtr[i] = -1;
	}
	//reset clear table
	int64_t *clearPtr = (int64_t *) gasptrLocalClear;
	for(uint64_t i = 0; i < noLocalRemoteChannels * dataQueueLen; i++)
	{
		clearPtr[i] = -1;
	}

	//distribute the pointers to channels
	for(AbstractChannel* temp : remoteChannelList)
	{
		temp->fixedCommInitPtr = gasptrRemoteLookup;
		temp->fixedDatabankInitPtr = gasptrDatabank;
		temp->localDatablockSize = minBlockSize;
		temp->fixedVecSizeInitPtr = gasptrRemoteVecSize;
		temp->cachePtr = gasptrLocalCache;
		temp->minBlockSize = minBlockSize;
		temp->initialized = true;
	}

	//increment the fixed ptrs of local channels to move them out of the space of incoming remotes (not really used, kept for completion sake)
	for(AbstractChannel* temp : localChannelList)
	{
		temp->fixedTriggerOffset += noRemoteLocalChannels;
		temp->minBlockSize = minBlockSize;
		temp->initialized = true;
	}
}

double ActorGraph::run()
{
	this->finalizeInitialization();
	//trigger all local actors
	for (int i = 0; i < localActorRefList.size(); i++)
	{
		if(!(localActorRefList[i]->finished))
		{
			localActorRefList[i]->trigger();
		}
	}
		//
	auto start = std::chrono::steady_clock::now();
	//if all actors finished, quit
	//std::cout << "Rank " <<threadRank << " Run " << runNo << std::endl;
	while(!finished)
	{
		finished = true;
		for (int i = 0; i < localActorRefList.size(); i++)
		{
			if(!(localActorRefList[i]->finished))
			{
				finished = false;
				break;
			}
		}
		//std::cout << "Rank " <<threadRank << " Run " << runNo << " actor finish checked " << std::endl;
		if(finished)
		{
			//gaspi_printf("Rank %d done running.\n",threadRank);
			clearDataAreas();
			auto end = std::chrono::steady_clock::now();
			double runTime = std::chrono::duration<double, std::ratio<1>>(end - start).count();
			return runTime;

		}
		
		//printTriggerSegment();
		//std::cout << "Rank " <<threadRank << " Run " << runNo << " pre local trigger " << std::endl;
		//trigger actors (local_local)
		int64_t curActor;
		while(!localChannelTriggers.empty())
		{
			curActor = localChannelTriggers.front();
			localChannelTriggers.pop();
			localActorRefMap[curActor]->trigger();
		}
		//std::cout << "Rank " <<threadRank << " Run " << runNo << " post local trigger " << std::endl;
		//trigger actors (remote_local)
		int64_t *triggerSegment = (int64_t *)gasptrLocalTrigger;
		for(uint64_t i = 0; i < noRemoteLocalChannels * dataQueueLen; i++)
		{
			if(triggerSegment[i] == -1)
				continue;
			localActorRefMap[triggerSegment[i]]->trigger();
			triggerSegment[i] = -1;
		}
		//std::cout << "Rank " <<threadRank << " Run " << runNo << " post remote trigger " << std::endl;
		//run triggred actors
		for (int i = 0; i < localActorRefList.size(); i++)
		{
			if(localActorRefList[i]->isTriggered())
			{
				//std::cout << localActorRefList[i]->actorGlobID << " triggered " << std::endl;
				localActorRefList[i]->act();
				//std::cout << localActorRefList[i]->actorGlobID << " acted " << std::endl;
				localActorRefList[i]->detrigger();
			}
		}
		//std::cout << "Rank " <<threadRank << " Run " << runNo << " post all actors run " << std::endl;
		//clear data banks
		//while clearing, read the channel offset in the slot, look into the channel map and increment queue current capacity
		clearDataAreas();
	}
	//std::cout << "Rank " <<threadRank << " Run " << runNo << " post clears " << std::endl;
	//printLookupSegment();
	//printCacheSegment();
	//if(threadRank == 1)
	// printLocalLookup();
	// auto end = std::chrono::steady_clock::now();
	// double runTime = std::chrono::duration<double, std::ratio<1>>(end - start).count();
	// return runTime;
}

void ActorGraph::clearDataAreas()
{
	int64_t *clearPtr = (int64_t *) gasptrLocalClear;
	int64_t *offsetPtr;
	for(uint64_t i = 0; i < noLocalRemoteChannels * dataQueueLen; i++)
	{
		if(clearPtr[i] == -1)
			continue;
		
		int64_t slot = clearPtr[i];
		slot /= minBlockSize;

		int64_t chId = AbstractChannel::lookupTable[slot];
		remoteChannelMap[chId]->curQueueSize++; 
		AbstractChannel::lookupTable[slot] = -1;
		//go thru neighbouring datablocks
		uint64_t j = 1;
		while(AbstractChannel::lookupTable[slot + j] == -2)
		{
			AbstractChannel::lookupTable[slot + j] = -1;
			j++;
		}
		clearPtr[i] = -1;
	}
}

void ActorGraph::printLookupSegment()
{
	std::cout << "Rank " << threadRank << " lookup segment :";
	int64_t *lookupPtr = (int64_t *) gasptrRemoteLookup;
	for(uint64_t i = 0; i < noLocalRemoteChannels * dataQueueLen; i++)
	{
		std::cout << lookupPtr[i] << " ";
	}
	std::cout << std::endl;
}

void ActorGraph::printCacheSegment()
{
	std::cout << "Rank " << threadRank << " cache segment :";
	int64_t *lookupPtr = (int64_t *) gasptrLocalCache;
	for(uint64_t i = 0; i <maxIncomingBlockSize; i++)
	{
		std::cout << lookupPtr[i] << " ";
	}
	std::cout << std::endl;
}

void ActorGraph::printTriggerSegment()
{
	std::cout << "Rank " << threadRank << " trigger segment :";
	int64_t *triggerPtr = (int64_t *) gasptrLocalTrigger;
	for(uint64_t i = 0; i < noRemoteLocalChannels * dataQueueLen; i++)
	{
		std::cout << triggerPtr[i] << " ";
	}
	std::cout << std::endl;
}

void ActorGraph::printLocalLookup()
{
	std::cout << "Rank " << threadRank << " local lookup :";
	for(auto it = AbstractChannel::lookupTable.begin(); it != AbstractChannel::lookupTable.end(); ++it)
	{
		std::cout << *it << " ";
	}
	std::cout << std::endl;
}