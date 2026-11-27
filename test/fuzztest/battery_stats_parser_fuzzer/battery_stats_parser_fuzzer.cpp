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

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <fuzzer/FuzzedDataProvider.h>

#include "battery_stats_parser.h"
#include "stats_utils.h"

using namespace OHOS::PowerMgr;

namespace {
constexpr size_t MAX_QUERY_STRING_BYTES = 32;
constexpr uint16_t MAX_LEVEL = 16;
constexpr size_t MAX_QUERY_COUNT = 8;

// Every power profile key that the parser of the component resolves
const std::vector<std::string>& GetQueryTypes()
{
    static const std::vector<std::string> types = {
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
    return types;
}
} // namespace

class BatteryStatsParserFuzzer {
public:
    // The parser is created once: Init() parses the power profile, doing it per input would slow the fuzzer down
    BatteryStatsParserFuzzer()
    {
        parser_ = std::make_shared<BatteryStatsParser>();
        if (parser_ != nullptr) {
            (void)parser_->Init();
        }
    }

    void Fuzz(FuzzedDataProvider& fdp)
    {
        if (parser_ == nullptr) {
            return;
        }

        std::string result;
        parser_->DumpInfo(result);
        FuzzQueryArgs(fdp);
        CoverAllQueryTypes();
    }

private:
    void FuzzQueryArgs(FuzzedDataProvider& fdp)
    {
        size_t iterations = fdp.ConsumeIntegralInRange<size_t>(0, MAX_QUERY_COUNT);
        for (size_t i = 0; i < iterations; i++) {
            std::string type = fdp.ConsumeRandomLengthString(MAX_QUERY_STRING_BYTES);
            if (!type.empty()) {
                BuildPowerLevels(type);
            }
        }

        (void)parser_->GetClusterNum();
        (void)parser_->GetSpeedNum(fdp.ConsumeIntegral<uint16_t>());
    }

    void BuildPowerLevels(const std::string& type)
    {
        (void)parser_->GetAveragePowerMa(type);
        for (uint16_t level = 0; level < MAX_LEVEL; level++) {
            (void)parser_->GetAveragePowerMa(type, level);
        }
    }

    void CoverAllQueryTypes()
    {
        for (const auto& type : GetQueryTypes()) {
            BuildPowerLevels(type);
        }
    }

    std::shared_ptr<BatteryStatsParser> parser_ = nullptr;
};

namespace {
BatteryStatsParserFuzzer& GetFuzzer()
{
    static BatteryStatsParserFuzzer fuzzer;
    return fuzzer;
}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }

    FuzzedDataProvider fdp(data, size);
    GetFuzzer().Fuzz(fdp);
    return 0;
}
