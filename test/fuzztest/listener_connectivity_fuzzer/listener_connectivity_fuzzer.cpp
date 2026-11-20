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

#include "listener_connectivity_fuzzer.h"
#include "../battery_listener_fuzz_support.h"

#include "stats_hisysevent.h"

namespace OHOS {
namespace PowerMgr {
namespace {
constexpr uint8_t DIRECT_SENSOR_PARSE = 0;
constexpr uint8_t DIRECT_GNSS_PARSE = 1;
constexpr uint8_t DIRECT_BLUETOOTH_PARSE = 2;
constexpr uint8_t DIRECT_WIFI_PARSE = 3;
constexpr uint8_t DIRECT_PHONE_PARSE = 4;
constexpr uint8_t PARSER_COUNT = 5;
constexpr uint8_t BOOLEAN_MASK = 1;

std::string PickBluetoothEvent(uint8_t selector)
{
    static constexpr const char* events[] = {
        StatsHiSysEvent::BR_SWITCH_STATE,
        StatsHiSysEvent::DISCOVERY_STATE,
        StatsHiSysEvent::BLE_SWITCH_STATE,
        StatsHiSysEvent::BLE_SCAN_START,
        StatsHiSysEvent::BLE_SCAN_STOP,
        "UNKNOWN_BLUETOOTH_EVENT"
    };
    return events[selector % (sizeof(events) / sizeof(events[0]))];
}
void Exercise(BatteryStatsListener& listener, const std::string&,
    const cJSON* root, FuzzedDataProvider& reader)
{
    StatsUtils::StatsData statsData;
    switch (reader.ConsumeIntegral<uint8_t>() % PARSER_COUNT) {
        case DIRECT_SENSOR_PARSE:
            listener.ProcessSensorEvent(statsData, root,
                (reader.ConsumeIntegral<uint8_t>() & BOOLEAN_MASK) ? StatsHiSysEvent::POWER_SENSOR_GRAVITY :
                                                StatsHiSysEvent::POWER_SENSOR_PROXIMITY);
            break;
        case DIRECT_GNSS_PARSE:
            listener.ProcessGnssEvent(statsData, root);
            break;
        case DIRECT_BLUETOOTH_PARSE:
            listener.ProcessBluetoothEvent(statsData, root, PickBluetoothEvent(reader.ConsumeIntegral<uint8_t>()));
            break;
        case DIRECT_WIFI_PARSE:
            listener.ProcessWifiEvent(statsData, root,
                (reader.ConsumeIntegral<uint8_t>() & BOOLEAN_MASK) ? StatsHiSysEvent::WIFI_CONNECTION :
                                                             StatsHiSysEvent::WIFI_SCAN);
            break;
        case DIRECT_PHONE_PARSE:
            listener.ProcessPhoneEvent(statsData, root,
                (reader.ConsumeIntegral<uint8_t>() & BOOLEAN_MASK) ? StatsHiSysEvent::CALL_STATE :
                                                             StatsHiSysEvent::DATA_CONNECTION_STATE);
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
