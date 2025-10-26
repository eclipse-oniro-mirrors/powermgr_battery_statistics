/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#define FUZZ_PROJECT_NAME "configparser_fuzzer"

#include "battery_stats_parser.h"
#include "stats_utils.h"
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <unistd.h>

using namespace OHOS::PowerMgr;

namespace {
    constexpr const char* FUZZ_CONFIG_PATH = "/data/local/tmp/fuzz_power_average.json";

    void FuzzParserWithJSON(const uint8_t* data, size_t size) {
        if (data == nullptr || size == 0 || size > 1024 * 1024) {
            return;
        }

        // Write fuzzer data to temp file
        FILE* fp = fopen(FUZZ_CONFIG_PATH, "w");
        if (fp == nullptr) {
            return;
        }

        size_t written = fwrite(data, 1, size, fp);
        fclose(fp);

        if (written != size) {
            unlink(FUZZ_CONFIG_PATH);
            return;
        }

        // Create parser and try to parse the fuzzed JSON
        auto parser = std::make_shared<BatteryStatsParser>();

        // Init will try to load the config file
        parser->Init();

        // Test various parser methods
        std::vector<std::string> testTypes = {
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

        for (const auto& type : testTypes) {
            parser->GetAveragePowerMa(type);

            // Test with various levels
            for (uint16_t level = 0; level < 10; level++) {
                parser->GetAveragePowerMa(type, level);
            }
        }

        // Test cluster and speed queries
        uint16_t clusterNum = parser->GetClusterNum();
        for (uint16_t cluster = 0; cluster < clusterNum && cluster < 16; cluster++) {
            parser->GetSpeedNum(cluster);
        }

        // Test dump
        std::string dumpResult;
        parser->DumpInfo(dumpResult);

        // Cleanup
        unlink(FUZZ_CONFIG_PATH);
    }
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzParserWithJSON(data, size);
    return 0;
}
