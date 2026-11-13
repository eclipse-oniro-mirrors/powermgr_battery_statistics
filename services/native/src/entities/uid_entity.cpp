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

#include "entities/uid_entity.h"

#ifdef SYS_MGR_CLIENT_ENABLE
#include <bundle_constants.h>
#include <bundle_mgr_interface.h>
#include <ipc_skeleton.h>
#include <system_ability_definition.h>
#include <sys_mgr_client.h>
#endif

#include <ohos_account_kits_impl.h>
#include "battery_stats_service.h"
#include "stats_log.h"

namespace OHOS {
namespace PowerMgr {
namespace {
}

UidEntity::UidEntity()
{
    consumptionType_ = BatteryStatsInfo::CONSUMPTION_TYPE_APP;
}

void UidEntity::UpdateUidMap(int32_t uid)
{
    std::lock_guard<std::mutex> lock(uidEntityMutex_);
    if (uid > StatsUtils::INVALID_VALUE) {
        auto iter = uidPowerMap_.find(uid);
        if (iter != uidPowerMap_.end()) {
            STATS_HILOGD(COMP_SVC, "Uid has already been added, ignore");
        } else {
            STATS_HILOGD(COMP_SVC, "Update %{public}d to uid power map", uid);
            uidPowerMap_.insert(std::pair<int32_t, double>(uid, StatsUtils::DEFAULT_VALUE));
        }
    }
}

std::vector<int32_t> UidEntity::GetUids()
{
    std::lock_guard<std::mutex> lock(uidEntityMutex_);
    std::vector<int32_t> uids;
    std::transform(uidPowerMap_.begin(), uidPowerMap_.end(), std::back_inserter(uids), [](const auto& item) {
        return item.first;
    });
    return uids;
}

double UidEntity::CalculateForConnectivity(int32_t uid)
{
    double power = StatsUtils::DEFAULT_VALUE;
    auto bss = BatteryStatsService::GetInstance();
    auto core = bss->GetBatteryStatsCore();
    auto bluetoothEntity = core->GetEntity(BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH);

    // Calculate bluetooth power consumption
    bluetoothEntity->Calculate(uid);
    power += bluetoothEntity->GetEntityPowerMah(uid);
    STATS_HILOGD(COMP_SVC, "Connectivity power consumption: %{public}lfmAh for uid: %{public}d", power, uid);
    return power;
}

double UidEntity::CalculateForCommon(int32_t uid)
{
    double power = StatsUtils::DEFAULT_VALUE;
    auto bss = BatteryStatsService::GetInstance();
    auto core = bss->GetBatteryStatsCore();
    auto cameraEntity = core->GetEntity(BatteryStatsInfo::CONSUMPTION_TYPE_CAMERA);
    auto flashlightEntity = core->GetEntity(BatteryStatsInfo::CONSUMPTION_TYPE_FLASHLIGHT);
    auto audioEntity = core->GetEntity(BatteryStatsInfo::CONSUMPTION_TYPE_AUDIO);
    auto sensorEntity = core->GetEntity(BatteryStatsInfo::CONSUMPTION_TYPE_SENSOR);
    auto gnssEntity = core->GetEntity(BatteryStatsInfo::CONSUMPTION_TYPE_GNSS);
    auto cpuEntity = core->GetEntity(BatteryStatsInfo::CONSUMPTION_TYPE_CPU);
    auto wakelockEntity = core->GetEntity(BatteryStatsInfo::CONSUMPTION_TYPE_WAKELOCK);
    auto alarmEntity = core->GetEntity(BatteryStatsInfo::CONSUMPTION_TYPE_ALARM);

    // Calculate camera power consumption
    cameraEntity->Calculate(uid);
    power += cameraEntity->GetEntityPowerMah(uid);
    // Calculate flashlight power consumption
    flashlightEntity->Calculate(uid);
    power += flashlightEntity->GetEntityPowerMah(uid);
    // Calculate audio power consumption
    audioEntity->Calculate(uid);
    power += audioEntity->GetEntityPowerMah(uid);
    // Calculate sensor power consumption
    sensorEntity->Calculate(uid);
    power += sensorEntity->GetEntityPowerMah(uid);
    // Calculate gnss power consumption
    gnssEntity->Calculate(uid);
    power += gnssEntity->GetEntityPowerMah(uid);
    // Calculate cpu power consumption
    cpuEntity->Calculate(uid);
    power += cpuEntity->GetEntityPowerMah(uid);
    // Calculate cpu power consumption
    wakelockEntity->Calculate(uid);
    power += wakelockEntity->GetEntityPowerMah(uid);
    // Calculate alarm power consumption
    alarmEntity->Calculate(uid);
    power += alarmEntity->GetEntityPowerMah(uid);

    STATS_HILOGD(COMP_SVC, "Common power consumption: %{public}lfmAh for uid: %{public}d", power, uid);
    return power;
}

void UidEntity::Calculate(int32_t uid)
{
    auto bss = BatteryStatsService::GetInstance();
    std::lock_guard<std::mutex> lock(uidEntityMutex_);
    auto core = bss->GetBatteryStatsCore();
    auto userEntity = core->GetEntity(BatteryStatsInfo::CONSUMPTION_TYPE_USER);
    for (auto& iter : uidPowerMap_) {
        double power = StatsUtils::DEFAULT_VALUE;
        power += CalculateForConnectivity(iter.first);
        power += CalculateForCommon(iter.first);
        iter.second = power;
        {
            std::lock_guard<std::mutex> statsLock(statsInfoListMutex_);
            totalPowerMah_ += power;
        }
        AddtoStatsList(iter.first, power);
        int32_t uid = iter.first;
        int32_t userId = AccountSA::OhosAccountKits::GetInstance().GetDeviceAccountIdByUID(uid);
        if (userEntity != nullptr) {
            userEntity->AggregateUserPowerMah(userId, power);
        }
    }
}

void UidEntity::AddtoStatsList(int32_t uid, double power)
{
    std::shared_ptr<BatteryStatsInfo> statsInfo = std::make_shared<BatteryStatsInfo>();
    statsInfo->SetConsumptioType(BatteryStatsInfo::CONSUMPTION_TYPE_APP);
    statsInfo->SetUid(uid);
    statsInfo->SetPower(power);
    UpdateStatsInfoList(statsInfo);
}

double UidEntity::GetEntityPowerMah(int32_t uidOrUserId)
{
    std::lock_guard<std::mutex> lock(uidEntityMutex_);
    double power = StatsUtils::DEFAULT_VALUE;
    auto iter = uidPowerMap_.find(uidOrUserId);
    if (iter != uidPowerMap_.end()) {
        power = iter->second;
        STATS_HILOGD(COMP_SVC, "Get app uid power consumption: %{public}lfmAh for uid: %{public}d",
            power, uidOrUserId);
    } else {
        STATS_HILOGD(COMP_SVC,
            "No app uid power consumption related to uid: %{public}d was found, return 0", uidOrUserId);
    }
    return power;
}

double UidEntity::GetPowerForConnectivity(StatsUtils::StatsType statsType, int32_t uid)
{
    double power = StatsUtils::DEFAULT_VALUE;
    auto bss = BatteryStatsService::GetInstance();
    auto core = bss->GetBatteryStatsCore();
    auto bluetoothEntity = core->GetEntity(BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH);

    if (statsType == StatsUtils::STATS_TYPE_BLUETOOTH_BR_SCAN) {
        power = bluetoothEntity->GetStatsPowerMah(StatsUtils::STATS_TYPE_BLUETOOTH_BR_SCAN, uid);
    } else if (statsType == StatsUtils::STATS_TYPE_BLUETOOTH_BLE_SCAN) {
        power = bluetoothEntity->GetStatsPowerMah(StatsUtils::STATS_TYPE_BLUETOOTH_BLE_SCAN, uid);
    }
    return power;
}

double UidEntity::GetPowerForCommon(StatsUtils::StatsType statsType, int32_t uid)
{
    double power = StatsUtils::DEFAULT_VALUE;
    auto bss = BatteryStatsService::GetInstance();
    auto core = bss->GetBatteryStatsCore();
    auto cameraEntity = core->GetEntity(BatteryStatsInfo::CONSUMPTION_TYPE_CAMERA);
    auto flashlightEntity = core->GetEntity(BatteryStatsInfo::CONSUMPTION_TYPE_FLASHLIGHT);
    auto audioEntity = core->GetEntity(BatteryStatsInfo::CONSUMPTION_TYPE_AUDIO);
    auto sensorEntity = core->GetEntity(BatteryStatsInfo::CONSUMPTION_TYPE_SENSOR);
    auto gnssEntity = core->GetEntity(BatteryStatsInfo::CONSUMPTION_TYPE_GNSS);
    auto cpuEntity = core->GetEntity(BatteryStatsInfo::CONSUMPTION_TYPE_CPU);
    auto wakelockEntity = core->GetEntity(BatteryStatsInfo::CONSUMPTION_TYPE_WAKELOCK);
    auto alarmEntity = core->GetEntity(BatteryStatsInfo::CONSUMPTION_TYPE_ALARM);

    if (statsType == StatsUtils::STATS_TYPE_CAMERA_ON) {
        power = cameraEntity->GetStatsPowerMah(StatsUtils::STATS_TYPE_CAMERA_ON, uid);
    } else if (statsType == StatsUtils::STATS_TYPE_FLASHLIGHT_ON) {
        power = flashlightEntity->GetStatsPowerMah(StatsUtils::STATS_TYPE_FLASHLIGHT_ON, uid);
    } else if (statsType == StatsUtils::STATS_TYPE_GNSS_ON) {
        power = gnssEntity->GetStatsPowerMah(StatsUtils::STATS_TYPE_GNSS_ON, uid);
    } else if (statsType == StatsUtils::STATS_TYPE_SENSOR_GRAVITY_ON) {
        power = sensorEntity->GetStatsPowerMah(StatsUtils::STATS_TYPE_SENSOR_GRAVITY_ON, uid);
    } else if (statsType == StatsUtils::STATS_TYPE_SENSOR_PROXIMITY_ON) {
        power = sensorEntity->GetStatsPowerMah(StatsUtils::STATS_TYPE_SENSOR_PROXIMITY_ON, uid);
    } else if (statsType == StatsUtils::STATS_TYPE_AUDIO_ON) {
        power = audioEntity->GetStatsPowerMah(StatsUtils::STATS_TYPE_AUDIO_ON, uid);
    } else if (statsType == StatsUtils::STATS_TYPE_WAKELOCK_HOLD) {
        power = wakelockEntity->GetStatsPowerMah(StatsUtils::STATS_TYPE_WAKELOCK_HOLD, uid);
    } else if (statsType == StatsUtils::STATS_TYPE_CPU_CLUSTER) {
        power = cpuEntity->GetStatsPowerMah(StatsUtils::STATS_TYPE_CPU_CLUSTER, uid);
    } else if (statsType == StatsUtils::STATS_TYPE_CPU_SPEED) {
        power = cpuEntity->GetStatsPowerMah(StatsUtils::STATS_TYPE_CPU_SPEED, uid);
    } else if (statsType == StatsUtils::STATS_TYPE_CPU_ACTIVE) {
        power = cpuEntity->GetStatsPowerMah(StatsUtils::STATS_TYPE_CPU_ACTIVE, uid);
    } else if (statsType == StatsUtils::STATS_TYPE_ALARM) {
        power = alarmEntity->GetStatsPowerMah(StatsUtils::STATS_TYPE_ALARM, uid);
    }
    return power;
}

double UidEntity::GetStatsPowerMah(StatsUtils::StatsType statsType, int32_t uid)
{
    double power = StatsUtils::DEFAULT_VALUE;

    switch (statsType) {
        case StatsUtils::STATS_TYPE_BLUETOOTH_BR_SCAN:
        case StatsUtils::STATS_TYPE_BLUETOOTH_BLE_SCAN:
            power = GetPowerForConnectivity(statsType, uid);
            break;
        case StatsUtils::STATS_TYPE_CAMERA_ON:
        case StatsUtils::STATS_TYPE_FLASHLIGHT_ON:
        case StatsUtils::STATS_TYPE_GNSS_ON:
        case StatsUtils::STATS_TYPE_SENSOR_GRAVITY_ON:
        case StatsUtils::STATS_TYPE_SENSOR_PROXIMITY_ON:
        case StatsUtils::STATS_TYPE_AUDIO_ON:
        case StatsUtils::STATS_TYPE_WAKELOCK_HOLD:
        case StatsUtils::STATS_TYPE_CPU_CLUSTER:
        case StatsUtils::STATS_TYPE_CPU_SPEED:
        case StatsUtils::STATS_TYPE_CPU_ACTIVE:
        case StatsUtils::STATS_TYPE_ALARM:
            power = GetPowerForCommon(statsType, uid);
            break;
        default:
            STATS_HILOGW(COMP_SVC, "Invalid or illegal type got, return 0");
            break;
    }

    STATS_HILOGD(COMP_SVC, "Get %{public}s power: %{public}lfmAh for uid: %{public}d",
        StatsUtils::ConvertStatsType(statsType).c_str(), power, uid);
    return power;
}

void UidEntity::Reset()
{
    std::lock_guard<std::mutex> lock(uidEntityMutex_);
    // Reset app Uid total power consumption
    for (auto& iter : uidPowerMap_) {
        iter.second = StatsUtils::DEFAULT_VALUE;
    }
}

void UidEntity::DumpForBluetooth(int32_t uid, std::string& result)
{
    auto bss = BatteryStatsService::GetInstance();
    // Dump for bluetooth realted info
    auto core = bss->GetBatteryStatsCore();
    int64_t bluetoothBrScanTime = core->GetTotalTimeMs(uid, StatsUtils::STATS_TYPE_BLUETOOTH_BR_SCAN);
    int64_t bluetoothBleScanTime = core->GetTotalTimeMs(uid, StatsUtils::STATS_TYPE_BLUETOOTH_BLE_SCAN);

    result.append("Bluetooth Br scan time: ")
        .append(ToString(bluetoothBrScanTime))
        .append("ms\n")
        .append("Bluetooth Ble scan time: ")
        .append(ToString(bluetoothBleScanTime))
        .append("ms\n");
}

void UidEntity::DumpForCommon(int32_t uid, std::string& result)
{
    auto bss = BatteryStatsService::GetInstance();
    auto core = bss->GetBatteryStatsCore();
    // Dump for camera related info
    int64_t cameraTime = core->GetTotalTimeMs(uid, StatsUtils::STATS_TYPE_CAMERA_ON);

    // Dump for flashlight related info
    int64_t flashlightTime = core->GetTotalTimeMs(uid, StatsUtils::STATS_TYPE_FLASHLIGHT_ON);

    // Dump for gnss related info
    int64_t gnssTime = core->GetTotalTimeMs(uid, StatsUtils::STATS_TYPE_GNSS_ON);

    // Dump for gravity sensor related info
    int64_t gravityTime = core->GetTotalTimeMs(uid, StatsUtils::STATS_TYPE_SENSOR_GRAVITY_ON);

    // Dump for proximity sensor related info
    int64_t proximityTime = core->GetTotalTimeMs(uid, StatsUtils::STATS_TYPE_SENSOR_PROXIMITY_ON);

    // Dump for audio related info
    int64_t audioTime = core->GetTotalTimeMs(uid, StatsUtils::STATS_TYPE_AUDIO_ON);

    // Dump for wakelock related info
    int64_t wakelockTime = core->GetTotalTimeMs(uid, StatsUtils::STATS_TYPE_WAKELOCK_HOLD);

    // Dump for alarm related info
    int64_t alarmCount = core->GetTotalConsumptionCount(StatsUtils::STATS_TYPE_ALARM, uid);

    result.append("Camera on time: ")
        .append(ToString(cameraTime))
        .append("ms\n")
        .append("Flashlight scan time: ")
        .append(ToString(flashlightTime))
        .append("ms\n")
        .append("GNSS scan time: ")
        .append(ToString(gnssTime))
        .append("ms\n")
        .append("Gravity sensor on time: ")
        .append(ToString(gravityTime))
        .append("ms\n")
        .append("Proximity sensor on time: ")
        .append(ToString(proximityTime))
        .append("ms\n")
        .append("Audio on time: ")
        .append(ToString(audioTime))
        .append("ms\n")
        .append("Wakelock hold time: ")
        .append(ToString(wakelockTime))
        .append("ms\n")
        .append("Alarm trigger count: ")
        .append(ToString(alarmCount))
        .append("times\n");
}

void UidEntity::DumpInfo(std::string& result, int32_t uid)
{
    auto bss = BatteryStatsService::GetInstance();
    std::lock_guard<std::mutex> lock(uidEntityMutex_);
    auto core = bss->GetBatteryStatsCore();
    for (auto& iter : uidPowerMap_) {
        std::string bundleName = "NULL";
#ifdef SYS_MGR_CLIENT_ENABLE
        auto bundleObj =
            DelayedSingleton<AppExecFwk::SysMrgClient>::GetInstance()
                ->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
        if (bundleObj == nullptr) {
            STATS_HILOGE(COMP_SVC, "Failed to get bundle manager service");
        } else {
            sptr<AppExecFwk::IBundleMgr> bmgr = iface_cast<AppExecFwk::IBundleMgr>(bundleObj);
            if (bmgr == nullptr) {
                STATS_HILOGE(COMP_SVC, "Failed to get bundle manager proxy");
            } else {
                std::string identity = IPCSkeleton::ResetCallingIdentity();
                ErrCode res = bmgr->GetNameForUid(iter.first, bundleName);
                IPCSkeleton::SetCallingIdentity(identity);
                if (res != ERR_OK) {
                    STATS_HILOGE(COMP_SVC, "Failed to get bundle name for uid=%{public}d, ErrCode=%{public}d",
                        iter.first, static_cast<int32_t>(res));
                }
            }
        }
#endif
        result.append("\n")
            .append(ToString(iter.first))
            .append("(Bundle name: ")
            .append(bundleName)
            .append(")")
            .append(":")
            .append("\n");
        DumpForBluetooth(iter.first, result);
        DumpForCommon(iter.first, result);
        auto cpuEntity = core->GetEntity(BatteryStatsInfo::CONSUMPTION_TYPE_CPU);
        if (cpuEntity) {
            cpuEntity->DumpInfo(result, iter.first);
        }
    }
}
} // namespace PowerMgr
} // namespace OHOS