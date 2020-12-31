#include "actorlib/Actor.hpp"
#include "actorlib/InPort.hpp"
#include "actorlib/OutPort.hpp"
#include "PrimeSourceActor.hpp"
#include <stdlib.h>
#include <vector>
#include <string>
#include <utility>
#include <cstdint>
#include <iostream>

PrimeSourceActor::PrimeSourceActor(uint64_t rank, uint64_t srno) : Actor(rank, srno) 
{
    noLeftToSend = 5;
}

void PrimeSourceActor::act()
{
    if(finished) return;
	
	if(numList.size() == 0)
    {
        numList.push_back(12);
        numList.push_back(500);
        numList.push_back(7);
        numList.push_back(1000);
        numList.push_back(50);
        this->triggers = 5; //one is for running now
    }

    auto port = getOutPort<int, 3>("NEXT");
    if(port->freeCapacity() > 0)
    {
        std::cout << "source sending " << numList[numList.size() - noLeftToSend] << std::endl;
        port->write(numList[numList.size() - noLeftToSend]);
        noLeftToSend--;
    }
    else
    {
        std::cout << "queue full for source, waiting... " << std::endl;
        this->trigger();
    }
    
    if(noLeftToSend == 0)
        finished = true;
	
}