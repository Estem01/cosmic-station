#pragma once

#include <sys/mman.h>
#include <common/types.h>
namespace cosmic::os {
    template<typename T>
    struct MappedMemory {
        MappedMemory() = default;
        explicit MappedMemory<T>(T* address, u64 sz, bool unmap = false) :
            blockSize(sz),
            unmapChunk(unmap),
            managedBlock(address) {
        }
        MappedMemory(MappedMemory&) = delete;
        auto operator=(MappedMemory&& mapped) {
            blockSize = mapped.blockSize;
            unmapChunk = mapped.unmapChunk;
            managedBlock = mapped.managedBlock;

            mapped.unmapChunk = {};
        }
        MappedMemory<T>(u64 mSize) :
            blockSize(mSize * sizeof(T)),
            managedBlock(reinterpret_cast<T*>(mmap(nullptr, blockSize,
                PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0))) {
            madvise(reinterpret_cast<void*>(managedBlock), blockSize, MADV_DONTDUMP);
#if !defined(NDEBUG)
            enableDump();
#endif
        }

        ~MappedMemory() {
            if (unmapChunk)
                munmap(reinterpret_cast<T*>(managedBlock), blockSize);
        }
        T& operator[](u32 address) {
            return managedBlock[address];
        }
        auto operator*() {
            return managedBlock;
        }
        auto getBlockSize() {
            return blockSize;
        }
        void enableDump() {
            madvise(reinterpret_cast<void*>(managedBlock), blockSize, MADV_DODUMP);
        }
        operator bool() const {
            return managedBlock != nullptr;
        }
    private:
        u64 blockSize{};
        bool unmapChunk{true};
        T* managedBlock{};
    };
}
