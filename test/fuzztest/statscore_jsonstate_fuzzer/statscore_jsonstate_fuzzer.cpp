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

#include "statscore_jsonstate_fuzzer.h"

#include "battery_stats_core.h"
#include "battery_stats_info.h"
#include "battery_stats_service.h"
#include "stats_utils.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <cJSON.h>

namespace OHOS {
namespace PowerMgr {

namespace {
constexpr size_t BYTE_BITS = 8;
constexpr size_t MIN_ITEM_COUNT = 1;
constexpr size_t STRING_LENGTH_RANGE_OFFSET = 1;
constexpr uint8_t BOOLEAN_MASK = 1;
constexpr size_t MAX_POWER_ITEMS = 12;
constexpr size_t MAX_STATE_EVENTS = 8;
constexpr size_t MAX_STRING_LENGTH = 48;
constexpr uint8_t CONTAINER_OBJECT_KIND = 0;
constexpr uint8_t CONTAINER_ARRAY_KIND = 1;
constexpr uint8_t CONTAINER_NUMBER_KIND = 2;
constexpr uint8_t CONTAINER_STRING_KIND = 3;
constexpr uint8_t CONTAINER_KIND_COUNT = 5;
constexpr uint8_t LEAF_NUMBER_KIND = 0;
constexpr uint8_t LEAF_STRING_KIND = 1;
constexpr uint8_t LEAF_NULL_KIND = 2;
constexpr uint8_t LEAF_OBJECT_KIND = 3;
constexpr uint8_t LEAF_ARRAY_KIND = 4;
constexpr uint8_t LEAF_VALUE_KIND_COUNT = 6;
constexpr double POWER_VALUE_SCALE = 1000.0;
constexpr uint8_t CUSTOM_POWER_KEY_SELECTOR = 4;
constexpr size_t MAX_UID_OBJECTS = 4;

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

void AddJsonItem(cJSON* root, const std::string& key, cJSON* value)
{
    std::vector<char> keyBuffer = MakeCStringBuffer(key);
    AddJsonItem(root, &keyBuffer[0], value);
}

cJSON* BuildMaybeWrongContainer(FuzzReader& reader)
{
    uint8_t operation = reader.Read<uint8_t>(0) % CONTAINER_KIND_COUNT;
    switch (operation) {
        case CONTAINER_OBJECT_KIND:
            return cJSON_CreateObject();
        case CONTAINER_ARRAY_KIND:
            return cJSON_CreateArray();
        case CONTAINER_NUMBER_KIND:
            return cJSON_CreateNumber(reader.Read<int32_t>(0));
        case CONTAINER_STRING_KIND: {
            std::string value = reader.ReadString(MAX_STRING_LENGTH);
            return CreateJsonString(value);
        }
        default:
            return cJSON_CreateNull();
    }
}

cJSON* BuildLeafValue(FuzzReader& reader)
{
    uint8_t operation = reader.Read<uint8_t>(0) % LEAF_VALUE_KIND_COUNT;
    switch (operation) {
        case LEAF_NUMBER_KIND:
            return cJSON_CreateNumber(static_cast<double>(reader.Read<int64_t>(0)) / POWER_VALUE_SCALE);
        case LEAF_STRING_KIND: {
            std::string value = reader.ReadString(MAX_STRING_LENGTH);
            return CreateJsonString(value);
        }
        case LEAF_NULL_KIND:
            return cJSON_CreateNull();
        case LEAF_OBJECT_KIND: {
            cJSON* object = cJSON_CreateObject();
            AddJsonItem(object, "power", cJSON_CreateNumber(reader.Read<int32_t>(0)));
            return object;
        }
        case LEAF_ARRAY_KIND: {
            cJSON* array = cJSON_CreateArray();
            AddArrayItem(array, cJSON_CreateNumber(reader.Read<int32_t>(0)));
            AddArrayItem(array, cJSON_CreateString("not-number"));
            return array;
        }
        default:
            return cJSON_CreateBool((reader.Read<uint8_t>(0) & BOOLEAN_MASK) != 0);
    }
}

std::string PickPowerKey(FuzzReader& reader)
{
    static constexpr const char* keys[] = {
        "100", "0", "1", "200000", "-2", "-3", "-8", "-16", "-17", "-99", "uid", "", "99999999999999999999"
    };
    if ((reader.Read<uint8_t>(0) % CUSTOM_POWER_KEY_SELECTOR) == 0) {
        return reader.ReadString(MAX_STRING_LENGTH);
    }
    return keys[reader.Read<uint8_t>(0) % (sizeof(keys) / sizeof(keys[0]))];
}

void PopulatePower(cJSON* power, FuzzReader& reader)
{
    if (power == nullptr) {
        return;
    }
    size_t itemCount = MIN_ITEM_COUNT + (reader.Read<uint8_t>(0) % MAX_POWER_ITEMS);
    for (size_t index = 0; index < itemCount; ++index) {
        std::string key = PickPowerKey(reader);
        if (cJSON_IsArray(power)) {
            AddArrayItem(power, BuildLeafValue(reader));
        } else if (cJSON_IsObject(power)) {
            AddJsonItem(power, key, BuildLeafValue(reader));
        }
    }
}

void PopulateSoftware(cJSON* software, FuzzReader& reader)
{
    if (!cJSON_IsObject(software)) {
        return;
    }
    static constexpr const char* metricKeys[] = {
        "camera_on", "flashlight_on", "gnss_on", "audio_on", "cpu_awake", "sensor_gravity", "sensor_proximity",
        "alarm", "cpu_time", "bluetooth_br_scan", "bluetooth_ble_scan", "unknown"
    };
    size_t uidCount = MIN_ITEM_COUNT + (reader.Read<uint8_t>(0) % MAX_UID_OBJECTS);
    for (size_t uidIndex = 0; uidIndex < uidCount; ++uidIndex) {
        std::string uidKey = PickPowerKey(reader);
        cJSON* uidObj = cJSON_CreateObject();
        if (uidObj == nullptr) {
            continue;
        }
        for (const char* key : metricKeys) {
            AddJsonItem(uidObj, key, BuildLeafValue(reader));
        }
        AddJsonItem(software, uidKey, uidObj);
    }
}

void PopulateHardware(cJSON* hardware, FuzzReader& reader)
{
    if (!cJSON_IsObject(hardware)) {
        return;
    }
    static constexpr const char* keys[] = {
        "bluetooth_br_on", "bluetooth_ble_on", "screen_on", "screen_brightness", "wifi_on", "wifi_scan",
        "cpu_idle", "radio_on", "radio_data", "unknown"
    };
    for (const char* key : keys) {
        AddJsonItem(hardware, key, BuildLeafValue(reader));
    }
}

StatsUtils::StatsType PickStatsType(uint8_t selector)
{
    static constexpr StatsUtils::StatsType types[] = {
        StatsUtils::STATS_TYPE_SCREEN_ON,
        StatsUtils::STATS_TYPE_SCREEN_BRIGHTNESS,
        StatsUtils::STATS_TYPE_BLUETOOTH_BR_ON,
        StatsUtils::STATS_TYPE_WIFI_ON,
        StatsUtils::STATS_TYPE_PHONE_ACTIVE,
        StatsUtils::STATS_TYPE_CAMERA_ON,
        StatsUtils::STATS_TYPE_FLASHLIGHT_ON,
        StatsUtils::STATS_TYPE_GNSS_ON,
        StatsUtils::STATS_TYPE_SENSOR_GRAVITY_ON,
        StatsUtils::STATS_TYPE_AUDIO_ON,
        StatsUtils::STATS_TYPE_WAKELOCK_HOLD,
        StatsUtils::STATS_TYPE_WIFI_SCAN,
        StatsUtils::STATS_TYPE_ALARM,
        StatsUtils::STATS_TYPE_INVALID,
    };
    return types[selector % (sizeof(types) / sizeof(types[0]))];
}

StatsUtils::StatsState PickStatsState(uint8_t selector)
{
    static constexpr StatsUtils::StatsState states[] = {
        StatsUtils::STATS_STATE_ACTIVATED,
        StatsUtils::STATS_STATE_DEACTIVATED,
        StatsUtils::STATS_STATE_INVALID,
    };
    return states[selector % (sizeof(states) / sizeof(states[0]))];
}

void SeedCoreState(const std::shared_ptr<BatteryStatsCore>& core, FuzzReader& reader)
{
    size_t eventCount = MIN_ITEM_COUNT + (reader.Read<uint8_t>(0) % MAX_STATE_EVENTS);
    for (size_t index = 0; index < eventCount; ++index) {
        auto type = PickStatsType(reader.Read<uint8_t>(0));
        int32_t uid = reader.Read<int32_t>(StatsUtils::INVALID_VALUE);
        int16_t level = reader.Read<int16_t>(StatsUtils::INVALID_VALUE);
        if ((reader.Read<uint8_t>(0) & BOOLEAN_MASK) == 0) {
            core->UpdateStats(type, PickStatsState(reader.Read<uint8_t>(0)), level, uid,
                reader.ReadString(MAX_STRING_LENGTH));
        } else {
            core->UpdateStats(type, reader.Read<int64_t>(0), reader.Read<int64_t>(0), uid);
        }
    }
}

cJSON* BuildStatsRoot(FuzzReader& reader)
{
    cJSON* root = cJSON_CreateObject();
    if (root == nullptr) {
        return nullptr;
    }

    cJSON* hardware = BuildMaybeWrongContainer(reader);
    PopulateHardware(hardware, reader);
    AddJsonItem(root, "Hardware", hardware);

    cJSON* software = BuildMaybeWrongContainer(reader);
    PopulateSoftware(software, reader);
    AddJsonItem(root, "Software", software);

    cJSON* power = BuildMaybeWrongContainer(reader);
    PopulatePower(power, reader);
    AddJsonItem(root, "Power", power);

    std::string extraKey = reader.ReadString(MAX_STRING_LENGTH);
    AddJsonItem(root, extraKey, BuildLeafValue(reader));
    return root;
}

class StatsCoreJsonStateFuzzer {
public:
    StatsCoreJsonStateFuzzer()
    {
        service_ = BatteryStatsService::GetInstance();
        if (service_ != nullptr) {
            service_->OnStart();
        }
    }

    ~StatsCoreJsonStateFuzzer()
    {
        if (service_ != nullptr) {
            service_->OnStop();
        }
    }

    void FuzzCoreJsonState(const uint8_t* data, size_t size)
    {
        if (data == nullptr || size == 0 || service_ == nullptr) {
            return;
        }
        auto core = service_->GetBatteryStatsCore();
        if (core == nullptr) {
            return;
        }

        FuzzReader reader(data, size);
        SeedCoreState(core, reader);
        cJSON* root = BuildStatsRoot(reader);
        if (root == nullptr) {
            core->Reset();
            return;
        }

        core->UpdateStatsEntity(root);
        core->SaveForPower(root);
        core->SaveForHardware(root);
        core->SaveForSoftware(root);
        if ((reader.Read<uint8_t>(0) & BOOLEAN_MASK) != 0) {
            core->SaveBatteryStatsData();
        }
        if ((reader.Read<uint8_t>(0) & BOOLEAN_MASK) != 0) {
            core->LoadBatteryStatsData();
        }

        std::string dump;
        core->DumpInfo(dump);
        cJSON_Delete(root);
        core->Reset();
    }

private:
    OHOS::sptr<BatteryStatsService> service_ = nullptr;
};

StatsCoreJsonStateFuzzer g_fuzzer;
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    g_fuzzer.FuzzCoreJsonState(data, size);
    return 0;
}

} // namespace PowerMgr
} // namespace OHOS
