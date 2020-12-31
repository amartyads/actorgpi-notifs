#ifndef DUMMY_REMOTE_ACTOR_HPP
#define DUMMY_REMOTE_ACTOR_HPP

#include "Actor.hpp"

class DummyRemoteActor : public Actor
{
public:
    void act();
    DummyRemoteActor(std::string name, uint64_t threadRank, uint64_t actorSrNo);
};

#endif