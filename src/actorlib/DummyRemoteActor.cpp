#include "DummyRemoteActor.hpp"

DummyRemoteActor::DummyRemoteActor(std::string name, uint64_t threadRank, uint64_t actorSrNo) : Actor(name, threadRank, actorSrNo) {}

void DummyRemoteActor::act()
{
    ;
}
