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

#include "BatteryStatsApi_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <fuzzer/FuzzedDataProvider.h>

#include "battery_stats_client.h"
#include "battery_stats_info.h"
#include "stats_utils.h"

using namespace OHOS::PowerMgr;

namespace OHOS {
namespace {
constexpr int32_t MIN_UID = -1;
constexpr int32_t MAX_UID = 200000;
constexpr int32_t CONSUMPTION_TYPE_MIN = static_cast<int32_t>(BatteryStatsInfo::CONSUMPTION_TYPE_INVALID);
constexpr int32_t CONSUMPTION_TYPE_MAX = static_cast<int32_t>(BatteryStatsInfo::CONSUMPTION_TYPE_ALARM);
constexpr uint8_t CONSUMPTION_TYPE_SIZE = static_cast<uint8_t>(CONSUMPTION_TYPE_MAX - CONSUMPTION_TYPE_MIN + 1);
constexpr int32_t STATS_TYPE_MIN = static_cast<int32_t>(StatsUtils::STATS_TYPE_INVALID);
constexpr int32_t STATS_TYPE_MAX = static_cast<int32_t>(StatsUtils::STATS_TYPE_ALARM);
constexpr uint8_t STATS_TYPE_SIZE = static_cast<uint8_t>(STATS_TYPE_MAX - STATS_TYPE_MIN + 1);
constexpr uint8_t MAX_DUMP_ARGS = 8;
constexpr size_t MAX_ARG_LEN = 32;

int32_t ConsumeUid(FuzzedDataProvider &fdp)
{
    return fdp.ConsumeIntegralInRange<int32_t>(MIN_UID, MAX_UID);
}

uint8_t ConsumeEnumOffset(FuzzedDataProvider &fdp, uint8_t size)
{
    return fdp.ConsumeIntegralInRange<uint8_t>(0, size - 1);
}

BatteryStatsInfo::ConsumptionType ConsumeConsumptionType(FuzzedDataProvider &fdp)
{
    int32_t value = CONSUMPTION_TYPE_MIN + static_cast<int32_t>(ConsumeEnumOffset(fdp, CONSUMPTION_TYPE_SIZE));
    return static_cast<BatteryStatsInfo::ConsumptionType>(value);
}

StatsUtils::StatsType ConsumeStatsType(FuzzedDataProvider &fdp)
{
    int32_t value = STATS_TYPE_MIN + static_cast<int32_t>(ConsumeEnumOffset(fdp, STATS_TYPE_SIZE));
    return static_cast<StatsUtils::StatsType>(value);
}

std::vector<std::string> ConsumeDumpArgs(FuzzedDataProvider &fdp)
{
    std::vector<std::string> args;
    uint8_t count = fdp.ConsumeIntegralInRange<uint8_t>(0, MAX_DUMP_ARGS);
    for (uint8_t i = 0; i < count; ++i) {
        args.emplace_back(fdp.ConsumeRandomLengthString(MAX_ARG_LEN));
    }
    return args;
}

} // namespace
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return 0;
    }

    FuzzedDataProvider fdp(data, size);
    auto &statsClient = BatteryStatsClient::GetInstance();

    // Each interface consumes independent fuzz data instead of reusing one value.
    statsClient.SetOnBattery(fdp.ConsumeBool());
    (void)statsClient.GetAppStatsMah(OHOS::ConsumeUid(fdp));
    (void)statsClient.GetAppStatsPercent(OHOS::ConsumeUid(fdp));
    (void)statsClient.GetPartStatsMah(OHOS::ConsumeConsumptionType(fdp));
    (void)statsClient.GetPartStatsPercent(OHOS::ConsumeConsumptionType(fdp));
    (void)statsClient.GetTotalTimeSecond(OHOS::ConsumeStatsType(fdp), OHOS::ConsumeUid(fdp));
    (void)statsClient.GetTotalDataBytes(OHOS::ConsumeStatsType(fdp), OHOS::ConsumeUid(fdp));
    (void)statsClient.Dump(OHOS::ConsumeDumpArgs(fdp));
    return 0;
}
