/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "getcount_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <securec.h>

#include "stats_helper.h"

using namespace OHOS::PowerMgr;

namespace {
constexpr size_t MAX_COUNT_SIZE = sizeof(int64_t);
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Create Counter instance */
    StatsHelper::Counter counter;
    
    /* Set on battery state based on first byte of fuzz data */
    if (size > 0) {
        bool onBattery = (data[0] % 2 == 0);
        StatsHelper::SetOnBattery(onBattery);
    }
    
    /* Add counts based on fuzz data */
    if (size >= MAX_COUNT_SIZE + 1) {
        int64_t count = 0;
        /* Use memcpy_s instead of std::copy for safer memory operation */
        memcpy_s(reinterpret_cast<uint8_t*>(&count), sizeof(count), data + 1, MAX_COUNT_SIZE);
        /* Ensure count is positive to avoid validation failure */
        if (count > 0) {
            counter.AddCount(count);
        }
    }
    
    /* Test GetCount method */
    int64_t result = counter.GetCount();
    
    /* Reset counter and test again */
    counter.Reset();
    result = counter.GetCount();
    
    return 0;
}
