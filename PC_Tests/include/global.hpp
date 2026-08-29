#pragma once
#include <cstdlib>
#include <time.h>
#include <stdint.h>

uint32_t millis(void);

// Allocator Modes
enum AllocationTarget {
    ALLOC_INTERNAL, // Standard DRAM / Internal Heap
    ALLOC_PSRAM     // External PSRAM
};

// Global allocation state
static AllocationTarget g_alloc_target = ALLOC_INTERNAL;

// 1. Target Selector
void setAllocTarget(AllocationTarget target);

void* calcemistMalloc(size_t size);

// 3. Custom Free Function
void calcemistFree(void* ptr);