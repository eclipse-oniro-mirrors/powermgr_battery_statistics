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
#include "battery_stats_core.h"

#include <cinttypes>
#include <cstdio>
#include <fstream>
#include <map>
#include <functional>
#include <list>
#include <utility>
#include <vector>

#include <cJSON.h>

#include "ios"
#include "ohos_account_kits.h"

#include "battery_info.h"
#include "battery_srv_client.h"
#include "entities/audio_entity.h"
#include "entities/bluetooth_entity.h"
#include "entities/camera_entity.h"
#include "entities/cpu_entity.h"
#include "entities/flashlight_entity.h"
#include "entities/gnss_entity.h"
#include "entities/idle_entity.h"
#include "entities/phone_entity.h"
#include "entities/screen_entity.h"
#include "entities/sensor_entity.h"
#include "entities/uid_entity.h"
#include "entities/user_entity.h"
#include "entities/wifi_entity.h"
#include "entities/wakelock_entity.h"
#include "entities/alarm_entity.h"
#include "stats_cjson_utils.h"
#include "stats_helper.h"

#include "xcollie/xcollie.h"
#include "xcollie/xcollie_define.h"

namespace OHOS {
namespace PowerMgr {
namespace {
static const std::string BATTERY_STATS_JSON = "/data/service/el0/stats/battery_stats.json";
} // namespace
void BatteryStatsCore::CreatePartEntity()
{
    if (bluetoothEntity_ == nullptr) {
        STATS_HILOGD(COMP_SVC, "Create bluetooth entity");
        bluetoothEntity_ = std::make_shared<BluetoothEntity>();
    }
    if (idleEntity_ == nullptr) {
        STATS_HILOGD(COMP_SVC, "Create idle entity");
        idleEntity_ = std::make_shared<IdleEntity>();
    }
    if (phoneEntity_ == nullptr) {
        STATS_HILOGD(COMP_SVC, "Create phone entity");
        phoneEntity_ = std::make_shared<PhoneEntity>();
    }
    if (screenEntity_ == nullptr) {
        STATS_HILOGD(COMP_SVC, "Create screen entity");
        screenEntity_ = std::make_shared<ScreenEntity>();
    }
    if (wifiEntity_ == nullptr) {
        STATS_HILOGD(COMP_SVC, "Create wifi entity");
        wifiEntity_ = std::make_shared<WifiEntity>();
    }
}

void BatteryStatsCore::CreateAppEntity()
{
    if (audioEntity_ == nullptr) {
        STATS_HILOGD(COMP_SVC, "Create audio entity");
        audioEntity_ = std::make_shared<AudioEntity>();
    }
    if (cameraEntity_ == nullptr) {
        STATS_HILOGD(COMP_SVC, "Create camera entity");
        cameraEntity_ = std::make_shared<CameraEntity>();
    }
    if (flashlightEntity_ == nullptr) {
        STATS_HILOGD(COMP_SVC, "Create flashlight entity");
        flashlightEntity_ = std::make_shared<FlashlightEntity>();
    }
    if (gnssEntity_ == nullptr) {
        STATS_HILOGD(COMP_SVC, "Create gnss entity");
        gnssEntity_ = std::make_shared<GnssEntity>();
    }
    if (sensorEntity_ == nullptr) {
        STATS_HILOGD(COMP_SVC, "Create sensor entity");
        sensorEntity_ = std::make_shared<SensorEntity>();
    }
    if (uidEntity_ == nullptr) {
        STATS_HILOGD(COMP_SVC, "Create uid entity");
        uidEntity_ = std::make_shared<UidEntity>();
    }
    if (userEntity_ == nullptr) {
        STATS_HILOGD(COMP_SVC, "Create user entity");
        userEntity_ = std::make_shared<UserEntity>();
    }
    if (wakelockEntity_ == nullptr) {
        STATS_HILOGD(COMP_SVC, "Create wakelock entity");
        wakelockEntity_ = std::make_shared<WakelockEntity>();
    }
    if (cpuEntity_ == nullptr) {
        STATS_HILOGD(COMP_SVC, "Create cpu entity");
        cpuEntity_ = std::make_shared<CpuEntity>();
    }
    if (alarmEntity_ == nullptr) {
        STATS_HILOGD(COMP_SVC, "Create alarm entity");
        alarmEntity_ = std::make_shared<AlarmEntity>();
    }
}

bool BatteryStatsCore::Init()
{
    STATS_HILOGI(COMP_SVC, "Battery stats core init");
    CreateAppEntity();
    CreatePartEntity();
    auto& batterySrvClient = BatterySrvClient::GetInstance();
    BatteryPluggedType plugType = batterySrvClient.GetPluggedType();
    if (plugType == BatteryPluggedType::PLUGGED_TYPE_NONE || plugType == BatteryPluggedType::PLUGGED_TYPE_BUTT) {
        StatsHelper::SetOnBattery(true);
    } else {
        StatsHelper::SetOnBattery(false);
    }

    if (!LoadBatteryStatsData()) {
        STATS_HILOGW(COMP_SVC, "Load battery stats data failed");
    }
    return true;
}

void BatteryStatsCore::ComputePower()
{
    std::lock_guard lock(mutex_);
    STATS_HILOGD(COMP_SVC, "Calculate battery stats");
    const uint32_t DFX_DELAY_S = 60;
    int id = HiviewDFX::XCollie::GetInstance().SetTimer("BatteryStatsCoreComputePower", DFX_DELAY_S, nullptr, nullptr,
        HiviewDFX::XCOLLIE_FLAG_LOG);

    BatteryStatsEntity::ResetStatsEntity();
    uidEntity_->Calculate();
    bluetoothEntity_->Calculate();
    idleEntity_->Calculate();
    phoneEntity_->Calculate();
    screenEntity_->Calculate();
    wifiEntity_->Calculate();
    userEntity_->Calculate();

    HiviewDFX::XCollie::GetInstance().CancelTimer(id);
}

BatteryStatsInfoList BatteryStatsCore::GetBatteryStats()
{
    std::lock_guard lock(mutex_);
    return BatteryStatsEntity::GetStatsInfoList();
}

std::shared_ptr<BatteryStatsEntity> BatteryStatsCore::GetEntity(const BatteryStatsInfo::ConsumptionType& type)
{
    STATS_HILOGD(COMP_SVC, "Get %{public}s entity", BatteryStatsInfo::ConvertConsumptionType(type).c_str());
    switch (type) {
        case BatteryStatsInfo::CONSUMPTION_TYPE_APP:
            return uidEntity_;
        case BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH:
            return bluetoothEntity_;
        case BatteryStatsInfo::CONSUMPTION_TYPE_IDLE:
            return idleEntity_;
        case BatteryStatsInfo::CONSUMPTION_TYPE_PHONE:
            return phoneEntity_;
        case BatteryStatsInfo::CONSUMPTION_TYPE_SCREEN:
            return screenEntity_;
        case BatteryStatsInfo::CONSUMPTION_TYPE_USER:
            return userEntity_;
        case BatteryStatsInfo::CONSUMPTION_TYPE_WIFI:
            return wifiEntity_;
        case BatteryStatsInfo::CONSUMPTION_TYPE_CAMERA:
            return cameraEntity_;
        case BatteryStatsInfo::CONSUMPTION_TYPE_FLASHLIGHT:
            return flashlightEntity_;
        case BatteryStatsInfo::CONSUMPTION_TYPE_AUDIO:
            return audioEntity_;
        case BatteryStatsInfo::CONSUMPTION_TYPE_SENSOR:
            return sensorEntity_;
        case BatteryStatsInfo::CONSUMPTION_TYPE_GNSS:
            return gnssEntity_;
        case BatteryStatsInfo::CONSUMPTION_TYPE_CPU:
            return cpuEntity_;
        case BatteryStatsInfo::CONSUMPTION_TYPE_WAKELOCK:
            return wakelockEntity_;
        case BatteryStatsInfo::CONSUMPTION_TYPE_ALARM:
            return alarmEntity_;
        case BatteryStatsInfo::CONSUMPTION_TYPE_INVALID:
        default:
            return nullptr;
    }
}

void BatteryStatsCore::UpdateStats(StatsUtils::StatsType statsType, int64_t time, int64_t data, int32_t uid)
{
    STATS_HILOGD(COMP_SVC,
        "Update for duration, statsType: %{public}s, uid: %{public}d, time: %{public}" PRId64 ", "  \
        "data: %{public}" PRId64 "",
        StatsUtils::ConvertStatsType(statsType).c_str(), uid, time, data);
    if (uid > StatsUtils::INVALID_VALUE) {
        uidEntity_->UpdateUidMap(uid);
    }

    switch (statsType) {
        case StatsUtils::STATS_TYPE_WIFI_SCAN:
            UpdateCounter(wifiEntity_, statsType, data, uid);
            break;
        case StatsUtils::STATS_TYPE_ALARM:
            UpdateCounter(alarmEntity_, statsType, data, uid);
            break;
        default:
            break;
    }
}

void BatteryStatsCore::UpdateConnectivityStats(StatsUtils::StatsType statsType, StatsUtils::StatsState state,
    int32_t uid)
{
    switch (statsType) {
        case StatsUtils::STATS_TYPE_BLUETOOTH_BR_ON:
        case StatsUtils::STATS_TYPE_BLUETOOTH_BLE_ON:
            UpdateTimer(bluetoothEntity_, statsType, state);
            break;
        case StatsUtils::STATS_TYPE_WIFI_ON:
            UpdateTimer(wifiEntity_, statsType, state);
            break;
        case StatsUtils::STATS_TYPE_BLUETOOTH_BR_SCAN:
        case StatsUtils::STATS_TYPE_BLUETOOTH_BLE_SCAN:
            UpdateTimer(bluetoothEntity_, statsType, state, uid);
            break;
        default:
            break;
    }
}

void BatteryStatsCore::UpdateCommonStats(StatsUtils::StatsType statsType, StatsUtils::StatsState state, int32_t uid)
{
    switch (statsType) {
        case StatsUtils::STATS_TYPE_FLASHLIGHT_ON:
            UpdateTimer(flashlightEntity_, statsType, state, uid);
            break;
        case StatsUtils::STATS_TYPE_GNSS_ON:
            UpdateTimer(gnssEntity_, statsType, state, uid);
            break;
        case StatsUtils::STATS_TYPE_SENSOR_GRAVITY_ON:
            UpdateTimer(sensorEntity_, statsType, state, uid);
            break;
        case StatsUtils::STATS_TYPE_SENSOR_PROXIMITY_ON:
            UpdateTimer(sensorEntity_, statsType, state, uid);
            break;
        case StatsUtils::STATS_TYPE_AUDIO_ON:
            UpdateTimer(audioEntity_, statsType, state, uid);
            break;
        case StatsUtils::STATS_TYPE_WAKELOCK_HOLD:
            UpdateTimer(wakelockEntity_, statsType, state, uid);
            break;
        default:
            break;
    }
}

void BatteryStatsCore::UpdateStats(StatsUtils::StatsType statsType, StatsUtils::StatsState state, int16_t level,
    int32_t uid, const std::string& deviceId)
{
    STATS_HILOGD(COMP_SVC,
        "Update for state, statsType: %{public}s, uid: %{public}d, state: %{public}d, level: %{public}d,"   \
        "deviceId: %{private}s",
        StatsUtils::ConvertStatsType(statsType).c_str(), uid, state, level, deviceId.c_str());
    if (uid > StatsUtils::INVALID_VALUE) {
        uidEntity_->UpdateUidMap(uid);
    }

    switch (statsType) {
        case StatsUtils::STATS_TYPE_SCREEN_ON:
        case StatsUtils::STATS_TYPE_SCREEN_BRIGHTNESS:
            UpdateScreenStats(statsType, state, level);
            break;
        case StatsUtils::STATS_TYPE_CAMERA_ON:
        case StatsUtils::STATS_TYPE_CAMERA_FLASHLIGHT_ON:
            UpdateCameraStats(statsType, state, uid, deviceId);
            break;
        case StatsUtils::STATS_TYPE_PHONE_ACTIVE:
        case StatsUtils::STATS_TYPE_PHONE_DATA:
            UpdatePhoneStats(statsType, state, level);
            break;
        case StatsUtils::STATS_TYPE_BLUETOOTH_BR_ON:
        case StatsUtils::STATS_TYPE_BLUETOOTH_BLE_ON:
        case StatsUtils::STATS_TYPE_WIFI_ON:
        case StatsUtils::STATS_TYPE_BLUETOOTH_BR_SCAN:
        case StatsUtils::STATS_TYPE_BLUETOOTH_BLE_SCAN:
            UpdateConnectivityStats(statsType, state, uid);
            break;
        case StatsUtils::STATS_TYPE_FLASHLIGHT_ON:
        case StatsUtils::STATS_TYPE_GNSS_ON:
        case StatsUtils::STATS_TYPE_SENSOR_GRAVITY_ON:
        case StatsUtils::STATS_TYPE_SENSOR_PROXIMITY_ON:
        case StatsUtils::STATS_TYPE_AUDIO_ON:
        case StatsUtils::STATS_TYPE_WAKELOCK_HOLD:
            UpdateCommonStats(statsType, state, uid);
            break;
        default:
            break;
    }
}

void BatteryStatsCore::UpdateScreenStats(StatsUtils::StatsType statsType, StatsUtils::StatsState state, int16_t level)
{
    STATS_HILOGD(COMP_SVC,
        "statsType: %{public}s, state: %{public}d, level: %{public}d, last brightness level: %{public}d",
        StatsUtils::ConvertStatsType(statsType).c_str(), state, level, lastBrightnessLevel_);
    if (statsType == StatsUtils::STATS_TYPE_SCREEN_ON) {
        UpdateScreenTimer(state);
    } else if (statsType == StatsUtils::STATS_TYPE_SCREEN_BRIGHTNESS) {
        if (!isScreenOn_) {
            STATS_HILOGD(COMP_SVC, "Screen is off, return");
            return;
        }
        UpdateBrightnessTimer(state, level);
    }
}

void BatteryStatsCore::UpdateCameraStats(StatsUtils::StatsType statsType, StatsUtils::StatsState state,
    int32_t uid, const std::string& deviceId)
{
    STATS_HILOGD(COMP_SVC, "Camera status: %{public}d, Last camera uid: %{public}d", isCameraOn_, lastCameraUid_);
    if (statsType == StatsUtils::STATS_TYPE_CAMERA_ON) {
        if (state == StatsUtils::STATS_STATE_ACTIVATED) {
            if (isCameraOn_) {
                STATS_HILOGW(COMP_SVC, "Camera is already opened, return");
                return;
            }
            UpdateCameraTimer(state, uid, deviceId);
        } else if (state == StatsUtils::STATS_STATE_DEACTIVATED) {
            if (!isCameraOn_) {
                STATS_HILOGW(COMP_SVC, "Camera is off, return");
                return;
            }
            UpdateCameraTimer(state, lastCameraUid_, deviceId);
        }
    } else if (statsType == StatsUtils::STATS_TYPE_CAMERA_FLASHLIGHT_ON) {
        if (!isCameraOn_) {
            STATS_HILOGW(COMP_SVC, "Camera is off, return");
            return;
        }
        UpdateTimer(flashlightEntity_, StatsUtils::STATS_TYPE_FLASHLIGHT_ON, state, lastCameraUid_);
    }
}

void BatteryStatsCore::UpdatePhoneStats(StatsUtils::StatsType statsType, StatsUtils::StatsState state, int16_t level)
{
    STATS_HILOGD(COMP_SVC, "statsType: %{public}s, state: %{public}d, level: %{public}d",
        StatsUtils::ConvertStatsType(statsType).c_str(), state, level);
    std::shared_ptr<StatsHelper::ActiveTimer> timer;
    timer = phoneEntity_->GetOrCreateTimer(statsType, level);
    if (timer == nullptr) {
        STATS_HILOGW(COMP_SVC, "Timer is null, return");
        return;
    }

    switch (state) {
        case StatsUtils::STATS_STATE_ACTIVATED:
            timer->StartRunning();
            break;
        case StatsUtils::STATS_STATE_DEACTIVATED:
            timer->StopRunning();
            break;
        default:
            break;
    }
}

void BatteryStatsCore::UpdateTimer(std::shared_ptr<BatteryStatsEntity> entity, StatsUtils::StatsType statsType,
    StatsUtils::StatsState state, int32_t uid)
{
    STATS_HILOGD(COMP_SVC,
        "entity: %{public}s, statsType: %{public}s, state: %{public}d, uid: %{public}d",
        BatteryStatsInfo::ConvertConsumptionType(entity->GetConsumptionType()).c_str(),
        StatsUtils::ConvertStatsType(statsType).c_str(),
        state,
        uid);
    std::shared_ptr<StatsHelper::ActiveTimer> timer;
    if (uid > StatsUtils::INVALID_VALUE) {
        timer = entity->GetOrCreateTimer(uid, statsType);
    } else {
        timer = entity->GetOrCreateTimer(statsType);
    }

    if (timer == nullptr) {
        STATS_HILOGW(COMP_SVC, "Timer is null, return");
        return;
    }

    switch (state) {
        case StatsUtils::STATS_STATE_ACTIVATED:
            timer->StartRunning();
            break;
        case StatsUtils::STATS_STATE_DEACTIVATED:
            timer->StopRunning();
            break;
        default:
            break;
    }
}

void BatteryStatsCore::UpdateTimer(std::shared_ptr<BatteryStatsEntity> entity, StatsUtils::StatsType statsType,
    int64_t time, int32_t uid)
{
    STATS_HILOGD(COMP_SVC,
        "entity: %{public}s, statsType: %{public}s, time: %{public}" PRId64 ", uid: %{public}d",
        BatteryStatsInfo::ConvertConsumptionType(entity->GetConsumptionType()).c_str(),
        StatsUtils::ConvertStatsType(statsType).c_str(),
        time,
        uid);
    std::shared_ptr<StatsHelper::ActiveTimer> timer;
    if (uid > StatsUtils::INVALID_VALUE) {
        timer = entity->GetOrCreateTimer(uid, statsType);
    } else {
        timer = entity->GetOrCreateTimer(statsType);
    }

    if (timer == nullptr) {
        STATS_HILOGW(COMP_SVC, "Timer is null, return");
        return;
    }
    timer->AddRunningTimeMs(time);
}

void BatteryStatsCore::UpdateCameraTimer(StatsUtils::StatsState state, int32_t uid, const std::string& deviceId)
{
    STATS_HILOGD(COMP_SVC, "Camera status: %{public}d, uid: %{public}d, deviceId: %{private}s",
        state, uid, deviceId.c_str());
    std::shared_ptr<StatsHelper::ActiveTimer> timer;
    if (uid > StatsUtils::INVALID_VALUE && deviceId != "") {
        timer = cameraEntity_->GetOrCreateTimer(deviceId, uid, StatsUtils::STATS_TYPE_CAMERA_ON);
    } else {
        timer = cameraEntity_->GetOrCreateTimer(StatsUtils::STATS_TYPE_CAMERA_ON);
    }

    if (timer == nullptr) {
        STATS_HILOGW(COMP_SVC, "Timer is null, return");
        return;
    }

    switch (state) {
        case StatsUtils::STATS_STATE_ACTIVATED: {
            if (timer->StartRunning()) {
                isCameraOn_ = true;
                lastCameraUid_ = uid;
            }
            break;
        }
        case StatsUtils::STATS_STATE_DEACTIVATED: {
            if (timer->StopRunning()) {
                UpdateTimer(flashlightEntity_,
                            StatsUtils::STATS_TYPE_FLASHLIGHT_ON,
                            StatsUtils::STATS_STATE_DEACTIVATED,
                            lastCameraUid_);
                isCameraOn_ = false;
                lastCameraUid_ = StatsUtils::INVALID_VALUE;
            }
            break;
        }
        default:
            break;
    }
}

void BatteryStatsCore::UpdateScreenTimer(StatsUtils::StatsState state)
{
    std::shared_ptr<StatsHelper::ActiveTimer> screenOnTimer = nullptr;
    std::shared_ptr<StatsHelper::ActiveTimer> brightnessTimer = nullptr;
    screenOnTimer = screenEntity_->GetOrCreateTimer(StatsUtils::STATS_TYPE_SCREEN_ON);
    if (lastBrightnessLevel_ > StatsUtils::INVALID_VALUE) {
        brightnessTimer = screenEntity_->GetOrCreateTimer(StatsUtils::STATS_TYPE_SCREEN_BRIGHTNESS,
            lastBrightnessLevel_);
    }
    if (state == StatsUtils::STATS_STATE_ACTIVATED) {
        if (screenOnTimer != nullptr) {
            screenOnTimer->StartRunning();
        }
        if (brightnessTimer != nullptr) {
            brightnessTimer->StartRunning();
        }
        isScreenOn_ = true;
    } else if (state == StatsUtils::STATS_STATE_DEACTIVATED) {
        if (screenOnTimer != nullptr) {
            screenOnTimer->StopRunning();
        }
        if (brightnessTimer != nullptr) {
            brightnessTimer->StopRunning();
        }
        isScreenOn_ = false;
    }
}

void BatteryStatsCore::UpdateBrightnessTimer(StatsUtils::StatsState state, int16_t level)
{
    if (level <= StatsUtils::INVALID_VALUE || level > StatsUtils::SCREEN_BRIGHTNESS_BIN) {
        STATS_HILOGW(COMP_SVC, "Screen brightness level is out of range");
        return;
    }

    if (lastBrightnessLevel_ <= StatsUtils::INVALID_VALUE ||
        (level > StatsUtils::INVALID_VALUE && level == lastBrightnessLevel_)) {
        auto brightnessTimer = screenEntity_->GetOrCreateTimer(StatsUtils::STATS_TYPE_SCREEN_BRIGHTNESS,
            level);
        if (brightnessTimer != nullptr) {
            brightnessTimer->StartRunning();
        }
    } else if (level != lastBrightnessLevel_) {
        auto oldBrightnessTimer = screenEntity_->GetOrCreateTimer(StatsUtils::STATS_TYPE_SCREEN_BRIGHTNESS,
            lastBrightnessLevel_);
        auto newBrightnessTimer = screenEntity_->GetOrCreateTimer(StatsUtils::STATS_TYPE_SCREEN_BRIGHTNESS, level);
        if (oldBrightnessTimer != nullptr) {
            STATS_HILOGI(COMP_SVC, "Stop screen brightness timer for last level: %{public}d",
                lastBrightnessLevel_);
            oldBrightnessTimer->StopRunning();
        }
        if (newBrightnessTimer != nullptr) {
            STATS_HILOGI(COMP_SVC, "Start screen brightness timer for latest level: %{public}d", level);
            newBrightnessTimer->StartRunning();
        }
    }
    lastBrightnessLevel_ = level;
}

void BatteryStatsCore::UpdateCounter(std::shared_ptr<BatteryStatsEntity> entity, StatsUtils::StatsType statsType,
    int64_t data, int32_t uid)
{
    STATS_HILOGD(COMP_SVC,
        "entity: %{public}s, statsType: %{public}s, data: %{public}" PRId64 ", uid: %{public}d",
        BatteryStatsInfo::ConvertConsumptionType(entity->GetConsumptionType()).c_str(),
        StatsUtils::ConvertStatsType(statsType).c_str(),
        data,
        uid);
    std::shared_ptr<StatsHelper::Counter> counter;
    if (uid > StatsUtils::INVALID_VALUE) {
        counter = entity->GetOrCreateCounter(statsType, uid);
    } else {
        counter = entity->GetOrCreateCounter(statsType);
    }

    if (counter == nullptr) {
        STATS_HILOGW(COMP_SVC, "Counter is null, return");
        return;
    }
    counter->AddCount(data);
}

int64_t BatteryStatsCore::GetTotalTimeMs(StatsUtils::StatsType statsType, int16_t level)
{
    STATS_HILOGD(COMP_SVC, "Handle statsType: %{public}s, level: %{public}d",
        StatsUtils::ConvertStatsType(statsType).c_str(), level);
    int64_t time = StatsUtils::DEFAULT_VALUE;
    switch (statsType) {
        case StatsUtils::STATS_TYPE_SCREEN_BRIGHTNESS:
            time = screenEntity_->GetActiveTimeMs(statsType, level);
            break;
        case StatsUtils::STATS_TYPE_SCREEN_ON:
            time = screenEntity_->GetActiveTimeMs(statsType);
            break;
        case StatsUtils::STATS_TYPE_BLUETOOTH_BR_ON:
        case StatsUtils::STATS_TYPE_BLUETOOTH_BLE_ON:
            time = bluetoothEntity_->GetActiveTimeMs(statsType);
            break;
        case StatsUtils::STATS_TYPE_WIFI_ON:
            time = wifiEntity_->GetActiveTimeMs(statsType);
            break;
        case StatsUtils::STATS_TYPE_PHONE_ACTIVE:
        case StatsUtils::STATS_TYPE_PHONE_DATA:
            time = phoneEntity_->GetActiveTimeMs(statsType, level);
            break;
        case StatsUtils::STATS_TYPE_PHONE_IDLE:
        case StatsUtils::STATS_TYPE_CPU_SUSPEND:
            time = idleEntity_->GetActiveTimeMs(statsType);
            break;
        default:
            break;
    }
    STATS_HILOGD(COMP_SVC, "Get active time: %{public}sms for %{public}s", std::to_string(time).c_str(),
        StatsUtils::ConvertStatsType(statsType).c_str());
    return time;
}

void BatteryStatsCore::DumpInfo(std::string& result)
{
    result.append("BATTERY STATS DUMP:\n");
    result.append("\n");
    if (bluetoothEntity_) {
        bluetoothEntity_->DumpInfo(result);
        result.append("\n");
    }
    if (idleEntity_) {
        idleEntity_->DumpInfo(result);
        result.append("\n");
    }
    if (phoneEntity_) {
        phoneEntity_->DumpInfo(result);
        result.append("\n");
    }
    if (screenEntity_) {
        screenEntity_->DumpInfo(result);
        result.append("\n");
    }
    if (wifiEntity_) {
        wifiEntity_->DumpInfo(result);
        result.append("\n");
    }
    if (uidEntity_) {
        uidEntity_->DumpInfo(result);
        result.append("\n");
    }
    GetDebugInfo(result);
}

void BatteryStatsCore::UpdateDebugInfo(const std::string& info)
{
    std::lock_guard lock(mutex_);
    debugInfo_.append(info);
}

void BatteryStatsCore::GetDebugInfo(std::string& result)
{
    std::lock_guard lock(mutex_);
    if (debugInfo_.size() > 0) {
        result.append("Misc stats info dump:\n");
        result.append(debugInfo_);
    }
}

int64_t BatteryStatsCore::GetTotalTimeMs(int32_t uid, StatsUtils::StatsType statsType, int16_t level)
{
    STATS_HILOGD(COMP_SVC, "Handle statsType: %{public}s, uid: %{public}d, level: %{public}d",
        StatsUtils::ConvertStatsType(statsType).c_str(), uid, level);
    int64_t time = StatsUtils::DEFAULT_VALUE;
    switch (statsType) {
        case StatsUtils::STATS_TYPE_BLUETOOTH_BR_SCAN:
        case StatsUtils::STATS_TYPE_BLUETOOTH_BLE_SCAN:
            time = bluetoothEntity_->GetActiveTimeMs(uid, statsType);
            break;
        case StatsUtils::STATS_TYPE_CAMERA_ON:
            time = cameraEntity_->GetActiveTimeMs(uid, statsType);
            break;
        case StatsUtils::STATS_TYPE_FLASHLIGHT_ON:
            time = flashlightEntity_->GetActiveTimeMs(uid, statsType);
            break;
        case StatsUtils::STATS_TYPE_GNSS_ON:
            time = gnssEntity_->GetActiveTimeMs(uid, statsType);
            break;
        case StatsUtils::STATS_TYPE_SENSOR_GRAVITY_ON:
            time = sensorEntity_->GetActiveTimeMs(uid, statsType);
            break;
        case StatsUtils::STATS_TYPE_SENSOR_PROXIMITY_ON:
            time = sensorEntity_->GetActiveTimeMs(uid, statsType);
            break;
        case StatsUtils::STATS_TYPE_AUDIO_ON:
            time = audioEntity_->GetActiveTimeMs(uid, statsType);
            break;
        case StatsUtils::STATS_TYPE_WAKELOCK_HOLD:
            time = wakelockEntity_->GetActiveTimeMs(uid, statsType);
            break;
        case StatsUtils::STATS_TYPE_CPU_CLUSTER:
        case StatsUtils::STATS_TYPE_CPU_SPEED:
        case StatsUtils::STATS_TYPE_CPU_ACTIVE:
            time = cpuEntity_->GetCpuTimeMs(uid);
            break;
        default:
            break;
    }
    STATS_HILOGD(COMP_SVC, "Get active time: %{public}sms for %{public}s of uid: %{public}d",
        std::to_string(time).c_str(), StatsUtils::ConvertStatsType(statsType).c_str(), uid);
    return time;
}

int64_t BatteryStatsCore::GetTotalDataCount(StatsUtils::StatsType statsType, int32_t uid)
{
    STATS_HILOGD(COMP_SVC, "no traffic data bytes of %{public}s for uid: %{public}d",
        StatsUtils::ConvertStatsType(statsType).c_str(), uid);
    return StatsUtils::DEFAULT_VALUE;
}

int64_t BatteryStatsCore::GetTotalConsumptionCount(StatsUtils::StatsType statsType, int32_t uid)
{
    int64_t data = StatsUtils::DEFAULT_VALUE;
    switch (statsType) {
        case StatsUtils::STATS_TYPE_WIFI_SCAN:
            data = wifiEntity_->GetConsumptionCount(statsType, uid);
            break;
        case StatsUtils::STATS_TYPE_ALARM:
            data = alarmEntity_->GetConsumptionCount(statsType, uid);
            break;
        default:
            break;
    }
    STATS_HILOGD(COMP_SVC, "Get consumption count: %{public}" PRId64 " of %{public}s for uid: %{public}d",
        data, StatsUtils::ConvertStatsType(statsType).c_str(), uid);
    return data;
}

double BatteryStatsCore::GetAppStatsMah(const int32_t& uid)
{
    double appStatsMah = StatsUtils::DEFAULT_VALUE;
    auto statsInfoList = GetBatteryStats();
    for (auto iter = statsInfoList.begin(); iter != statsInfoList.end(); iter++) {
        if ((*iter)->GetConsumptionType() == BatteryStatsInfo::CONSUMPTION_TYPE_APP) {
            if ((*iter)->GetUid() == uid) {
                appStatsMah = (*iter)->GetPower();
                break;
            }
        }
    }
    STATS_HILOGD(COMP_SVC, "Get stats mah: %{public}lf for uid: %{public}d", appStatsMah, uid);
    return appStatsMah;
}

double BatteryStatsCore::GetAppStatsPercent(const int32_t& uid)
{
    double appStatsPercent = StatsUtils::DEFAULT_VALUE;
    auto statsInfoList = GetBatteryStats();
    auto totalConsumption = BatteryStatsEntity::GetTotalPowerMah();
    if (totalConsumption <= StatsUtils::DEFAULT_VALUE) {
        STATS_HILOGW(COMP_SVC, "No consumption got, return 0");
        return appStatsPercent;
    }
    for (auto iter = statsInfoList.begin(); iter != statsInfoList.end(); iter++) {
        if ((*iter)->GetConsumptionType() == BatteryStatsInfo::CONSUMPTION_TYPE_APP) {
            if ((*iter)->GetUid() == uid && totalConsumption != StatsUtils::DEFAULT_VALUE) {
                appStatsPercent = (*iter)->GetPower() / totalConsumption;
                break;
            }
        }
    }
    STATS_HILOGD(COMP_SVC, "Get stats percent: %{public}lf for uid: %{public}d", appStatsPercent, uid);
    return appStatsPercent;
}

double BatteryStatsCore::GetPartStatsMah(const BatteryStatsInfo::ConsumptionType& type)
{
    double partStatsMah = StatsUtils::DEFAULT_VALUE;
    auto statsInfoList = GetBatteryStats();
    for (auto iter = statsInfoList.begin(); iter != statsInfoList.end(); iter++) {
        if ((*iter)->GetConsumptionType() == type) {
            partStatsMah = (*iter)->GetPower();
            break;
        }
    }
    STATS_HILOGD(COMP_SVC, "Get stats mah: %{public}lf for type: %{public}d", partStatsMah, type);
    return partStatsMah;
}

double BatteryStatsCore::GetPartStatsPercent(const BatteryStatsInfo::ConsumptionType& type)
{
    double partStatsPercent = StatsUtils::DEFAULT_VALUE;
    auto statsInfoList = GetBatteryStats();
    auto totalConsumption = BatteryStatsEntity::GetTotalPowerMah();
    for (auto iter = statsInfoList.begin(); iter != statsInfoList.end(); iter++) {
        if ((*iter)->GetConsumptionType() == type && totalConsumption != StatsUtils::DEFAULT_VALUE) {
            partStatsPercent = (*iter)->GetPower() / totalConsumption;
            break;
        }
    }
    STATS_HILOGD(COMP_SVC, "Get stats percent: %{public}lf for type: %{public}d", partStatsPercent, type);
    return partStatsPercent;
}

void BatteryStatsCore::SaveForHardware(cJSON* root)
{
    STATS_HILOGD(COMP_SVC, "Save hardware battery stats");
    cJSON* hardwareObj = cJSON_CreateObject();
    if (!hardwareObj) {
        STATS_HILOGE(COMP_SVC, "Failed to create 'Hardware' object");
        return;
    }
    if (!cJSON_AddItemToObject(root, "Hardware", hardwareObj)) {
        cJSON_Delete(hardwareObj);
        STATS_HILOGW(COMP_SVC, "Add Hardware object to root failed.");
        return;
    }
    // Save for Bluetooth
    if (cJSON_AddNumberToObject(hardwareObj, "bluetooth_br_on",
        GetTotalTimeMs(StatsUtils::STATS_TYPE_BLUETOOTH_BR_ON)) == nullptr) {
        STATS_HILOGW(COMP_SVC, "Add bluetooth_br_on to Hardware failed.");
    }
    if (cJSON_AddNumberToObject(hardwareObj, "bluetooth_ble_on",
        GetTotalTimeMs(StatsUtils::STATS_TYPE_BLUETOOTH_BLE_ON)) == nullptr) {
        STATS_HILOGW(COMP_SVC, "Add bluetooth_ble_on to Hardware failed.");
    }
    // Save for Screen
    if (cJSON_AddNumberToObject(hardwareObj, "screen_on",
        GetTotalTimeMs(StatsUtils::STATS_TYPE_SCREEN_ON)) == nullptr) {
        STATS_HILOGW(COMP_SVC, "Add screen_on to Hardware failed.");
    }
    cJSON* screenBrightnessArray = cJSON_CreateArray();
    if (screenBrightnessArray) {
        for (uint16_t brightness = 0; brightness <= StatsUtils::SCREEN_BRIGHTNESS_BIN; brightness++) {
            if (!cJSON_AddItemToArray(screenBrightnessArray,
                cJSON_CreateNumber(GetTotalTimeMs(StatsUtils::STATS_TYPE_SCREEN_BRIGHTNESS, brightness)))) {
                STATS_HILOGW(COMP_SVC, "Add screen_brightness array failed.");
            }
        }
        if (!cJSON_AddItemToObject(hardwareObj, "screen_brightness", screenBrightnessArray)) {
            cJSON_Delete(screenBrightnessArray);
            STATS_HILOGW(COMP_SVC, "Add screen_brightness to Hardware failed.");
        }
    }
    // Save for Wifi
    if (cJSON_AddNumberToObject(hardwareObj, "wifi_on", GetTotalTimeMs(StatsUtils::STATS_TYPE_WIFI_ON)) == nullptr) {
        STATS_HILOGW(COMP_SVC, "Add wifi_on to Hardware failed.");
    }
    if (cJSON_AddNumberToObject(hardwareObj, "wifi_scan",
        GetTotalConsumptionCount(StatsUtils::STATS_TYPE_WIFI_SCAN)) == nullptr) {
        STATS_HILOGW(COMP_SVC, "Add wifi_scan to Hardware failed.");
    }
    SaveForHardwareInternal(hardwareObj);
}

void BatteryStatsCore::SaveForHardwareInternal(cJSON* hardwareObj)
{
    // Save for CPU idle
    if (cJSON_AddNumberToObject(hardwareObj, "cpu_idle",
        GetTotalTimeMs(StatsUtils::STATS_TYPE_PHONE_IDLE)) == nullptr) {
        STATS_HILOGW(COMP_SVC, "Add cpu_idle to Hardware failed.");
    }

    // Save for Phone
    cJSON* radioOnArray = cJSON_CreateArray();
    if (radioOnArray) {
        for (uint16_t signalOn = 0; signalOn < StatsUtils::RADIO_SIGNAL_BIN; signalOn++) {
            if (!cJSON_AddItemToArray(radioOnArray,
                cJSON_CreateNumber(GetTotalTimeMs(StatsUtils::STATS_TYPE_PHONE_ACTIVE, signalOn)))) {
                STATS_HILOGW(COMP_SVC, "Add radio_on array failed.");
            }
        }
        if (!cJSON_AddItemToObject(hardwareObj, "radio_on", radioOnArray)) {
            cJSON_Delete(radioOnArray);
            STATS_HILOGW(COMP_SVC, "Add radio_on to Hardware failed.");
        }
    }
    cJSON* radioDataArray = cJSON_CreateArray();
    if (radioDataArray) {
        for (uint16_t signalData = 0; signalData < StatsUtils::RADIO_SIGNAL_BIN; signalData++) {
            if (!cJSON_AddItemToArray(radioDataArray,
                cJSON_CreateNumber(GetTotalTimeMs(StatsUtils::STATS_TYPE_PHONE_DATA, signalData)))) {
                STATS_HILOGW(COMP_SVC, "Add radio_data array failed.");
            }
        }
        if (!cJSON_AddItemToObject(hardwareObj, "radio_data", radioDataArray)) {
            cJSON_Delete(radioDataArray);
            STATS_HILOGW(COMP_SVC, "Add radio_data to Hardware failed.");
        }
    }
}

void BatteryStatsCore::SaveForSoftware(cJSON* root)
{
    for (auto it : uidEntity_->GetUids()) {
        SaveForSoftwareCommon(root, it);
        SaveForSoftwareConnectivity(root, it);
    }
}

void BatteryStatsCore::SaveForSoftwareCommon(cJSON* root, int32_t uid)
{
    STATS_HILOGD(COMP_SVC, "Save software common battery stats, uid: %{public}d", uid);
    std::string strUid = std::to_string(uid);

    cJSON* softwareObj = cJSON_GetObjectItemCaseSensitive(root, "Software");
    if (!StatsJsonUtils::IsValidJsonObject(softwareObj)) {
        softwareObj = cJSON_CreateObject();
        if (!softwareObj) {
            STATS_HILOGE(COMP_SVC, "Failed to create 'software' object");
            return;
        }
        if (!cJSON_AddItemToObject(root, "Software", softwareObj)) {
            cJSON_Delete(softwareObj);
            STATS_HILOGW(COMP_SVC, "Add Software object to root failed.");
            return;
        }
    }

    cJSON* uidObj = cJSON_GetObjectItemCaseSensitive(softwareObj, strUid.c_str());
    if (!StatsJsonUtils::IsValidJsonObject(uidObj)) {
        uidObj = cJSON_CreateObject();
        if (!uidObj) {
            STATS_HILOGE(COMP_SVC, "Failed to create 'uid' object");
            return;
        }
        if (!cJSON_AddItemToObject(softwareObj, strUid.c_str(), uidObj)) {
            cJSON_Delete(uidObj);
            STATS_HILOGW(COMP_SVC, "Add uidObj object to Software failed.");
            return;
        }
    }
    // Save for camera related
    if (cJSON_AddNumberToObject(uidObj, "camera_on",
        GetTotalTimeMs(uid, StatsUtils::STATS_TYPE_CAMERA_ON)) == nullptr) {
        STATS_HILOGW(COMP_SVC, "Add camera_on to uidObj failed.");
    }

    // Save for flashlight related
    if (cJSON_AddNumberToObject(uidObj, "flashlight_on",
        GetTotalTimeMs(uid, StatsUtils::STATS_TYPE_FLASHLIGHT_ON)) == nullptr) {
        STATS_HILOGW(COMP_SVC, "Add flashlight_on to uidObj failed.");
    }
    SaveForSoftwareCommonInternal(uidObj, uid);
}

void BatteryStatsCore::SaveForSoftwareCommonInternal(cJSON* uidObj, int32_t uid)
{
    // Save for gnss related
    if (cJSON_AddNumberToObject(uidObj, "gnss_on", GetTotalTimeMs(uid, StatsUtils::STATS_TYPE_GNSS_ON)) == nullptr) {
        STATS_HILOGW(COMP_SVC, "Add gnss_on to uidObj failed.");
    }

    // Save for audio related
    if (cJSON_AddNumberToObject(uidObj, "audio_on",
        GetTotalTimeMs(uid, StatsUtils::STATS_TYPE_AUDIO_ON)) == nullptr) {
        STATS_HILOGW(COMP_SVC, "Add audio_on to uidObj failed.");
    }

    // Save for wakelock related
    if (cJSON_AddNumberToObject(uidObj, "cpu_awake",
        GetTotalTimeMs(uid, StatsUtils::STATS_TYPE_WAKELOCK_HOLD)) == nullptr) {
        STATS_HILOGW(COMP_SVC, "Add cpu_awake to uidObj failed.");
    }

    // Save for sensor related
    if (cJSON_AddNumberToObject(uidObj, "sensor_gravity",
        GetTotalTimeMs(uid, StatsUtils::STATS_TYPE_SENSOR_GRAVITY_ON)) == nullptr) {
        STATS_HILOGW(COMP_SVC, "Add sensor_gravity to uidObj failed.");
    }
    if (cJSON_AddNumberToObject(uidObj, "sensor_proximity",
        GetTotalTimeMs(uid, StatsUtils::STATS_TYPE_SENSOR_PROXIMITY_ON)) == nullptr) {
        STATS_HILOGW(COMP_SVC, "Add sensor_proximity to uidObj failed.");
    }

    // Save for alarm related
    if (cJSON_AddNumberToObject(uidObj, "alarm",
        GetTotalConsumptionCount(StatsUtils::STATS_TYPE_ALARM, uid)) == nullptr) {
        STATS_HILOGW(COMP_SVC, "Add alarm to uidObj failed.");
    }
    // Save for cpu related
    if (cJSON_AddNumberToObject(uidObj, "cpu_time", cpuEntity_->GetCpuTimeMs(uid)) == nullptr) {
        STATS_HILOGW(COMP_SVC, "Add cpu_time to uidObj failed.");
    }
}

void BatteryStatsCore::SaveForSoftwareConnectivity(cJSON* root, int32_t uid)
{
    STATS_HILOGD(COMP_SVC, "Save software connectivity battery stats, uid: %{public}d", uid);
    std::string strUid = std::to_string(uid);

    cJSON* softwareObj = cJSON_GetObjectItemCaseSensitive(root, "Software");
    if (!StatsJsonUtils::IsValidJsonObject(softwareObj)) {
        softwareObj = cJSON_CreateObject();
        if (!softwareObj) {
            STATS_HILOGE(COMP_SVC, "Failed to create 'software' object");
            return;
        }
        if (!cJSON_AddItemToObject(root, "Software", softwareObj)) {
            cJSON_Delete(softwareObj);
            STATS_HILOGW(COMP_SVC, "Add Software object to root failed.");
            return;
        }
    }

    cJSON* uidObj = cJSON_GetObjectItemCaseSensitive(softwareObj, strUid.c_str());
    if (!StatsJsonUtils::IsValidJsonObject(uidObj)) {
        uidObj = cJSON_CreateObject();
        if (!uidObj) {
            STATS_HILOGE(COMP_SVC, "Failed to create 'uid' object");
            return;
        }
        if (!cJSON_AddItemToObject(softwareObj, strUid.c_str(), uidObj)) {
            cJSON_Delete(uidObj);
            STATS_HILOGW(COMP_SVC, "Add uidObj object to software failed.");
            return;
        }
    }

    // Save for Bluetooth related
    if (cJSON_AddNumberToObject(uidObj, "bluetooth_br_scan",
        GetTotalTimeMs(uid, StatsUtils::STATS_TYPE_BLUETOOTH_BR_SCAN)) == nullptr) {
        STATS_HILOGW(COMP_SVC, "Add bluetooth_br_scan to uidObj failed.");
    }

    if (cJSON_AddNumberToObject(uidObj, "bluetooth_ble_scan",
        GetTotalTimeMs(uid, StatsUtils::STATS_TYPE_BLUETOOTH_BLE_SCAN)) == nullptr) {
        STATS_HILOGW(COMP_SVC, "Add bluetooth_ble_scan to uidObj failed.");
    }
}

void BatteryStatsCore::SaveForPower(cJSON* root)
{
    STATS_HILOGD(COMP_SVC, "Save power battery stats");
    cJSON* powerObj = cJSON_GetObjectItemCaseSensitive(root, "Power");
    if (!StatsJsonUtils::IsValidJsonObject(powerObj)) {
        powerObj = cJSON_CreateObject();
        if (!powerObj) {
            STATS_HILOGE(COMP_SVC, "Failed to create 'Power' object");
            return;
        }
        if (!cJSON_AddItemToObject(root, "Power", powerObj)) {
            cJSON_Delete(powerObj);
            STATS_HILOGW(COMP_SVC, "Add Power object to root failed.");
            return;
        }
    }

    auto statsInfoList = BatteryStatsEntity::GetStatsInfoList();
    for (auto iter = statsInfoList.begin(); iter != statsInfoList.end(); iter++) {
        if ((*iter)->GetConsumptionType() == BatteryStatsInfo::CONSUMPTION_TYPE_APP) {
            std::string name = std::to_string((*iter)->GetUid());
            if (cJSON_AddNumberToObject(powerObj, name.c_str(), (*iter)->GetPower()) == nullptr) {
                STATS_HILOGW(COMP_SVC, "Add %{public}s to powerObj failed.", name.c_str());
            }
            STATS_HILOGD(COMP_SVC, "Saved power: %{public}lf for uid: %{public}s", (*iter)->GetPower(),
                name.c_str());
        } else if ((*iter)->GetConsumptionType() != BatteryStatsInfo::CONSUMPTION_TYPE_USER) {
            std::string name = std::to_string((*iter)->GetConsumptionType());
            if (cJSON_AddNumberToObject(powerObj, name.c_str(), (*iter)->GetPower()) == nullptr) {
                STATS_HILOGW(COMP_SVC, "Add %{public}s to powerObj failed.", name.c_str());
            }
            STATS_HILOGD(COMP_SVC, "Saved power: %{public}lf for type: %{public}s", (*iter)->GetPower(),
                name.c_str());
        }
    }
}

bool BatteryStatsCore::SaveBatteryStatsData()
{
    ComputePower();
    cJSON* root = cJSON_CreateObject();
    if (!root) {
        STATS_HILOGE(COMP_SVC, "Failed to create cJSON root object");
        return false;
    }

    // Save for power
    SaveForPower(root);

    // Save for hardware
    SaveForHardware(root);

    // Save for software
    SaveForSoftware(root);

    char* jsonStr = cJSON_Print(root);
    if (!jsonStr) {
        STATS_HILOGE(COMP_SVC, "Failed to print cJSON to string");
        cJSON_Delete(root);
        return false;
    }
    cJSON_Delete(root);

    FILE* fp = std::fopen(BATTERY_STATS_JSON.c_str(), "w");
    if (!fp) {
        STATS_HILOGE(COMP_SVC, "Opening json file failed");
        cJSON_free(jsonStr);
        return false;
    }

    auto len = fwrite(jsonStr, sizeof(char), strlen(jsonStr), fp);
    std::fclose(fp);
    if (len != strlen(jsonStr)) {
        STATS_HILOGE(COMP_SVC, "Failed to write file");
        cJSON_free(jsonStr);
        return false;
    }
    cJSON_free(jsonStr);
    return true;
}

void BatteryStatsCore::UpdateStatsEntity(cJSON* root)
{
    BatteryStatsEntity::ResetStatsEntity();
    cJSON* powerObj = cJSON_GetObjectItemCaseSensitive(root, "Power");
    if (!StatsJsonUtils::IsValidJsonObjectOrJsonArray(powerObj)) {
        STATS_HILOGE(COMP_SVC, "Failed to get 'Power' object from json");
        return;
    }
    std::map<int32_t, double> tmpUserPowerMap;
    cJSON* currentElement = nullptr;
    cJSON_ArrayForEach(currentElement, powerObj) {
        const char* key = currentElement->string;
        if (!key || !StatsJsonUtils::IsValidJsonNumber(currentElement)) {
            continue;
        }
        std::string keyStr(key);
        int64_t result = 0;
        if (!StatsUtils::ParseStrtollResult(keyStr, result)) {
            continue;
        }
        auto id = static_cast<int32_t>(result);
        int32_t usr = StatsUtils::INVALID_VALUE;
        std::shared_ptr<BatteryStatsInfo> info = std::make_shared<BatteryStatsInfo>();
        if (id > StatsUtils::INVALID_VALUE) {
            info->SetUid(id);
            info->SetConsumptioType(BatteryStatsInfo::CONSUMPTION_TYPE_APP);
            info->SetPower(currentElement->valuedouble);
            usr = AccountSA::OhosAccountKits::GetInstance().GetDeviceAccountIdByUID(id);
            const auto& userPower = tmpUserPowerMap.find(usr);
            if (userPower != tmpUserPowerMap.end()) {
                userPower->second += info->GetPower();
            } else {
                tmpUserPowerMap.insert(std::pair<int32_t, double>(usr, info->GetPower()));
            }
        } else if (id < StatsUtils::INVALID_VALUE && id > BatteryStatsInfo::CONSUMPTION_TYPE_INVALID) {
            info->SetUid(StatsUtils::INVALID_VALUE);
            info->SetConsumptioType(static_cast<BatteryStatsInfo::ConsumptionType>(id));
            info->SetPower(currentElement->valuedouble);
        }
        STATS_HILOGD(COMP_SVC, "Load power:%{public}lfmAh,id:%{public}d,user:%{public}d", info->GetPower(), id, usr);
        BatteryStatsEntity::UpdateStatsInfoList(info);
    }
    for (auto& iter : tmpUserPowerMap) {
        std::shared_ptr<BatteryStatsInfo> statsInfo = std::make_shared<BatteryStatsInfo>();
        statsInfo->SetConsumptioType(BatteryStatsInfo::CONSUMPTION_TYPE_USER);
        statsInfo->SetUserId(iter.first);
        statsInfo->SetPower(iter.second);
        BatteryStatsEntity::UpdateStatsInfoList(statsInfo);
    }
}

bool BatteryStatsCore::LoadBatteryStatsData()
{
    std::ifstream ifs(BATTERY_STATS_JSON, std::ios::binary);
    if (!ifs.is_open()) {
        STATS_HILOGE(COMP_SVC, "Json file doesn't exist");
        return false;
    }

    ifs.seekg(0, std::ios::end);
    size_t fileSize = static_cast<size_t>(ifs.tellg());
    ifs.seekg(0, std::ios::beg);

    if (fileSize == 0) {
        STATS_HILOGE(COMP_SVC, "File is empty or invalid size");
        return false;
    }

    std::string jsonBuffer(fileSize, '\0');
    ifs.read(&jsonBuffer[0], fileSize);

    if (ifs.fail()) {
        STATS_HILOGE(COMP_SVC, "Failed to read the JSON file");
        return false;
    }
    jsonBuffer.push_back('\0');
    ifs.close();
    cJSON* root = cJSON_Parse(jsonBuffer.c_str());
    if (!root) {
        STATS_HILOGE(COMP_SVC, "Failed to parse the JSON file");
        return false;
    }
    if (!cJSON_IsObject(root)) {
        STATS_HILOGE(COMP_SVC, "Root is not a valid JSON object");
        cJSON_Delete(root);
        return false;
    }

    UpdateStatsEntity(root);
    cJSON_Delete(root);
    return true;
}

void BatteryStatsCore::Reset()
{
    std::lock_guard lock(mutex_);
    audioEntity_->Reset();
    bluetoothEntity_->Reset();
    cameraEntity_->Reset();
    cpuEntity_->Reset();
    flashlightEntity_->Reset();
    gnssEntity_->Reset();
    idleEntity_->Reset();
    phoneEntity_->Reset();
    screenEntity_->Reset();
    sensorEntity_->Reset();
    uidEntity_->Reset();
    userEntity_->Reset();
    wifiEntity_->Reset();
    wakelockEntity_->Reset();
    alarmEntity_->Reset();
    BatteryStatsEntity::ResetStatsEntity();
    debugInfo_.clear();
}
} // namespace PowerMgr
} // namespace OHOS
