#pragma once

#include <cstddef>

#include "envoy/common/pure.h"

namespace Envoy {
namespace Buffer {

class MemoryInterface {
public:
  virtual ~MemoryInterface() = default;

  virtual void memoryCopy(void* dst, const void* src, size_t n) const PURE;
};

} // namespace Buffer
} // namespace Envoy
