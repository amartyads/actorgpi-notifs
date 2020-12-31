#include "actorlib/Actor.hpp"

class PrimePrintActor: public Actor
{
public:
    void act();
    int recdNos;
    PrimePrintActor(uint64_t rank, uint64_t srno);
};