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

#include "statshelpergetonbatteryuptimems_fuzzer.h"
#include "stats_helper.h"

using namespace OHOS::PowerMgr;

namespace OHOS {
namespace {
const int32_t MIN_DATA_LEN = 4;
} // namespace

void StatsHelperGetOnBatteryUpTimeMsFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < MIN_DATA_LEN) {
        return;
    }

    bool onBattery = (*data % 2) == 0;
    StatsHelper::SetOnBattery(onBattery);

    StatsHelper::GetOnBatteryUpTimeMs();

    StatsHelper::SetOnBattery(!onBattery);
    StatsHelper::GetOnBatteryUpTimeMs();
}

void StatsHelperSetScreenOffFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < MIN_DATA_LEN) {
        return;
    }

    // Test SetScreenOff with fuzz data
    bool screenOff = (*data % 2) == 0;
    StatsHelper::SetScreenOff(screenOff);

    // Test IsOnBatteryScreenOff
    StatsHelper::IsOnBatteryScreenOff();

    // Test with opposite value
    StatsHelper::SetScreenOff(!screenOff);
    StatsHelper::IsOnBatteryScreenOff();
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::StatsHelperGetOnBatteryUpTimeMsFuzzTest(data, size);
    OHOS::StatsHelperSetScreenOffFuzzTest(data, size);
    return 0;
}

