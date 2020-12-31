#include "actorlib/Actor.hpp"

class PingPongActor: public Actor
{
public:
    void act();
    PingPongActor(uint64_t rank, uint64_t srno);
    PingPongActor();
};