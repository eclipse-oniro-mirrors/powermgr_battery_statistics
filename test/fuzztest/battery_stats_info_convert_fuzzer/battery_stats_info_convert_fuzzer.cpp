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

#include "battery_stats_info.h"
#include <cstddef>
#include <cstdint>

using namespace OHOS::PowerMgr;

namespace {
// ConsumptionType enum values range from -17 (INVALID) to -1 (ALARM)
constexpr int32_t CONSUMPTION_TYPE_MIN = -17;
constexpr int32_t CONSUMPTION_TYPE_MAX = -1;
constexpr size_t CONSUMPTION_TYPE_COUNT = 17; // Total number of enum values

BatteryStatsInfo::ConsumptionType ToConsumptionType(uint8_t raw)
{
    int32_t index = static_cast<int32_t>(raw) % static_cast<uint8_t>(CONSUMPTION_TYPE_COUNT);
    return static_cast<BatteryStatsInfo::ConsumptionType>(CONSUMPTION_TYPE_MIN + index);
}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }

    // Test ConvertConsumptionType with fuzz-driven enum values
    for (size_t i = 0; i < size && i < CONSUMPTION_TYPE_COUNT; i++) {
        BatteryStatsInfo::ConsumptionType type = ToConsumptionType(data[i]);
        std::string result = BatteryStatsInfo::ConvertConsumptionType(type);
        (void)result;
    }

    // Exhaustively test all valid enum values for full coverage
    for (int32_t i = CONSUMPTION_TYPE_MIN; i <= CONSUMPTION_TYPE_MAX; i++) {
        BatteryStatsInfo::ConsumptionType type =
            static_cast<BatteryStatsInfo::ConsumptionType>(i);
        std::string result = BatteryStatsInfo::ConvertConsumptionType(type);
        (void)result;
    }

    return 0;
}
