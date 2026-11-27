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

#include <fuzzer/FuzzedDataProvider.h>

#include "stats_utils.h"

using namespace OHOS::PowerMgr;

namespace {
constexpr int32_t STATS_TYPE_MIN = static_cast<int32_t>(StatsUtils::STATS_TYPE_INVALID);
constexpr int32_t STATS_TYPE_MAX = static_cast<int32_t>(StatsUtils::STATS_TYPE_ALARM);
constexpr int32_t OUT_OF_RANGE_SPAN = 8;
constexpr size_t MAX_FUZZ_ROUNDS = 8;

void ConvertOneType(StatsUtils::StatsType type)
{
    (void)StatsUtils::ConvertStatsType(type);
}

void FuzzConvertType(FuzzedDataProvider& fdp)
{
    size_t rounds = fdp.ConsumeIntegralInRange<size_t>(0, MAX_FUZZ_ROUNDS);
    for (size_t i = 0; i < rounds; i++) {
        ConvertOneType(static_cast<StatsUtils::StatsType>(
            fdp.ConsumeIntegralInRange<int32_t>(STATS_TYPE_MIN, STATS_TYPE_MAX)));
    }
}

// Exhaustively cover every enumerator, then the values the conversion must reject
void CoverAllStatsTypes()
{
    for (int32_t i = STATS_TYPE_MIN; i <= STATS_TYPE_MAX; i++) {
        ConvertOneType(static_cast<StatsUtils::StatsType>(i));
    }
    for (int32_t i = STATS_TYPE_MAX + 1; i <= STATS_TYPE_MAX + OUT_OF_RANGE_SPAN; i++) {
        ConvertOneType(static_cast<StatsUtils::StatsType>(i));
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
    CoverAllStatsTypes();
    return 0;
}
