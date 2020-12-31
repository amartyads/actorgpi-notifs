#ifndef ACTORMPI_MPI_DATATYPES_HPP
#define ACTORMPI_MPI_DATATYPES_HPP

#include "mpi.h"

namespace mpi {
namespace data {

constexpr int MAX_ACTOR_NAME_LENGTH = 64;

struct ActorData {
  int rank;
  char name[MAX_ACTOR_NAME_LENGTH];
};

static void createActorDataType(MPI_Datatype &type) {
  int numberOfElements = 2;
  int blockLength[2] = {1, MAX_ACTOR_NAME_LENGTH};
  MPI_Aint displacements[2] = {offsetof(ActorData, rank),
                               offsetof(ActorData, name)};
  MPI_Datatype types[2] = {MPI_INT, MPI_CHAR};
  MPI_Datatype tmpType;

  MPI_Type_create_struct(numberOfElements, blockLength, displacements, types,
                         &tmpType);

  // https://stackoverflow.com/questions/33618937/trouble-understanding-mpi-type-create-struct
  MPI_Aint lb, extent;
  MPI_Type_get_extent(tmpType, &lb, &extent);
  MPI_Type_create_resized(tmpType, lb, extent, &type);

  MPI_Type_commit(&type);
}
} // namespace data
} // namespace mpi

#endif