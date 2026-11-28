#pragma once

#include <pthread.h>
#include <cstddef>

// Only for Linux!

namespace StackTracker {
    size_t GetStackUsage();
}
