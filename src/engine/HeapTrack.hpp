#pragma once

#include <cstddef>
#include <cstdlib>
#include <mutex>

namespace HeapTrack {
    static inline size_t cur  = 0;
    static inline size_t peak = 0;
    static inline std::mutex mutex;
}
