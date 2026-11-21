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

#define FUZZ_PROJECT_NAME "battery_stats_core_update_fuzzer"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include <fuzzer/FuzzedDataProvider.h>

#include "battery_stats_core.h"
#include "battery_stats_info.h"
#include "battery_stats_service.h"
#include "stats_utils.h"

using namespace OHOS::PowerMgr;

namespace {
constexpr int32_t STATS_TYPE_MIN = static_cast<int32_t>(StatsUtils::STATS_TYPE_INVALID);
constexpr int32_t STATS_TYPE_MAX = static_cast<int32_t>(StatsUtils::STATS_TYPE_ALARM);
constexpr int32_t STATS_STATE_MIN = static_cast<int32_t>(StatsUtils::STATS_STATE_INVALID);
constexpr int32_t STATS_STATE_MAX = static_cast<int32_t>(StatsUtils::STATS_STATE_WORKSCHEDULER_EXECUTED);
constexpr int32_t CONSUMPTION_TYPE_MIN = static_cast<int32_t>(BatteryStatsInfo::CONSUMPTION_TYPE_INVALID);
constexpr int32_t CONSUMPTION_TYPE_MAX = static_cast<int32_t>(BatteryStatsInfo::CONSUMPTION_TYPE_ALARM);
constexpr size_t MAX_STRING_BYTES = 32;

StatsUtils::StatsData ConsumeStatsData(FuzzedDataProvider& fdp)
{
    StatsUtils::StatsData statsData;
    statsData.type = static_cast<StatsUtils::StatsType>(
        fdp.ConsumeIntegralInRange<int32_t>(STATS_TYPE_MIN, STATS_TYPE_MAX));
    statsData.state = static_cast<StatsUtils::StatsState>(
        fdp.ConsumeIntegralInRange<int32_t>(STATS_STATE_MIN, STATS_STATE_MAX));
    statsData.uid = fdp.ConsumeIntegral<int32_t>();
    statsData.pid = fdp.ConsumeIntegral<int32_t>();
    statsData.eventDataName = fdp.ConsumeRandomLengthString(MAX_STRING_BYTES);
    statsData.eventDebugInfo = fdp.ConsumeRandomLengthString(MAX_STRING_BYTES);
    statsData.eventDataType = fdp.ConsumeIntegral<int32_t>();
    statsData.eventDataExtra = fdp.ConsumeIntegral<int32_t>();
    statsData.level = fdp.ConsumeIntegral<int16_t>();
    statsData.time = fdp.ConsumeIntegral<int64_t>();
    statsData.traffic = fdp.ConsumeIntegral<int64_t>();
    statsData.deviceId = fdp.ConsumeRandomLengthString(MAX_STRING_BYTES);
    return statsData;
}

BatteryStatsInfo::ConsumptionType ConsumeConsumptionType(FuzzedDataProvider& fdp)
{
    return static_cast<BatteryStatsInfo::ConsumptionType>(
        fdp.ConsumeIntegralInRange<int32_t>(CONSUMPTION_TYPE_MIN, CONSUMPTION_TYPE_MAX));
}
} // namespace

class BatteryStatsCoreUpdateFuzzer {
public:
    BatteryStatsCoreUpdateFuzzer()
    {
        OHOS::sptr<BatteryStatsService> service = BatteryStatsService::GetInstance();
        if (service == nullptr) {
            return;
        }
        service->OnStart();
        core_ = service->GetBatteryStatsCore();
        service_ = service;
    }

    ~BatteryStatsCoreUpdateFuzzer()
    {
        if (service_ != nullptr) {
            service_->OnStop();
        }
        core_ = nullptr;
        service_ = nullptr;
    }

    void Fuzz(FuzzedDataProvider& fdp)
    {
        if (core_ == nullptr) {
            return;
        }

        // The state based overload walks the whole entity dispatch chain of the core
        StatsUtils::StatsData statsData = ConsumeStatsData(fdp);
        core_->UpdateStats(statsData.type, statsData.state, statsData.level, statsData.uid, statsData.deviceId);
        core_->UpdateStats(statsData.type, statsData.time, statsData.traffic, statsData.uid);
        (void)core_->GetEntity(ConsumeConsumptionType(fdp));

        std::string debugInfo = fdp.ConsumeRandomLengthString(MAX_STRING_BYTES);
        core_->UpdateDebugInfo(debugInfo);
        std::string result;
        core_->GetDebugInfo(result);
        core_->DumpInfo(result);
        core_->Reset();
    }

private:
    std::shared_ptr<BatteryStatsCore> core_ = nullptr;
    OHOS::sptr<BatteryStatsService> service_ = nullptr;
};

namespace {
BatteryStatsCoreUpdateFuzzer& GetFuzzer()
{
    static BatteryStatsCoreUpdateFuzzer fuzzer;
    return fuzzer;
}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }

    FuzzedDataProvider fdp(data, size);
    GetFuzzer().Fuzz(fdp);
    return 0;
}
