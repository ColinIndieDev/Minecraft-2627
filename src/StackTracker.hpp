#pragma once

#include <pthread.h>
#include <cstddef>
#include <cstdint>

// Only for Linux!

namespace StackTracker {
    inline size_t GetStackUsage() {
        pthread_attr_t attr;
        pthread_getattr_np(pthread_self(), &attr);

        void*   stackBase;
        size_t  stackSize;
        pthread_attr_getstack(&attr, &stackBase, &stackSize);
        pthread_attr_destroy(&attr);

        uintptr_t stackTop = (uintptr_t)stackBase + stackSize;

        char localVar;
        uintptr_t currentSp = (uintptr_t)&localVar;

        size_t usedBytes = (size_t)(stackTop - currentSp);

        return usedBytes;
    }
}
