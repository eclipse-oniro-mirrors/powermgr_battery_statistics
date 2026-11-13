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

#include "entities/screen_entity.h"

#include <cinttypes>

#include "battery_stats_service.h"
#include "stats_log.h"

namespace OHOS {
namespace PowerMgr {
namespace {
}
ScreenEntity::ScreenEntity()
{
    consumptionType_ = BatteryStatsInfo::CONSUMPTION_TYPE_SCREEN;
}

int64_t ScreenEntity::GetActiveTimeMs(StatsUtils::StatsType statsType, int16_t level)
{
    int64_t activeTimeMs = StatsUtils::DEFAULT_VALUE;
    switch (statsType) {
        case StatsUtils::STATS_TYPE_SCREEN_ON: {
            if (screenOnTimer_ != nullptr) {
                activeTimeMs = screenOnTimer_->GetRunningTimeMs();
                STATS_HILOGD(COMP_SVC, "Get screen on time: %{public}" PRId64 "ms", activeTimeMs);
                break;
            }
            STATS_HILOGD(COMP_SVC, "Didn't find related timer, return 0");
            break;
        }
        case StatsUtils::STATS_TYPE_SCREEN_BRIGHTNESS: {
            if (level != StatsUtils::INVALID_VALUE) {
                auto iter = screenBrightnessTimerMap_.find(level);
                if (iter != screenBrightnessTimerMap_.end() && iter->second != nullptr) {
                    activeTimeMs = iter->second->GetRunningTimeMs();
                    STATS_HILOGD(COMP_SVC,
                        "Get screen brightness time: %{public}" PRId64 "ms of brightness level: %{public}d",
                        activeTimeMs, level);
                    break;
                }
                STATS_HILOGD(COMP_SVC, "No screen brightness timer found, return 0");
                break;
            }
            activeTimeMs = GetBrightnessTotalTimeMs();
            STATS_HILOGD(COMP_SVC, "Get screen brightness total time: %{public}" PRId64 "ms", activeTimeMs);
            break;
        }
        default:
            break;
    }
    return activeTimeMs;
}

int64_t ScreenEntity::GetBrightnessTotalTimeMs()
{
    int64_t totalTimeMs = StatsUtils::DEFAULT_VALUE;
    for (auto timerIter : screenBrightnessTimerMap_) {
        totalTimeMs += timerIter.second->GetRunningTimeMs();
    }
    return totalTimeMs;
}

void ScreenEntity::Calculate(int32_t uid)
{
    auto bss = BatteryStatsService::GetInstance();
    auto screenOnAverageMa =
        bss->GetBatteryStatsParser()->GetAveragePowerMa(StatsUtils::CURRENT_SCREEN_ON);
    auto screenOnTimeMs = GetActiveTimeMs(StatsUtils::STATS_TYPE_SCREEN_ON);
    double screenOnPowerMah = screenOnAverageMa * screenOnTimeMs;

    auto brightnessAverageMa =
        bss->GetBatteryStatsParser()->GetAveragePowerMa(StatsUtils::CURRENT_SCREEN_BRIGHTNESS);
    double brightnessPowerMah = StatsUtils::DEFAULT_VALUE;
    for (auto& iter : screenBrightnessTimerMap_) {
        if (iter.second != nullptr) {
            auto averageMa = brightnessAverageMa * iter.first;
            auto timeMs = GetActiveTimeMs(StatsUtils::STATS_TYPE_SCREEN_BRIGHTNESS, iter.first);
            brightnessPowerMah += averageMa * timeMs;
        }
    }

    screenPowerMah_ = (screenOnPowerMah + brightnessPowerMah) / StatsUtils::MS_IN_HOUR;
    {
        std::lock_guard<std::mutex> statsLock(statsInfoListMutex_);
        totalPowerMah_ += screenPowerMah_;
    }
    std::shared_ptr<BatteryStatsInfo> statsInfo = std::make_shared<BatteryStatsInfo>();
    statsInfo->SetConsumptioType(BatteryStatsInfo::CONSUMPTION_TYPE_SCREEN);
    statsInfo->SetPower(screenPowerMah_);
    UpdateStatsInfoList(statsInfo);
    STATS_HILOGD(COMP_SVC, "Calculate screen active power consumption: %{public}lfmAh", screenPowerMah_);
}

double ScreenEntity::GetEntityPowerMah(int32_t uidOrUserId)
{
    return screenPowerMah_;
}

double ScreenEntity::GetStatsPowerMah(StatsUtils::StatsType statsType, int32_t uid)
{
    return screenPowerMah_;
}

std::shared_ptr<StatsHelper::ActiveTimer> ScreenEntity::GetOrCreateTimer(StatsUtils::StatsType statsType, int16_t level)
{
    std::shared_ptr<StatsHelper::ActiveTimer> timer = nullptr;
    switch (statsType) {
        case StatsUtils::STATS_TYPE_SCREEN_ON: {
            if (screenOnTimer_ != nullptr) {
                STATS_HILOGD(COMP_SVC, "Get screen on timer");
                timer = screenOnTimer_;
                break;
            }
            STATS_HILOGD(COMP_SVC, "Create screen on timer");
            screenOnTimer_ = std::make_shared<StatsHelper::ActiveTimer>();
            timer = screenOnTimer_;
            break;
        }
        case StatsUtils::STATS_TYPE_SCREEN_BRIGHTNESS: {
            if (level <= StatsUtils::INVALID_VALUE || level > StatsUtils::SCREEN_BRIGHTNESS_BIN) {
                STATS_HILOGD(COMP_SVC, "Illegal brightness");
                break;
            }
            auto iter = screenBrightnessTimerMap_.find(level);
            if (iter != screenBrightnessTimerMap_.end() && iter->second != nullptr) {
                STATS_HILOGD(COMP_SVC, "Get screen brightness timer of brightness level: %{public}d", level);
                timer = iter->second;
                break;
            }
            STATS_HILOGD(COMP_SVC, "Create screen brightness timer of brightness level: %{public}d", level);
            std::shared_ptr<StatsHelper::ActiveTimer> brightnessTimer = std::make_shared<StatsHelper::ActiveTimer>();
            screenBrightnessTimerMap_.insert(
                std::pair<int32_t, std::shared_ptr<StatsHelper::ActiveTimer>>(level, brightnessTimer));
            timer = brightnessTimer;
            break;
        }
        default:
            STATS_HILOGW(COMP_SVC, "Create active timer failed");
            break;
    }
    return timer;
}

void ScreenEntity::Reset()
{
    // Reset app Screen total power consumption
    screenPowerMah_ = StatsUtils::DEFAULT_VALUE;

    // Reset Screen on timer
    if (screenOnTimer_ != nullptr) {
        screenOnTimer_->Reset();
    }

    // Reset Screen brightness timer
    for (auto& iter : screenBrightnessTimerMap_) {
        if (iter.second != nullptr) {
            iter.second->Reset();
        }
    }
}

void ScreenEntity::DumpInfo(std::string& result, int32_t uid)
{
    int64_t onTime = GetActiveTimeMs(StatsUtils::STATS_TYPE_SCREEN_ON);
    result.append("Screen dump:\n")
        .append("Screen on time: ")
        .append(ToString(onTime))
        .append("ms")
        .append("\n");
}
} // namespace PowerMgr
} // namespace OHOS