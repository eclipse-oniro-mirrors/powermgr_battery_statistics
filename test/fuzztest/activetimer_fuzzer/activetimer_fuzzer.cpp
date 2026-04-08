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

#include "activetimer_fuzzer.h"
#include "stats_helper.h"

using namespace OHOS::PowerMgr;

namespace OHOS {
namespace {
const int32_t MIN_DATA_LEN = 8;
} // namespace

// Test ActiveTimer::StartRunning
void ActiveTimerStartRunningFuzzTest(const uint8_t *data, size_t size)
{
    StatsHelper::ActiveTimer timer;
    timer.StartRunning();
    timer.StartRunning();
    timer.StopRunning();
}

// Test ActiveTimer::StopRunning
void ActiveTimerStopRunningFuzzTest(const uint8_t *data, size_t size)
{
    StatsHelper::ActiveTimer timer;
    timer.StopRunning();
    timer.StartRunning();
    timer.StopRunning();
    timer.StopRunning();
}

// Test ActiveTimer::GetRunningTimeMs
void ActiveTimerGetRunningTimeMsFuzzTest(const uint8_t *data, size_t size)
{
    StatsHelper::ActiveTimer timer;
    timer.GetRunningTimeMs();
    timer.StartRunning();
    timer.GetRunningTimeMs();
    timer.StopRunning();
    timer.GetRunningTimeMs();
}

// Test ActiveTimer::AddRunningTimeMs
void ActiveTimerAddRunningTimeMsFuzzTest(const uint8_t *data, size_t size)
{
    StatsHelper::ActiveTimer timer;
    int64_t activeTime = *reinterpret_cast<const int64_t *>(data);
    timer.AddRunningTimeMs(activeTime);
    timer.GetRunningTimeMs();
}

// Test ActiveTimer::Reset
void ActiveTimerResetFuzzTest(const uint8_t *data, size_t size)
{
    StatsHelper::ActiveTimer timer;
    int64_t activeTime = *reinterpret_cast<const int64_t *>(data);
    timer.AddRunningTimeMs(activeTime);
    timer.StartRunning();
    timer.Reset();
    timer.GetRunningTimeMs();
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < OHOS::MIN_DATA_LEN) {
        return 0;
    }
    OHOS::ActiveTimerStartRunningFuzzTest(data, size);
    OHOS::ActiveTimerStopRunningFuzzTest(data, size);
    OHOS::ActiveTimerGetRunningTimeMsFuzzTest(data, size);
    OHOS::ActiveTimerAddRunningTimeMsFuzzTest(data, size);
    OHOS::ActiveTimerResetFuzzTest(data, size);
    return 0;
}

