target_sources(
    actorlib
    
    PUBLIC
    AbstractChannel.hpp
    Actor.hpp
    ActorGraph.hpp
    Channel.hpp
    DummyRemoteActor.hpp
    InPort.hpp
    LocalChannel.hpp
    OutPort.hpp
    Port.hpp
    RemoteChannel.hpp
    config.hpp
    utils/gpi_utils.hpp
    utils/connection_type_util.hpp
    utils/gen_utils.hpp
    utils/mpi_consts.hpp
    utils/mpi_type_traits.h
    utils/mpi_synchronization.hpp
    
    PRIVATE
    AbstractChannel.cpp
    Actor.cpp
    ActorGraph.cpp
    DummyRemoteActor.cpp
    config.cpp
)
