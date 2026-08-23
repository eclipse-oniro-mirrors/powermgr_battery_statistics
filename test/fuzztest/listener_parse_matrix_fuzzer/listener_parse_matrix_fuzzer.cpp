/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "listener_parse_matrix_fuzzer.h"

#include "battery_stats_core.h"
#include "battery_stats_listener.h"
#include "battery_stats_service.h"
#include "stats_hisysevent.h"
#include "stats_utils.h"

#include <algorithm>
#include <memory>
#include <vector>

#include <cJSON.h>

namespace OHOS {
namespace PowerMgr {

namespace {
constexpr size_t BYTE_BITS = 8;
constexpr size_t MIN_ITEM_COUNT = 1;
constexpr size_t STRING_LENGTH_RANGE_OFFSET = 1;
constexpr uint8_t BOOLEAN_MASK = 1;
constexpr size_t MAX_EVENT_COUNT = 6;
constexpr size_t MAX_STRING_LENGTH = 64;
constexpr size_t MAX_ARRAY_ITEMS = 4;
constexpr uint8_t JSON_NUMBER_VALUE_KIND = 0;
constexpr uint8_t JSON_STRING_VALUE_KIND = 1;
constexpr uint8_t JSON_BOOL_VALUE_KIND = 2;
constexpr uint8_t JSON_NULL_VALUE_KIND = 3;
constexpr uint8_t JSON_ARRAY_VALUE_KIND = 4;
constexpr uint8_t JSON_VALUE_KIND_COUNT = 6;
constexpr uint8_t DIRECT_CAMERA_PARSE = 0;
constexpr uint8_t DIRECT_AUDIO_PARSE = 1;
constexpr uint8_t DIRECT_SENSOR_PARSE = 2;
constexpr uint8_t DIRECT_GNSS_PARSE = 3;
constexpr uint8_t DIRECT_BLUETOOTH_PARSE = 4;
constexpr uint8_t DIRECT_WIFI_PARSE = 5;
constexpr uint8_t DIRECT_PHONE_PARSE = 6;
constexpr uint8_t DIRECT_WAKELOCK_PARSE = 7;
constexpr uint8_t DIRECT_DISPLAY_PARSE = 8;
constexpr uint8_t DIRECT_BATTERY_PARSE = 9;
constexpr uint8_t DIRECT_THERMAL_PARSE = 10;
constexpr uint8_t DIRECT_WORKSCHEDULER_PARSE = 11;
constexpr uint8_t DIRECT_POWER_WORKSCHEDULER_PARSE = 12;
constexpr uint8_t DIRECT_OTHERS_WORKSCHEDULER_PARSE = 13;
constexpr uint8_t DIRECT_DISTRIBUTED_PARSE = 14;
constexpr uint8_t DIRECT_ALARM_PARSE = 15;
constexpr uint8_t DIRECT_DEBUG_PARSE = 16;
constexpr uint8_t DIRECT_INTERNAL_PARSE = 17;
constexpr uint8_t DIRECT_PARSE_KIND_COUNT = 18;

class FuzzReader {
public:
    FuzzReader(const uint8_t* data, size_t size) : data_(data), size_(size) {}

    template <typename T>
    T Read(T fallback = T())
    {
        if (Remaining() < sizeof(T)) {
            return fallback;
        }
        uint64_t value = 0;
        for (size_t index = 0; index < sizeof(T); ++index) {
            value |= static_cast<uint64_t>(data_[offset_ + index]) << (BYTE_BITS * index);
        }
        offset_ += sizeof(T);
        return static_cast<T>(value);
    }

    std::string ReadString(size_t maxLength)
    {
        if (Remaining() == 0) {
            return "";
        }
        size_t length = Read<uint8_t>(0) % (maxLength + STRING_LENGTH_RANGE_OFFSET);
        length = std::min(length, Remaining());
        std::string value;
        value.reserve(length);
        for (size_t index = 0; index < length; ++index) {
            value.push_back(static_cast<char>(data_[offset_++]));
        }
        return value;
    }

private:
    size_t Remaining() const
    {
        return size_ - offset_;
    }

    const uint8_t* data_ = nullptr;
    size_t size_ = 0;
    size_t offset_ = 0;
};

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

std::vector<char> MakeCStringBuffer(const std::string& value)
{
    std::vector<char> buffer(value.begin(), value.end());
    buffer.push_back('\0');
    return buffer;
}

cJSON* CreateJsonString(const std::string& value)
{
    std::vector<char> buffer = MakeCStringBuffer(value);
    return cJSON_CreateString(&buffer[0]);
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

cJSON* CreateFuzzJsonValue(FuzzReader& reader)
{
    switch (reader.Read<uint8_t>(0) % JSON_VALUE_KIND_COUNT) {
        case JSON_NUMBER_VALUE_KIND:
            return cJSON_CreateNumber(reader.Read<int32_t>(0));
        case JSON_STRING_VALUE_KIND: {
            std::string value = reader.ReadString(MAX_STRING_LENGTH);
            return CreateJsonString(value);
        }
        case JSON_BOOL_VALUE_KIND:
            return cJSON_CreateBool((reader.Read<uint8_t>(0) & BOOLEAN_MASK) != 0);
        case JSON_NULL_VALUE_KIND:
            return cJSON_CreateNull();
        case JSON_ARRAY_VALUE_KIND: {
            cJSON* array = cJSON_CreateArray();
            size_t itemCount = reader.Read<uint8_t>(0) % (MAX_ARRAY_ITEMS + STRING_LENGTH_RANGE_OFFSET);
            for (size_t index = 0; index < itemCount; ++index) {
                AddArrayItem(array, cJSON_CreateNumber(reader.Read<int32_t>(0)));
            }
            return array;
        }
        default: {
            cJSON* object = cJSON_CreateObject();
            AddJsonItem(object, "nested", cJSON_CreateNumber(reader.Read<int32_t>(0)));
            return object;
        }
    }
}

void PopulateEventJson(cJSON* root, FuzzReader& reader)
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

void ExerciseConnectivityParser(BatteryStatsListener& listener, StatsUtils::StatsData& statsData, const cJSON* root,
    uint8_t parserKind, FuzzReader& reader)
{
    switch (parserKind) {
        case DIRECT_SENSOR_PARSE:
            listener.ProcessSensorEvent(statsData, root,
                (reader.Read<uint8_t>(0) & BOOLEAN_MASK) ? StatsHiSysEvent::POWER_SENSOR_GRAVITY :
                                                StatsHiSysEvent::POWER_SENSOR_PROXIMITY);
            break;
        case DIRECT_GNSS_PARSE:
            listener.ProcessGnssEvent(statsData, root);
            break;
        case DIRECT_BLUETOOTH_PARSE:
            listener.ProcessBluetoothEvent(statsData, root, PickBluetoothEvent(reader.Read<uint8_t>(0)));
            break;
        case DIRECT_WIFI_PARSE:
            listener.ProcessWifiEvent(statsData, root,
                (reader.Read<uint8_t>(0) & BOOLEAN_MASK) ? StatsHiSysEvent::WIFI_CONNECTION :
                                                             StatsHiSysEvent::WIFI_SCAN);
            break;
        case DIRECT_PHONE_PARSE:
            listener.ProcessPhoneEvent(statsData, root,
                (reader.Read<uint8_t>(0) & BOOLEAN_MASK) ? StatsHiSysEvent::CALL_STATE :
                                                             StatsHiSysEvent::DATA_CONNECTION_STATE);
            break;
        default:
            break;
    }
}

void ExerciseDeviceParser(BatteryStatsListener& listener, StatsUtils::StatsData& statsData, const cJSON* root,
    uint8_t parserKind, FuzzReader& reader)
{
    switch (parserKind) {
        case DIRECT_CAMERA_PARSE:
            listener.ProcessCameraEvent(statsData, root, PickCameraEvent(reader.Read<uint8_t>(0)));
            break;
        case DIRECT_AUDIO_PARSE:
            listener.ProcessAudioEvent(statsData, root);
            break;
        case DIRECT_WAKELOCK_PARSE:
            listener.ProcessWakelockEvent(statsData, root);
            break;
        case DIRECT_DISPLAY_PARSE:
            listener.ProcessDisplayEvent(statsData, root, PickDisplayEvent(reader.Read<uint8_t>(0)));
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

void ExerciseWorkParser(BatteryStatsListener& listener, StatsUtils::StatsData& statsData, const cJSON* root,
    uint8_t parserKind)
{
    switch (parserKind) {
        case DIRECT_WORKSCHEDULER_PARSE:
            listener.ProcessWorkschedulerEvent(statsData, root);
            break;
        case DIRECT_POWER_WORKSCHEDULER_PARSE:
            listener.ProcessPowerWorkschedulerEvent(statsData, root);
            break;
        case DIRECT_OTHERS_WORKSCHEDULER_PARSE:
            listener.ProcessOthersWorkschedulerEvent(statsData, root);
            break;
        case DIRECT_DISTRIBUTED_PARSE:
            listener.ProcessDistributedSchedulerEvent(statsData, root);
            break;
        case DIRECT_ALARM_PARSE:
            listener.ProcessAlarmEvent(statsData, root);
            break;
        default:
            break;
    }
}

void ExerciseDebugParser(BatteryStatsListener& listener, StatsUtils::StatsData& statsData, const std::string& eventName,
    const cJSON* root, uint8_t parserKind)
{
    switch (parserKind) {
        case DIRECT_DEBUG_PARSE:
            listener.ProcessDisplayDebugInfo(statsData, root);
            listener.ProcessPhoneDebugInfo(statsData, root);
            break;
        case DIRECT_INTERNAL_PARSE:
            listener.ProcessHiSysEventInternal(statsData, eventName, root);
            listener.ProcessOthersWorkschedulerEventInternal(statsData, root);
            listener.ProcessThermalEventInternal(statsData, root);
            listener.ProcessDisplayDebugInfoInternal(statsData, root);
            break;
        default:
            break;
    }
}

void ExerciseDirectParser(BatteryStatsListener& listener, const std::string& eventName, const cJSON* root,
    FuzzReader& reader)
{
    StatsUtils::StatsData statsData;
    uint8_t parserKind = reader.Read<uint8_t>(0) % DIRECT_PARSE_KIND_COUNT;
    ExerciseConnectivityParser(listener, statsData, root, parserKind, reader);
    ExerciseDeviceParser(listener, statsData, root, parserKind, reader);
    ExerciseWorkParser(listener, statsData, root, parserKind);
    ExerciseDebugParser(listener, statsData, eventName, root, parserKind);
}

class ListenerParseMatrixFuzzer {
public:
    ListenerParseMatrixFuzzer()
    {
        service_ = BatteryStatsService::GetInstance();
        if (service_ != nullptr) {
            service_->OnStart();
        }
    }

    ~ListenerParseMatrixFuzzer()
    {
        if (service_ != nullptr) {
            service_->OnStop();
        }
    }

    void FuzzListener(const uint8_t* data, size_t size)
    {
        if (data == nullptr || size == 0 || service_ == nullptr) {
            return;
        }

        BatteryStatsListener listener;
        FuzzReader reader(data, size);
        size_t eventCount = MIN_ITEM_COUNT + (reader.Read<uint8_t>(0) % MAX_EVENT_COUNT);
        for (size_t eventIndex = 0; eventIndex < eventCount; ++eventIndex) {
            cJSON* root = cJSON_CreateObject();
            if (root == nullptr) {
                return;
            }
            PopulateEventJson(root, reader);
            std::string eventName = PickHiSysEvent(reader.Read<uint8_t>(0));
            listener.ProcessHiSysEvent(eventName, root);
            StatsUtils::StatsData workData;
            listener.ProcessWorkschedulerEvent(workData, root);
            ExerciseDirectParser(listener, eventName, root, reader);
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

ListenerParseMatrixFuzzer g_fuzzer;
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    g_fuzzer.FuzzListener(data, size);
    return 0;
}

} // namespace PowerMgr
} // namespace OHOS

