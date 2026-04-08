/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#define FUZZ_PROJECT_NAME "napi_fuzzer"

#include "napi_utils.h"
#include "napi_error.h"
#include "battery_stats_client.h"
#include "battery_stats_info.h"
#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <string>
#include <vector>

using namespace OHOS::PowerMgr;

namespace {
constexpr size_t MIN_NAPI_DATA_SIZE = sizeof(int32_t);
constexpr size_t CLIENT_BLOCK_BYTES = 8;
constexpr size_t TOTALS_BLOCK_BYTES = 12;
constexpr size_t DUMP_GUARD_BYTES = 8;
constexpr size_t MAX_DUMP_ARG_MOD = 5;
constexpr size_t MAX_DUMP_ARG_LENGTH = 16;
constexpr uint8_t BATTERY_STATE_MASK = 0x01;
constexpr int32_t STATS_TYPE_MOD = 32;

void FuzzClientStats(const uint8_t* data, size_t size)
{
    size_t offset = 0;
    auto& client = BatteryStatsClient::GetInstance();

    while (offset + CLIENT_BLOCK_BYTES <= size) {
        int32_t uid = *reinterpret_cast<const int32_t*>(&data[offset]);
        offset += sizeof(int32_t);

        int32_t type = *reinterpret_cast<const int32_t*>(&data[offset]);
        offset += sizeof(int32_t);

        client.GetAppStatsMah(uid);
        client.GetAppStatsPercent(uid);

        auto consumptionType = static_cast<BatteryStatsInfo::ConsumptionType>(
            type % static_cast<int32_t>(BatteryStatsInfo::CONSUMPTION_TYPE_INVALID));

        client.GetPartStatsMah(consumptionType);
        client.GetPartStatsPercent(consumptionType);
        (void)client.GetBatteryStats();

        if (offset + DUMP_GUARD_BYTES > size) {
            break;
        }
    }
}

size_t FuzzTotalTimeQueries(const uint8_t* data, size_t size)
{
    size_t offset = 0;
    auto& client = BatteryStatsClient::GetInstance();

    while (offset + TOTALS_BLOCK_BYTES <= size) {
        int32_t statsTypeRaw = *reinterpret_cast<const int32_t*>(&data[offset]);
        offset += sizeof(int32_t);

        int32_t uid = *reinterpret_cast<const int32_t*>(&data[offset]);
        offset += sizeof(int32_t);

        auto statsType = static_cast<StatsUtils::StatsType>(statsTypeRaw % STATS_TYPE_MOD);

        client.GetTotalTimeSecond(statsType, uid);
        client.GetTotalDataBytes(statsType, uid);

        if (offset + DUMP_GUARD_BYTES > size) {
            break;
        }
    }

    return offset;
}

void FuzzStateToggle(const uint8_t* data, size_t size)
{
    if (size == 0) {
        return;
    }

    auto& client = BatteryStatsClient::GetInstance();
    bool onBattery = (data[0] & BATTERY_STATE_MASK) != 0;
    client.SetOnBattery(onBattery);
}

void FuzzDump(const uint8_t* data, size_t size, size_t offset)
{
    if (offset + DUMP_GUARD_BYTES > size) {
        return;
    }

    auto& client = BatteryStatsClient::GetInstance();
    std::vector<std::string> args;
    size_t numArgs = (data[offset] % MAX_DUMP_ARG_MOD) + 1;
    offset++;

    for (size_t i = 0; i < numArgs && offset < size; i++) {
        size_t remaining = size - offset;
        if (remaining <= 1) {
            break;
        }

        size_t argLen = std::min(static_cast<size_t>(data[offset] % MAX_DUMP_ARG_LENGTH), remaining - 1);
        offset++;

        if (offset + argLen <= size) {
            std::string arg(reinterpret_cast<const char*>(&data[offset]), argLen);
            args.push_back(arg);
            offset += argLen;
        }
    }

    client.Dump(args);
}

void FuzzNapiUtilsAndError(const uint8_t* data, size_t size)
{
    if (data == nullptr || size < MIN_NAPI_DATA_SIZE) {
        return;
    }

    FuzzClientStats(data, size);
    size_t totalsOffset = FuzzTotalTimeQueries(data, size);

    auto& client = BatteryStatsClient::GetInstance();
    client.Reset();

    FuzzStateToggle(data, size);
    FuzzDump(data, size, totalsOffset);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzNapiUtilsAndError(data, size);
    return 0;
}
