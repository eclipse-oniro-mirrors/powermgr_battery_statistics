/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#include "entities/wifi_entity.h"

#include <cinttypes>

#include "battery_stats_service.h"
#include "stats_log.h"

namespace OHOS {
namespace PowerMgr {
namespace {
}

WifiEntity::WifiEntity()
{
    consumptionType_ = BatteryStatsInfo::CONSUMPTION_TYPE_WIFI;
}

void WifiEntity::Calculate(int32_t uid)
{
    auto bss = BatteryStatsService::GetInstance();
    // Calculate Wifi on power
    auto wifiOnAverageMa = bss->GetBatteryStatsParser()->GetAveragePowerMa(StatsUtils::CURRENT_WIFI_ON);
    auto wifiOnTimeMs = GetActiveTimeMs(StatsUtils::STATS_TYPE_WIFI_ON);
    auto wifiOnPowerMah = wifiOnAverageMa * wifiOnTimeMs / StatsUtils::MS_IN_HOUR;

    // Calculate Wifi scan power
    auto wifiScanAverageMa = bss->GetBatteryStatsParser()->GetAveragePowerMa(StatsUtils::CURRENT_WIFI_SCAN);
    auto wifiScanCount = GetConsumptionCount(StatsUtils::STATS_TYPE_WIFI_SCAN);
    auto wifiScanPowerMah = wifiScanAverageMa * wifiScanCount;

    wifiPowerMah_ = wifiOnPowerMah + wifiScanPowerMah;
    {
        std::lock_guard<std::mutex> statsLock(statsMutex_);
        totalPowerMah_ += wifiPowerMah_;
    }
    std::shared_ptr<BatteryStatsInfo> statsInfo = std::make_shared<BatteryStatsInfo>();
    statsInfo->SetConsumptioType(BatteryStatsInfo::CONSUMPTION_TYPE_WIFI);
    statsInfo->SetPower(wifiPowerMah_);
    UpdateStatsInfoList(statsInfo);
    STATS_HILOGD(COMP_SVC, "Calculate wifi power consumption: %{public}lfmAh", wifiPowerMah_);
}

int64_t WifiEntity::GetActiveTimeMs(StatsUtils::StatsType statsType, int16_t level)
{
    int64_t time = StatsUtils::DEFAULT_VALUE;
    if (statsType != StatsUtils::STATS_TYPE_WIFI_ON) {
        return time;
    }
    if (wifiOnTimer_) {
        time = wifiOnTimer_->GetRunningTimeMs();
        STATS_HILOGD(COMP_SVC, "Get wifi on time: %{public}" PRId64 "ms", time);
        return time;
    }
    STATS_HILOGD(COMP_SVC, "Wifi has not been turned on yet, return 0");
    return time;
}

double WifiEntity::GetEntityPowerMah(int32_t uidOrUserId)
{
    return wifiPowerMah_;
}

double WifiEntity::GetStatsPowerMah(StatsUtils::StatsType statsType, int32_t uid)
{
    return wifiPowerMah_;
}

int64_t WifiEntity::GetConsumptionCount(StatsUtils::StatsType statsType, int32_t uid)
{
    int64_t count = StatsUtils::DEFAULT_VALUE;
    if (statsType != StatsUtils::STATS_TYPE_WIFI_SCAN) {
        return count;
    }
    if (wifiScanCounter_) {
        count = wifiScanCounter_->GetCount();
        STATS_HILOGD(COMP_SVC, "Get wifi scan count: %{public}" PRId64 "", count);
        return count;
    }
    STATS_HILOGD(COMP_SVC, "Wifi scan has not been triggered yet, return 0");
    return count;
}

std::shared_ptr<StatsHelper::ActiveTimer> WifiEntity::GetOrCreateTimer(StatsUtils::StatsType statsType, int16_t level)
{
    if (statsType != StatsUtils::STATS_TYPE_WIFI_ON) {
        return nullptr;
    }

    if (wifiOnTimer_ != nullptr) {
        STATS_HILOGD(COMP_SVC, "Get wifi on timer");
        return wifiOnTimer_;
    }
    wifiOnTimer_ = std::make_shared<StatsHelper::ActiveTimer>();
    return wifiOnTimer_;
}

std::shared_ptr<StatsHelper::Counter> WifiEntity::GetOrCreateCounter(StatsUtils::StatsType statsType, int32_t uid)
{
    if (statsType != StatsUtils::STATS_TYPE_WIFI_SCAN) {
        return nullptr;
    }
    if (wifiScanCounter_ != nullptr) {
        STATS_HILOGD(COMP_SVC, "Get wifi scan counter");
        return wifiScanCounter_;
    }
    STATS_HILOGD(COMP_SVC, "Create wifi scan counter");
    wifiScanCounter_ = std::make_shared<StatsHelper::Counter>();
    return wifiScanCounter_;
}

void WifiEntity::Reset()
{
    // Reset Wifi power consumption
    wifiPowerMah_ = StatsUtils::DEFAULT_VALUE;

    // Reset Wifi on timer
    if (wifiOnTimer_) {
        wifiOnTimer_->Reset();
    }

    // Reset Wifi scan counter
    if (wifiScanCounter_) {
        wifiScanCounter_->Reset();
    }
}

void WifiEntity::DumpInfo(std::string& result, int32_t uid)
{
    int64_t time = GetActiveTimeMs(StatsUtils::STATS_TYPE_WIFI_ON);
    int64_t conut = GetConsumptionCount(StatsUtils::STATS_TYPE_WIFI_SCAN);
    result.append("Wifi dump:\n")
        .append("Wifi on time: ")
        .append(ToString(time))
        .append("ms")
        .append("\n")
        .append("Wifi scan count: ")
        .append(ToString(conut))
        .append("\n");
}
} // namespace PowerMgr
} // namespace OHOS
