#include "actorlib/Actor.hpp"
#include "actorlib/InPort.hpp"
#include "actorlib/OutPort.hpp"
#include "PrimePrintActor.hpp"
#include <stdlib.h>
#include <vector>
#include <string>
#include <utility>
#include <cstdint>
#include <iostream>

PrimePrintActor::PrimePrintActor(uint64_t rank, uint64_t srno) : Actor(rank, srno)
{
    recdNos = 0;
}

void PrimePrintActor::act()
{
    if(finished) return;
    auto port = getInPort<std::vector<int>, 3>("PREV");
    
    if(port-> available())
    {
        std::vector<int> data = port->read();
        recdNos++;
        std::cout << "sink received: ";
        for(auto it = data.begin(); it != data.end(); ++it)
            std::cout << *it << " ";
        std::cout << std::endl;
    }
    else
    {
        std::cout << "print triggered without input data!" << std::endl;
        //this->trigger();
    }
	
	if(recdNos == 5)
        finished = true;

}