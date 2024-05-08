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

#ifndef INCLUDED_MWCMA_RAWPTR
#define INCLUDED_MWCMA_RAWPTR

#include <bsls_ident.h>
BSLS_IDENT_RCSID(mwcma_rawptr_h, "$Id$ $CSID$")
BSLS_IDENT_PRAGMA_ONCE

///PURPOSE: A smart pointer type which collaborates with PoisonAllocator to prevent use-after-free bugs.
///
///CLASSES:
/// mwcma::RawPtr: 
///
///DESCRIPTION: 

namespace BloombergLP {
namespace mwcma {

/// RawPtr<T> is meant to replace T* in your code
template <typename T>
class RawPtr {
  public:
    // TYPEDEFS
    // CREATORS
    RawPtr();
    ~RawPtr();
    RawPtr(const RawPtr& other);
    RawPtr& operator=(const RawPtr& other);
    RawPtr(bslmf::MovableRef<RawPtr> other);
    RawPtr& operator=(bslmf::MovableRef<RawPtr> other);

    // MANIPULATORS
    T* operator->();
    const T* operator->() const;
    T& operator*();
    const T& operator*() const;    

    private:

};

// ============================================================================
//                          INLINE FUNCTION DEFINITIONS
// ============================================================================

RawPtr::~RawPtr() {}

RawPtr::RawPtr(const RawPtr& other)
: d_other(other)
{}

RawPtr& RawPtr::operator=(const RawPtr&)
: 
{}

RawPtr::RawPtr(bslmf::MovableRef<RawPtr>)
: 
{}

RawPtr& RawPtr::operator=(bslmf::MovableRef<RawPtr>)
: 
{}

}  // close namespace mwcma
}  // close namespace BloombergLP
#endif
