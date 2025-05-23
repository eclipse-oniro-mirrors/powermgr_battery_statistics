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

#include "stats_utils.h"

#include "stats_log.h"

namespace OHOS {
namespace PowerMgr {
std::string StatsUtils::ConvertTypeForConn(StatsType statsType)
{
    std::string result = "";
    switch (statsType) {
        case STATS_TYPE_BLUETOOTH_BR_ON:
            result = GET_VARIABLE_NAME(STATS_TYPE_BLUETOOTH_BR_ON);
            break;
        case STATS_TYPE_BLUETOOTH_BR_SCAN:
            result = GET_VARIABLE_NAME(STATS_TYPE_BLUETOOTH_BR_SCAN);
            break;
        case STATS_TYPE_BLUETOOTH_BLE_ON:
            result = GET_VARIABLE_NAME(STATS_TYPE_BLUETOOTH_BLE_ON);
            break;
        case STATS_TYPE_BLUETOOTH_BLE_SCAN:
            result = GET_VARIABLE_NAME(STATS_TYPE_BLUETOOTH_BLE_SCAN);
            break;
        case STATS_TYPE_WIFI_ON:
            result = GET_VARIABLE_NAME(STATS_TYPE_WIFI_ON);
            break;
        case STATS_TYPE_WIFI_SCAN:
            result = GET_VARIABLE_NAME(STATS_TYPE_WIFI_SCAN);
            break;
        case STATS_TYPE_PHONE_ACTIVE:
            result = GET_VARIABLE_NAME(STATS_TYPE_PHONE_ACTIVE);
            break;
        case STATS_TYPE_PHONE_DATA:
            result = GET_VARIABLE_NAME(STATS_TYPE_PHONE_DATA);
            break;
        default:
            break;
    }
    STATS_HILOGD(COMP_UTILS, "Convert to %{public}s", result.c_str());
    return result;
}

std::string StatsUtils::ConvertTypeForCpu(StatsType statsType)
{
    std::string result = "";
    switch (statsType) {
        case STATS_TYPE_WAKELOCK_HOLD:
            result = GET_VARIABLE_NAME(STATS_TYPE_WAKELOCK_HOLD);
            break;
        case STATS_TYPE_PHONE_IDLE:
            result = GET_VARIABLE_NAME(STATS_TYPE_PHONE_IDLE);
            break;
        case STATS_TYPE_CPU_CLUSTER:
            result = GET_VARIABLE_NAME(STATS_TYPE_CPU_CLUSTER);
            break;
        case STATS_TYPE_CPU_SPEED:
            result = GET_VARIABLE_NAME(STATS_TYPE_CPU_SPEED);
            break;
        case STATS_TYPE_CPU_ACTIVE:
            result = GET_VARIABLE_NAME(STATS_TYPE_CPU_ACTIVE);
            break;
        case STATS_TYPE_CPU_SUSPEND:
            result = GET_VARIABLE_NAME(STATS_TYPE_CPU_SUSPEND);
            break;
        default:
            break;
    }
    STATS_HILOGD(COMP_UTILS, "Convert to %{public}s", result.c_str());
    return result;
}

std::string StatsUtils::ConvertTypeForCommon(StatsType statsType)
{
    std::string result = "";
    switch (statsType) {
        case STATS_TYPE_CAMERA_ON:
            result = GET_VARIABLE_NAME(STATS_TYPE_CAMERA_ON);
            break;
        case STATS_TYPE_CAMERA_FLASHLIGHT_ON:
            result = GET_VARIABLE_NAME(STATS_TYPE_CAMERA_FLASHLIGHT_ON);
            break;
        case STATS_TYPE_FLASHLIGHT_ON:
            result = GET_VARIABLE_NAME(STATS_TYPE_FLASHLIGHT_ON);
            break;
        case STATS_TYPE_GNSS_ON:
            result = GET_VARIABLE_NAME(STATS_TYPE_GNSS_ON);
            break;
        case STATS_TYPE_SENSOR_GRAVITY_ON:
            result = GET_VARIABLE_NAME(STATS_TYPE_SENSOR_GRAVITY_ON);
            break;
        case STATS_TYPE_SENSOR_PROXIMITY_ON:
            result = GET_VARIABLE_NAME(STATS_TYPE_SENSOR_PROXIMITY_ON);
            break;
        case STATS_TYPE_AUDIO_ON:
            result = GET_VARIABLE_NAME(STATS_TYPE_AUDIO_ON);
            break;
        case STATS_TYPE_SCREEN_ON:
            result = GET_VARIABLE_NAME(STATS_TYPE_SCREEN_ON);
            break;
        case STATS_TYPE_SCREEN_BRIGHTNESS:
            result = GET_VARIABLE_NAME(STATS_TYPE_SCREEN_BRIGHTNESS);
            break;
        case STATS_TYPE_ALARM:
            result = GET_VARIABLE_NAME(STATS_TYPE_ALARM);
            break;
        default:
            break;
    }
    STATS_HILOGD(COMP_UTILS, "Convert to %{public}s", result.c_str());
    return result;
}

std::string StatsUtils::ConvertTypeForDebug(StatsType statsType)
{
    std::string result = "";
    switch (statsType) {
        case STATS_TYPE_BATTERY:
            result = GET_VARIABLE_NAME(STATS_TYPE_BATTERY);
            break;
        case STATS_TYPE_WORKSCHEDULER:
            result = GET_VARIABLE_NAME(STATS_TYPE_WORKSCHEDULER);
            break;
        case STATS_TYPE_THERMAL:
            result = GET_VARIABLE_NAME(STATS_TYPE_THERMAL);
            break;
        case STATS_TYPE_DISTRIBUTEDSCHEDULER:
            result = GET_VARIABLE_NAME(STATS_TYPE_DISTRIBUTEDSCHEDULER);
            break;
        default:
            break;
    }
    STATS_HILOGD(COMP_UTILS, "Convert to %{public}s", result.c_str());
    return result;
}

std::string StatsUtils::ConvertStatsType(StatsType statsType)
{
    std::string result = "";
    switch (statsType) {
        case STATS_TYPE_BLUETOOTH_BR_ON:
        case STATS_TYPE_BLUETOOTH_BR_SCAN:
        case STATS_TYPE_BLUETOOTH_BLE_ON:
        case STATS_TYPE_BLUETOOTH_BLE_SCAN:
        case STATS_TYPE_WIFI_ON:
        case STATS_TYPE_WIFI_SCAN:
        case STATS_TYPE_PHONE_ACTIVE:
        case STATS_TYPE_PHONE_DATA:
            result = ConvertTypeForConn(statsType);
            break;
        case STATS_TYPE_CAMERA_ON:
        case STATS_TYPE_CAMERA_FLASHLIGHT_ON:
        case STATS_TYPE_FLASHLIGHT_ON:
        case STATS_TYPE_GNSS_ON:
        case STATS_TYPE_SENSOR_GRAVITY_ON:
        case STATS_TYPE_SENSOR_PROXIMITY_ON:
        case STATS_TYPE_AUDIO_ON:
        case STATS_TYPE_SCREEN_ON:
        case STATS_TYPE_SCREEN_BRIGHTNESS:
        case STATS_TYPE_ALARM:
            result = ConvertTypeForCommon(statsType);
            break;
        case STATS_TYPE_WAKELOCK_HOLD:
        case STATS_TYPE_PHONE_IDLE:
        case STATS_TYPE_CPU_CLUSTER:
        case STATS_TYPE_CPU_SPEED:
        case STATS_TYPE_CPU_ACTIVE:
        case STATS_TYPE_CPU_SUSPEND:
            result = ConvertTypeForCpu(statsType);
            break;
        case STATS_TYPE_BATTERY:
        case STATS_TYPE_WORKSCHEDULER:
        case STATS_TYPE_THERMAL:
        case STATS_TYPE_DISTRIBUTEDSCHEDULER:
            result = ConvertTypeForDebug(statsType);
            break;
        default:
            break;
    }
    return result;
}

bool StatsUtils::ParseStrtollResult(const std::string& str, int64_t& result)
{
    constexpr int PARAMETER_TEN = 10;
    errno = 0;
    char* endptr = nullptr;
    result = strtoll(str.c_str(), &endptr, PARAMETER_TEN);
    if (endptr == str.c_str()) {
        STATS_HILOGE(COMP_UTILS, "String have no numbers, string:%{public}s", str.c_str());
        return false;
    }
    if (errno == ERANGE && (result == LLONG_MAX || result == LLONG_MIN)) {
        STATS_HILOGE(COMP_UTILS, "Transit result out of range, string:%{public}s", str.c_str());
        return false;
    }
    if (*endptr != '\0') {
        STATS_HILOGE(COMP_UTILS, "String contain non-numeric characters, string:%{public}s", str.c_str());
    }
    return true;
}
} // namespace PowerMgr
} // namespace OHOS