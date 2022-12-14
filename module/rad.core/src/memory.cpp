#include <rad/core/memory.h>

#include <cstdlib>
#if defined(_MSC_VER)
#include <malloc.h>
#endif

namespace Rad {

void* AlignedMalloc(std::size_t alignment, std::size_t size) {
  //sb ms
#if defined(_MSC_VER)
  return _aligned_malloc(size, alignment);
#else
  return std::aligned_alloc(alignment, size);
#endif
}

void AlignedFree(void* ptr) {
#if defined(_MSC_VER)
  _aligned_free(ptr);
#else
  std::free(ptr);
#endif
}

}  // namespace Rad
