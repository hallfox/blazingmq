// Copyright 2017-2023 Bloomberg Finance L.P.
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef INCLUDED_MWCMA_POISONALLOCATOR
#define INCLUDED_MWCMA_POISONALLOCATOR

#include <bslma_memoryresource.h>
#include <bslma_stdallocator.h>
#include <bsls_ident.h>
BSLS_IDENT_RCSID(mwcma_poisonallocator_h, "$Id$ $CSID$")
BSLS_IDENT_PRAGMA_ONCE

#include <bslmf_movableref.h>
#include <bsls_keyword.h>

/// PURPOSE: This component implements an allocator which keeps a refcount of
/// storage allocated and poisons the underlying data on release.
///
/// CLASSES:
///  mwcma::PoisonAllocator:
///
/// DESCRIPTION:

namespace BloombergLP {
namespace mwcma {

///
class PoisonAllocator : public bsl::memory_resource {
  private:
  public:
    // TYPES
    typedef bsl::allocator<unsigned char> allocator_type;
    typedef bsl::size_t size_type;

    // CREATORS

    /// @brief Create a counting allocator with the specified `name` having an
    /// initial byte count of 0.  Optionally specify an `allocator` used to
    /// supply memory.  If `allocator` is 0, the currently installed default
    /// allocator is used.  If `allocator` is itself a counting allocator,
    /// the stat context is created as a child of the stat context of
    /// `allocator`; otherwise, no stat context is created.
    PoisonAllocator();
    explicit PoisonAllocator(const allocator_type& alloc);

    ~PoisonAllocator() BSLS_KEYWORD_OVERRIDE;

    // GETTERS
    allocator_type get_allocator() const;

  protected:
    // MANIPULATORS
    void* do_allocate(size_type bytes, size_type alignment) BSLS_KEYWORD_OVERRIDE;

    void do_deallocate(void* address, size_type bytes, size_type alignment) BSLS_KEYWORD_OVERRIDE;

  private:
    // Non copyable type
    PoisonAllocator(const PoisonAllocator&) BSLS_KEYWORD_DELETED;
    PoisonAllocator& operator=(const PoisonAllocator&) BSLS_KEYWORD_DELETED;
    PoisonAllocator(bslmf::MovableRef<PoisonAllocator>) BSLS_KEYWORD_DELETED;
    PoisonAllocator&
    operator=(bslmf::MovableRef<PoisonAllocator>) BSLS_KEYWORD_DELETED;

    allocator_type d_allocator;
};

// ============================================================================
//                          INLINE FUNCTION DEFINITIONS
// ============================================================================

inline PoisonAllocator::~PoisonAllocator()
{
}

inline PoisonAllocator::allocator_type PoisonAllocator::get_allocator() const
{
    return d_allocator;
}

}  // close namespace mwcma
}  // close namespace BloombergLP
#endif
