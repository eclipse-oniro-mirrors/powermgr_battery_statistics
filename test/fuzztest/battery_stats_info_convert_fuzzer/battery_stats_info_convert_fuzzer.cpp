/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define FUZZ_PROJECT_NAME "battery_stats_info_convert_fuzzer"

#include <cstddef>
#include <cstdint>

#include <fuzzer/FuzzedDataProvider.h>

#include "battery_stats_info.h"

using namespace OHOS::PowerMgr;

namespace {
// ConsumptionType enumerators range from CONSUMPTION_TYPE_INVALID (-17) to CONSUMPTION_TYPE_ALARM (-1)
constexpr int32_t CONSUMPTION_TYPE_MIN = static_cast<int32_t>(BatteryStatsInfo::CONSUMPTION_TYPE_INVALID);
constexpr int32_t CONSUMPTION_TYPE_MAX = static_cast<int32_t>(BatteryStatsInfo::CONSUMPTION_TYPE_ALARM);
constexpr size_t MAX_FUZZ_ROUNDS = 8;

void ConvertOneType(BatteryStatsInfo::ConsumptionType type)
{
    (void)BatteryStatsInfo::ConvertConsumptionType(type);
}

void FuzzConvertType(FuzzedDataProvider& fdp)
{
    size_t rounds = fdp.ConsumeIntegralInRange<size_t>(0, MAX_FUZZ_ROUNDS);
    for (size_t i = 0; i < rounds; i++) {
        ConvertOneType(static_cast<BatteryStatsInfo::ConsumptionType>(
            fdp.ConsumeIntegralInRange<int32_t>(CONSUMPTION_TYPE_MIN, CONSUMPTION_TYPE_MAX)));
    }
}

// Exhaustively cover every enumerator, then the out of range values the switch statement must reject
void CoverAllConsumptionTypes()
{
    for (int32_t i = CONSUMPTION_TYPE_MIN; i <= CONSUMPTION_TYPE_MAX; i++) {
        ConvertOneType(static_cast<BatteryStatsInfo::ConsumptionType>(i));
    }
    for (int32_t i = 0; i <= -CONSUMPTION_TYPE_MIN; i++) {
        ConvertOneType(static_cast<BatteryStatsInfo::ConsumptionType>(i));
    }
}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }

    FuzzedDataProvider fdp(data, size);
    FuzzConvertType(fdp);
    CoverAllConsumptionTypes();
    return 0;
}
