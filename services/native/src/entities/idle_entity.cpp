/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "entities/idle_entity.h"

#include "battery_stats_service.h"
#include "stats_log.h"

namespace OHOS {
namespace PowerMgr {
namespace {
}

IdleEntity::IdleEntity()
{
    consumptionType_ = BatteryStatsInfo::CONSUMPTION_TYPE_IDLE;
    Reset();
}

int64_t IdleEntity::GetActiveTimeMs(StatsUtils::StatsType statsType, int16_t level)
{
    int64_t activeTimeMs = StatsUtils::DEFAULT_VALUE;
    switch (statsType) {
        case StatsUtils::STATS_TYPE_PHONE_IDLE:
            activeTimeMs = StatsHelper::GetOnBatteryUpTimeMs();
            break;
        case StatsUtils::STATS_TYPE_CPU_SUSPEND:
            activeTimeMs = StatsHelper::GetOnBatteryBootTimeMs();
            break;
        default:
            break;
    }
    return activeTimeMs;
}

void IdleEntity::Calculate(int32_t uid)
{
    auto cpuSuspendPower = CalculateCpuSuspendPower();
    auto cpuIdlePower = CalculateCpuIdlePower();
    idleTotalPowerMah_ = cpuSuspendPower + cpuIdlePower;
    {
        std::lock_guard<std::mutex> statsLock(statsInfoListMutex_);
        totalPowerMah_ += idleTotalPowerMah_;
    }
    std::shared_ptr<BatteryStatsInfo> statsInfo = std::make_shared<BatteryStatsInfo>();
    statsInfo->SetConsumptioType(BatteryStatsInfo::CONSUMPTION_TYPE_IDLE);
    statsInfo->SetPower(idleTotalPowerMah_);
    UpdateStatsInfoList(statsInfo);

    STATS_HILOGD(COMP_SVC, "Calculate idle total power consumption: %{public}lfmAh", idleTotalPowerMah_);
}

double IdleEntity::CalculateCpuSuspendPower()
{
    auto bss = BatteryStatsService::GetInstance();
    auto cpuSuspendAverageMa =
        bss->GetBatteryStatsParser()->GetAveragePowerMa(StatsUtils::CURRENT_CPU_SUSPEND);
    auto bootOnBatteryTimeMs = GetActiveTimeMs(StatsUtils::STATS_TYPE_CPU_SUSPEND);
    auto cpuSuspendPowerMah = cpuSuspendAverageMa * bootOnBatteryTimeMs / StatsUtils::MS_IN_HOUR;
    cpuSuspendPowerMah_ = cpuSuspendPowerMah;
    STATS_HILOGD(COMP_SVC, "Calculate cpu suspend power consumption: %{public}lfmAh", cpuSuspendPowerMah);
    return cpuSuspendPowerMah_;
}

double IdleEntity::CalculateCpuIdlePower()
{
    auto bss = BatteryStatsService::GetInstance();
    auto cpuIdleAverageMa =
        bss->GetBatteryStatsParser()->GetAveragePowerMa(StatsUtils::CURRENT_CPU_IDLE);
    auto upOnBatteryTimeMs = GetActiveTimeMs(StatsUtils::STATS_TYPE_PHONE_IDLE);
    auto cpuIdlePowerMah = cpuIdleAverageMa * upOnBatteryTimeMs / StatsUtils::MS_IN_HOUR;
    cpuIdlePowerMah_ = cpuIdlePowerMah;
    STATS_HILOGD(COMP_SVC, "Calculate cpu idle power consumption: %{public}lfmAh", cpuIdlePowerMah);
    return cpuIdlePowerMah_;
}

double IdleEntity::GetEntityPowerMah(int32_t uidOrUserId)
{
    return idleTotalPowerMah_;
}

double IdleEntity::GetStatsPowerMah(StatsUtils::StatsType statsType, int32_t uid)
{
    double power = StatsUtils::DEFAULT_VALUE;
    if (statsType == StatsUtils::STATS_TYPE_PHONE_IDLE) {
        power = cpuIdlePowerMah_;
        STATS_HILOGD(COMP_SVC, "Get cpu idle power consumption: %{public}lfmAh", power);
    } else if (statsType == StatsUtils::STATS_TYPE_CPU_SUSPEND) {
        power = cpuSuspendPowerMah_;
        STATS_HILOGD(COMP_SVC, "Get cpu suspend power consumption: %{public}lfmAh", power);
    }
    return power;
}

void IdleEntity::Reset()
{
    // Reset Idle total power consumption
    idleTotalPowerMah_ = StatsUtils::DEFAULT_VALUE;

    // Reset cpu idle power consumption
    cpuIdlePowerMah_ = StatsUtils::DEFAULT_VALUE;

    // Reset cpu suspend power consumption
    cpuSuspendPowerMah_ = StatsUtils::DEFAULT_VALUE;
}

void IdleEntity::DumpInfo(std::string& result, int32_t uid)
{
    int64_t upTime = GetActiveTimeMs(StatsUtils::STATS_TYPE_PHONE_IDLE);
    int64_t bootTime = GetActiveTimeMs(StatsUtils::STATS_TYPE_CPU_SUSPEND);
    result.append("Idle dump:\n")
        .append("Phone boot time: ")
        .append(ToString(bootTime))
        .append("ms")
        .append("\n")
        .append("Phone up time: ")
        .append(ToString(upTime))
        .append("ms")
        .append("\n");
}
} // namespace PowerMgr
} // namespace OHOS