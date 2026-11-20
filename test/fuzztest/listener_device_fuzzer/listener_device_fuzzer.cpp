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

#include "listener_device_fuzzer.h"
#include "../battery_listener_fuzz_support.h"

#include "stats_hisysevent.h"

namespace OHOS {
namespace PowerMgr {
namespace {
constexpr uint8_t DIRECT_CAMERA_PARSE = 0;
constexpr uint8_t DIRECT_AUDIO_PARSE = 1;
constexpr uint8_t DIRECT_WAKELOCK_PARSE = 2;
constexpr uint8_t DIRECT_DISPLAY_PARSE = 3;
constexpr uint8_t DIRECT_BATTERY_PARSE = 4;
constexpr uint8_t DIRECT_THERMAL_PARSE = 5;
constexpr uint8_t PARSER_COUNT = 6;

std::string PickCameraEvent(uint8_t selector)
{
    static constexpr const char* events[] = {
        StatsHiSysEvent::CAMERA_CONNECT,
        StatsHiSysEvent::CAMERA_DISCONNECT,
        StatsHiSysEvent::FLASHLIGHT_ON,
        StatsHiSysEvent::FLASHLIGHT_OFF,
        "UNKNOWN_CAMERA_EVENT"
    };
    return events[selector % (sizeof(events) / sizeof(events[0]))];
}

std::string PickDisplayEvent(uint8_t selector)
{
    static constexpr const char* events[] = {
        StatsHiSysEvent::SCREEN_STATE,
        StatsHiSysEvent::BRIGHTNESS_NIT,
        StatsHiSysEvent::BACKLIGHT_DISCOUNT,
        StatsHiSysEvent::AMBIENT_LIGHT,
        "UNKNOWN_DISPLAY_EVENT"
    };
    return events[selector % (sizeof(events) / sizeof(events[0]))];
}
void Exercise(BatteryStatsListener& listener, const std::string&,
    const cJSON* root, FuzzedDataProvider& reader)
{
    StatsUtils::StatsData statsData;
    switch (reader.ConsumeIntegral<uint8_t>() % PARSER_COUNT) {
        case DIRECT_CAMERA_PARSE:
            listener.ProcessCameraEvent(statsData, root, PickCameraEvent(reader.ConsumeIntegral<uint8_t>()));
            break;
        case DIRECT_AUDIO_PARSE:
            listener.ProcessAudioEvent(statsData, root);
            break;
        case DIRECT_WAKELOCK_PARSE:
            listener.ProcessWakelockEvent(statsData, root);
            break;
        case DIRECT_DISPLAY_PARSE:
            listener.ProcessDisplayEvent(statsData, root, PickDisplayEvent(reader.ConsumeIntegral<uint8_t>()));
            break;
        case DIRECT_BATTERY_PARSE:
            listener.ProcessBatteryEvent(statsData, root);
            break;
        case DIRECT_THERMAL_PARSE:
            listener.ProcessThermalEvent(statsData, root);
            break;
        default:
            break;
    }
}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    RunListenerFuzzCase(data, size, Exercise);
    return 0;
}

} // namespace PowerMgr
} // namespace OHOS
