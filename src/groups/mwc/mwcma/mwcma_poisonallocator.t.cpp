// Copyright 2023 Bloomberg Finance L.P.
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

#include <bslma_allocator.h>
#include <bslma_default.h>
#include <bslma_defaultallocatorguard.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_assert.h>
#include <mwcma_poisonallocator.h>

#include <mwctst_testhelper.h>

using namespace BloombergLP;

class PoisonAllocatorTest : protected mwctst::Test {
    BSLMF_ASSERT((bsl::uses_allocator<mwcma::PoisonAllocator,
                                      bsl::allocator<unsigned char> >::value));
    BSLMF_ASSERT(bslma::UsesBslmaAllocator<mwcma::PoisonAllocator>::value);

  public:
    PoisonAllocatorTest() {}
};

TEST_F(PoisonAllocatorTest, BreathingTest)
{
    mwcma::PoisonAllocator alloc;

    bslma::Allocator*      global = bslma::Default::defaultAllocator();
    mwcma::PoisonAllocator alloc2(global);

    bslma::DefaultAllocatorGuard guard(&alloc2);
}

TEST_F(PoisonAllocatorTest, Allocate)
{
    mwcma::PoisonAllocator alloc;
    void*                  mem = alloc.allocate(0);
    ASSERT(NULL == mem);

    char* buffer = static_cast<char*>(alloc.allocate(256));
    ASSERT(NULL != buffer);

    alloc.deallocate(buffer);
}

int main(int argc, char* argv[])
{
    TEST_PROLOG(mwctst::TestHelper::e_DEFAULT);

    mwctst::runTest(_testCase);

    TEST_EPILOG(mwctst::TestHelper::e_CHECK_DEF_GBL_ALLOC);
}
