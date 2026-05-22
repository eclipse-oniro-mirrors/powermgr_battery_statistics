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

#include "battery_stats_detector.h"
#include "battery_stats_service.h"
#include "stats_utils.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

using namespace OHOS::PowerMgr;

namespace {
constexpr size_t MIN_INPUT_SIZE = sizeof(uint32_t);
constexpr size_t MAX_STRING_BYTES = 32;
constexpr size_t STATS_TYPE_COUNT = 30;
constexpr size_t STATS_STATE_COUNT = 5;

template<typename T>
T ReadValue(const uint8_t* data, size_t size, size_t& offset)
{
    T value {};
    if (data == nullptr || offset + sizeof(T) > size) {
        return value;
    }
    value = *reinterpret_cast<const T*>(data + offset);
    offset += sizeof(T);
    return value;
}

std::string ReadString(const uint8_t* data, size_t size, size_t& offset)
{
    if (data == nullptr || offset >= size) {
        return {};
    }

    size_t length = std::min(static_cast<size_t>(data[offset] % MAX_STRING_BYTES), size - offset - 1);
    offset++;
    if (length == 0 || offset + length > size) {
        return {};
    }

    std::string value(reinterpret_cast<const char*>(data + offset), length);
    offset += length;
    return value;
}

StatsUtils::StatsType ToStatsType(uint8_t raw)
{
    return static_cast<StatsUtils::StatsType>(static_cast<int32_t>(raw % STATS_TYPE_COUNT) - 1);
}

StatsUtils::StatsState ToStatsState(uint8_t raw)
{
    return static_cast<StatsUtils::StatsState>(static_cast<int32_t>(raw % STATS_STATE_COUNT) - 1);
}

StatsUtils::StatsData BuildStatsData(const uint8_t* data, size_t size, size_t& offset, StatsUtils::StatsType type)
{
    StatsUtils::StatsData statsData;
    statsData.type = type;
    statsData.state = ToStatsState(ReadValue<uint8_t>(data, size, offset));
    statsData.uid = ReadValue<int32_t>(data, size, offset);
    statsData.pid = ReadValue<int32_t>(data, size, offset);
    statsData.eventDataName = ReadString(data, size, offset);
    statsData.eventDebugInfo = ReadString(data, size, offset);
    statsData.eventDataType = ReadValue<int32_t>(data, size, offset);
    statsData.eventDataExtra = ReadValue<int32_t>(data, size, offset);
    statsData.level = ReadValue<int16_t>(data, size, offset);
    statsData.time = ReadValue<int64_t>(data, size, offset);
    statsData.traffic = ReadValue<int64_t>(data, size, offset);
    statsData.deviceId = ReadString(data, size, offset);
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
        service_ = BatteryStatsService::GetInstance();
        service_->OnStart();
    }

    ~BatteryStatsDetectorFuzzer()
    {
        if (service_ != nullptr) {
            service_->OnStop();
        }
    }

    void Fuzz(const uint8_t* data, size_t size)
    {
        if (data == nullptr || size < MIN_INPUT_SIZE) {
            return;
        }

        auto detector = service_->GetBatteryStatsDetector();
        auto core = service_->GetBatteryStatsCore();
        if (detector == nullptr || core == nullptr) {
            return;
        }

        size_t offset = 0;
        for (auto type : GetDetectorTypes()) {
            StatsUtils::StatsData statsData = BuildStatsData(data, size, offset, type);
            detector->HandleStatsChangedEvent(statsData);
            if (offset >= size) {
                offset = 0;
            }
        }

        StatsUtils::StatsData fallbackData = BuildStatsData(data, size, offset, ToStatsType(data[0]));
        detector->HandleStatsChangedEvent(fallbackData);
        core->Reset();
    }

private:
    OHOS::sptr<BatteryStatsService> service_ = nullptr;
};

namespace {
BatteryStatsDetectorFuzzer g_fuzzer;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    g_fuzzer.Fuzz(data, size);
    return 0;
}
