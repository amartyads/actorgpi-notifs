#include "actorlib/Actor.hpp"
#include <vector>

class PrimeSourceActor: public Actor
{
public:
    void act();
    int noLeftToSend;
    std::vector<int> numList;
    PrimeSourceActor(uint64_t rank, uint64_t srno);
};