#include "source/common/network/io_uring_socket_handle_impl.h"

#include <asm-generic/errno-base.h>
#include <fcntl.h>
#include <sys/socket.h>

#include "envoy/buffer/buffer.h"
#include "envoy/common/exception.h"
#include "envoy/event/dispatcher.h"

#include "source/common/api/os_sys_calls_impl.h"
#include "source/common/common/assert.h"
#include "source/common/common/utility.h"
#include "source/common/network/address_impl.h"
#include "source/common/network/io_socket_error_impl.h"
#include "source/common/network/io_socket_handle_impl.h"
#include "source/common/network/socket_interface_impl.h"

namespace Envoy {
namespace Network {

IoUringSocketHandleImpl::IoUringSocketHandleImpl(Io::IoUringFactory& io_uring_factory, os_fd_t fd,
                                                 bool socket_v6only, absl::optional<int> domain,
                                                 bool is_server_socket)
    : IoSocketHandleBaseImpl(fd, socket_v6only, domain), io_uring_factory_(io_uring_factory) {
  ENVOY_LOG(trace, "construct io uring socket handle, fd = {}, is_server_socket = {}", fd_,
            is_server_socket);
  if (is_server_socket) {
    io_uring_socket_type_ = IoUringSocketType::Server;
    if (!enable_server_socket_) {
      shadow_io_handle_ = std::make_unique<IoSocketHandleImpl>(fd_, socket_v6only_, domain_);
      shadow_io_handle_->setBlocking(false);
    }
  }
}

IoUringSocketHandleImpl::~IoUringSocketHandleImpl() {
  if (SOCKET_VALID(fd_)) {
    // The TLS slot has been shut down by this moment with IoUring wiped out, thus
    // better use this posix system call instead of IoUringSocketHandleImpl::close().
    ::close(fd_);
  }
}

Api::IoCallUint64Result IoUringSocketHandleImpl::close() {
  ASSERT(SOCKET_VALID(fd_));
  ENVOY_LOG(trace, "close, fd = {}, type = {}", fd_, ioUringSocketTypeStr());

  // Fall back to shadow io handle if client/server socket disabled.
  // This will be removed when all the debug work done.
  if ((io_uring_socket_type_ == IoUringSocketType::Client && !enable_client_socket_) ||
      (io_uring_socket_type_ == IoUringSocketType::Server && !enable_server_socket_) ||
      (io_uring_socket_type_ == IoUringSocketType::Accept && !enable_accept_socket_)) {
    ENVOY_LOG(trace, "fallback to shadow io uring handle, fd = {}, type = {}", fd_,
              ioUringSocketTypeStr());
    if (shadow_io_handle_ == nullptr) {
      ::close(fd_);
      SET_SOCKET_INVALID(fd_);
      return Api::ioCallUint64ResultNoError();
    }
    SET_SOCKET_INVALID(fd_);
    return shadow_io_handle_->close();
  }

  ::close(fd_);
  SET_SOCKET_INVALID(fd_);
  return Api::ioCallUint64ResultNoError();
}

Api::IoCallUint64Result
IoUringSocketHandleImpl::readv(uint64_t max_length, Buffer::RawSlice* slices, uint64_t num_slice) {
  ASSERT(io_uring_socket_type_ != IoUringSocketType::Unknown);
  ASSERT(io_uring_socket_type_ != IoUringSocketType::Accept);
  ENVOY_LOG(debug, "readv, fd = {}, type = {}", fd_, ioUringSocketTypeStr());

  // Fall back to shadow io handle if client/server socket disabled.
  // This will be removed when all the debug work done.
  if ((io_uring_socket_type_ == IoUringSocketType::Client && !enable_client_socket_) ||
      (io_uring_socket_type_ == IoUringSocketType::Server && !enable_server_socket_) ||
      (io_uring_socket_type_ == IoUringSocketType::Accept && !enable_accept_socket_)) {
    ENVOY_LOG(debug, "readv fallback to shadow io handle, fd = {}, type = {}", fd_,
              ioUringSocketTypeStr());
    return shadow_io_handle_->readv(max_length, slices, num_slice);
  }

  return Api::ioCallUint64ResultNoError();
}

Api::IoCallUint64Result IoUringSocketHandleImpl::read(Buffer::Instance& buffer,
                                                      absl::optional<uint64_t> max_length_opt) {
  ASSERT(io_uring_socket_type_ != IoUringSocketType::Unknown);
  ASSERT(io_uring_socket_type_ != IoUringSocketType::Accept);
  ENVOY_LOG(trace, "read, fd = {}, type = {}", fd_, ioUringSocketTypeStr());

  // Fall back to shadow io handle if client/server socket disabled.
  // This will be removed when all the debug work done.
  if ((io_uring_socket_type_ == IoUringSocketType::Client && !enable_client_socket_) ||
      (io_uring_socket_type_ == IoUringSocketType::Server && !enable_server_socket_) ||
      (io_uring_socket_type_ == IoUringSocketType::Accept && !enable_accept_socket_)) {
    return shadow_io_handle_->read(buffer, max_length_opt);
  }

  return Api::ioCallUint64ResultNoError();
}

Api::IoCallUint64Result IoUringSocketHandleImpl::writev(const Buffer::RawSlice* slices,
                                                        uint64_t num_slice) {
  ASSERT(io_uring_socket_type_ != IoUringSocketType::Unknown);
  ASSERT(io_uring_socket_type_ != IoUringSocketType::Accept);
  ENVOY_LOG(trace, "writev, fd = {}, type = {}", fd_, ioUringSocketTypeStr());

  if ((io_uring_socket_type_ == IoUringSocketType::Client && !enable_client_socket_) ||
      (io_uring_socket_type_ == IoUringSocketType::Server && !enable_server_socket_) ||
      (io_uring_socket_type_ == IoUringSocketType::Accept && !enable_accept_socket_)) {
    return shadow_io_handle_->writev(slices, num_slice);
  }

  return Api::ioCallUint64ResultNoError();
}

Api::IoCallUint64Result IoUringSocketHandleImpl::write(Buffer::Instance& buffer) {
  ASSERT(io_uring_socket_type_ != IoUringSocketType::Unknown);
  ENVOY_LOG(trace, "write, length = {}, fd = {}, type = {}", buffer.length(), fd_,
            ioUringSocketTypeStr());

  if ((io_uring_socket_type_ == IoUringSocketType::Client && !enable_client_socket_) ||
      (io_uring_socket_type_ == IoUringSocketType::Server && !enable_server_socket_) ||
      (io_uring_socket_type_ == IoUringSocketType::Accept && !enable_accept_socket_)) {
    return shadow_io_handle_->write(buffer);
  }

  return Api::ioCallUint64ResultNoError();
}

Api::IoCallUint64Result IoUringSocketHandleImpl::sendmsg(const Buffer::RawSlice* slices,
                                                         uint64_t num_slice, int flags,
                                                         const Address::Ip* self_ip,
                                                         const Address::Instance& peer_address) {
  ASSERT(io_uring_socket_type_ != IoUringSocketType::Unknown);
  ASSERT(io_uring_socket_type_ != IoUringSocketType::Accept);
  ENVOY_LOG(trace, "sendmsg, fd = {}, type = {}", fd_, ioUringSocketTypeStr());

  if ((io_uring_socket_type_ == IoUringSocketType::Client && !enable_client_socket_) ||
      (io_uring_socket_type_ == IoUringSocketType::Server && !enable_server_socket_) ||
      (io_uring_socket_type_ == IoUringSocketType::Accept && !enable_accept_socket_)) {
    return shadow_io_handle_->sendmsg(slices, num_slice, flags, self_ip, peer_address);
  }
  PANIC("not implement");
}

Api::IoCallUint64Result IoUringSocketHandleImpl::recvmsg(Buffer::RawSlice* slices,
                                                         const uint64_t num_slice,
                                                         uint32_t self_port,
                                                         RecvMsgOutput& output) {
  ASSERT(io_uring_socket_type_ != IoUringSocketType::Unknown);
  ASSERT(io_uring_socket_type_ != IoUringSocketType::Accept);
  ENVOY_LOG(trace, "recvmsg, fd = {}, type = {}", fd_, ioUringSocketTypeStr());

  if ((io_uring_socket_type_ == IoUringSocketType::Client && !enable_client_socket_) ||
      (io_uring_socket_type_ == IoUringSocketType::Server && !enable_server_socket_) ||
      (io_uring_socket_type_ == IoUringSocketType::Accept && !enable_accept_socket_)) {
    return shadow_io_handle_->recvmsg(slices, num_slice, self_port, output);
  }
  PANIC("not implemented");
}

Api::IoCallUint64Result IoUringSocketHandleImpl::recvmmsg(RawSliceArrays& slices,
                                                          uint32_t self_port,
                                                          RecvMsgOutput& output) {
  ASSERT(io_uring_socket_type_ != IoUringSocketType::Unknown);
  ASSERT(io_uring_socket_type_ != IoUringSocketType::Accept);
  ENVOY_LOG(trace, "recvmmsg, fd = {}, type = {}", fd_, ioUringSocketTypeStr());

  if ((io_uring_socket_type_ == IoUringSocketType::Client && !enable_client_socket_) ||
      (io_uring_socket_type_ == IoUringSocketType::Server && !enable_server_socket_) ||
      (io_uring_socket_type_ == IoUringSocketType::Accept && !enable_accept_socket_)) {
    return shadow_io_handle_->recvmmsg(slices, self_port, output);
  }
  PANIC("not implemented");
}

Api::IoCallUint64Result IoUringSocketHandleImpl::recv(void* buffer, size_t length, int flags) {
  ASSERT(io_uring_socket_type_ != IoUringSocketType::Unknown);
  ASSERT(io_uring_socket_type_ != IoUringSocketType::Accept);
  ENVOY_LOG(trace, "recv, fd = {}, type = {}", fd_, ioUringSocketTypeStr());

  if ((io_uring_socket_type_ == IoUringSocketType::Client && !enable_client_socket_) ||
      (io_uring_socket_type_ == IoUringSocketType::Server && !enable_server_socket_) ||
      (io_uring_socket_type_ == IoUringSocketType::Accept && !enable_accept_socket_)) {
    return shadow_io_handle_->recv(buffer, length, flags);
  }
  PANIC("not implemented");
}

Api::SysCallIntResult IoUringSocketHandleImpl::bind(Address::InstanceConstSharedPtr address) {
  ENVOY_LOG(trace, "bind to address {}", address->asString());
  ENVOY_LOG(trace, "bind, fd = {}, io_uring_socket_type = {}", fd_, ioUringSocketTypeStr());
  return Api::OsSysCallsSingleton::get().bind(fd_, address->sockAddr(), address->sockAddrLen());
}

Api::SysCallIntResult IoUringSocketHandleImpl::listen(int backlog) {
  ASSERT(io_uring_socket_type_ == IoUringSocketType::Unknown);
  ENVOY_LOG(trace, "listen, fd = {}, io_uring_socket_type = {}", fd_, ioUringSocketTypeStr());
  io_uring_socket_type_ = IoUringSocketType::Accept;
  if (!enable_accept_socket_) {
    ENVOY_LOG(trace, "fallback to create IoSocketHandle, fd = {}, io_uring_socket_type = {}", fd_,
              ioUringSocketTypeStr());
    shadow_io_handle_ = std::make_unique<IoSocketHandleImpl>(fd_, socket_v6only_, domain_);
    shadow_io_handle_->setBlocking(false);
    return shadow_io_handle_->listen(backlog);
  }
  return Api::OsSysCallsSingleton::get().listen(fd_, backlog);
}

IoHandlePtr IoUringSocketHandleImpl::accept(struct sockaddr* addr, socklen_t* addrlen) {
  ASSERT(io_uring_socket_type_ == IoUringSocketType::Accept);
  ENVOY_LOG(trace, "accept, fd = {}, io_uring_socket_type = {}", fd_, ioUringSocketTypeStr());

  if (!enable_accept_socket_) {
    ENVOY_LOG(trace, "fallback to IoSocketHandle for accept socket");
    auto result = Api::OsSysCallsSingleton::get().accept(fd_, addr, addrlen);
    if (SOCKET_INVALID(result.return_value_)) {
      ENVOY_LOG(trace, "accept return invalid socket");
      return nullptr;
    }
    return std::make_unique<IoUringSocketHandleImpl>(io_uring_factory_, result.return_value_,
                                                     socket_v6only_, domain_, true);
  }

  return nullptr;
}

Api::SysCallIntResult IoUringSocketHandleImpl::connect(Address::InstanceConstSharedPtr address) {
  ASSERT(io_uring_socket_type_ == IoUringSocketType::Client);
  ENVOY_LOG(trace, "connect, fd = {}, io_uring_socket_type = {}", fd_, ioUringSocketTypeStr());

  if (!enable_client_socket_) {
    ENVOY_LOG(trace, "fallback to IoSocketHandle for client socket");
    return shadow_io_handle_->connect(address);
  }

  return Api::SysCallIntResult{0, 0};
}

void IoUringSocketHandleImpl::initializeFileEvent(Event::Dispatcher& dispatcher,
                                                  Event::FileReadyCb cb,
                                                  Event::FileTriggerType trigger, uint32_t events) {
  ENVOY_LOG(trace, "initialize file event fd = {}", fd_);

  switch (io_uring_socket_type_) {
  case IoUringSocketType::Server: {
    ENVOY_LOG(trace, "initialize file event for server socket, fd = {}", fd_);
    if (!enable_server_socket_) {
      ENVOY_LOG(trace, "fallback to IoSocketHandle for server socket");
      shadow_io_handle_->initializeFileEvent(dispatcher, std::move(cb), trigger, events);
      return;
    }
    break;
  }
  case IoUringSocketType::Accept: {
    ENVOY_LOG(trace, "initialize file event for accept socket, fd = {}", fd_);
    if (!enable_accept_socket_) {
      ENVOY_LOG(trace, "fallback to IoSocketHandle for accept socket");
      shadow_io_handle_->initializeFileEvent(dispatcher, std::move(cb), trigger, events);
      return;
    }
    break;
  }
  case IoUringSocketType::Client:
  case IoUringSocketType::Unknown:
    ENVOY_LOG(trace, "initialize file event for client socket, fd = {}", fd_);
    io_uring_socket_type_ = IoUringSocketType::Client;
    if (!enable_client_socket_) {
      ENVOY_LOG(trace, "fallback to IoSocketHandle for client socket");
      shadow_io_handle_ = std::make_unique<IoSocketHandleImpl>(fd_, socket_v6only_, domain_);
      shadow_io_handle_->setBlocking(false);
      shadow_io_handle_->initializeFileEvent(dispatcher, std::move(cb), trigger, events);
      return;
    }
  }
}

void IoUringSocketHandleImpl::activateFileEvents(uint32_t events) {
  ASSERT(io_uring_socket_type_ != IoUringSocketType::Unknown);
  ENVOY_LOG(trace, "activate file events {}, fd = {}, io_uring_socket_type = {}", events, fd_,
            ioUringSocketTypeStr());

  if ((io_uring_socket_type_ == IoUringSocketType::Client && !enable_client_socket_) ||
      (io_uring_socket_type_ == IoUringSocketType::Server && !enable_server_socket_) ||
      (io_uring_socket_type_ == IoUringSocketType::Accept && !enable_accept_socket_)) {
    shadow_io_handle_->activateFileEvents(events);
    return;
  }
}

void IoUringSocketHandleImpl::enableFileEvents(uint32_t events) {
  ENVOY_LOG(trace, "enable file events {}, fd = {}, io_uring_socket_type = {}", events, fd_,
            ioUringSocketTypeStr());
  ASSERT(io_uring_socket_type_ != IoUringSocketType::Unknown);

  if ((io_uring_socket_type_ == IoUringSocketType::Client && !enable_client_socket_) ||
      (io_uring_socket_type_ == IoUringSocketType::Server && !enable_server_socket_) ||
      (io_uring_socket_type_ == IoUringSocketType::Accept && !enable_accept_socket_)) {
    shadow_io_handle_->enableFileEvents(events);
    return;
  }
}

void IoUringSocketHandleImpl::resetFileEvents() {
  ASSERT(io_uring_socket_type_ != IoUringSocketType::Unknown);
  ENVOY_LOG(trace, "reset file, fd = {}, io_uring_socket_type = {}", fd_, ioUringSocketTypeStr());

  if ((io_uring_socket_type_ == IoUringSocketType::Client && !enable_client_socket_) ||
      (io_uring_socket_type_ == IoUringSocketType::Server && !enable_server_socket_) ||
      (io_uring_socket_type_ == IoUringSocketType::Accept && !enable_accept_socket_)) {
    shadow_io_handle_->resetFileEvents();
    return;
  }
}

IoHandlePtr IoUringSocketHandleImpl::duplicate() {
  ENVOY_LOG(trace, "duplicate, fd = {},io_uring_socket_type = {}", fd_, ioUringSocketTypeStr());
  auto result = Api::OsSysCallsSingleton::get().duplicate(fd_);
  RELEASE_ASSERT(result.return_value_ != -1,
                 fmt::format("duplicate failed for '{}': ({}) {}", fd_, result.errno_,
                             errorDetails(result.errno_)));
  return SocketInterfaceImpl::makePlatformSpecificSocket(result.return_value_, socket_v6only_,
                                                         domain_, &io_uring_factory_);
}

void IoUringSocketHandleImpl::onAcceptSocket(Io::AcceptedSocketParam&) {}

void IoUringSocketHandleImpl::onRead(Io::ReadParam&) {}

void IoUringSocketHandleImpl::onWrite(Io::WriteParam&) {}

} // namespace Network
} // namespace Envoy
