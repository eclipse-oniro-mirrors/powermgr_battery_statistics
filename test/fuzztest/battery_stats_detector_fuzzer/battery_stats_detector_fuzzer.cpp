/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define FUZZ_PROJECT_NAME "battery_stats_detector_fuzzer"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <fuzzer/FuzzedDataProvider.h>

#include "battery_stats_core.h"
#include "battery_stats_detector.h"
#include "battery_stats_service.h"
#include "stats_utils.h"

using namespace OHOS::PowerMgr;

namespace {
constexpr int32_t STATS_TYPE_MIN = static_cast<int32_t>(StatsUtils::STATS_TYPE_INVALID);
constexpr int32_t STATS_TYPE_MAX = static_cast<int32_t>(StatsUtils::STATS_TYPE_ALARM);
constexpr int32_t STATS_STATE_MIN = static_cast<int32_t>(StatsUtils::STATS_STATE_INVALID);
constexpr int32_t STATS_STATE_MAX = static_cast<int32_t>(StatsUtils::STATS_STATE_WORKSCHEDULER_EXECUTED);
constexpr size_t MAX_STRING_BYTES = 32;

StatsUtils::StatsData ConsumeStatsData(FuzzedDataProvider& fdp)
{
    StatsUtils::StatsData statsData;
    statsData.state = static_cast<StatsUtils::StatsState>(
        fdp.ConsumeIntegralInRange<int32_t>(STATS_STATE_MIN, STATS_STATE_MAX));
    statsData.uid = fdp.ConsumeIntegral<int32_t>();
    statsData.pid = fdp.ConsumeIntegral<int32_t>();
    statsData.eventDataName = fdp.ConsumeRandomLengthString(MAX_STRING_BYTES);
    statsData.eventDebugInfo = fdp.ConsumeRandomLengthString(MAX_STRING_BYTES);
    statsData.eventDataType = fdp.ConsumeIntegral<int32_t>();
    statsData.eventDataExtra = fdp.ConsumeIntegral<int32_t>();
    statsData.level = fdp.ConsumeIntegral<int16_t>();
    statsData.time = fdp.ConsumeIntegral<int64_t>();
    statsData.traffic = fdp.ConsumeIntegral<int64_t>();
    statsData.deviceId = fdp.ConsumeRandomLengthString(MAX_STRING_BYTES);
    return statsData;
}

const std::vector<StatsUtils::StatsType>& GetDetectorTypes()
{
    static const std::vector<StatsUtils::StatsType> types = {
        StatsUtils::STATS_TYPE_WIFI_SCAN,
        StatsUtils::STATS_TYPE_ALARM,
        StatsUtils::STATS_TYPE_SENSOR_GRAVITY_ON,
        StatsUtils::STATS_TYPE_SENSOR_PROXIMITY_ON,
        StatsUtils::STATS_TYPE_SCREEN_ON,
        StatsUtils::STATS_TYPE_SCREEN_BRIGHTNESS,
        StatsUtils::STATS_TYPE_BLUETOOTH_BR_ON,
        StatsUtils::STATS_TYPE_BLUETOOTH_BR_SCAN,
        StatsUtils::STATS_TYPE_BLUETOOTH_BLE_ON,
        StatsUtils::STATS_TYPE_BLUETOOTH_BLE_SCAN,
        StatsUtils::STATS_TYPE_WIFI_ON,
        StatsUtils::STATS_TYPE_PHONE_ACTIVE,
        StatsUtils::STATS_TYPE_PHONE_DATA,
        StatsUtils::STATS_TYPE_CAMERA_ON,
        StatsUtils::STATS_TYPE_CAMERA_FLASHLIGHT_ON,
        StatsUtils::STATS_TYPE_FLASHLIGHT_ON,
        StatsUtils::STATS_TYPE_GNSS_ON,
        StatsUtils::STATS_TYPE_AUDIO_ON,
        StatsUtils::STATS_TYPE_WAKELOCK_HOLD,
        StatsUtils::STATS_TYPE_BATTERY,
        StatsUtils::STATS_TYPE_WORKSCHEDULER,
        StatsUtils::STATS_TYPE_THERMAL,
        StatsUtils::STATS_TYPE_DISTRIBUTEDSCHEDULER,
        StatsUtils::STATS_TYPE_DISPLAY
    };
    return types;
}
} // namespace

class BatteryStatsDetectorFuzzer {
public:
    BatteryStatsDetectorFuzzer()
    {
        OHOS::sptr<BatteryStatsService> service = BatteryStatsService::GetInstance();
        if (service == nullptr) {
            return;
        }
        service->OnStart();
        detector_ = service->GetBatteryStatsDetector();
        core_ = service->GetBatteryStatsCore();
        service_ = service;
    }

    ~BatteryStatsDetectorFuzzer()
    {
        if (service_ != nullptr) {
            service_->OnStop();
        }
        core_ = nullptr;
        detector_ = nullptr;
        service_ = nullptr;
    }

    void Fuzz(FuzzedDataProvider& fdp)
    {
        if (detector_ == nullptr) {
            return;
        }

        // The event payload is consumed once and replayed for every stats type, so that one input still
        // walks all detector branches instead of being spent on the first few types
        StatsUtils::StatsData statsData = ConsumeStatsData(fdp);
        for (auto type : GetDetectorTypes()) {
            statsData.type = type;
            detector_->HandleStatsChangedEvent(statsData);
        }

        statsData.type = static_cast<StatsUtils::StatsType>(
            fdp.ConsumeIntegralInRange<int32_t>(STATS_TYPE_MIN, STATS_TYPE_MAX));
        detector_->HandleStatsChangedEvent(statsData);

        if (core_ != nullptr) {
            core_->Reset();
        }
    }

private:
    std::shared_ptr<BatteryStatsDetector> detector_ = nullptr;
    std::shared_ptr<BatteryStatsCore> core_ = nullptr;
    OHOS::sptr<BatteryStatsService> service_ = nullptr;
};

namespace {
BatteryStatsDetectorFuzzer& GetFuzzer()
{
    static BatteryStatsDetectorFuzzer fuzzer;
    return fuzzer;
}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }

    FuzzedDataProvider fdp(data, size);
    GetFuzzer().Fuzz(fdp);
    return 0;
}
