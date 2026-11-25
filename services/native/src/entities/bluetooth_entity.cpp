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

#include "entities/bluetooth_entity.h"

#include <cinttypes>
#ifdef SYS_MGR_CLIENT_ENABLE
#include <ipc_skeleton.h>
#include "bundle_constants.h"
#include "bundle_mgr_interface.h"
#include "system_ability_definition.h"
#include "sys_mgr_client.h"
#endif

#include "battery_stats_service.h"
#include "stats_log.h"

namespace OHOS {
namespace PowerMgr {
namespace {
}

BluetoothEntity::BluetoothEntity()
{
    consumptionType_ = BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH;
}

void BluetoothEntity::Calculate(int32_t uid)
{
    if (uid > StatsUtils::INVALID_VALUE) {
        // Calculate Bluetooth scan caused by app
        CalculateBtPowerForApp(uid);
    } else {
        // Calculate Bluetooth on and Bluetooth app power consumption caused by Bluetooth hardware
        CalculateBtPower();
    }
}

void BluetoothEntity::CalculateBtPower()
{
    auto bss = BatteryStatsService::GetInstance();
    // Calculate Bluetooth BR on power
    auto bluetoothBrOnAverageMa =
        bss->GetBatteryStatsParser()->GetAveragePowerMa(StatsUtils::CURRENT_BLUETOOTH_BR_ON);
    auto bluetoothBrOnTimeMs = GetActiveTimeMs(StatsUtils::STATS_TYPE_BLUETOOTH_BR_ON);
    auto bluetoothBrOnPowerMah = bluetoothBrOnAverageMa * bluetoothBrOnTimeMs / StatsUtils::MS_IN_HOUR;
    bluetoothBrPowerMah_ += bluetoothBrOnPowerMah;

    // Calculate Bluetooth BLE on power
    auto bluetoothBleOnAverageMa =
        bss->GetBatteryStatsParser()->GetAveragePowerMa(StatsUtils::CURRENT_BLUETOOTH_BLE_ON);
    auto bluetoothBleOnTimeMs = GetActiveTimeMs(StatsUtils::STATS_TYPE_BLUETOOTH_BLE_ON);
    auto bluetoothBleOnPowerMah = bluetoothBleOnAverageMa * bluetoothBleOnTimeMs / StatsUtils::MS_IN_HOUR;
    bluetoothBlePowerMah_ += bluetoothBleOnPowerMah;
    
    auto bluetoothUidPowerMah = GetBluetoothUidPower();

    bluetoothPowerMah_ = bluetoothBrOnPowerMah + bluetoothBleOnPowerMah + bluetoothUidPowerMah;
    {
        std::lock_guard<std::mutex> statsLock(statsMutex_);
        totalPowerMah_ += bluetoothPowerMah_;
    }

    std::shared_ptr<BatteryStatsInfo> statsInfo = std::make_shared<BatteryStatsInfo>();
    statsInfo->SetConsumptioType(BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH);
    statsInfo->SetPower(bluetoothPowerMah_);
    UpdateStatsInfoList(statsInfo);

    STATS_HILOGD(COMP_SVC, "Calculate bluetooth Br time: %{public}" PRId64 "ms, Br power average: %{public}lfma,"    \
        "Br power consumption: %{public}lfmAh, bluetooth Ble time: %{public}" PRId64 "ms, "                          \
        "Ble power average: %{public}lfma, Ble power consumption: %{public}lfmAh, "                                  \
        "uid power consumption: %{public}lfmAh, total power consumption: %{public}lfmAh",
        bluetoothBrOnTimeMs,
        bluetoothBrOnAverageMa,
        bluetoothBrOnPowerMah,
        bluetoothBleOnTimeMs,
        bluetoothBleOnAverageMa,
        bluetoothBleOnPowerMah,
        bluetoothUidPowerMah,
        bluetoothPowerMah_);
}

void BluetoothEntity::CalculateBtPowerForApp(int32_t uid)
{
    auto bss = BatteryStatsService::GetInstance();
    // Calculate Bluetooth Br scan power consumption
    auto bluetoothBrScanAverageMa =
        bss->GetBatteryStatsParser()->GetAveragePowerMa(StatsUtils::CURRENT_BLUETOOTH_BR_SCAN);
    auto bluetoothBrScanTimeMs = GetActiveTimeMs(uid, StatsUtils::STATS_TYPE_BLUETOOTH_BR_SCAN);
    auto bluetoothBrScanPowerMah = bluetoothBrScanTimeMs * bluetoothBrScanAverageMa / StatsUtils::MS_IN_HOUR;
    UpdateAppBluetoothBlePower(POWER_TYPE_BR, uid, bluetoothBrScanPowerMah);

    // Calculate Bluetooth Ble scan power consumption
    auto bluetoothBleScanAverageMa =
        bss->GetBatteryStatsParser()->GetAveragePowerMa(StatsUtils::CURRENT_BLUETOOTH_BLE_SCAN);
    auto bluetoothBleScanTimeMs = GetActiveTimeMs(uid, StatsUtils::STATS_TYPE_BLUETOOTH_BLE_SCAN);
    auto bluetoothBleScanPowerMah = bluetoothBleScanTimeMs * bluetoothBleScanAverageMa / StatsUtils::MS_IN_HOUR;
    UpdateAppBluetoothBlePower(POWER_TYPE_BLE, uid, bluetoothBleScanPowerMah);

    auto bluetoothUidPowerMah = bluetoothBrScanPowerMah + bluetoothBleScanPowerMah;
    UpdateAppBluetoothBlePower(POWER_TYPE_ALL, uid, bluetoothUidPowerMah);

    STATS_HILOGD(COMP_SVC, "Calculate bluetooth Br scan time: %{public}" PRId64 "ms, "                         \
        "Br scan power average: %{public}lfma, Br scan power consumption: %{public}lfmAh "                     \
        "bluetooth Ble scan time: %{public}" PRId64 "ms, Ble scan power average: %{public}lfma, "              \
        "Ble scan power consumption: %{public}lfmAh, total power consumption: %{public}lfmAh, uid:%{public}d",
        bluetoothBrScanTimeMs,
        bluetoothBrScanAverageMa,
        bluetoothBrScanPowerMah,
        bluetoothBleScanTimeMs,
        bluetoothBleScanAverageMa,
        bluetoothBleScanPowerMah,
        bluetoothUidPowerMah,
        uid);
}

void BluetoothEntity::UpdateAppBluetoothBlePower(PowerType type, int32_t uid, double powerMah)
{
    switch (type) {
        case POWER_TYPE_BR: {
            auto iter = appBluetoothBrPowerMap_.find(uid);
            if (iter != appBluetoothBrPowerMap_.end()) {
                iter->second = powerMah;
                STATS_HILOGD(COMP_SVC, "Update app bluetooth Br power consumption: %{public}lfmAh for uid: %{public}d",
                    powerMah, uid);
                break;
            }
            appBluetoothBrPowerMap_.insert(std::pair<int32_t, double>(uid, powerMah));
            STATS_HILOGD(COMP_SVC, "Create app bluetooth Br power consumption: %{public}lfmAh for uid: %{public}d",
                powerMah, uid);
            break;
        }
        case POWER_TYPE_BLE: {
            auto iter = appBluetoothBlePowerMap_.find(uid);
            if (iter != appBluetoothBlePowerMap_.end()) {
                iter->second = powerMah;
                STATS_HILOGD(COMP_SVC, "Update app bluetooth Ble power consumption: %{public}lfmAh for uid: %{public}d",
                    powerMah, uid);
                break;
            }
            appBluetoothBlePowerMap_.insert(std::pair<int32_t, double>(uid, powerMah));
            STATS_HILOGD(COMP_SVC, "Create app bluetooth Ble power consumption: %{public}lfmAh for uid: %{public}d",
                powerMah, uid);
            break;
        }
        case POWER_TYPE_ALL: {
            auto iter = appBluetoothPowerMap_.find(uid);
            if (iter != appBluetoothPowerMap_.end()) {
                iter->second = powerMah;
                STATS_HILOGD(COMP_SVC, "Update app bluetooth power consumption: %{public}lfmAh for uid: %{public}d",
                    powerMah, uid);
                break;
            }
            appBluetoothPowerMap_.insert(std::pair<int32_t, double>(uid, powerMah));
            STATS_HILOGD(COMP_SVC, "Create app bluetooth power consumption: %{public}lfmAh for uid: %{public}d",
                powerMah, uid);
            break;
        }
        default:
            break;
    }
}

int64_t BluetoothEntity::GetActiveTimeMs(int32_t uid, StatsUtils::StatsType statsType, int16_t level)
{
    int64_t time = StatsUtils::DEFAULT_VALUE;
    switch (statsType) {
        case StatsUtils::STATS_TYPE_BLUETOOTH_BR_SCAN: {
            auto brScanIter = appBluetoothBrScanTimerMap_.find(uid);
            if (brScanIter != appBluetoothBrScanTimerMap_.end()) {
                time = brScanIter->second->GetRunningTimeMs();
                STATS_HILOGD(COMP_SVC, "Get blueooth Br scan time: %{public}" PRId64 "ms for uid: %{public}d",
                    time, uid);
                break;
            }
            STATS_HILOGD(COMP_SVC, "No bluetooth Br scan timer related to uid: %{public}d was found, return 0", uid);
            break;
        }
        case StatsUtils::STATS_TYPE_BLUETOOTH_BLE_SCAN: {
            auto bleScanIter = appBluetoothBleScanTimerMap_.find(uid);
            if (bleScanIter != appBluetoothBleScanTimerMap_.end()) {
                time = bleScanIter->second->GetRunningTimeMs();
                STATS_HILOGD(COMP_SVC, "Get blueooth Ble scan time: %{public}" PRId64 "ms for uid: %{public}d",
                    time, uid);
                break;
            }
            STATS_HILOGD(COMP_SVC, "No bluetooth Ble scan timer related to uid: %{public}d was found, return 0", uid);
            break;
        }
        default:
            break;
    }
    return time;
}

int64_t BluetoothEntity::GetActiveTimeMs(StatsUtils::StatsType statsType, int16_t level)
{
    int64_t time = StatsUtils::DEFAULT_VALUE;
    switch (statsType) {
        case StatsUtils::STATS_TYPE_BLUETOOTH_BR_ON: {
            if (bluetoothBrOnTimer_) {
                time = bluetoothBrOnTimer_->GetRunningTimeMs();
                STATS_HILOGD(COMP_SVC, "Get blueooth Br on time: %{public}" PRId64 "ms", time);
                break;
            }
            STATS_HILOGD(COMP_SVC, "Bluetooth Br has not been turned on yet, return 0");
            break;
        }
        case StatsUtils::STATS_TYPE_BLUETOOTH_BLE_ON: {
            if (bluetoothBleOnTimer_) {
                time = bluetoothBleOnTimer_->GetRunningTimeMs();
                STATS_HILOGD(COMP_SVC, "Get blueooth Ble on time: %{public}" PRId64 "ms", time);
                break;
            }
            STATS_HILOGD(COMP_SVC, "Bluetooth Ble has not been turned on yet, return 0");
            break;
        }
        default:
            break;
    }
    return time;
}

double BluetoothEntity::GetEntityPowerMah(int32_t uidOrUserId)
{
    double power = StatsUtils::DEFAULT_VALUE;
    if (uidOrUserId > StatsUtils::INVALID_VALUE) {
        auto iter = appBluetoothPowerMap_.find(uidOrUserId);
        if (iter != appBluetoothPowerMap_.end()) {
            power = iter->second;
            STATS_HILOGD(COMP_SVC, "Get app blueooth power consumption: %{public}lfmAh for uid: %{public}d",
                power, uidOrUserId);
        } else {
            STATS_HILOGD(COMP_SVC,
                "No app blueooth power consumption related to uid: %{public}d was found, return 0", uidOrUserId);
        }
    } else {
        power = bluetoothPowerMah_;
        STATS_HILOGD(COMP_SVC, "Get blueooth power consumption: %{public}lfmAh", power);
    }
    return power;
}

double BluetoothEntity::GetStatsPowerMah(StatsUtils::StatsType statsType, int32_t uid)
{
    double power = StatsUtils::DEFAULT_VALUE;
    switch (statsType) {
        case StatsUtils::STATS_TYPE_BLUETOOTH_BR_ON: {
            power = bluetoothBrPowerMah_;
            STATS_HILOGD(COMP_SVC, "Get blueooth Br on power consumption: %{public}lfmAh", power);
            break;
        }
        case StatsUtils::STATS_TYPE_BLUETOOTH_BLE_ON: {
            power = bluetoothBlePowerMah_;
            STATS_HILOGD(COMP_SVC, "Get blueooth Ble on power consumption: %{public}lfmAh", power);
            break;
        }
        case StatsUtils::STATS_TYPE_BLUETOOTH_BR_SCAN: {
            auto brIter = appBluetoothBrPowerMap_.find(uid);
            if (brIter != appBluetoothBrPowerMap_.end()) {
                power = brIter->second;
                STATS_HILOGD(COMP_SVC, "Get blueooth Br scan power consumption: %{public}lfmAh for uid: %{public}d",
                    power, uid);
                break;
            }
            STATS_HILOGD(COMP_SVC,
                "No bluetooth Br scan power consumption related to uid: %{public}d was found, return 0", uid);
            break;
        }
        case StatsUtils::STATS_TYPE_BLUETOOTH_BLE_SCAN: {
            auto bleIter = appBluetoothBlePowerMap_.find(uid);
            if (bleIter != appBluetoothBlePowerMap_.end()) {
                power = bleIter->second;
                STATS_HILOGD(COMP_SVC, "Get blueooth Ble scan power consumption: %{public}lfmAh for uid: %{public}d",
                    power, uid);
                break;
            }
            STATS_HILOGD(COMP_SVC,
                "No bluetooth Ble scan power consumption related to uid: %{public}d was found, return 0", uid);
            break;
        }
        default:
            break;
    }
    return power;
}

void BluetoothEntity::Reset()
{
    // Reset Bluetooth on timer and power consumption
    bluetoothBrPowerMah_ = StatsUtils::DEFAULT_VALUE;
    bluetoothBlePowerMah_ = StatsUtils::DEFAULT_VALUE;
    bluetoothPowerMah_ = StatsUtils::DEFAULT_VALUE;
    if (bluetoothBrOnTimer_) {
        bluetoothBrOnTimer_->Reset();
    }

    if (bluetoothBleOnTimer_) {
        bluetoothBleOnTimer_->Reset();
    }

    // Reset app Bluetooth scan power consumption
    for (auto& iter : appBluetoothBrPowerMap_) {
        iter.second = StatsUtils::DEFAULT_VALUE;
    }

    for (auto& iter : appBluetoothBlePowerMap_) {
        iter.second = StatsUtils::DEFAULT_VALUE;
    }

    for (auto& iter : appBluetoothPowerMap_) {
        iter.second = StatsUtils::DEFAULT_VALUE;
    }

    // Reset Bluetooth scan timer
    for (auto& iter : appBluetoothBrScanTimerMap_) {
        if (iter.second) {
            iter.second->Reset();
        }
    }

    for (auto& iter : appBluetoothBleScanTimerMap_) {
        if (iter.second) {
            iter.second->Reset();
        }
    }
}

std::shared_ptr<StatsHelper::ActiveTimer> BluetoothEntity::GetOrCreateTimer(int32_t uid,
    StatsUtils::StatsType statsType, int16_t level)
{
    std::shared_ptr<StatsHelper::ActiveTimer> timer = nullptr;
    switch (statsType) {
        case StatsUtils::STATS_TYPE_BLUETOOTH_BR_SCAN: {
            auto brScanIter = appBluetoothBrScanTimerMap_.find(uid);
            if (brScanIter != appBluetoothBrScanTimerMap_.end()) {
                STATS_HILOGD(COMP_SVC, "Get blueooth Br scan timer for uid: %{public}d", uid);
                timer = brScanIter->second;
                break;
            }
            STATS_HILOGD(COMP_SVC, "Create bluetooth Br scan timer for uid: %{public}d", uid);
            std::shared_ptr<StatsHelper::ActiveTimer> brScanTimer = std::make_shared<StatsHelper::ActiveTimer>();
            appBluetoothBrScanTimerMap_.insert(
                std::pair<int32_t, std::shared_ptr<StatsHelper::ActiveTimer>>(uid, brScanTimer));
            timer = brScanTimer;
            break;
        }
        case StatsUtils::STATS_TYPE_BLUETOOTH_BLE_SCAN: {
            auto bleScanIter = appBluetoothBleScanTimerMap_.find(uid);
            if (bleScanIter != appBluetoothBleScanTimerMap_.end()) {
                STATS_HILOGD(COMP_SVC, "Get blueooth Ble scan timer for uid: %{public}d", uid);
                timer = bleScanIter->second;
                break;
            }
            STATS_HILOGD(COMP_SVC, "Create bluetooth Ble scan timer for uid: %{public}d", uid);
            std::shared_ptr<StatsHelper::ActiveTimer> bleScanTimer = std::make_shared<StatsHelper::ActiveTimer>();
            appBluetoothBleScanTimerMap_.insert(
                std::pair<int32_t, std::shared_ptr<StatsHelper::ActiveTimer>>(uid, bleScanTimer));
            timer = bleScanTimer;
            break;
        }
        default:
            STATS_HILOGW(COMP_SVC, "Create active timer failed");
            break;
    }
    return timer;
}

std::shared_ptr<StatsHelper::ActiveTimer> BluetoothEntity::GetOrCreateTimer(StatsUtils::StatsType statsType,
    int16_t level)
{
    std::shared_ptr<StatsHelper::ActiveTimer> timer = nullptr;
    switch (statsType) {
        case StatsUtils::STATS_TYPE_BLUETOOTH_BR_ON: {
            if (bluetoothBrOnTimer_ != nullptr) {
                STATS_HILOGD(COMP_SVC, "Get blueooth Br on timer");
                timer = bluetoothBrOnTimer_;
                break;
            }
            STATS_HILOGD(COMP_SVC, "Create blueooth Br on timer");
            bluetoothBrOnTimer_ = std::make_shared<StatsHelper::ActiveTimer>();
            timer = bluetoothBrOnTimer_;
            break;
        }
        case StatsUtils::STATS_TYPE_BLUETOOTH_BLE_ON: {
            if (bluetoothBleOnTimer_ != nullptr) {
                STATS_HILOGD(COMP_SVC, "Get blueooth Ble on timer");
                timer = bluetoothBleOnTimer_;
                break;
            }
            STATS_HILOGD(COMP_SVC, "Create blueooth Ble on timer");
            bluetoothBleOnTimer_ = std::make_shared<StatsHelper::ActiveTimer>();
            timer = bluetoothBleOnTimer_;
            break;
        }
        default:
            STATS_HILOGW(COMP_SVC, "Create active timer failed");
            break;
    }
    return timer;
}

double BluetoothEntity::GetBluetoothUidPower()
{
    double bluetoothUidPower = StatsUtils::DEFAULT_VALUE;
#ifdef SYS_MGR_CLIENT_ENABLE
    auto bundleObj =
        DelayedSingleton<AppExecFwk::SysMrgClient>::GetInstance()->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (bundleObj == nullptr) {
        STATS_HILOGW(COMP_SVC, "Failed to get bundle manager service, return 0");
        return bluetoothUidPower;
    }

    sptr<AppExecFwk::IBundleMgr> bmgr = iface_cast<AppExecFwk::IBundleMgr>(bundleObj);
    if (bmgr == nullptr) {
        STATS_HILOGW(COMP_SVC, "Failed to get bundle manager proxy, return 0");
        return bluetoothUidPower;
    }

    std::string bundleName = "com.ohos.bluetooth";
    std::string identity = IPCSkeleton::ResetCallingIdentity();
    int32_t bluetoothUid = bmgr->GetUidByBundleName(bundleName, AppExecFwk::Constants::DEFAULT_USERID);
    IPCSkeleton::SetCallingIdentity(identity);

    auto bss = BatteryStatsService::GetInstance();
    auto core = bss->GetBatteryStatsCore();
    auto uidEntity = core->GetEntity(BatteryStatsInfo::CONSUMPTION_TYPE_APP);
    if (uidEntity != nullptr) {
        bluetoothUidPower = uidEntity->GetEntityPowerMah(bluetoothUid);
    }
    STATS_HILOGD(COMP_SVC, "Get bluetooth uid power consumption: %{public}lfmAh", bluetoothUidPower);
#endif
    return bluetoothUidPower;
}

void BluetoothEntity::DumpInfo(std::string& result, int32_t uid)
{
    int64_t brOntime = GetActiveTimeMs(StatsUtils::STATS_TYPE_BLUETOOTH_BR_ON);
    int64_t bleOntime = GetActiveTimeMs(StatsUtils::STATS_TYPE_BLUETOOTH_BR_ON);
    result.append("Bluetooth dump:\n")
        .append("Bluetooth Br on time: ")
        .append(ToString(brOntime))
        .append("ms")
        .append("\n")
        .append("Bluetooth Ble on time: ")
        .append(ToString(bleOntime))
        .append("ms")
        .append("\n");
}
} // namespace PowerMgr
} // namespace OHOS
