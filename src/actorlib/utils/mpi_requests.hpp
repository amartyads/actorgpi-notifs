//
// Created by Bruno Macedo Miguel on 2019-05-26.
//

#ifndef ACTORMPI_MPI_REQUESTS_HPP
#define ACTORMPI_MPI_REQUESTS_HPP

#include "mpi_consts.hpp"
#include "mpi_type_traits.h"
#include <type_traits>

namespace mpi {

enum class RequestState { INVALID, READY, LOADING, LOADED };

class Request {
public:
  Request()
      : rank(mpi::INVALID_RANK_ID), tag(mpi::INVALID_TAG_ID),
        state(RequestState::INVALID), request(), status() {}

  Request(rank rank, tag tag)
      : rank(rank), tag(tag), state(RequestState::READY), request(), status() {}

  virtual void load() = 0;

  bool canBeLoaded() const { return state == RequestState::READY; }

  bool hasCompleted() {
    if (state == RequestState::LOADED)
      return true;

    if (state != RequestState::LOADING)
      return false;

    int flag = 0;
    MPI_Test(&request, &flag, &status);

    if (static_cast<bool>(flag))
      state = RequestState::LOADED;

    return state == RequestState::LOADED;
  }

  bool isInvalid() const { return state == RequestState::INVALID; }

protected:
  mpi::rank rank;
  mpi::tag tag;
  RequestState state;

  MPI_Request request;
  MPI_Status status;
};

template <class T> class SendRequest : public Request {
public:
  SendRequest() : Request(), localBuffer() {}

  SendRequest(mpi::rank rank, mpi::tag tag, const T &data)
      : Request(rank, tag), localBuffer(data) {}

  void load() override final {
    MPI_Issend(mpi_type_traits<T>::get_addr(localBuffer),
               mpi_type_traits<T>::get_size(localBuffer),
               mpi_type_traits<T>::get_type(std::move(localBuffer)), this->rank,
               this->tag, MPI_COMM_WORLD, &this->request);

    state = RequestState::LOADING;
  }

private:
  T localBuffer;
};

template <typename> struct is_std_vector : std::false_type {};

template <typename T, typename A>
struct is_std_vector<std::vector<T, A>> : std::true_type {};

template <class T> class ReceiveRequest : public Request {
public:
  ReceiveRequest() : Request(), channelBuffer(nullptr) {}

  ReceiveRequest(mpi::rank rank, mpi::tag tag, T *buffer,
                 int commCount = mpi::INFERRED_FROM_T)
      : Request(rank, tag), channelBuffer(buffer),
        communicationCount(commCount) {
    if (is_std_vector<T>::value && commCount == mpi::INFERRED_FROM_T)
      throw std::runtime_error(
          "For std::vector types, please specify ActorGraph::connectPorts' "
          "parameter: communicationCount.");
  }

  void load() override final {
    prepareBuffer(is_std_vector<T>{});

    MPI_Irecv(mpi_type_traits<T>::get_addr(*channelBuffer),
              mpi_type_traits<T>::get_size(*channelBuffer),
              mpi_type_traits<T>::get_type(std::move(*channelBuffer)),
              this->rank, this->tag, MPI_COMM_WORLD, &this->request);

    state = RequestState::LOADING;
  }

  T *getBuffer() { return channelBuffer; }

private:
  T *channelBuffer;
  int communicationCount;

  void prepareBuffer(std::true_type) {
    channelBuffer->resize(communicationCount);
  }

  void prepareBuffer(std::false_type) {}
};

} // namespace mpi

#endif // ACTORMPI_MPI_REQUESTS_HPP
