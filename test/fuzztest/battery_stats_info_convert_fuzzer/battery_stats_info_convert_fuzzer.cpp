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
#include <string>

#include "battery_stats_info.h"

using namespace OHOS::PowerMgr;

namespace {
// ConsumptionType enumerators range from CONSUMPTION_TYPE_INVALID (-17) to CONSUMPTION_TYPE_ALARM (-1)
constexpr int32_t CONSUMPTION_TYPE_MIN = static_cast<int32_t>(BatteryStatsInfo::CONSUMPTION_TYPE_INVALID);
constexpr int32_t CONSUMPTION_TYPE_MAX = static_cast<int32_t>(BatteryStatsInfo::CONSUMPTION_TYPE_ALARM);
constexpr uint32_t CONSUMPTION_TYPE_RANGE =
    static_cast<uint32_t>(CONSUMPTION_TYPE_MAX - CONSUMPTION_TYPE_MIN + 1);

BatteryStatsInfo::ConsumptionType ToConsumptionType(uint8_t raw)
{
    uint32_t offset = static_cast<uint32_t>(raw) % CONSUMPTION_TYPE_RANGE;
    return static_cast<BatteryStatsInfo::ConsumptionType>(CONSUMPTION_TYPE_MIN + static_cast<int32_t>(offset));
}

void ConvertOneType(BatteryStatsInfo::ConsumptionType type)
{
    (void)BatteryStatsInfo::ConvertConsumptionType(type);
}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }

    // Cover the enum values driven by the fuzz input
    for (size_t i = 0; i < size && i < CONSUMPTION_TYPE_RANGE; i++) {
        ConvertOneType(ToConsumptionType(data[i]));
    }

    // Exhaustively cover every valid enum value
    for (int32_t i = CONSUMPTION_TYPE_MIN; i <= CONSUMPTION_TYPE_MAX; i++) {
        ConvertOneType(static_cast<BatteryStatsInfo::ConsumptionType>(i));
    }

    // Cover the out of range values that the switch statement must reject
    for (int32_t i = 0; i <= static_cast<int32_t>(CONSUMPTION_TYPE_RANGE); i++) {
        ConvertOneType(static_cast<BatteryStatsInfo::ConsumptionType>(i));
    }

    return 0;
}
