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

#include "statsutils_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <string>

#include "stats_utils.h"
#include "stats_hisysevent.h"
#include "stats_common.h"
#include "battery_stats_info.h"

using namespace OHOS::PowerMgr;

namespace OHOS {
namespace PowerMgr {
namespace {
const int32_t MIN_DATA_LEN = 4;
} // namespace

// Test StatsUtils::ParseStrtollResult
void StatsUtilsParseStrtollFuzzTest(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return;
    }

    std::string str(reinterpret_cast<const char*>(data), size);
    int64_t result = 0;

    StatsUtils::ParseStrtollResult(str, result);

    // Test edge cases
    std::string emptyStr = "";
    StatsUtils::ParseStrtollResult(emptyStr, result);

    std::string numStr = "12345";
    StatsUtils::ParseStrtollResult(numStr, result);

    std::string mixedStr = "123abc";
    StatsUtils::ParseStrtollResult(mixedStr, result);
}

// Test StatsHiSysEvent::CheckHiSysEvent
void StatsHiSysEventFuzzTest(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return;
    }

    std::string eventName(reinterpret_cast<const char*>(data), size);

    StatsHiSysEvent::CheckHiSysEvent(eventName);

    // Test with known event names
    StatsHiSysEvent::CheckHiSysEvent(StatsHiSysEvent::POWER_RUNNINGLOCK);
    StatsHiSysEvent::CheckHiSysEvent(StatsHiSysEvent::SCREEN_STATE);
    StatsHiSysEvent::CheckHiSysEvent(StatsHiSysEvent::BATTERY_CHANGED);
    StatsHiSysEvent::CheckHiSysEvent("");
}

// Test StatsToUnderlying
void StatsToUnderlyingFuzzTest(const uint8_t* data, size_t size)
{
    if (data == nullptr || size < MIN_DATA_LEN) {
        return;
    }

    int32_t typeValue = *reinterpret_cast<const int32_t*>(data);
    BatteryStatsInfo::ConsumptionType type = static_cast<BatteryStatsInfo::ConsumptionType>(typeValue);

    StatsToUnderlying(type);
    StatsToUnderlying(BatteryStatsInfo::CONSUMPTION_TYPE_INVALID);
    StatsToUnderlying(BatteryStatsInfo::CONSUMPTION_TYPE_APP);
    StatsToUnderlying(BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH);
    StatsToUnderlying(BatteryStatsInfo::CONSUMPTION_TYPE_IDLE);
    StatsToUnderlying(BatteryStatsInfo::CONSUMPTION_TYPE_PHONE);
    StatsToUnderlying(BatteryStatsInfo::CONSUMPTION_TYPE_SCREEN);
    StatsToUnderlying(BatteryStatsInfo::CONSUMPTION_TYPE_USER);
    StatsToUnderlying(BatteryStatsInfo::CONSUMPTION_TYPE_WIFI);
    StatsToUnderlying(BatteryStatsInfo::CONSUMPTION_TYPE_CAMERA);
    StatsToUnderlying(BatteryStatsInfo::CONSUMPTION_TYPE_FLASHLIGHT);
    StatsToUnderlying(BatteryStatsInfo::CONSUMPTION_TYPE_AUDIO);
    StatsToUnderlying(BatteryStatsInfo::CONSUMPTION_TYPE_SENSOR);
    StatsToUnderlying(BatteryStatsInfo::CONSUMPTION_TYPE_GNSS);
    StatsToUnderlying(BatteryStatsInfo::CONSUMPTION_TYPE_CPU);
    StatsToUnderlying(BatteryStatsInfo::CONSUMPTION_TYPE_WAKELOCK);
    StatsToUnderlying(BatteryStatsInfo::CONSUMPTION_TYPE_ALARM);
}
} // namespace PowerMgr
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    OHOS::PowerMgr::StatsUtilsParseStrtollFuzzTest(data, size);
    OHOS::PowerMgr::StatsHiSysEventFuzzTest(data, size);
    OHOS::PowerMgr::StatsToUnderlyingFuzzTest(data, size);
    return 0;
}

