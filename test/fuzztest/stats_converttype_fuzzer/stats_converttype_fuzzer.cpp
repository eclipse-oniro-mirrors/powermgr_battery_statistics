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

#include "stats_utils.h"
#include <cstddef>
#include <cstdint>

using namespace OHOS::PowerMgr;

namespace {
constexpr int32_t STATS_TYPE_MIN = -1;
constexpr int32_t STATS_TYPE_MAX = static_cast<int32_t>(StatsUtils::STATS_TYPE_DISTRIBUTEDSCHEDULER);
constexpr size_t STATS_TYPE_RANGE = STATS_TYPE_MAX - STATS_TYPE_MIN + 1;

StatsUtils::StatsType ToStatsType(uint8_t raw)
{
    int32_t offset = static_cast<int32_t>(raw) % static_cast<uint8_t>(STATS_TYPE_RANGE);
    return static_cast<StatsUtils::StatsType>(STATS_TYPE_MIN + offset);
}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }

    // Test ConvertStatsType with fuzz-driven enum values
    for (size_t i = 0; i < size && i < STATS_TYPE_RANGE; i++) {
        StatsUtils::StatsType type = ToStatsType(data[i]);
        std::string result = StatsUtils::ConvertStatsType(type);
        (void)result;
    }

    // Also exhaustively test all valid enum values for full coverage
    for (int32_t i = STATS_TYPE_MIN; i <= STATS_TYPE_MAX; i++) {
        StatsUtils::StatsType type = static_cast<StatsUtils::StatsType>(i);
        std::string result = StatsUtils::ConvertStatsType(type);
        (void)result;
    }

    return 0;
}
