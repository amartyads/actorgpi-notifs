#include "actorlib/Actor.hpp"
#include "actorlib/InPort.hpp"
#include "actorlib/OutPort.hpp"
#include "PingPongActor.hpp"
#include <stdlib.h>
#include <vector>
#include <string>
#include <utility>
#include <cstdint>
#include <iostream>

PingPongActor::PingPongActor(uint64_t rank, uint64_t srno) : Actor(rank, srno) { }

PingPongActor::PingPongActor() : Actor() { }

void PingPongActor::act()
{
    if(finished) return;
	
	//std::cout << "Actor " << globID << std::endl;
	if(this->actorGlobID == 0 || this->actorGlobID == 2) //starter
	{
		if(!finished)
		{
			std::cout << "Actor " << actorGlobID << " commencing pingpong" <<std::endl;
			finished = true;
			double data = 42.42;
            std::string portname = "NEXT";
            std::cout << "getting port" <<std::endl;
			auto port = getOutPort<double, 3>(portname);
            std::cout << "writing" <<std::endl;
            port->write(data);
            std::cout << "written" <<std::endl;
		}
		else
		{
			std::cout << "Actor 0 already commenced pingpong" <<std::endl;
		}
		
	}
	else
	{
		bool hasInData = false;
        auto port = getInPort<double, 3>("PREV");
		if(port-> available())
		{
			double data = port->read();
			std::cout << "Actor " << actorGlobID << " received "<< data << std::endl;
			data = data + 10.0;
            if(actorGlobID != 1 && actorGlobID != 3)
            {
                std::cout << "getting port" <<std::endl;
                auto port2 = getOutPort<double, 3>("NEXT");
                std::cout << "writng" <<std::endl;
                port2->write(data);
                std::cout << "written" <<std::endl;
            }
            else 
            {
                std::cout << "Done" << std::endl;
            }
			finished = true;
		}
		else
		{
			std::cout << "Actor " << actorGlobID << " received nothing yet." << std::endl;
		}
		
	}
	
}