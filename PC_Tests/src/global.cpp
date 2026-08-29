#include "global.hpp"
#include <cstdlib>
#include <time.h>
#include <stdint.h>

// 1. Target Selector
void setAllocTarget(AllocationTarget target) {
    g_alloc_target = target;
}

void* calcemistMalloc(size_t size) {
#if defined(ESP_PLATFORM)
    if (g_alloc_target == ALLOC_PSRAM) {
        // Fall back to internal RAM if PSRAM allocation fails
        void* ptr = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (ptr) return ptr;
    }
    // Default to internal heap
    return heap_caps_malloc(size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
#else
    // Fallback for non-ESP32 host environments (e.g., PC unit testing)
    return std::malloc(size);
#endif
}

// 3. Custom Free Function
void calcemistFree(void* ptr) {
    if (!ptr) return;
    // std::free / heap_caps_free handle both internal and PSRAM pointers automatically
    std::free(ptr); 
}



uint32_t millis(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}