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

#include "LXJBattery_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "battery_stats_client.h"
#include "battery_stats_info.h"
#include "stats_utils.h"

using namespace OHOS::PowerMgr;

namespace OHOS {
constexpr size_t MIN_FUZZ_SIZE = 1;
constexpr size_t MAX_DUMP_ARGS = 8;
constexpr size_t MAX_ARG_LEN = 32;
constexpr int32_t MIN_UID = -1;
constexpr int32_t MAX_UID = 200000;
constexpr int32_t CONSUMPTION_TYPE_MIN = static_cast<int32_t>(BatteryStatsInfo::CONSUMPTION_TYPE_INVALID);
constexpr int32_t CONSUMPTION_TYPE_MAX = static_cast<int32_t>(BatteryStatsInfo::CONSUMPTION_TYPE_ALARM);
constexpr int32_t STATS_TYPE_MIN = static_cast<int32_t>(StatsUtils::STATS_TYPE_INVALID);
constexpr int32_t STATS_TYPE_MAX = static_cast<int32_t>(StatsUtils::STATS_TYPE_ALARM);
constexpr uint32_t BYTE_BITS = 8;

uint8_t ConsumeByte(const uint8_t *data, size_t size, size_t &pos)
{
    if (pos >= size) {
        return 0;
    }
    return data[pos++];
}

int32_t ConsumeInt32(const uint8_t *data, size_t size, size_t &pos)
{
    uint32_t value = 0;
    for (size_t i = 0; i < sizeof(uint32_t); ++i) {
        value |= static_cast<uint32_t>(ConsumeByte(data, size, pos)) << (i * BYTE_BITS);
    }
    return static_cast<int32_t>(value);
}

int32_t ConsumeInt32InRange(const uint8_t *data, size_t size, size_t &pos, int32_t min, int32_t max)
{
    uint32_t range = static_cast<uint32_t>(max - min + 1);
    uint32_t value = static_cast<uint32_t>(ConsumeInt32(data, size, pos));
    return min + static_cast<int32_t>(value % range);
}

std::string ConsumeString(const uint8_t *data, size_t size, size_t &pos)
{
    size_t len = ConsumeByte(data, size, pos) % (MAX_ARG_LEN + 1);
    if (pos + len > size) {
        len = size - pos;
    }
    std::string value(reinterpret_cast<const char *>(data + pos), len);
    pos += len;
    return value;
}

std::vector<std::string> ConsumeDumpArgs(const uint8_t *data, size_t size, size_t &pos)
{
    std::vector<std::string> args;
    size_t count = ConsumeByte(data, size, pos) % (MAX_DUMP_ARGS + 1);
    for (size_t i = 0; i < count; ++i) {
        args.emplace_back(ConsumeString(data, size, pos));
    }
    return args;
}

BatteryStatsInfo::ConsumptionType ConsumeConsumptionType(const uint8_t *data, size_t size, size_t &pos)
{
    int32_t value = ConsumeInt32InRange(data, size, pos, CONSUMPTION_TYPE_MIN, CONSUMPTION_TYPE_MAX);
    return static_cast<BatteryStatsInfo::ConsumptionType>(value);
}

StatsUtils::StatsType ConsumeStatsType(const uint8_t *data, size_t size, size_t &pos)
{
    int32_t value = ConsumeInt32InRange(data, size, pos, STATS_TYPE_MIN, STATS_TYPE_MAX);
    return static_cast<StatsUtils::StatsType>(value);
}

void FuzzBatteryStatsClient(const uint8_t *data, size_t size)
{
    size_t pos = 0;
    auto &statsClient = BatteryStatsClient::GetInstance();

    bool isOnBattery = (ConsumeByte(data, size, pos) & 1U) != 0;
    int32_t uid = ConsumeInt32InRange(data, size, pos, MIN_UID, MAX_UID);
    BatteryStatsInfo::ConsumptionType consumptionType = ConsumeConsumptionType(data, size, pos);
    StatsUtils::StatsType statsType = ConsumeStatsType(data, size, pos);
    std::vector<std::string> dumpArgs = ConsumeDumpArgs(data, size, pos);

    statsClient.SetOnBattery(isOnBattery);
    (void)statsClient.GetBatteryStats();
    (void)statsClient.GetAppStatsMah(uid);
    (void)statsClient.GetAppStatsPercent(uid);
    (void)statsClient.GetPartStatsMah(consumptionType);
    (void)statsClient.GetPartStatsPercent(consumptionType);
    (void)statsClient.GetTotalTimeSecond(statsType, uid);
    (void)statsClient.GetTotalDataBytes(statsType, uid);
    (void)statsClient.Dump(dumpArgs);
    (void)statsClient.GetLastError();

    if ((ConsumeByte(data, size, pos) & 1U) != 0) {
        statsClient.Reset();
        (void)statsClient.GetBatteryStats();
    }
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < OHOS::MIN_FUZZ_SIZE) {
        return 0;
    }
    OHOS::FuzzBatteryStatsClient(data, size);
    return 0;
}
