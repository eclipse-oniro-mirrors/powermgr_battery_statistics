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

#define FUZZ_PROJECT_NAME "battery_stats_parser_fuzzer"

#include "battery_stats_parser.h"
#include "stats_utils.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

using namespace OHOS::PowerMgr;

namespace {
constexpr size_t MIN_INPUT_SIZE = 1;
constexpr size_t MAX_QUERY_STRING_BYTES = 32;
constexpr size_t MAX_LEVEL = 16;
constexpr size_t MAX_QUERY_COUNT = 8;
constexpr size_t MAX_LOOP_ITERATIONS = 64;
constexpr size_t QUERY_TYPE_COUNT = 19;

const std::string QUERY_TYPES[QUERY_TYPE_COUNT] = {
    StatsUtils::CURRENT_CPU_CLUSTER,
    StatsUtils::CURRENT_CPU_SPEED,
    StatsUtils::CURRENT_RADIO_ON,
    StatsUtils::CURRENT_RADIO_DATA,
    StatsUtils::CURRENT_WIFI_ON,
    StatsUtils::CURRENT_WIFI_SCAN,
    StatsUtils::CURRENT_BLUETOOTH_BR_ON,
    StatsUtils::CURRENT_BLUETOOTH_BR_SCAN,
    StatsUtils::CURRENT_GNSS_ON,
    StatsUtils::CURRENT_CAMERA_ON,
    StatsUtils::CURRENT_FLASHLIGHT_ON,
    StatsUtils::CURRENT_AUDIO_ON,
    StatsUtils::CURRENT_SENSOR_GRAVITY,
    StatsUtils::CURRENT_SCREEN_ON,
    StatsUtils::CURRENT_SCREEN_BRIGHTNESS,
    StatsUtils::CURRENT_CPU_AWAKE,
    StatsUtils::CURRENT_ALARM_ON,
    StatsUtils::CURRENT_CPU_ACTIVE,
    StatsUtils::CURRENT_CPU_IDLE
};

std::string ReadString(const uint8_t* data, size_t size, size_t& offset)
{
    if (data == nullptr || offset >= size) {
        return {};
    }

    size_t length = std::min(static_cast<size_t>(data[offset] % MAX_QUERY_STRING_BYTES), size - offset - 1);
    offset++;
    if (length == 0 || offset + length > size) {
        return {};
    }

    std::string value(reinterpret_cast<const char*>(data + offset), length);
    offset += length;
    return value;
}
} // namespace

class BatteryStatsParserFuzzer {
public:
    void Fuzz(const uint8_t* data, size_t size)
    {
        if (data == nullptr || size < MIN_INPUT_SIZE) {
            return;
        }

        auto parser = std::make_shared<BatteryStatsParser>();
        (void)parser->Init();

        std::string result;
        parser->DumpInfo(result);

        size_t offset = 0;
        size_t iterations = 0;
        while (offset < size && iterations < MAX_LOOP_ITERATIONS) {
            iterations++;
            std::string type = ReadString(data, size, offset);
            if (type.empty()) {
                type = QUERY_TYPES[data[offset % size] % QUERY_TYPE_COUNT];
                offset++;
            }

            parser->GetAveragePowerMa(type);
            parser->GetAveragePowerMa(type, static_cast<uint16_t>(data[offset % size] % MAX_LEVEL));
            parser->GetClusterNum();
            parser->GetSpeedNum(static_cast<uint16_t>(data[offset % size] % MAX_QUERY_COUNT));
        }

        for (size_t i = 0; i < QUERY_TYPE_COUNT; i++) {
            parser->GetAveragePowerMa(QUERY_TYPES[i]);
            parser->GetAveragePowerMa(QUERY_TYPES[i], static_cast<uint16_t>(MAX_LEVEL));
        }
    }
};

namespace {
BatteryStatsParserFuzzer g_fuzzer;
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    g_fuzzer.Fuzz(data, size);
    return 0;
}
