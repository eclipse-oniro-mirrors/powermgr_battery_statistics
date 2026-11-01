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

#define FUZZ_PROJECT_NAME "stats_converttype_fuzzer"

#include <cstddef>
#include <cstdint>
#include <string>

#include "stats_utils.h"

using namespace OHOS::PowerMgr;

namespace {
constexpr int32_t STATS_TYPE_MIN = static_cast<int32_t>(StatsUtils::STATS_TYPE_INVALID);
constexpr int32_t STATS_TYPE_MAX = static_cast<int32_t>(StatsUtils::STATS_TYPE_ALARM);
constexpr uint32_t STATS_TYPE_RANGE = static_cast<uint32_t>(STATS_TYPE_MAX - STATS_TYPE_MIN + 1);

StatsUtils::StatsType ToStatsType(uint8_t raw)
{
    uint32_t offset = static_cast<uint32_t>(raw) % STATS_TYPE_RANGE;
    return static_cast<StatsUtils::StatsType>(STATS_TYPE_MIN + static_cast<int32_t>(offset));
}

void ConvertOneType(StatsUtils::StatsType type)
{
    (void)StatsUtils::ConvertStatsType(type);
}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }

    // Cover the enum values driven by the fuzz input
    for (size_t i = 0; i < size && i < STATS_TYPE_RANGE; i++) {
        ConvertOneType(ToStatsType(data[i]));
    }

    // Exhaustively cover every valid enum value, including STATS_TYPE_ALARM
    for (int32_t i = STATS_TYPE_MIN; i <= STATS_TYPE_MAX; i++) {
        ConvertOneType(static_cast<StatsUtils::StatsType>(i));
    }

    return 0;
}
