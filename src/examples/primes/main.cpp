#include <GASPI.h>
#include <GASPI_Ext.h>
#include "actorlib/utils/gpi_utils.hpp"
#include "PrimeSourceActor.hpp"
#include "PrimeCalcActor.hpp"
#include "PrimePrintActor.hpp"
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

	ActorGraph ag;

	ASSERT( gaspi_proc_rank(&rank));
	ASSERT( gaspi_proc_num(&num) );

    Actor *localActor1;
    
    if(rank == 0)
        localActor1 = new PrimeSourceActor(rank, 0);
    else if (rank == 1)
    {
        localActor1 = new PrimeCalcActor(rank, 0);
    }
    
    else
    {
        localActor1 = new PrimePrintActor(rank, 0);
    }
    
	ag.addActor(localActor1);

	ASSERT (gaspi_barrier (GASPI_GROUP_ALL, GASPI_BLOCK));

	ag.syncActors();
	ag.printActors();

	//if(localActor1->actorGlobID == 0) localActor1->triggers = 5;

	
	ag.connectPorts<int, 3>(Actor::encodeGlobID(0,0),"NEXT", Actor::encodeGlobID(1,0), "PREV");
    ag.connectPorts<std::vector<int>, 3>(Actor::encodeGlobID(1,0),"NEXT", Actor::encodeGlobID(2,0), "PREV");
	//std::cout << "rank " <<rank << "inited" <<std::endl;

	double rt = ag.run();
	gaspi_printf("Runtime from rank %d: %lf\n",rank,rt);
	//if(rank == 0)
	gaspi_printf("Rank %d done.\n",rank);	
	
	//gpi_util::wait_for_flush_queues();
	ASSERT (gaspi_barrier (GASPI_GROUP_ALL, GASPI_BLOCK));
	//ag.sortConnections();
	//ag.genOffsets();
	//std::string offstr = ag.getOffsetString();

	//std::cout << "Rank " <<rank<<" offset string " << offstr << std::endl;*/

	ASSERT( gaspi_proc_term(GASPI_BLOCK) );
	MPI_Finalize();
	std::cout << "post proc term rank " <<rank <<std::endl;
	return EXIT_SUCCESS;
}