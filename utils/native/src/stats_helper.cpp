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
#include "stats_helper.h"

#include <ctime>

#include "battery_stats_info.h"

namespace OHOS {
namespace PowerMgr {
int64_t StatsHelper::latestUnplugBootTimeMs_ = StatsUtils::DEFAULT_VALUE;
int64_t StatsHelper::latestUnplugUpTimeMs_ = StatsUtils::DEFAULT_VALUE;
int64_t StatsHelper::onBatteryBootTimeMs_ = StatsUtils::DEFAULT_VALUE;
int64_t StatsHelper::onBatteryUpTimeMs_ = StatsUtils::DEFAULT_VALUE;
bool StatsHelper::onBattery_ = false;
bool StatsHelper::screenOff_ = false;

int64_t StatsHelper::GetBootTimeMs()
{
    int64_t bootTimeMs = StatsUtils::DEFAULT_VALUE;
    struct timespec rawBootTime;
    int errCode = clock_gettime(CLOCK_BOOTTIME, &rawBootTime);
    if (errCode != 0) {
        STATS_HILOGE(COMP_SVC, "Get boot time failed, return default time");
    } else {
        bootTimeMs = static_cast<int64_t>(rawBootTime.tv_sec * StatsUtils::MS_IN_SECOND +
            rawBootTime.tv_nsec / StatsUtils::NS_IN_MS);
        STATS_HILOGD(COMP_SVC, "Get boot time: %{public}" PRId64 "", bootTimeMs);
    }
    return bootTimeMs;
}

int64_t StatsHelper::GetUpTimeMs()
{
    int64_t upTimeMs = StatsUtils::DEFAULT_VALUE;
    struct timespec rawUpTime;
    int errCode = clock_gettime(CLOCK_MONOTONIC, &rawUpTime);
    if (errCode != 0) {
        STATS_HILOGE(COMP_SVC, "Get up time failed, return default time");
    } else {
        upTimeMs = static_cast<int64_t>(rawUpTime.tv_sec * StatsUtils::MS_IN_SECOND +
            rawUpTime.tv_nsec / StatsUtils::NS_IN_MS);
        STATS_HILOGD(COMP_SVC, "Get up time: %{public}" PRId64 "", upTimeMs);
    }
    return upTimeMs;
}

void StatsHelper::SetOnBattery(bool onBattery)
{
    if (onBattery_ != onBattery) {
        onBattery_ = onBattery;
        // when onBattery is ture, status is unplugin.
        int64_t currentBootTimeMs = GetBootTimeMs();
        int64_t currentUpTimeMs = GetUpTimeMs();
        if (onBattery) {
            latestUnplugBootTimeMs_ = currentBootTimeMs;
            latestUnplugUpTimeMs_ = currentUpTimeMs;
        } else {
            onBatteryBootTimeMs_ += currentBootTimeMs - latestUnplugBootTimeMs_;
            onBatteryUpTimeMs_ += currentUpTimeMs - latestUnplugUpTimeMs_;
        }
        STATS_HILOGI(COMP_SVC, "Update battery state:  %{public}d", onBattery);
    }
}

void StatsHelper::SetScreenOff(bool screenOff)
{
    if (screenOff_ != screenOff) {
        screenOff_ = screenOff;
        STATS_HILOGD(COMP_SVC, "Update screen off state: %{public}d", screenOff);
    }
}

bool StatsHelper::IsOnBattery()
{
    return onBattery_;
}

bool StatsHelper::IsOnBatteryScreenOff()
{
    return onBattery_ && screenOff_;
}

int64_t StatsHelper::GetOnBatteryBootTimeMs()
{
    int64_t onBatteryBootTimeMs = onBatteryBootTimeMs_;
    int64_t currentBootTimeMs = GetBootTimeMs();
    if (IsOnBattery()) {
        onBatteryBootTimeMs += currentBootTimeMs - latestUnplugBootTimeMs_;
    }
    STATS_HILOGD(COMP_SVC, "Get on battery boot time: %{public}" PRId64 ", currentBootTimeMs: %{public}" PRId64 "," \
        "latestUnplugBootTimeMs_: %{public}" PRId64 "",
        onBatteryBootTimeMs, currentBootTimeMs, latestUnplugBootTimeMs_);
    return onBatteryBootTimeMs;
}

int64_t StatsHelper::GetOnBatteryUpTimeMs()
{
    int64_t onBatteryUpTimeMs = onBatteryUpTimeMs_;
    int64_t currentUpTimeMs = GetUpTimeMs();
    if (IsOnBattery()) {
        onBatteryUpTimeMs += currentUpTimeMs - latestUnplugUpTimeMs_;
    }
    STATS_HILOGD(COMP_SVC, "Get on battery up time: %{public}" PRId64 "", onBatteryUpTimeMs);
    return onBatteryUpTimeMs;
}
} // namespace PowerMgr
} // namespace OHOS