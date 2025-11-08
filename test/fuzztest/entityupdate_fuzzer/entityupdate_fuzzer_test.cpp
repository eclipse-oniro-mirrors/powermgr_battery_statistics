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

#define FUZZ_PROJECT_NAME "entityupdate_fuzzer"

#include "battery_stats_service.h"
#include "battery_stats_core.h"
#include "stats_utils.h"
#include <cstdint>
#include <cstddef>
#include <vector>

using namespace OHOS::PowerMgr;

namespace {
constexpr size_t MIN_ENTITY_DATA_SIZE = 16;
constexpr size_t ENTITY_PARAM_BYTES = 16;
constexpr size_t INT16_BYTES = sizeof(int16_t);
constexpr size_t INT32_BYTES = sizeof(int32_t);
constexpr size_t INT64_BYTES = sizeof(int64_t);
constexpr uint8_t STATS_TYPE_MOD = 32;
constexpr uint8_t STATS_STATE_MOD = 4;

class EntityUpdateFuzzer {
public:
    EntityUpdateFuzzer()
    {
        service_ = BatteryStatsService::GetInstance();
        service_->OnStart();
    }

    ~EntityUpdateFuzzer()
    {
        if (service_ != nullptr) {
            service_->OnStop();
        }
    }

    void FuzzEntityUpdates(const uint8_t* data, size_t size)
    {
        if (data == nullptr || size < MIN_ENTITY_DATA_SIZE) {
            return;
        }

        auto core = service_->GetBatteryStatsCore();
        if (core == nullptr) {
            return;
        }

        // Parse fuzzer input
        size_t offset = 0;

        while (offset + ENTITY_PARAM_BYTES <= size) {
            // Extract parameters from fuzz data
            uint8_t statsTypeRaw = data[offset++];
            uint8_t stateRaw = data[offset++];
            int16_t level = *reinterpret_cast<const int16_t*>(&data[offset]);
            offset += INT16_BYTES;
            int32_t uid = *reinterpret_cast<const int32_t*>(&data[offset]);
            offset += INT32_BYTES;
            int64_t time = *reinterpret_cast<const int64_t*>(&data[offset]);
            offset += INT64_BYTES;

            // Map to valid enum values
            StatsUtils::StatsType statsType =
                static_cast<StatsUtils::StatsType>(statsTypeRaw % STATS_TYPE_MOD);
            StatsUtils::StatsState state =
                static_cast<StatsUtils::StatsState>(stateRaw % STATS_STATE_MOD);

            // Test UpdateStats with state changes
            core->UpdateStats(statsType, state, level, uid);

            // Test UpdateStats with time/data
            if (offset + INT64_BYTES <= size) {
                int64_t dataValue = *reinterpret_cast<const int64_t*>(&data[offset]);
                offset += INT64_BYTES;
                core->UpdateStats(statsType, time, dataValue, uid);
            }

            // Test entity-specific queries
            core->GetTotalTimeMs(statsType, level);
            core->GetTotalTimeMs(uid, statsType, level);
            core->GetTotalDataCount(statsType, uid);
            core->GetTotalConsumptionCount(statsType, uid);
        }

        // Test power computation
        core->ComputePower();

        // Test various entity queries with extracted UID
        if (offset >= INT32_BYTES) {
            int32_t testUid = *reinterpret_cast<const int32_t*>(&data[offset - INT32_BYTES]);
            core->GetAppStatsMah(testUid);
            core->GetAppStatsPercent(testUid);
        }
    }

private:
    OHOS::sptr<BatteryStatsService> service_ = nullptr;
};

EntityUpdateFuzzer g_fuzzer;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    g_fuzzer.FuzzEntityUpdates(data, size);
    return 0;
}
