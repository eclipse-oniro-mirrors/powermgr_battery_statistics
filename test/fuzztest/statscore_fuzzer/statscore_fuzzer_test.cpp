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

#define FUZZ_PROJECT_NAME "statscore_fuzzer"

#include "battery_stats_service.h"
#include "battery_stats_core.h"
#include "battery_stats_info.h"
#include "stats_utils.h"
#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>

using namespace OHOS::PowerMgr;

namespace {
    class StatsCoreFuzzer {
    public:
        StatsCoreFuzzer() {
            service_ = BatteryStatsService::GetInstance();
            service_->OnStart();
        }

        ~StatsCoreFuzzer() {
            if (service_ != nullptr) {
                service_->OnStop();
            }
        }

        void FuzzCoreOperations(const uint8_t* data, size_t size) {
            if (data == nullptr || size < 8) {
                return;
            }

            auto core = service_->GetBatteryStatsCore();
            if (core == nullptr) {
                return;
            }

            size_t offset = 0;

            // Fuzz GetEntity operations
            while (offset + 1 <= size) {
                uint8_t typeRaw = data[offset++];
                auto type = static_cast<BatteryStatsInfo::ConsumptionType>(
                    typeRaw % static_cast<uint8_t>(BatteryStatsInfo::CONSUMPTION_TYPE_INVALID));

                auto entity = core->GetEntity(type);
                if (entity != nullptr) {
                    // Fuzz entity calculate
                    entity->Calculate();

                    // Get stats info list (static method)
                    auto statsInfo = BatteryStatsEntity::GetStatsInfoList();
                }

                if (offset + 4 > size) break;
            }

            // Reset offset for different fuzzing
            offset = 0;

            // Fuzz aggregation queries
            while (offset + 8 <= size) {
                int32_t uid = *reinterpret_cast<const int32_t*>(&data[offset]);
                offset += 4;

                uint8_t typeRaw = data[offset++];
                auto consumptionType = static_cast<BatteryStatsInfo::ConsumptionType>(
                    typeRaw % static_cast<uint8_t>(BatteryStatsInfo::CONSUMPTION_TYPE_INVALID));

                // Test per-app queries
                core->GetAppStatsMah(uid);
                core->GetAppStatsPercent(uid);

                // Test per-component queries
                core->GetPartStatsMah(consumptionType);
                core->GetPartStatsPercent(consumptionType);

                // Test battery stats list
                auto statsList = core->GetBatteryStats();

                if (offset + 8 > size) break;
            }

            // Fuzz save/load operations
            core->SaveBatteryStatsData();
            core->LoadBatteryStatsData();

            // Fuzz dump operations
            std::string dumpResult;
            core->DumpInfo(dumpResult);

            // Fuzz debug info
            if (offset + 16 <= size) {
                std::string debugInfo(reinterpret_cast<const char*>(&data[offset]),
                                    std::min(size - offset, size_t(64)));
                core->UpdateDebugInfo(debugInfo);

                std::string getDebugResult;
                core->GetDebugInfo(getDebugResult);
            }

            // Fuzz reset
            core->Reset();
        }

    private:
        OHOS::sptr<BatteryStatsService> service_ = nullptr;
    };

    StatsCoreFuzzer g_fuzzer;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    g_fuzzer.FuzzCoreOperations(data, size);
    return 0;
}
