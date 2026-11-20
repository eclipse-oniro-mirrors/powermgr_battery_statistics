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

#include "battery_listener_fuzz_support.h"
#include <algorithm>
#include "battery_stats_core.h"
#include "battery_stats_service.h"
#include "stats_hisysevent.h"

namespace OHOS {
namespace PowerMgr {
namespace {
constexpr size_t MIN_ITEM_COUNT = 1;
constexpr size_t PARSER_INPUT_DIVISOR = 2;
constexpr size_t STRING_LENGTH_RANGE_OFFSET = 1;
constexpr uint8_t BOOLEAN_MASK = 1;
constexpr size_t MAX_EVENT_COUNT = 6;
constexpr size_t MAX_PARSER_INPUT = 8;
constexpr size_t MAX_STRING_LENGTH = 64;
constexpr size_t MAX_ARRAY_ITEMS = 4;
constexpr uint8_t JSON_NUMBER_VALUE_KIND = 0;
constexpr uint8_t JSON_STRING_VALUE_KIND = 1;
constexpr uint8_t JSON_BOOL_VALUE_KIND = 2;
constexpr uint8_t JSON_NULL_VALUE_KIND = 3;
constexpr uint8_t JSON_ARRAY_VALUE_KIND = 4;
constexpr uint8_t JSON_VALUE_KIND_COUNT = 6;

void AddJsonItem(cJSON* root, const char* key, cJSON* value)
{
    if (root == nullptr || key == nullptr || value == nullptr) {
        cJSON_Delete(value);
        return;
    }
    if (!cJSON_AddItemToObject(root, key, value)) {
        cJSON_Delete(value);
    }
}

cJSON* CreateJsonString(std::string value)
{
    std::replace(value.begin(), value.end(), char{}, ' ');
    return cJSON_CreateString(value.c_str());
}

void AddArrayItem(cJSON* array, cJSON* value)
{
    if (array == nullptr || value == nullptr) {
        cJSON_Delete(value);
        return;
    }
    if (!cJSON_AddItemToArray(array, value)) {
        cJSON_Delete(value);
    }
}

cJSON* CreateFuzzJsonValue(FuzzedDataProvider& reader)
{
    switch (reader.ConsumeIntegral<uint8_t>() % JSON_VALUE_KIND_COUNT) {
        case JSON_NUMBER_VALUE_KIND:
            return cJSON_CreateNumber(reader.ConsumeIntegral<int32_t>());
        case JSON_STRING_VALUE_KIND: {
            std::string value = reader.ConsumeRandomLengthString(MAX_STRING_LENGTH);
            return CreateJsonString(value);
        }
        case JSON_BOOL_VALUE_KIND:
            return cJSON_CreateBool((reader.ConsumeIntegral<uint8_t>() & BOOLEAN_MASK) != 0);
        case JSON_NULL_VALUE_KIND:
            return cJSON_CreateNull();
        case JSON_ARRAY_VALUE_KIND: {
            cJSON* array = cJSON_CreateArray();
            size_t itemCount = reader.ConsumeIntegral<uint8_t>() % (MAX_ARRAY_ITEMS + STRING_LENGTH_RANGE_OFFSET);
            for (size_t index = 0; index < itemCount; ++index) {
                AddArrayItem(array, cJSON_CreateNumber(reader.ConsumeIntegral<int32_t>()));
            }
            return array;
        }
        default: {
            cJSON* object = cJSON_CreateObject();
            AddJsonItem(object, "nested", cJSON_CreateNumber(reader.ConsumeIntegral<int32_t>()));
            return object;
        }
    }
}

void PopulateEventJson(cJSON* root, FuzzedDataProvider& reader)
{
    static constexpr const char* keys[] = {
        "UID", "PID", "STATE", "ID", "TYPE", "INTERVAL", "CALLING_TYPE", "CALLING_UID", "CALLING_PID",
        "TARGET_BUNDLE", "TARGET_ABILITY", "CALLING_APP_UID", "RESULT", "CALLER_UID", "CALLER_PID", "ACTION",
        "VALUE", "RATIO", "WORKID", "TRIGGER", "NAME", "name_", "SLOT_ID", "INDEX_ID", "LEVEL", "CHARGER",
        "VOLTAGE", "HEALTH", "TEMPERATURE", "NIT", "BRIGHTNESS"
    };

    for (const char* key : keys) {
        AddJsonItem(root, key, CreateFuzzJsonValue(reader));
    }
}

std::string PickHiSysEvent(uint8_t selector)
{
    static constexpr const char* events[] = {
        StatsHiSysEvent::POWER_RUNNINGLOCK,
        StatsHiSysEvent::SCREEN_STATE,
        StatsHiSysEvent::BRIGHTNESS_NIT,
        StatsHiSysEvent::BACKLIGHT_DISCOUNT,
        StatsHiSysEvent::AMBIENT_LIGHT,
        StatsHiSysEvent::BATTERY_CHANGED,
        StatsHiSysEvent::POWER_TEMPERATURE,
        StatsHiSysEvent::THERMAL_LEVEL_CHANGED,
        StatsHiSysEvent::THERMAL_ACTION_TRIGGERED,
        StatsHiSysEvent::POWER_WORKSCHEDULER,
        StatsHiSysEvent::WORK_ADD,
        StatsHiSysEvent::WORK_REMOVE,
        StatsHiSysEvent::WORK_START,
        StatsHiSysEvent::WORK_STOP,
        StatsHiSysEvent::CALL_STATE,
        StatsHiSysEvent::DATA_CONNECTION_STATE,
        StatsHiSysEvent::TORCH_STATE,
        StatsHiSysEvent::CAMERA_CONNECT,
        StatsHiSysEvent::CAMERA_DISCONNECT,
        StatsHiSysEvent::FLASHLIGHT_ON,
        StatsHiSysEvent::FLASHLIGHT_OFF,
        StatsHiSysEvent::STREAM_CHANGE,
        StatsHiSysEvent::POWER_SENSOR_GRAVITY,
        StatsHiSysEvent::POWER_SENSOR_PROXIMITY,
        StatsHiSysEvent::GNSS_STATE,
        StatsHiSysEvent::BR_SWITCH_STATE,
        StatsHiSysEvent::DISCOVERY_STATE,
        StatsHiSysEvent::BLE_SWITCH_STATE,
        StatsHiSysEvent::BLE_SCAN_START,
        StatsHiSysEvent::BLE_SCAN_STOP,
        StatsHiSysEvent::WIFI_CONNECTION,
        StatsHiSysEvent::WIFI_SCAN,
        StatsHiSysEvent::START_REMOTE_ABILITY,
        StatsHiSysEvent::MISC_TIME_STATISTIC_REPORT,
        "UNKNOWN_POWER_EVENT",
        ""
    };
    return events[selector % (sizeof(events) / sizeof(events[0]))];
}

class ListenerFuzzRunner {
public:
    ListenerFuzzRunner()
    {
        service_ = BatteryStatsService::GetInstance();
        if (service_ != nullptr) {
            service_->OnStart();
        }
    }

    ~ListenerFuzzRunner()
    {
        if (service_ != nullptr) {
            service_->OnStop();
        }
    }

    void FuzzListener(const uint8_t* data, size_t size, ListenerFuzzCase exercise)
    {
        if (data == nullptr || size == 0 || service_ == nullptr) {
            return;
        }

        BatteryStatsListener listener;
        FuzzedDataProvider reader(data, size);
        size_t eventCount = MIN_ITEM_COUNT + (reader.ConsumeIntegral<uint8_t>() % MAX_EVENT_COUNT);
        for (size_t eventIndex = 0; eventIndex < eventCount; ++eventIndex) {
            cJSON* root = cJSON_CreateObject();
            if (root == nullptr) {
                return;
            }
            std::string eventName = PickHiSysEvent(reader.ConsumeIntegral<uint8_t>());
            const auto parserInput = reader.ConsumeBytes<uint8_t>(
                std::min(MAX_PARSER_INPUT, reader.remaining_bytes() / PARSER_INPUT_DIVISOR));
            FuzzedDataProvider parserReader(parserInput.data(), parserInput.size());
            PopulateEventJson(root, reader);
            listener.ProcessHiSysEvent(eventName, root);
            exercise(listener, eventName, root, parserReader);
            cJSON_Delete(root);
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


void RunListenerFuzzCase(const uint8_t* data, size_t size, ListenerFuzzCase exercise)
{
    if (data == nullptr || size == 0) {
        return;
    }
    static ListenerFuzzRunner fuzzer;
    fuzzer.FuzzListener(data, size, exercise);
}

} // namespace PowerMgr
} // namespace OHOS
