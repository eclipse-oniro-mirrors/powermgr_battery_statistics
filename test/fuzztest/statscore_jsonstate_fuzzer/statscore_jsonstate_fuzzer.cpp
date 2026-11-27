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

#include "statscore_jsonstate_fuzzer.h"

#include "../battery_fuzz_input.h"

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
constexpr size_t MIN_ITEM_COUNT = 1;
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

cJSON* CreateJsonString(const std::string& value)
{
    std::vector<char> buffer(value.begin(), value.end());
    std::replace(buffer.begin(), buffer.end(), char{}, ' ');
    buffer.push_back(char{});
    return cJSON_CreateString(buffer.data());
}

void AddJsonItem(cJSON* root, const std::string& key, cJSON* value)
{
    std::vector<char> buffer(key.begin(), key.end());
    std::replace(buffer.begin(), buffer.end(), char{}, ' ');
    buffer.push_back(char{});
    AddJsonItem(root, buffer.data(), value);
}

cJSON* BuildMaybeWrongContainer(FuzzedDataProvider& reader)
{
    uint8_t operation = reader.ConsumeIntegral<uint8_t>() % CONTAINER_KIND_COUNT;
    switch (operation) {
        case CONTAINER_OBJECT_KIND:
            return cJSON_CreateObject();
        case CONTAINER_ARRAY_KIND:
            return cJSON_CreateArray();
        case CONTAINER_NUMBER_KIND:
            return cJSON_CreateNumber(reader.ConsumeIntegral<int32_t>());
        case CONTAINER_STRING_KIND: {
            std::string value = reader.ConsumeRandomLengthString(MAX_STRING_LENGTH);
            return CreateJsonString(value);
        }
        default:
            return cJSON_CreateNull();
    }
}

cJSON* BuildLeafValue(FuzzedDataProvider& reader)
{
    uint8_t operation = reader.ConsumeIntegral<uint8_t>() % LEAF_VALUE_KIND_COUNT;
    switch (operation) {
        case LEAF_NUMBER_KIND:
            return cJSON_CreateNumber(static_cast<double>(reader.ConsumeIntegral<int64_t>()) / POWER_VALUE_SCALE);
        case LEAF_STRING_KIND: {
            std::string value = reader.ConsumeRandomLengthString(MAX_STRING_LENGTH);
            return CreateJsonString(value);
        }
        case LEAF_NULL_KIND:
            return cJSON_CreateNull();
        case LEAF_OBJECT_KIND: {
            cJSON* object = cJSON_CreateObject();
            AddJsonItem(object, "power", cJSON_CreateNumber(reader.ConsumeIntegral<int32_t>()));
            return object;
        }
        case LEAF_ARRAY_KIND: {
            cJSON* array = cJSON_CreateArray();
            AddArrayItem(array, cJSON_CreateNumber(reader.ConsumeIntegral<int32_t>()));
            AddArrayItem(array, cJSON_CreateString("not-number"));
            return array;
        }
        default:
            return cJSON_CreateBool((reader.ConsumeIntegral<uint8_t>() & BOOLEAN_MASK) != 0);
    }
}

std::string PickPowerKey(FuzzedDataProvider& reader)
{
    static constexpr const char* keys[] = {
        "100", "0", "1", "200000", "-2", "-3", "-8", "-16", "-17", "-99", "uid", "", "99999999999999999999"
    };
    if ((reader.ConsumeIntegral<uint8_t>() % CUSTOM_POWER_KEY_SELECTOR) == 0) {
        return reader.ConsumeRandomLengthString(MAX_STRING_LENGTH);
    }
    return keys[reader.ConsumeIntegral<uint8_t>() % (sizeof(keys) / sizeof(keys[0]))];
}

void PopulatePower(cJSON* power, FuzzedDataProvider& reader)
{
    if (power == nullptr) {
        return;
    }
    size_t itemCount = MIN_ITEM_COUNT + (reader.ConsumeIntegral<uint8_t>() % MAX_POWER_ITEMS);
    for (size_t index = 0; index < itemCount; ++index) {
        std::string key = PickPowerKey(reader);
        if (cJSON_IsArray(power)) {
            AddArrayItem(power, BuildLeafValue(reader));
        } else if (cJSON_IsObject(power)) {
            AddJsonItem(power, key, BuildLeafValue(reader));
        }
    }
}

void PopulateSoftware(cJSON* software, FuzzedDataProvider& reader)
{
    if (!cJSON_IsObject(software)) {
        return;
    }
    static constexpr const char* metricKeys[] = {
        "camera_on", "flashlight_on", "gnss_on", "audio_on", "cpu_awake", "sensor_gravity", "sensor_proximity",
        "alarm", "cpu_time", "bluetooth_br_scan", "bluetooth_ble_scan", "unknown"
    };
    size_t uidCount = MIN_ITEM_COUNT + (reader.ConsumeIntegral<uint8_t>() % MAX_UID_OBJECTS);
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

void PopulateHardware(cJSON* hardware, FuzzedDataProvider& reader)
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

void SeedCoreState(const std::shared_ptr<BatteryStatsCore>& core, FuzzedDataProvider& reader)
{
    size_t eventCount = MIN_ITEM_COUNT + (reader.ConsumeIntegral<uint8_t>() % MAX_STATE_EVENTS);
    for (size_t index = 0; index < eventCount; ++index) {
        auto type = PickStatsType(reader.ConsumeIntegral<uint8_t>());
        int32_t uid = ConsumeOr<int32_t>(reader, StatsUtils::INVALID_VALUE);
        int16_t level = ConsumeOr<int16_t>(reader, StatsUtils::INVALID_VALUE);
        if ((reader.ConsumeIntegral<uint8_t>() & BOOLEAN_MASK) == 0) {
            core->UpdateStats(type, PickStatsState(reader.ConsumeIntegral<uint8_t>()), level, uid,
                reader.ConsumeRandomLengthString(MAX_STRING_LENGTH));
        } else {
            core->UpdateStats(type, reader.ConsumeIntegral<int64_t>(), reader.ConsumeIntegral<int64_t>(), uid);
        }
    }
}

cJSON* BuildStatsRoot(FuzzedDataProvider& reader)
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

    std::string extraKey = reader.ConsumeRandomLengthString(MAX_STRING_LENGTH);
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

        FuzzedDataProvider reader(data, size);
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
        if ((reader.ConsumeIntegral<uint8_t>() & BOOLEAN_MASK) != 0) {
            core->SaveBatteryStatsData();
        }
        if ((reader.ConsumeIntegral<uint8_t>() & BOOLEAN_MASK) != 0) {
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
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    static StatsCoreJsonStateFuzzer fuzzer;
    fuzzer.FuzzCoreJsonState(data, size);
    return 0;
}

} // namespace PowerMgr
} // namespace OHOS
