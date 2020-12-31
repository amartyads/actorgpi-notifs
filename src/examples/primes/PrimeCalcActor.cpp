#include "actorlib/Actor.hpp"
#include "actorlib/InPort.hpp"
#include "actorlib/OutPort.hpp"
#include "PrimeCalcActor.hpp"
#include <stdlib.h>
#include <vector>
#include <string>
#include <utility>
#include <cstdint>
#include <iostream>

PrimeCalcActor::PrimeCalcActor(uint64_t rank, uint64_t srno) : Actor(rank, srno) 
{
    noListsSent = 0;
    listPending = false;
}

void PrimeCalcActor::act()
{
    if(finished) return;
	
	if(listPending)
    {
        std::cout << "list pending to be sent to print, resending..." <<std::endl;
    }
    else
    {
        //must populate list
        primes.clear();
        auto port = getInPort<int, 3>("PREV");
        int data = 0;
        if(port-> available())
        {
            data = port->read();
            std::cout << "primer received "<< data << std::endl;
        }
        else
        {
            std::cout << "primer triggered without input data!" << std::endl;
            //this->trigger();
            return;
        }
        //at this point data is received
        primes.push_back(2);
        primes.push_back(3);
        bool isPrime;
        for(int i = 5; i < data; i++)
        {
            isPrime = true;
            for(int j = i - 1; j > 1; j--)
            {
                if(i%j == 0)
                    isPrime = false;
                if(!isPrime)
                    break;
            }
            if(isPrime)
                primes.push_back(i);
        }
        listPending = true;
    }
    //at this point we have a list to send in either case
    auto printPort = getOutPort<std::vector<int>, 3>("NEXT");
    if(printPort->freeCapacity() >= sizeof(int) * primes.size())
    {
        //std::cout << "Found space: "<< sizeof(int) * primes.size() << " Full size: " <<printPort->freeCapacity()<<std::endl;
        printPort->write(primes);
        noListsSent++;
        listPending = false;
    }
    else
    {
        std::cout<< "primer outqueue full, trying again soon" <<std::endl;
        this->trigger(); //will go thru true branch of listpending with this trigger
    }
    
    if(noListsSent == 5)
    {
        this->triggers = 0;
        finished = true;

	}
}