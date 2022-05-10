#pragma once

#include <cstddef>
#include <vector>

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
  virtual void memoryCopy(void* dest, const void* src, size_t n) PURE;

  /**
   * Copies bytes from source to destination in a batch.
   * @param dests vector of pointers to the memory location to copy to
   * @param src vector of pointers to the memory location to copy from
   * @param n vector of numbers of bytes to copy
   */
  virtual void batchMemoryCopy(const std::vector<void*>& dests,
                               const std::vector<const void*>& srcs,
                               const std::vector<size_t>& ns) PURE;
};

} // namespace Buffer
} // namespace Envoy
