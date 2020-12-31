#include <GASPI.h>
#include <GASPI_Ext.h>
#include "actorlib/utils/gpi_utils.hpp"
#include "PingPongActor.hpp"
#include "actorlib/ActorGraph.hpp"
#include <stdlib.h>
#include <cstdint>
#include <iostream>
#include <string>

#include <mpi.h>
//#include <omp.h>

#ifndef ASSERT
#define ASSERT(ec) gpi_util::success_or_exit(__FILE__,__LINE__,ec)
#endif

int main(int argc, char *argv[])
{
	MPI_Init(&argc, &argv);

	gaspi_rank_t rank, num;
	gaspi_return_t ret;

	ASSERT( gaspi_proc_init(GASPI_BLOCK) );

	int maxVals = 1;
	int queueMax = 3;

	ActorGraph ag(maxVals, queueMax);

	// ASSERT( gaspi_proc_rank(&rank));
	// ASSERT( gaspi_proc_num(&num) );
	rank = gpi_util::get_local_rank();
	num = gpi_util::get_total_ranks();
	//std::cout << "Rank " << rank << " Num " << num << std::endl;

	Actor *localActor1 = new PingPongActor();
	Actor *localActor2 = new PingPongActor();
	Actor *localActor3 = new PingPongActor();
	Actor *localActor4 = new PingPongActor();

	//ag.addActor(localActor3);
	ag.addActor(localActor1);
	ag.addActor(localActor2);
	ag.addActor(localActor3);
	ag.addActor(localActor4);

	ASSERT (gaspi_barrier (GASPI_GROUP_ALL, GASPI_BLOCK));

	ag.syncActors();
	ag.printActors();

	if(rank == 0)
	{
		auto &temp = ag.getActor("A-1-1");
		std::cout << "Found " << temp.actorGlobID << std::endl;
	}
	
	ag.connectPorts<double, 1>(0,"NEXT", Actor::encodeGlobID(1,0), "PREV");
	ag.connectPorts<double, 1>(Actor::encodeGlobID(1,0),"NEXT", Actor::encodeGlobID(2,1), "PREV");
	ag.connectPorts<double, 1>(Actor::encodeGlobID(2,1),"NEXT", Actor::encodeGlobID(2,0), "PREV");
	ag.connectPorts<double, 1>(Actor::encodeGlobID(2,0),"NEXT", Actor::encodeGlobID(1,1), "PREV");
	ag.connectPorts<double, 1>(Actor::encodeGlobID(1,1),"NEXT", Actor::encodeGlobID(0,1), "PREV");

	ag.connectPorts<double, 1>(2,"NEXT", Actor::encodeGlobID(1,2), "PREV");
	ag.connectPorts<double, 1>(Actor::encodeGlobID(1,2),"NEXT", Actor::encodeGlobID(2,2), "PREV");
	ag.connectPorts<double, 1>(Actor::encodeGlobID(2,2),"NEXT", Actor::encodeGlobID(2,3), "PREV");
	ag.connectPorts<double, 1>(Actor::encodeGlobID(2,3),"NEXT", Actor::encodeGlobID(1,3), "PREV");
	ag.connectPorts<double, 1>(Actor::encodeGlobID(1,3),"NEXT", Actor::encodeGlobID(0,3), "PREV");
	//std::cout << "rank " <<rank << "made connections" <<std::endl;
	ASSERT (gaspi_barrier (GASPI_GROUP_ALL, GASPI_BLOCK));
	//std::cout << "rank " <<rank << "inited" <<std::endl;
	
	/*int i;
	for(i = 0; i < num - 1; i++)
	{
		ag.pushConnection(Actor::encodeGlobID(i,0), Actor::encodeGlobID(i+1,0));
	}
	ag.pushConnection(Actor::encodeGlobID(i,0), Actor::encodeGlobID(i,1));
	for(; i > 0; i--)
	{
		ag.pushConnection(Actor::encodeGlobID(i,1), Actor::encodeGlobID(i-1,1));
	}
	
	/*
	//ag.pushConnection(1,Actor::encodeGlobID(1,0));
	ag.pushConnection(0,Actor::encodeGlobID(1,0));
	ag.pushConnection(Actor::encodeGlobID(1,0),Actor::encodeGlobID(2,0));
	//ag.pushConnection(Actor::encodeGlobID(2,0),Actor::encodeGlobID(2,1));
	//ag.pushConnection(Actor::encodeGlobID(1,0),Actor::encodeGlobID(1,1));
	//ag.pushConnection(Actor::encodeGlobID(1,0),1);
	//ag.pushConnection(1,Actor::encodeGlobID(1,1));
	//ag.pushConnection(Actor::encodeGlobID(1,1),Actor::encodeGlobID(2,0));
	ag.pushConnection(Actor::encodeGlobID(2,0),Actor::encodeGlobID(3,0));
	ag.pushConnection(Actor::encodeGlobID(3,0),Actor::encodeGlobID(3,1));
	ag.pushConnection(Actor::encodeGlobID(3,1),Actor::encodeGlobID(2,1));
	ag.pushConnection(Actor::encodeGlobID(2,1),Actor::encodeGlobID(1,1));
	ag.pushConnection(Actor::encodeGlobID(1,1),1);
	*/

	//ag.makeConnections();

	
	double rt = ag.run();
	gaspi_printf("Runtime from rank %d: %lf\n",rank,rt);
	//if(rank == 0)
	gaspi_printf("Rank %d done.\n",rank);	
	
	gpi_util::wait_for_flush_queues();
	//ag.printActors();
	ASSERT (gaspi_barrier (GASPI_GROUP_ALL, GASPI_BLOCK));
	//ag.sortConnections();
	//ag.genOffsets();
	//std::string offstr = ag.getOffsetString();

	//std::cout << "Rank " <<rank<<" offset string " << offstr << std::endl;*/

	ASSERT( gaspi_proc_term(GASPI_BLOCK) );
	MPI_Finalize();
	std::cout << "post proc term" <<std::endl;
	return EXIT_SUCCESS;
}