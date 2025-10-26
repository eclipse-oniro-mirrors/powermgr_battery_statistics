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
#include <string>

using namespace OHOS::PowerMgr;

namespace {
    void FuzzNapiUtilsAndError(const uint8_t* data, size_t size) {
        if (data == nullptr || size < 4) {
            return;
        }

        size_t offset = 0;

        // Fuzz BatteryStatsClient directly (tested via NAPI layer)
        while (offset + 8 <= size) {
            int32_t uid = *reinterpret_cast<const int32_t*>(&data[offset]);
            offset += 4;

            int32_t type = *reinterpret_cast<const int32_t*>(&data[offset]);
            offset += 4;

            auto& client = BatteryStatsClient::GetInstance();

            // Test various client methods
            client.GetAppStatsMah(uid);
            client.GetAppStatsPercent(uid);

            auto consumptionType = static_cast<BatteryStatsInfo::ConsumptionType>(
                type % static_cast<int32_t>(BatteryStatsInfo::CONSUMPTION_TYPE_INVALID));

            client.GetPartStatsMah(consumptionType);
            client.GetPartStatsPercent(consumptionType);

            // Test stats list
            auto statsList = client.GetBatteryStats();

            if (offset + 8 > size) break;
        }

        // Fuzz GetTotalTimeSecond and GetTotalDataBytes
        offset = 0;
        while (offset + 12 <= size) {
            int32_t statsTypeRaw = *reinterpret_cast<const int32_t*>(&data[offset]);
            offset += 4;

            int32_t uid = *reinterpret_cast<const int32_t*>(&data[offset]);
            offset += 4;

            auto& client = BatteryStatsClient::GetInstance();

            auto statsType = static_cast<StatsUtils::StatsType>(statsTypeRaw % 32);

            client.GetTotalTimeSecond(statsType, uid);
            client.GetTotalDataBytes(statsType, uid);

            if (offset + 8 > size) break;
        }

        // Fuzz Reset
        auto& client = BatteryStatsClient::GetInstance();
        client.Reset();

        // Fuzz SetOnBattery
        if (size > 0) {
            bool onBattery = data[0] & 0x01;
            client.SetOnBattery(onBattery);
        }

        // Fuzz Dump
        if (offset + 8 <= size) {
            std::vector<std::string> args;
            size_t numArgs = (data[offset] % 5) + 1;
            offset++;

            for (size_t i = 0; i < numArgs && offset < size; i++) {
                size_t argLen = std::min(size_t(data[offset] % 16), size - offset - 1);
                offset++;

                if (offset + argLen <= size) {
                    std::string arg(reinterpret_cast<const char*>(&data[offset]), argLen);
                    args.push_back(arg);
                    offset += argLen;
                }
            }

            client.Dump(args);
        }
    }
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzNapiUtilsAndError(data, size);
    return 0;
}
