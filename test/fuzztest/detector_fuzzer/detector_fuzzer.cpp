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

#include "detector_fuzzer.h"

#include "../battery_fuzz_input.h"

#include "battery_stats_core.h"
#include "battery_stats_detector.h"
#include "battery_stats_service.h"
#include "stats_utils.h"

#include <algorithm>

namespace OHOS {
namespace PowerMgr {

namespace {
constexpr size_t MIN_ITEM_COUNT = 1;
constexpr int32_t UNKNOWN_STATS_TYPE_VALUE = 30;
constexpr int32_t NEGATIVE_UNKNOWN_STATS_TYPE_VALUE = -8;
constexpr int32_t UNKNOWN_STATS_STATE_VALUE = 9;
constexpr size_t MAX_EVENTS_PER_INPUT = 8;
constexpr size_t MAX_STRING_LENGTH = 64;

StatsUtils::StatsType PickStatsType(uint8_t selector)
{
    static constexpr StatsUtils::StatsType types[] = {
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
        StatsUtils::STATS_TYPE_THERMAL,
        StatsUtils::STATS_TYPE_BATTERY,
        StatsUtils::STATS_TYPE_WORKSCHEDULER,
        StatsUtils::STATS_TYPE_DISPLAY,
        StatsUtils::STATS_TYPE_DISTRIBUTEDSCHEDULER,
        StatsUtils::STATS_TYPE_INVALID,
        static_cast<StatsUtils::StatsType>(UNKNOWN_STATS_TYPE_VALUE),
        static_cast<StatsUtils::StatsType>(NEGATIVE_UNKNOWN_STATS_TYPE_VALUE),
    };
    return types[selector % (sizeof(types) / sizeof(types[0]))];
}

StatsUtils::StatsState PickStatsState(uint8_t selector)
{
    static constexpr StatsUtils::StatsState states[] = {
        StatsUtils::STATS_STATE_INVALID,
        StatsUtils::STATS_STATE_ACTIVATED,
        StatsUtils::STATS_STATE_DEACTIVATED,
        StatsUtils::STATS_STATE_WORKSCHEDULER_SCHEDULED,
        StatsUtils::STATS_STATE_WORKSCHEDULER_EXECUTED,
        static_cast<StatsUtils::StatsState>(UNKNOWN_STATS_STATE_VALUE),
    };
    return states[selector % (sizeof(states) / sizeof(states[0]))];
}

StatsUtils::StatsData BuildStatsData(FuzzedDataProvider& reader)
{
    StatsUtils::StatsData statsData;
    uint8_t operation = reader.ConsumeIntegral<uint8_t>();
    statsData.type = PickStatsType(operation);
    statsData.state = PickStatsState(reader.ConsumeIntegral<uint8_t>());
    statsData.uid = ConsumeOr<int32_t>(reader, StatsUtils::INVALID_VALUE);
    statsData.pid = ConsumeOr<int32_t>(reader, StatsUtils::INVALID_VALUE);
    statsData.level = ConsumeOr<int16_t>(reader, StatsUtils::INVALID_VALUE);
    statsData.time = ConsumeOr<int64_t>(reader, StatsUtils::DEFAULT_VALUE);
    statsData.traffic = ConsumeOr<int64_t>(reader, StatsUtils::DEFAULT_VALUE);
    statsData.eventDataType = ConsumeOr<int32_t>(reader, StatsUtils::INVALID_VALUE);
    statsData.eventDataExtra = ConsumeOr<int32_t>(reader, StatsUtils::INVALID_VALUE);
    statsData.eventDataName = reader.ConsumeRandomLengthString(MAX_STRING_LENGTH);
    statsData.eventDebugInfo = reader.ConsumeRandomLengthString(MAX_STRING_LENGTH);
    statsData.deviceId = reader.ConsumeRandomLengthString(MAX_STRING_LENGTH);
    return statsData;
}

class DetectorFuzzer {
public:
    DetectorFuzzer()
    {
        service_ = BatteryStatsService::GetInstance();
        if (service_ != nullptr) {
            service_->OnStart();
        }
    }

    ~DetectorFuzzer()
    {
        if (service_ != nullptr) {
            service_->OnStop();
        }
    }

    void FuzzDetector(const uint8_t* data, size_t size)
    {
        if (data == nullptr || size == 0 || service_ == nullptr) {
            return;
        }

        auto detector = service_->GetBatteryStatsDetector();
        if (detector == nullptr) {
            return;
        }

        FuzzedDataProvider reader(data, size);
        size_t eventCount = MIN_ITEM_COUNT + (reader.ConsumeIntegral<uint8_t>() % MAX_EVENTS_PER_INPUT);
        for (size_t eventIndex = 0; eventIndex < eventCount; ++eventIndex) {
            detector->HandleStatsChangedEvent(BuildStatsData(reader));
        }

        auto core = service_->GetBatteryStatsCore();
        if (core != nullptr) {
            core->Reset();
        }
    }

private:
    OHOS::sptr<BatteryStatsService> service_ = nullptr;
};
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    static DetectorFuzzer fuzzer;
    fuzzer.FuzzDetector(data, size);
    return 0;
}

} // namespace PowerMgr
} // namespace OHOS
