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

#include "counter_fuzzer.h"
#include "stats_helper.h"

using namespace OHOS::PowerMgr;

namespace OHOS {
namespace {
const int32_t MIN_DATA_LEN = 8;
} // namespace

// Test Counter::GetCount
void CounterGetCountFuzzTest(const uint8_t *data, size_t size)
{
    StatsHelper::Counter counter;
    int64_t count = static_cast<int64_t>(*reinterpret_cast<const int32_t *>(data));
    counter.AddCount(count);
    counter.GetCount();
}

// Test Counter::AddCount
void CounterAddCountFuzzTest(const uint8_t *data, size_t size)
{
    StatsHelper::Counter counter;
    int64_t count = *reinterpret_cast<const int64_t *>(data);
    counter.AddCount(count);
    counter.AddCount(0);
    counter.AddCount(-1);
    counter.GetCount();
}

// Test Counter::Reset
void CounterResetFuzzTest(const uint8_t *data, size_t size)
{
    StatsHelper::Counter counter;
    int64_t count = static_cast<int64_t>(*reinterpret_cast<const int32_t *>(data));
    counter.AddCount(count);
    counter.Reset();
    counter.GetCount();
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < OHOS::MIN_DATA_LEN) {
        return 0;
    }
    OHOS::CounterGetCountFuzzTest(data, size);
    OHOS::CounterAddCountFuzzTest(data, size);
    OHOS::CounterResetFuzzTest(data, size);
    return 0;
}

