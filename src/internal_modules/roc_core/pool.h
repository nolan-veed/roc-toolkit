/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/pool.h
//! @brief Memory pool.

#ifndef ROC_CORE_POOL_H_
#define ROC_CORE_POOL_H_

#include "roc_core/aligned_storage.h"
#include "roc_core/attributes.h"
#include "roc_core/iarena.h"
#include "roc_core/ipool.h"
#include "roc_core/noncopyable.h"
#include "roc_core/pool_impl.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Memory pool flags.
enum PoolFlags {
    //! Panic when buffer overflow and invalid ownership is detected.
    PoolFlag_EnableGuards = (1 << 0),
};

//! Default memory pool flags.
enum { DefaultPoolFlags = (PoolFlag_EnableGuards) };

//! Memory pool.
//!
//! Implements slab allocator algorithm. Allocates large chunks of memory ("slabs") from
//! given arena, and uses them for multiple smaller fixed-sized objects ("slots").
//!
//! Keeps track of free slots and uses them when possible. Automatically allocates new
//! slabs when there are no free slots available.
//!
//! Automatically grows size of new slabs exponentially. The user can also specify the
//! minimum and maximum limits for the slabs.
//!
//! The returned memory is always maximum-aligned.
//!
//! Supports memory "poisoning" to make memory-related bugs (out of bound writes, use
//! after free, etc) more noticeable.
//!
//! @tparam T defines pool object type. It is used to determine allocation size. If
//! runtime size is different from static size of T, it can be provided via constructor.
//!
//! @tparam EmbeddedCapacity defines number of slots embedded directly into Pool
//! instance. If non-zero, this memory will be used for first allocations, before
//! using memory arena.
//!
//! Thread-safe.
template <class T, size_t EmbeddedCapacity = 0>
class Pool : public IPool, public NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p name defines pool name, used for logging
    //!  - @p arena is used to allocate slabs
    //!  - @p object_size defines size of single object in bytes
    //!  - @p min_alloc_bytes defines minimum size in bytes per request to arena
    //!  - @p max_alloc_bytes defines maximum size in bytes per request to arena
    //!  - @p flags defines options to modify behaviour as indicated in PoolFlags
    explicit Pool(const char* name,
                  IArena& arena,
                  size_t object_size = sizeof(T),
                  size_t min_alloc_bytes = 0,
                  size_t max_alloc_bytes = 0,
                  size_t flags = DefaultPoolFlags)
        : impl_(name,
                arena,
                object_size,
                min_alloc_bytes,
                max_alloc_bytes,
                embedded_data_.memory(),
                embedded_data_.size(),
                flags) {
    }

    //! Get size of objects in pool.
    size_t object_size() const {
        return impl_.object_size();
    }

    //! Reserve memory for given number of objects.
    ROC_ATTR_NODISCARD bool reserve(size_t n_objects) {
        return impl_.reserve(n_objects);
    }

    //! Allocate memory for an object.
    void* allocate() {
        return impl_.allocate();
    }

    //! Return memory to pool.
    void deallocate(void* memory) {
        impl_.deallocate(memory);
    }

    //! Get number of buffer overflows detected.
    size_t num_buffer_overflows() const {
        return impl_.num_buffer_overflows();
    }

    //! Get number of invalid ownerships detected.
    size_t num_invalid_ownerships() const {
        return impl_.num_invalid_ownerships();
    }

private:
    enum {
        SlotSize = (sizeof(PoolImpl::SlotHeader) + sizeof(PoolImpl::SlotCanary)
                    + sizeof(T) + sizeof(PoolImpl::SlotCanary) + sizeof(AlignMax) - 1)
            / sizeof(AlignMax) * sizeof(AlignMax)
    };
    AlignedStorage<EmbeddedCapacity * SlotSize> embedded_data_;
    PoolImpl impl_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_POOL_H_
