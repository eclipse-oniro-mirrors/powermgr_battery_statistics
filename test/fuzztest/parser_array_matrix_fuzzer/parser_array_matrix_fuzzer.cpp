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

#include "parser_array_matrix_fuzzer.h"

#include "battery_stats_parser.h"
#include "stats_utils.h"

#include <algorithm>
#include <string>
#include <vector>

#include <cJSON.h>

#include <fuzzer/FuzzedDataProvider.h>

namespace OHOS {
namespace PowerMgr {

namespace {
constexpr size_t MIN_ITEM_COUNT = 1;
constexpr size_t STRING_LENGTH_RANGE_OFFSET = 1;
constexpr uint8_t BOOLEAN_MASK = 1;
constexpr size_t MAX_ARRAY_ITEMS = 12;
constexpr size_t MAX_PARSE_ROUNDS = 8;
constexpr size_t MAX_QUERY_ROUNDS = 16;
constexpr size_t MAX_STRING_LENGTH = 64;
constexpr uint8_t NESTED_NUMBER_KIND = 0;
constexpr uint8_t NESTED_STRING_KIND = 1;
constexpr uint8_t NESTED_NULL_KIND = 2;
constexpr uint8_t NESTED_BOOL_KIND = 3;
constexpr uint8_t NESTED_OBJECT_KIND = 4;
constexpr uint8_t NESTED_VALUE_KIND_COUNT = 6;
constexpr uint8_t PARSER_ARRAY_KIND = 0;
constexpr uint8_t PARSER_NUMBER_KIND = 1;
constexpr uint8_t PARSER_STRING_KIND = 2;
constexpr uint8_t PARSER_OBJECT_KIND = 3;
constexpr uint8_t PARSER_INPUT_KIND_COUNT = 5;
constexpr uint8_t CUSTOM_KEY_SELECTOR = 5;
constexpr uint16_t MAX_CPU_CLUSTER_NUM = 8;
constexpr uint16_t MAX_CPU_SPEED_ENTRY_COUNT = 8;
constexpr uint16_t MAX_CPU_SPEED_NUM = 16;
constexpr double POWER_VALUE_SCALE = 1000.0;

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

void AddJsonItem(cJSON* object, const char* key, cJSON* value)
{
    if (object == nullptr || key == nullptr || value == nullptr) {
        cJSON_Delete(value);
        return;
    }
    if (!cJSON_AddItemToObject(object, key, value)) {
        cJSON_Delete(value);
    }
}

cJSON* CreateJsonString(std::string value)
{
    std::replace(value.begin(), value.end(), char{}, ' ');
    return cJSON_CreateString(value.c_str());
}

cJSON* CreateNestedValue(FuzzedDataProvider& reader)
{
    uint8_t operation = reader.ConsumeIntegral<uint8_t>() % NESTED_VALUE_KIND_COUNT;
    switch (operation) {
        case NESTED_NUMBER_KIND:
            return cJSON_CreateNumber(static_cast<double>(reader.ConsumeIntegral<int64_t>()) / POWER_VALUE_SCALE);
        case NESTED_STRING_KIND: {
            std::string value = reader.ConsumeRandomLengthString(MAX_STRING_LENGTH);
            return CreateJsonString(value);
        }
        case NESTED_NULL_KIND:
            return cJSON_CreateNull();
        case NESTED_BOOL_KIND:
            return cJSON_CreateBool((reader.ConsumeIntegral<uint8_t>() & BOOLEAN_MASK) != 0);
        case NESTED_OBJECT_KIND: {
            cJSON* object = cJSON_CreateObject();
            AddJsonItem(object, "value", cJSON_CreateNumber(reader.ConsumeIntegral<int32_t>()));
            return object;
        }
        default: {
            cJSON* array = cJSON_CreateArray();
            AddArrayItem(array, cJSON_CreateNumber(reader.ConsumeIntegral<int32_t>()));
            AddArrayItem(array, cJSON_CreateString("nested"));
            return array;
        }
    }
}

cJSON* CreateParserInput(FuzzedDataProvider& reader)
{
    uint8_t operation = reader.ConsumeIntegral<uint8_t>() % PARSER_INPUT_KIND_COUNT;
    switch (operation) {
        case PARSER_ARRAY_KIND: {
            cJSON* array = cJSON_CreateArray();
            size_t itemCount = reader.ConsumeIntegral<uint8_t>() % (MAX_ARRAY_ITEMS + STRING_LENGTH_RANGE_OFFSET);
            for (size_t index = 0; index < itemCount; ++index) {
                AddArrayItem(array, CreateNestedValue(reader));
            }
            return array;
        }
        case PARSER_NUMBER_KIND:
            return cJSON_CreateNumber(static_cast<double>(reader.ConsumeIntegral<int64_t>()) / POWER_VALUE_SCALE);
        case PARSER_STRING_KIND: {
            std::string value = reader.ConsumeRandomLengthString(MAX_STRING_LENGTH);
            return CreateJsonString(value);
        }
        case PARSER_OBJECT_KIND: {
            cJSON* object = cJSON_CreateObject();
            AddJsonItem(object, "array", CreateNestedValue(reader));
            return object;
        }
        default:
            return cJSON_CreateNull();
    }
}

std::string PickPowerKey(FuzzedDataProvider& reader)
{
    static constexpr const char* keys[] = {
        StatsUtils::CURRENT_CPU_CLUSTER,
        StatsUtils::CURRENT_CPU_SPEED,
        "cpu_speed_cluster0",
        "cpu_speed_cluster1",
        "cpu_speed_cluster999",
        StatsUtils::CURRENT_RADIO_ON,
        StatsUtils::CURRENT_RADIO_DATA,
        StatsUtils::CURRENT_WIFI_ON,
        StatsUtils::CURRENT_WIFI_SCAN,
        StatsUtils::CURRENT_BLUETOOTH_BR_ON,
        StatsUtils::CURRENT_BLUETOOTH_BR_SCAN,
        StatsUtils::CURRENT_BLUETOOTH_BLE_ON,
        StatsUtils::CURRENT_BLUETOOTH_BLE_SCAN,
        StatsUtils::CURRENT_CAMERA_ON,
        StatsUtils::CURRENT_FLASHLIGHT_ON,
        StatsUtils::CURRENT_GNSS_ON,
        StatsUtils::CURRENT_SENSOR_GRAVITY,
        StatsUtils::CURRENT_SENSOR_PROXIMITY,
        StatsUtils::CURRENT_AUDIO_ON,
        StatsUtils::CURRENT_SCREEN_ON,
        StatsUtils::CURRENT_SCREEN_BRIGHTNESS,
        StatsUtils::CURRENT_CPU_AWAKE,
        StatsUtils::CURRENT_CPU_IDLE,
        StatsUtils::CURRENT_CPU_ACTIVE,
        StatsUtils::CURRENT_CPU_SUSPEND,
        StatsUtils::CURRENT_ALARM_ON,
        StatsUtils::CURRENT_INVALID,
        "unknown_average_key",
        ""
    };
    if ((reader.ConsumeIntegral<uint8_t>() % CUSTOM_KEY_SELECTOR) == 0) {
        return reader.ConsumeRandomLengthString(MAX_STRING_LENGTH);
    }
    return keys[reader.ConsumeIntegral<uint8_t>() % (sizeof(keys) / sizeof(keys[0]))];
}

void PrimeCpuShape(BatteryStatsParser& parser, FuzzedDataProvider& reader)
{
    parser.clusterNum_ = reader.ConsumeIntegral<uint16_t>() % MAX_CPU_CLUSTER_NUM;
    parser.speedNum_.clear();
    uint16_t speedEntries = reader.ConsumeIntegral<uint16_t>() % MAX_CPU_SPEED_ENTRY_COUNT;
    for (uint16_t index = 0; index < speedEntries; ++index) {
        parser.speedNum_.push_back(reader.ConsumeIntegral<uint16_t>() % MAX_CPU_SPEED_NUM);
    }
}

void ExerciseQueries(BatteryStatsParser& parser, FuzzedDataProvider& reader)
{
    size_t queryCount = MIN_ITEM_COUNT + (reader.ConsumeIntegral<uint8_t>() % MAX_QUERY_ROUNDS);
    for (size_t index = 0; index < queryCount; ++index) {
        std::string key = PickPowerKey(reader);
        parser.GetAveragePowerMa(key);
        parser.GetAveragePowerMa(key, reader.ConsumeIntegral<uint16_t>());
        parser.GetSpeedNum(reader.ConsumeIntegral<uint16_t>());
    }
    parser.GetClusterNum();
    std::string dump;
    parser.DumpInfo(dump);
}

void FuzzParserArrayMatrix(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return;
    }

    FuzzedDataProvider reader(data, size);
    BatteryStatsParser parser;
    PrimeCpuShape(parser, reader);
    size_t parseRounds = MIN_ITEM_COUNT + (reader.ConsumeIntegral<uint8_t>() % MAX_PARSE_ROUNDS);
    for (size_t round = 0; round < parseRounds; ++round) {
        std::string key = PickPowerKey(reader);
        cJSON* value = CreateParserInput(reader);
        parser.ParsingArray(key, value);
        cJSON_Delete(value);
    }
    ExerciseQueries(parser, reader);
}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzParserArrayMatrix(data, size);
    return 0;
}

} // namespace PowerMgr
} // namespace OHOS
