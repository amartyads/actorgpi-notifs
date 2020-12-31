#include "actorlib/Actor.hpp"

class PrimeCalcActor: public Actor
{
public:
    void act();
    int noListsSent;
    std::vector<int> primes;
    bool listPending;
    PrimeCalcActor(uint64_t rank, uint64_t srno);
};