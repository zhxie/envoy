#pragma once

#include <cstddef>

#include "envoy/common/pure.h"

namespace Envoy {
namespace Buffer {

/**
 * Interface for memory operation.
 */
class MemoryInterface {
public:
  virtual ~MemoryInterface() = default;

  /**
   * Copies bytes from source to destination.
   * @param dest pointer to the memory location to copy to
   * @param src pointer to the memory location to copy from
   * @param n number of bytes to copy
   */
  virtual void memoryCopy(void* dest, const void* src, size_t n) const PURE;
};

} // namespace Buffer
} // namespace Envoy
