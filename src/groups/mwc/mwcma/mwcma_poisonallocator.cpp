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

#include <bsls_ident.h>
BSLS_IDENT_RCSID(mwcma_poisonallocator_h, "$Id$ $CSID$")

#include <mwcma_poisonallocator.h>

#include <ball_log.h>
#include <bsls_assert.h>

namespace {

const char LOG_CATEGORY[] = "MWCMA.POISONALLOCATOR";

}  // close unnamed namespace

BALL_LOG_SET_NAMESPACE_CATEGORY(LOG_CATEGORY);

namespace BloombergLP {
namespace mwcma {

PoisonAllocator::PoisonAllocator()
: d_allocator()
{
}

PoisonAllocator::PoisonAllocator(const allocator_type& alloc)
: d_allocator(alloc)
{
}

void* PoisonAllocator::do_allocate(size_type bytes, size_type alignment)
{
    // For every allocation, we want to add an additional reference count which
    // will be kept one sizeof(int) behind the return storage.
    // ----------------------------------------
    // | 0x00 | 0x00 | 0x00 | 0x00 | bytes... |
    // ----------------------------------------
    //                              ^ return value

    void* storage = d_allocator.resource().allocate(sizeof(int) + bytes,
                                                    alignment);
    return static_cast<int*>(storage) + 1;
}

void PoisonAllocator::do_deallocate(void*     address,
                                    size_type bytes,
                                    size_type alignment)
{
    // On deallocation, the reference count may or may not actually be zero. If
    // not, we want to quarantine this storage.
    atomic<int>& rc = static_cast<int*>(address) - 1;

    if (rc > 1) {
        // Something is still referencing us
        --rc;
        return;
    }

    // Poisoning
    bsl::memset(address, 0xcf, bytes);

    d_allocator.resource().deallocate(static_cast<int*>(address) - 1,
                                      sizeof(int) + bytes,
                                      alignment);
}

}  // close namespace mwcma
}  // close namespace BloombergLP
