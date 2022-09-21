#include <cstddef>

namespace Rad::Memory {

void* AlignedAlloc(std::size_t alignment, std::size_t size);
void AlignedFree(void* ptr);

}  // namespace Rad::Memory
