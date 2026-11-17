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

#include "entities/phone_entity.h"

#include <cinttypes>

#include "battery_stats_service.h"
#include "stats_log.h"

namespace OHOS {
namespace PowerMgr {
namespace {
}

PhoneEntity::PhoneEntity()
{
    consumptionType_ = BatteryStatsInfo::CONSUMPTION_TYPE_PHONE;
}

int64_t PhoneEntity::GetActiveTimeMs(StatsUtils::StatsType statsType, int16_t level)
{
    int64_t activeTimeMs = StatsUtils::DEFAULT_VALUE;
    switch (statsType) {
        case StatsUtils::STATS_TYPE_PHONE_ACTIVE: {
            if (level != StatsUtils::INVALID_VALUE) {
                auto iter = phoneOnTimerMap_.find(level);
                if (iter != phoneOnTimerMap_.end() && iter->second != nullptr) {
                    activeTimeMs = iter->second->GetRunningTimeMs();
                    STATS_HILOGD(COMP_SVC, "Get phone on time: %{public}" PRId64 "ms of signal level: %{public}d",
                        activeTimeMs, level);
                    break;
                }
                STATS_HILOGD(COMP_SVC, "No phone on timer found, return 0");
                break;
            }
            activeTimeMs = GetTotalTimeMs(StatsUtils::STATS_TYPE_PHONE_ACTIVE);
            STATS_HILOGD(COMP_SVC, "Get phone on total time: %{public}" PRId64 "ms", activeTimeMs);
            break;
        }
        case StatsUtils::STATS_TYPE_PHONE_DATA: {
            if (level != StatsUtils::INVALID_VALUE) {
                auto iter = phoneDataTimerMap_.find(level);
                if (iter != phoneDataTimerMap_.end() && iter->second != nullptr) {
                    activeTimeMs = iter->second->GetRunningTimeMs();
                    STATS_HILOGD(COMP_SVC, "Get phone data time: %{public}" PRId64 "ms of signal level: %{public}d",
                        activeTimeMs, level);
                    break;
                }
                STATS_HILOGD(COMP_SVC, "No phone data timer found, return 0");
                break;
            }
            activeTimeMs = GetTotalTimeMs(StatsUtils::STATS_TYPE_PHONE_DATA);
            STATS_HILOGD(COMP_SVC, "Get phone data total time: %{public}" PRId64 "ms", activeTimeMs);
            break;
        }
        default:
            break;
    }
    return activeTimeMs;
}

int64_t PhoneEntity::GetTotalTimeMs(StatsUtils::StatsType statsType)
{
    int64_t totalTimeMs = StatsUtils::DEFAULT_VALUE;
    switch (statsType) {
        case StatsUtils::STATS_TYPE_PHONE_ACTIVE: {
            for (auto timerIter : phoneOnTimerMap_) {
                totalTimeMs += timerIter.second->GetRunningTimeMs();
            }
            break;
        }
        case StatsUtils::STATS_TYPE_PHONE_DATA: {
            for (auto timerIter : phoneDataTimerMap_) {
                totalTimeMs += timerIter.second->GetRunningTimeMs();
            }
            break;
        }
        default:
            break;
    }
    return totalTimeMs;
}

void PhoneEntity::Calculate(int32_t uid)
{
    auto bss = BatteryStatsService::GetInstance();
    // Calculate phone on power
    double phoneOnPowerMah = StatsUtils::DEFAULT_VALUE;
    for (int32_t i = 0; i < StatsUtils::RADIO_SIGNAL_BIN; i++) {
        auto phoneOnAverageMa =
            bss->GetBatteryStatsParser()->GetAveragePowerMa(StatsUtils::CURRENT_RADIO_ON, i);
        auto phoneOnLevelTimeMs = GetActiveTimeMs(StatsUtils::STATS_TYPE_PHONE_ACTIVE, i);
        double phoneOnLevelPowerMah = phoneOnAverageMa * phoneOnLevelTimeMs / StatsUtils::MS_IN_HOUR;
        phoneOnPowerMah += phoneOnLevelPowerMah;
    }

    // Calculate phone data power
    double phoneDataPowerMah = StatsUtils::DEFAULT_VALUE;
    for (int32_t i = 0; i < StatsUtils::RADIO_SIGNAL_BIN; i++) {
        auto phoneDataAverageMa =
            bss->GetBatteryStatsParser()->GetAveragePowerMa(StatsUtils::CURRENT_RADIO_DATA, i);
        auto phoneDataLevelTimeMs = GetActiveTimeMs(StatsUtils::STATS_TYPE_PHONE_DATA, i);
        double phoneDataLevelPowerMah = phoneDataAverageMa * phoneDataLevelTimeMs / StatsUtils::MS_IN_HOUR;
        phoneDataPowerMah += phoneDataLevelPowerMah;
    }
    phonePowerMah_ = phoneOnPowerMah + phoneDataPowerMah;
    {
        std::lock_guard<std::mutex> statsLock(statsMutex_);
        totalPowerMah_ += phonePowerMah_;
    }
    std::shared_ptr<BatteryStatsInfo> statsInfo = std::make_shared<BatteryStatsInfo>();
    statsInfo->SetConsumptioType(BatteryStatsInfo::CONSUMPTION_TYPE_PHONE);
    statsInfo->SetPower(phonePowerMah_);
    UpdateStatsInfoList(statsInfo);
    STATS_HILOGD(COMP_SVC, "Calculate phone active power consumption: %{public}lfmAh", phonePowerMah_);
}

double PhoneEntity::GetEntityPowerMah(int32_t uidOrUserId)
{
    return phonePowerMah_;
}

double PhoneEntity::GetStatsPowerMah(StatsUtils::StatsType statsType, int32_t uid)
{
    return phonePowerMah_;
}

std::shared_ptr<StatsHelper::ActiveTimer> PhoneEntity::GetOrCreateTimer(StatsUtils::StatsType statsType, int16_t level)
{
    std::shared_ptr<StatsHelper::ActiveTimer> timer = nullptr;
    if (level <= StatsUtils::INVALID_VALUE || level > StatsUtils::RADIO_SIGNAL_BIN) {
        STATS_HILOGD(COMP_SVC, "Illegal signal level");
        return timer;
    }

    switch (statsType) {
        case StatsUtils::STATS_TYPE_PHONE_ACTIVE: {
            auto onIter = phoneOnTimerMap_.find(level);
            if (onIter != phoneOnTimerMap_.end()) {
                STATS_HILOGD(COMP_SVC, "Get phone on timer for level: %{public}d", level);
                timer = onIter->second;
                break;
            }
            STATS_HILOGD(COMP_SVC, "Create phone on timer for level: %{public}d", level);
            std::shared_ptr<StatsHelper::ActiveTimer> phoneOnTimer = std::make_shared<StatsHelper::ActiveTimer>();
            phoneOnTimerMap_.insert(
                std::pair<int32_t, std::shared_ptr<StatsHelper::ActiveTimer>>(level, phoneOnTimer));
            timer = phoneOnTimer;
            break;
        }
        case StatsUtils::STATS_TYPE_PHONE_DATA: {
            auto dataIter = phoneDataTimerMap_.find(level);
            if (dataIter != phoneDataTimerMap_.end()) {
                STATS_HILOGD(COMP_SVC, "Get phone data timer for level: %{public}d", level);
                timer = dataIter->second;
                break;
            }
            STATS_HILOGD(COMP_SVC, "Create phone data timer for level: %{public}d", level);
            std::shared_ptr<StatsHelper::ActiveTimer> phoneDataTimer = std::make_shared<StatsHelper::ActiveTimer>();
            phoneDataTimerMap_.insert(
                std::pair<int32_t, std::shared_ptr<StatsHelper::ActiveTimer>>(level, phoneDataTimer));
            timer = phoneDataTimer;
            break;
        }
        default:
            STATS_HILOGW(COMP_SVC, "Create phone timer failed");
            break;
    }
    return timer;
}

void PhoneEntity::Reset()
{
    // Reset app Phone total power consumption
    phonePowerMah_ = StatsUtils::DEFAULT_VALUE;

    // Reset Phone on timer
    for (auto& iter : phoneOnTimerMap_) {
        if (iter.second) {
            iter.second->Reset();
        }
    }

    // Reset Phone data timer
    for (auto& iter : phoneDataTimerMap_) {
        if (iter.second) {
            iter.second->Reset();
        }
    }
}

void PhoneEntity::DumpInfo(std::string& result, int32_t uid)
{
    int64_t phoneOnTime = StatsUtils::DEFAULT_VALUE;
    int64_t phoneDataTime = StatsUtils::DEFAULT_VALUE;
    for (int32_t i = 0; i < StatsUtils::RADIO_SIGNAL_BIN; i++) {
        phoneOnTime += GetActiveTimeMs(StatsUtils::STATS_TYPE_PHONE_ACTIVE, i);
        phoneDataTime += GetActiveTimeMs(StatsUtils::STATS_TYPE_PHONE_DATA, i);
    }

    result.append("Phone dump:\n")
        .append("Phone active time: ")
        .append(ToString(phoneOnTime))
        .append("ms\n")
        .append("Phone data time: ")
        .append(ToString(phoneDataTime))
        .append("ms")
        .append("\n");
}
} // namespace PowerMgr
} // namespace OHOS