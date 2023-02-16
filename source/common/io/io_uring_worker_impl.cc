#include "source/common/io/io_uring_worker_impl.h"

#include <sys/socket.h>

#include "io_uring_impl.h"

namespace Envoy {
namespace Io {

IoUringSocketEntry::IoUringSocketEntry(os_fd_t fd, IoUringWorkerImpl& parent)
    : fd_(fd), parent_(parent) {}

void IoUringSocketEntry::cleanup() {
  parent_.removeInjectedCompletion(*this);
  IoUringSocketEntryPtr socket = parent_.removeSocket(*this);
  parent_.dispatcher().deferredDelete(std::move(socket));
}

void IoUringSocketEntry::injectCompletion(uint32_t type) {
  // Avoid injecting the same completion type multiple times.
  if (injected_completions_ & type) {
    ENVOY_LOG(trace,
              "ignore injected completion since there already has one, injected_completions_: {}, "
              "type: {}",
              injected_completions_, type);
    return;
  }
  injected_completions_ |= type;
  parent_.injectCompletion(*this, type, -EAGAIN);
}

IoUringWorkerImpl::IoUringWorkerImpl(uint32_t io_uring_size, bool use_submission_queue_polling,
                                     Event::Dispatcher& dispatcher)
    : IoUringWorkerImpl(std::make_unique<IoUringImpl>(io_uring_size, use_submission_queue_polling),
                        dispatcher) {}

IoUringWorkerImpl::IoUringWorkerImpl(std::unique_ptr<IoUring> io_uring_instance,
                                     Event::Dispatcher& dispatcher)
    : io_uring_instance_(std::move(io_uring_instance)), dispatcher_(dispatcher) {
  const os_fd_t event_fd = io_uring_instance_->registerEventfd();
  // We only care about the read event of Eventfd, since we only receive the
  // event here.
  file_event_ = dispatcher_.createFileEvent(
      event_fd, [this](uint32_t) { onFileEvent(); }, Event::PlatformDefaultTriggerType,
      Event::FileReadyType::Read);
}

IoUringWorkerImpl::~IoUringWorkerImpl() {
  ENVOY_LOG(trace, "destruct io uring worker");
  dispatcher_.clearDeferredDeleteList();
}

IoUringSocket& IoUringWorkerImpl::addAcceptSocket(os_fd_t fd, IoUringHandler&) {
  ENVOY_LOG(trace, "add accept socket, fd = {}", fd);
  PANIC("not implemented");
}

IoUringSocket& IoUringWorkerImpl::addServerSocket(os_fd_t fd, IoUringHandler&, uint32_t) {
  ENVOY_LOG(trace, "add server socket, fd = {}", fd);
  PANIC("not implemented");
}

IoUringSocket& IoUringWorkerImpl::addClientSocket(os_fd_t fd, IoUringHandler&, uint32_t) {
  ENVOY_LOG(trace, "add client socket, fd = {}", fd);
  PANIC("not implemented");
}

Event::Dispatcher& IoUringWorkerImpl::dispatcher() { return dispatcher_; }

Request* IoUringWorkerImpl::submitAcceptRequest(IoUringSocket& socket,
                                                sockaddr_storage* remote_addr,
                                                socklen_t* remote_addr_len) {
  Request* req = new Request{RequestType::Accept, socket};

  ENVOY_LOG(trace, "submit accept request, fd = {}, accept req = {}", socket.fd(), fmt::ptr(req));

  *remote_addr_len = sizeof(sockaddr_storage);
  auto res = io_uring_instance_->prepareAccept(
      socket.fd(), reinterpret_cast<struct sockaddr*>(remote_addr), remote_addr_len, req);
  if (res == Io::IoUringResult::Failed) {
    // TODO(rojkov): handle `EBUSY` in case the completion queue is never reaped.
    submit();
    res = io_uring_instance_->prepareAccept(
        socket.fd(), reinterpret_cast<struct sockaddr*>(remote_addr), remote_addr_len, req);
    RELEASE_ASSERT(res == Io::IoUringResult::Ok, "unable to prepare accept");
  }
  submit();
  return req;
}

Request* IoUringWorkerImpl::submitCancelRequest(IoUringSocket& socket, Request* request_to_cancel) {
  Request* req = new Request{RequestType::Cancel, socket};

  ENVOY_LOG(trace, "submit cancel request, fd = {}, cancel req = {}", socket.fd(), fmt::ptr(req));

  auto res = io_uring_instance_->prepareCancel(request_to_cancel, req);
  if (res == Io::IoUringResult::Failed) {
    // TODO(rojkov): handle `EBUSY` in case the completion queue is never reaped.
    submit();
    res = io_uring_instance_->prepareCancel(request_to_cancel, req);
    RELEASE_ASSERT(res == Io::IoUringResult::Ok, "unable to prepare cancel");
  }
  submit();
  return req;
}

Request* IoUringWorkerImpl::submitCloseRequest(IoUringSocket& socket) {
  Request* req = new Request{RequestType::Close, socket};

  ENVOY_LOG(trace, "submit close request, fd = {}, close req = {}", socket.fd(), fmt::ptr(req));

  auto res = io_uring_instance_->prepareClose(socket.fd(), req);
  if (res == Io::IoUringResult::Failed) {
    // TODO(rojkov): handle `EBUSY` in case the completion queue is never reaped.
    submit();
    res = io_uring_instance_->prepareClose(socket.fd(), req);
    RELEASE_ASSERT(res == Io::IoUringResult::Ok, "unable to prepare close");
  }
  submit();
  return req;
}

Request* IoUringWorkerImpl::submitReadRequest(IoUringSocket& socket, struct iovec* iov) {
  Request* req = new Request{RequestType::Read, socket};

  ENVOY_LOG(trace, "submit read request, fd = {}, read req = {}", socket.fd(), fmt::ptr(req));

  auto res = io_uring_instance_->prepareReadv(socket.fd(), iov, 1, 0, req);
  if (res == Io::IoUringResult::Failed) {
    // TODO(rojkov): handle `EBUSY` in case the completion queue is never reaped.
    submit();
    res = io_uring_instance_->prepareReadv(socket.fd(), iov, 1, 0, req);
    RELEASE_ASSERT(res == Io::IoUringResult::Ok, "unable to prepare readv");
  }
  submit();
  return req;
}

Request* IoUringWorkerImpl::submitWritevRequest(IoUringSocket& socket, struct iovec* iovecs,
                                                uint64_t num_vecs) {
  Request* req = new Request{RequestType::Write, socket};

  ENVOY_LOG(trace, "submit write request, fd = {}, req = {}", socket.fd(), fmt::ptr(req));

  auto res = io_uring_instance_->prepareWritev(socket.fd(), iovecs, num_vecs, 0, req);
  if (res == Io::IoUringResult::Failed) {
    // TODO(rojkov): handle `EBUSY` in case the completion queue is never reaped.
    submit();
    res = io_uring_instance_->prepareWritev(socket.fd(), iovecs, num_vecs, 0, req);
    RELEASE_ASSERT(res == Io::IoUringResult::Ok, "unable to prepare writev");
  }
  submit();
  return req;
}

Request*
IoUringWorkerImpl::submitConnectRequest(IoUringSocket& socket,
                                        const Network::Address::InstanceConstSharedPtr& address) {
  Request* req = new Request{RequestType::Connect, socket};

  ENVOY_LOG(trace, "submit connect request, fd = {}, req = {}", socket.fd(), fmt::ptr(req));

  auto res = io_uring_instance_->prepareConnect(socket.fd(), address, req);
  if (res == Io::IoUringResult::Failed) {
    // TODO(rojkov): handle `EBUSY` in case the completion queue is never reaped.
    submit();
    res = io_uring_instance_->prepareConnect(socket.fd(), address, req);
    RELEASE_ASSERT(res == Io::IoUringResult::Ok, "unable to prepare writev");
  }
  submit();
  return req;
}

void IoUringWorkerImpl::onFileEvent() {
  ENVOY_LOG(trace, "io uring worker, on file event");
  delay_submit_ = true;
  io_uring_instance_->forEveryCompletion([](void* user_data, int32_t result, bool injected) {
    auto req = static_cast<Io::Request*>(user_data);

    ENVOY_LOG(debug, "receive request completion, result = {}, req = {}", result, fmt::ptr(req));

    switch (req->type_) {
    case RequestType::Accept:
      ENVOY_LOG(trace, "receive accept request completion, fd = {}", req->io_uring_socket_.fd());
      req->io_uring_socket_.onAccept(result, injected);
      break;
    case RequestType::Connect:
      ENVOY_LOG(trace, "receive connect request completion, fd = {}", req->io_uring_socket_.fd());
      req->io_uring_socket_.onConnect(result, injected);
      break;
    case RequestType::Read:
      ENVOY_LOG(trace, "receive Read request completion, fd = {}", req->io_uring_socket_.fd());
      req->io_uring_socket_.onRead(result, injected);
      break;
    case RequestType::Write:
      ENVOY_LOG(trace, "receive write request completion, fd = {}", req->io_uring_socket_.fd());
      req->io_uring_socket_.onWrite(result, injected);
      break;
    case RequestType::Close:
      ENVOY_LOG(trace, "receive close request completion, fd = {}", req->io_uring_socket_.fd());
      req->io_uring_socket_.onClose(result, injected);
      break;
    case RequestType::Cancel:
      ENVOY_LOG(trace, "receive cancel request completion, fd = {}", req->io_uring_socket_.fd());
      req->io_uring_socket_.onCancel(result, injected);
      break;
    }

    delete req;
  });
  delay_submit_ = false;
  submit();
}

void IoUringWorkerImpl::submit() {
  if (!delay_submit_) {
    io_uring_instance_->submit();
  }
}

IoUringSocketEntryPtr IoUringWorkerImpl::removeSocket(IoUringSocketEntry& socket) {
  return socket.removeFromList(sockets_);
}

void IoUringWorkerImpl::injectCompletion(IoUringSocket& socket, uint32_t type, int32_t result) {
  Request* req = new Request{type, socket};
  io_uring_instance_->injectCompletion(socket.fd(), req, result);
  file_event_->activate(Event::FileReadyType::Read);
}

void IoUringWorkerImpl::removeInjectedCompletion(IoUringSocket& socket) {
  io_uring_instance_->removeInjectedCompletion(socket.fd());
}

} // namespace Io
} // namespace Envoy
