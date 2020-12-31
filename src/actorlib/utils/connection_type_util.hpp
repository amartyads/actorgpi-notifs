#ifndef ACTORCONNECT_UTILS_HPP
#define ACTORCONNECT_UTILS_HPP

#pragma once

enum class ActorConnectionType
{
    LOCAL_LOCAL,
    LOCAL_REMOTE,
    REMOTE_LOCAL,
    REMOTE_REMOTE,
    ACTOR_DNE,
};

#endif