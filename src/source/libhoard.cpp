/* -- C++ -- */

/*
  The Hoard Multiprocessor Memory Allocator
  www.hoard.org

  Author: Emery Berger, http://www.emeryberger.com
  Copyright (c) 1998-2020 Emery Berger

  See the LICENSE file at the top-level directory of this distribution
  and at http://github.com/emeryberger/Hoard.
*/

/*
 * @file   libhoard.cpp
 * @brief  This file replaces malloc etc. in your application.
 */

#include <cstddef>
#include <new>
#include <iostream>

#include "VERSION.h"
#include "heaplayers.h"
#include "hoardtlab.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#pragma inline_depth(255)
#define inline __forceinline
#pragma warning(disable: 4273 4098 4355 4074 6326) // Consolidate warning disables
#endif

volatile bool anyThreadCreated = false;  // Default to no thread having been created

/// Maintain a single instance of the main Hoard heap.
Hoard::HoardHeapType * getMainHoardHeap() {
    static double thBuf[sizeof(Hoard::HoardHeapType) / sizeof(double) + 1];
    static auto * th = new (thBuf) Hoard::HoardHeapType;
    return th;
}

TheCustomHeapType * getCustomHeap();

static char initBuffer[256 * 131072]; // 256 * 131072 = MAX_LOCAL_BUFFER_SIZE
static char * initBufferPtr = initBuffer;

extern bool isCustomHeapInitialized();

#include "Heap-Layers/wrappers/generic-memalign.cpp"

extern "C" {
#if defined(_GNUC_)
#define FLATTEN _attribute_((flatten))
#define ALLOC_SIZE _attribute_((alloc_size(1)))
#define MALLOC _attribute_((malloc))
#else
#define FLATTEN
#define ALLOC_SIZE
#define MALLOC
#endif

    void * xxmalloc(size_t sz) FLATTEN ALLOC_SIZE MALLOC {
        if (isCustomHeapInitialized()) {
            void * ptr = getCustomHeap()->malloc(sz);
            if (ptr == nullptr) {
                std::cerr << "INTERNAL FAILURE.\n";
                abort();
            }
            return ptr;
        }
        // Fallback to initial buffer if heap is not initialized
        void * ptr = initBufferPtr;
        initBufferPtr += sz;
        if (initBufferPtr > initBuffer + sizeof(initBuffer)) {
            abort();
        }
        return ptr;
    }

    void xxfree(void * ptr) {
        getCustomHeap()->free(ptr);
    }

    void * xxmemalign(size_t alignment, size_t sz) {
        return generic_xxmemalign(alignment, sz);
    }

    size_t xxmalloc_usable_size(void * ptr) {
        return getCustomHeap()->getSize(ptr);
    }

    void xxmalloc_lock() {}
    void xxmalloc_unlock() {}
}

#if defined(_linux) && !defined(MUSL_)
#include "Heap-Layers/wrappers/gnuwrapper.cpp"
#endif
