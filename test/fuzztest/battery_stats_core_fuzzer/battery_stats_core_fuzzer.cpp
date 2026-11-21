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

#define FUZZ_PROJECT_NAME "battery_stats_core_fuzzer"

#include <cstddef>
#include <cstdint>
#include <memory>

#include <fuzzer/FuzzedDataProvider.h>

#include "battery_stats_core.h"
#include "battery_stats_info.h"
#include "battery_stats_service.h"
#include "stats_utils.h"

using namespace OHOS::PowerMgr;

namespace {
constexpr int32_t STATS_TYPE_MIN = static_cast<int32_t>(StatsUtils::STATS_TYPE_INVALID);
constexpr int32_t STATS_TYPE_MAX = static_cast<int32_t>(StatsUtils::STATS_TYPE_ALARM);
constexpr int32_t CONSUMPTION_TYPE_MIN = static_cast<int32_t>(BatteryStatsInfo::CONSUMPTION_TYPE_INVALID);
constexpr int32_t CONSUMPTION_TYPE_MAX = static_cast<int32_t>(BatteryStatsInfo::CONSUMPTION_TYPE_ALARM);
constexpr int32_t OUT_OF_RANGE_OFFSET = 1;

// A legal enumerator, so the switch statement of the core takes a real branch
BatteryStatsInfo::ConsumptionType ConsumeValidConsumptionType(FuzzedDataProvider& fdp)
{
    return static_cast<BatteryStatsInfo::ConsumptionType>(
        fdp.ConsumeIntegralInRange<int32_t>(CONSUMPTION_TYPE_MIN, CONSUMPTION_TYPE_MAX));
}

// A value outside the enumerator range, so the default branch is exercised
BatteryStatsInfo::ConsumptionType ConsumeOutOfRangeConsumptionType(FuzzedDataProvider& fdp)
{
    int32_t span = CONSUMPTION_TYPE_MAX - CONSUMPTION_TYPE_MIN + OUT_OF_RANGE_OFFSET;
    return static_cast<BatteryStatsInfo::ConsumptionType>(fdp.ConsumeIntegralInRange<int32_t>(0, span));
}

StatsUtils::StatsType ConsumeStatsType(FuzzedDataProvider& fdp)
{
    return static_cast<StatsUtils::StatsType>(
        fdp.ConsumeIntegralInRange<int32_t>(STATS_TYPE_MIN, STATS_TYPE_MAX));
}
} // namespace

class BatteryStatsCoreFuzzer {
public:
    BatteryStatsCoreFuzzer()
    {
        OHOS::sptr<BatteryStatsService> service = BatteryStatsService::GetInstance();
        if (service == nullptr) {
            return;
        }
        service->OnStart();
        core_ = service->GetBatteryStatsCore();
        service_ = service;
    }

    ~BatteryStatsCoreFuzzer()
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

        core_->ComputePower();
        FuzzPowerQueries(fdp);
        FuzzCountQueries(fdp);
        core_->Reset();
    }

private:
    void FuzzPowerQueries(FuzzedDataProvider& fdp)
    {
        int32_t uid = fdp.ConsumeIntegral<int32_t>();
        BatteryStatsInfo::ConsumptionType validType = ConsumeValidConsumptionType(fdp);
        BatteryStatsInfo::ConsumptionType outOfRangeType = ConsumeOutOfRangeConsumptionType(fdp);

        (void)core_->GetBatteryStats();
        (void)core_->GetAppStatsMah(uid);
        (void)core_->GetAppStatsPercent(uid);
        (void)core_->GetPartStatsMah(validType);
        (void)core_->GetPartStatsPercent(validType);
        (void)core_->GetPartStatsMah(outOfRangeType);
        (void)core_->GetPartStatsPercent(outOfRangeType);
    }

    void FuzzCountQueries(FuzzedDataProvider& fdp)
    {
        StatsUtils::StatsType statsType = ConsumeStatsType(fdp);
        int32_t uid = fdp.ConsumeIntegral<int32_t>();
        int16_t level = fdp.ConsumeIntegral<int16_t>();

        (void)core_->GetTotalTimeMs(statsType, level);
        (void)core_->GetTotalTimeMs(uid, statsType, level);
        (void)core_->GetTotalDataCount(statsType, uid);
        (void)core_->GetTotalConsumptionCount(statsType, uid);
    }

    std::shared_ptr<BatteryStatsCore> core_ = nullptr;
    OHOS::sptr<BatteryStatsService> service_ = nullptr;
};

namespace {
BatteryStatsCoreFuzzer& GetFuzzer()
{
    static BatteryStatsCoreFuzzer fuzzer;
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
