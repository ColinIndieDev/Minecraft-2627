#include "HeapTrack.hpp"

#include <new>

void* operator new(std::size_t size) {
    if (void* ptr = std::malloc(size)) {
        std::lock_guard<std::mutex> lock(HeapTrack::mutex);
        HeapTrack::cur += size;
        if (HeapTrack::cur > HeapTrack::peak) HeapTrack::peak = HeapTrack::cur;
        return ptr;
    }
    throw std::bad_alloc();
}

void operator delete(void* ptr, std::size_t size) noexcept {
    if (ptr) {
        std::lock_guard<std::mutex> lock(HeapTrack::mutex);
        HeapTrack::cur -= size;
        std::free(ptr);
    }
}

void operator delete(void* ptr) noexcept {
    if (ptr) {
        std::free(ptr);
    }
}
