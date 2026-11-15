#pragma once

#include <cstddef>
#include <iostream>
#include <cstdlib>
#include <mutex>

namespace HeapTrack {
    static inline size_t cur  = 0;
    static inline size_t peak = 0;
    static inline std::mutex mutex;
}

void* operator new(std::size_t size) {
    void* p = std::malloc(size);
    if (!p) throw std::bad_alloc();
    std::lock_guard<std::mutex> lock(HeapTrack::mutex);
    HeapTrack::cur += size;
    if (HeapTrack::cur > HeapTrack::peak) HeapTrack::peak = HeapTrack::cur;
    return p;
}

void operator delete(void* ptr, std::size_t size) noexcept {
    if (ptr) {
        std::lock_guard<std::mutex> lock(HeapTrack::mutex);
        HeapTrack::cur -= size;
        std::free(ptr);
    }
}
