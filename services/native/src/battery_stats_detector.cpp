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

#include "battery_stats_detector.h"

#include <cinttypes>

#include "battery_stats_service.h"

namespace OHOS {
namespace PowerMgr {
void BatteryStatsDetector::HandleStatsChangedEvent(StatsUtils::StatsData data)
{
    STATS_HILOGD(COMP_SVC,
        "Handle type: %{public}s, state: %{public}d, level: %{public}d, uid: %{public}d, pid: %{public}d, "    \
        "eventDataName: %{public}s, eventDataType: %{public}d, eventDataExtra: %{public}d, "                   \
        "time: %{public}" PRId64 ", traffic: %{public}" PRId64 ", deviceId: %{private}s",
        StatsUtils::ConvertStatsType(data.type).c_str(),
        data.state,
        data.level,
        data.uid,
        data.pid,
        data.eventDataName.c_str(),
        data.eventDataType,
        data.eventDataExtra,
        data.time,
        data.traffic,
        data.deviceId.c_str());

    auto bss = BatteryStatsService::GetInstance();
    if (bss == nullptr) {
        STATS_HILOGE(COMP_SVC, "Get battery stats service failed");
        return;
    }
    auto core = bss->GetBatteryStatsCore();
    if (IsDurationRelated(data.type)) {
        // Update related timer with reported time
        // The traffic won't participate the power consumption calculation, just for dump info
        core->UpdateStats(data.type, data.time, data.traffic, data.uid);
    } else if (IsStateRelated(data.type)) {
        // Update related timer based on state or level
        core->UpdateStats(data.type, data.state, data.level, data.uid, data.deviceId);
    }
    HandleDebugInfo(data);
}

bool BatteryStatsDetector::IsDurationRelated(StatsUtils::StatsType type)
{
    bool isMatch = false;
    switch (type) {
        case StatsUtils::STATS_TYPE_WIFI_SCAN:
        case StatsUtils::STATS_TYPE_ALARM:
            // Realated with duration
            isMatch = true;
            break;
        default:
            STATS_HILOGD(COMP_SVC, "No duration related type=%{public}d", static_cast<int32_t>(type));
            break;
    }
    return isMatch;
}

bool BatteryStatsDetector::IsStateRelated(StatsUtils::StatsType type)
{
    bool isMatch = false;
    switch (type) {
        case StatsUtils::STATS_TYPE_SENSOR_GRAVITY_ON:
        case StatsUtils::STATS_TYPE_SENSOR_PROXIMITY_ON:
        case StatsUtils::STATS_TYPE_SCREEN_ON:
        case StatsUtils::STATS_TYPE_SCREEN_BRIGHTNESS:
        case StatsUtils::STATS_TYPE_BLUETOOTH_BR_ON:
        case StatsUtils::STATS_TYPE_BLUETOOTH_BR_SCAN:
        case StatsUtils::STATS_TYPE_BLUETOOTH_BLE_ON:
        case StatsUtils::STATS_TYPE_BLUETOOTH_BLE_SCAN:
        case StatsUtils::STATS_TYPE_WIFI_ON:
        case StatsUtils::STATS_TYPE_PHONE_ACTIVE:
        case StatsUtils::STATS_TYPE_PHONE_DATA:
        case StatsUtils::STATS_TYPE_CAMERA_ON:
        case StatsUtils::STATS_TYPE_CAMERA_FLASHLIGHT_ON:
        case StatsUtils::STATS_TYPE_FLASHLIGHT_ON:
        case StatsUtils::STATS_TYPE_GNSS_ON:
        case StatsUtils::STATS_TYPE_AUDIO_ON:
        case StatsUtils::STATS_TYPE_WAKELOCK_HOLD:
            // Related with state
            isMatch = true;
            break;
        default:
            STATS_HILOGD(COMP_SVC, "No state related type=%{public}d", static_cast<int32_t>(type));
            break;
    }
    return isMatch;
}

void BatteryStatsDetector::HandleThermalInfo(StatsUtils::StatsData data, int64_t bootTimeMs, std::string& debugInfo)
{
    debugInfo.append("Thermal event: Boot time after boot = ")
        .append(ToString(bootTimeMs))
        .append("ms\n");
    if (!data.eventDebugInfo.empty()) {
        debugInfo.append("Additional debug info: ")
            .append(data.eventDebugInfo)
            .append("\n");
    }
}

void BatteryStatsDetector::HandleBatteryInfo(StatsUtils::StatsData data, int64_t bootTimeMs, std::string& debugInfo)
{
    debugInfo.append("Battery event: Battery level = ")
        .append(ToString(data.level))
        .append(", Charger type = ")
        .append(ToString(data.eventDataExtra))
        .append(", boot time after boot = ")
        .append(ToString(bootTimeMs))
        .append("ms\n");
    if (!data.eventDebugInfo.empty()) {
        debugInfo.append("Additional debug info: ")
            .append(data.eventDebugInfo);
    }
}

void BatteryStatsDetector::HandleDisplayInfo(StatsUtils::StatsData data, int64_t bootTimeMs, std::string& debugInfo)
{
    debugInfo.append("Dislpay event: Boot time after boot = ")
        .append(ToString(bootTimeMs))
        .append("ms\n");
    if (!data.eventDebugInfo.empty()) {
        debugInfo.append("Additional debug info: ")
            .append(data.eventDebugInfo)
            .append("\n");
    }
}

void BatteryStatsDetector::HandleWakelockInfo(StatsUtils::StatsData data, int64_t bootTimeMs, std::string& debugInfo)
{
    debugInfo.append("\n")
        .append("Wakelock event: UID = ")
        .append(ToString(data.uid))
        .append(", PID = ")
        .append(ToString(data.pid))
        .append(", wakelock type = ")
        .append(ToString(data.eventDataType))
        .append(", wakelock name = ")
        .append(data.eventDataName)
        .append(", boot time after boot = ")
        .append(ToString(bootTimeMs))
        .append("ms\n");
    if (!data.eventDebugInfo.empty()) {
        debugInfo.append("Additional debug info: ")
            .append(data.eventDebugInfo);
    }
}

void BatteryStatsDetector::HandleWorkschedulerInfo(StatsUtils::StatsData data, int64_t bootTimeMs,
    std::string& debugInfo)
{
    debugInfo.append("WorkScheduler event: UID = ")
        .append(ToString(data.uid))
        .append(", PID = ")
        .append(ToString(data.pid))
        .append(", work type = ")
        .append(ToString(data.eventDataType))
        .append(", work interval = ")
        .append(ToString(data.eventDataExtra))
        .append(", work state = ")
        .append(ToString(data.state))
        .append(", boot time after boot = ")
        .append(ToString(bootTimeMs))
        .append("ms\n");
    if (!data.eventDebugInfo.empty()) {
        debugInfo.append("Additional debug info: ")
            .append(data.eventDebugInfo);
    }
}

void BatteryStatsDetector::HandlePhoneInfo(StatsUtils::StatsData data, int64_t bootTimeMs, std::string& debugInfo)
{
    debugInfo.append("Phone event: Boot time after boot = ")
        .append(ToString(bootTimeMs))
        .append("ms\n");
    if (!data.eventDebugInfo.empty()) {
        debugInfo.append("Additional debug info: ")
            .append(data.eventDebugInfo)
            .append("\n");
    }
}

void BatteryStatsDetector::HandleFlashlightInfo(StatsUtils::StatsData data, int64_t bootTimeMs, std::string& debugInfo)
{
    std::string eventState;
    if (data.state == StatsUtils::STATS_STATE_ACTIVATED) {
        eventState = "ON";
    } else {
        eventState = "OFF";
    }
    debugInfo.append("Flashlight event: UID = ")
        .append(ToString(data.uid))
        .append(", PID = ")
        .append(ToString(data.pid))
        .append(", flashlight state = ")
        .append(eventState)
        .append(", boot time after boot = ")
        .append(ToString(bootTimeMs))
        .append("ms\n");
}

void BatteryStatsDetector::HandleDistributedSchedulerInfo(StatsUtils::StatsData data, int64_t bootTimeMs,
    std::string& debugInfo)
{
    debugInfo.append("Distributed schedule event")
        .append(", boot time after boot = ")
        .append(ToString(bootTimeMs))
        .append("ms\n");
    if (!data.eventDebugInfo.empty()) {
        debugInfo.append("Additional debug info: ")
            .append(data.eventDebugInfo)
            .append("\n");
    }
}

void BatteryStatsDetector::HandleDebugInfo(StatsUtils::StatsData data)
{
    int64_t bootTimeMs = StatsHelper::GetBootTimeMs();
    auto core = BatteryStatsService::GetInstance()->GetBatteryStatsCore();
    std::string debugInfo;
    switch (data.type) {
        case StatsUtils::STATS_TYPE_THERMAL:
            HandleThermalInfo(data, bootTimeMs, debugInfo);
            break;
        case StatsUtils::STATS_TYPE_BATTERY:
            HandleBatteryInfo(data, bootTimeMs, debugInfo);
            break;
        case StatsUtils::STATS_TYPE_WORKSCHEDULER:
            HandleWorkschedulerInfo(data, bootTimeMs, debugInfo);
            break;
        case StatsUtils::STATS_TYPE_WAKELOCK_HOLD:
            HandleWakelockInfo(data, bootTimeMs, debugInfo);
            break;
        case StatsUtils::STATS_TYPE_DISPLAY:
        case StatsUtils::STATS_TYPE_SCREEN_ON:
        case StatsUtils::STATS_TYPE_SCREEN_BRIGHTNESS:
            HandleDisplayInfo(data, bootTimeMs, debugInfo);
            break;
        case StatsUtils::STATS_TYPE_PHONE_ACTIVE:
        case StatsUtils::STATS_TYPE_PHONE_DATA:
            HandlePhoneInfo(data, bootTimeMs, debugInfo);
            break;
        case StatsUtils::STATS_TYPE_FLASHLIGHT_ON:
            HandleFlashlightInfo(data, bootTimeMs, debugInfo);
            break;
        case StatsUtils::STATS_TYPE_DISTRIBUTEDSCHEDULER:
            HandleDistributedSchedulerInfo(data, bootTimeMs, debugInfo);
            break;
        default:
            STATS_HILOGD(COMP_SVC, "Invalid type");
            break;
    }
    core->UpdateDebugInfo(debugInfo);
}
} // namespace PowerMgr
} // namespace OHOS
