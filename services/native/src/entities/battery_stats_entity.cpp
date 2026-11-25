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

#include "entities/battery_stats_entity.h"
#include "stats_log.h"

namespace OHOS {
namespace PowerMgr {
double BatteryStatsEntity::totalPowerMah_ = StatsUtils::DEFAULT_VALUE;
BatteryStatsInfoList BatteryStatsEntity::statsInfoList_;
std::mutex BatteryStatsEntity::statsMutex_;

void BatteryStatsEntity::AggregateUserPowerMah(int32_t userId, double power)
{
    STATS_HILOGE(COMP_SVC, "No need to add app power to related user");
}

void BatteryStatsEntity::Calculate(int32_t uid)
{
    STATS_HILOGE(COMP_SVC, "No need to calculate");
}

int64_t BatteryStatsEntity::GetTrafficByte(StatsUtils::StatsType statsType, int32_t uid)
{
    STATS_HILOGE(COMP_SVC, "No need to get traffic");
    return StatsUtils::DEFAULT_VALUE;
}

int64_t BatteryStatsEntity::GetConsumptionCount(StatsUtils::StatsType statsType, int32_t uid)
{
    STATS_HILOGE(COMP_SVC, "No need to consumption count");
    return StatsUtils::DEFAULT_VALUE;
}

BatteryStatsInfoList BatteryStatsEntity::GetStatsInfoList()
{
    std::lock_guard<std::mutex> lock(statsMutex_);
    return statsInfoList_;
}

void BatteryStatsEntity::UpdateStatsInfoList(std::shared_ptr<BatteryStatsInfo> info)
{
    std::lock_guard<std::mutex> lock(statsMutex_);
    statsInfoList_.push_back(info);
}

int64_t BatteryStatsEntity::GetActiveTimeMs(int32_t uid, StatsUtils::StatsType statsType, int16_t level)
{
    STATS_HILOGE(COMP_SVC, "No need to get active time, return 0");
    return StatsUtils::DEFAULT_VALUE;
}

int64_t BatteryStatsEntity::GetActiveTimeMs(StatsUtils::StatsType statsType, int16_t level)
{
    STATS_HILOGE(COMP_SVC, "No need to get active time, return 0");
    return StatsUtils::DEFAULT_VALUE;
}

void BatteryStatsEntity::DumpInfo(std::string& result, int32_t uid)
{
    STATS_HILOGE(COMP_SVC, "No need to dump");
}

void BatteryStatsEntity::UpdateUidMap(int32_t uid)
{
    STATS_HILOGE(COMP_SVC, "No need to update uid");
}

std::vector<int32_t> BatteryStatsEntity::GetUids()
{
    STATS_HILOGE(COMP_SVC, "No need to get uids");
    std::vector<int32_t> empty;
    return empty;
}

int64_t BatteryStatsEntity::GetCpuTimeMs(int32_t uid)
{
    STATS_HILOGE(COMP_SVC, "No need to get cpu time, return 0");
    return StatsUtils::DEFAULT_VALUE;
}

void BatteryStatsEntity::UpdateCpuTime()
{
    STATS_HILOGE(COMP_SVC, "No need to update cpu time");
}

double BatteryStatsEntity::GetStatsPowerMah(StatsUtils::StatsType statsType, int32_t uid)
{
    STATS_HILOGE(COMP_SVC, "No need to get stats power, return 0");
    return StatsUtils::DEFAULT_VALUE;
}

std::shared_ptr<StatsHelper::ActiveTimer> BatteryStatsEntity::GetOrCreateTimer(StatsUtils::StatsType statsType,
    int16_t level)
{
    STATS_HILOGE(COMP_SVC, "No need to get or create timer, return nullptr");
    return nullptr;
}

std::shared_ptr<StatsHelper::ActiveTimer> BatteryStatsEntity::GetOrCreateTimer(int32_t uid,
    StatsUtils::StatsType statsType, int16_t level)
{
    STATS_HILOGE(COMP_SVC, "No need to get or create timer, return nullptr");
    return nullptr;
}

std::shared_ptr<StatsHelper::ActiveTimer> BatteryStatsEntity::GetOrCreateTimer(const std::string& deviceId, int32_t uid,
    StatsUtils::StatsType statsType, int16_t level)
{
    STATS_HILOGE(COMP_SVC, "No need to get or create timer, return nullptr");
    return nullptr;
}

std::shared_ptr<StatsHelper::Counter> BatteryStatsEntity::GetOrCreateCounter(StatsUtils::StatsType statsType,
    int32_t uid)
{
    STATS_HILOGE(COMP_SVC, "No need to get or create counter, return nullptr");
    return nullptr;
}

BatteryStatsInfo::ConsumptionType BatteryStatsEntity::GetConsumptionType()
{
    return consumptionType_;
}

double BatteryStatsEntity::GetTotalPowerMah()
{
    std::lock_guard<std::mutex> lock(statsMutex_);
    return totalPowerMah_;
}

void BatteryStatsEntity::ResetStatsEntity()
{
    STATS_HILOGI(COMP_SVC, "Reset total consumption power and battery stats list");
    std::lock_guard<std::mutex> lock(statsMutex_);
    totalPowerMah_ = StatsUtils::DEFAULT_VALUE;
    statsInfoList_.clear();
}
} // namespace PowerMgr
} // namespace OHOS