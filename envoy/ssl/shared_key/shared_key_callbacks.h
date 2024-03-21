#pragma once

#include <functional>
#include <string>

#include "envoy/common/pure.h"

namespace Envoy {
namespace Ssl {

class SharedKeyConnectionCallbacks {
public:
  virtual ~SharedKeyConnectionCallbacks() = default;

  /**
   * Callback function which is called when the asynchronous shared key
   * operation has been completed (with either success or failure). The
   * provider will communicate the success status when SSL_do_handshake()
   * is called the next time.
   */
  virtual void onSharedKeyMethodComplete() PURE;
};

} // namespace Ssl
} // namespace Envoy
