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

#include "cputimereader_fuzzer.h"

#include "../battery_fuzz_input.h"

#include "battery_stats_core.h"
#include "battery_stats_service.h"
#include "cpu_time_reader.h"
#include "stats_utils.h"

#include <limits>

namespace OHOS {
namespace PowerMgr {

namespace {
constexpr size_t MIN_ITEM_COUNT = 1;
constexpr uint8_t BOOLEAN_MASK = 1;
constexpr size_t MAX_QUERIES_PER_INPUT = 16;
constexpr uint32_t MAX_CLUSTER_SELECTOR = 8;
constexpr uint32_t MAX_SPEED_SELECTOR = 16;
constexpr uint32_t FIRST_INDEX = 0;
constexpr uint32_t SECOND_INDEX = 1;
constexpr uint32_t THIRD_INDEX = 2;
constexpr uint32_t FOURTH_INDEX = 3;
constexpr uint32_t UINT32_MAX_EDGE_VALUE = std::numeric_limits<uint32_t>::max();

uint32_t PickCluster(uint8_t value)
{
    static constexpr uint32_t clusters[] = {
        FIRST_INDEX,
        SECOND_INDEX,
        THIRD_INDEX,
        FOURTH_INDEX,
        MAX_CLUSTER_SELECTOR,
        UINT32_MAX_EDGE_VALUE,
    };
    return clusters[value % (sizeof(clusters) / sizeof(clusters[0]))];
}

uint32_t PickSpeed(uint8_t value)
{
    static constexpr uint32_t speeds[] = {
        FIRST_INDEX,
        SECOND_INDEX,
        THIRD_INDEX,
        FOURTH_INDEX,
        MAX_SPEED_SELECTOR,
        UINT32_MAX_EDGE_VALUE,
    };
    return speeds[value % (sizeof(speeds) / sizeof(speeds[0]))];
}

class CpuTimeReaderFuzzer {
public:
    CpuTimeReaderFuzzer()
    {
        service_ = BatteryStatsService::GetInstance();
        if (service_ != nullptr) {
            service_->OnStart();
        }
    }

    ~CpuTimeReaderFuzzer()
    {
        if (service_ != nullptr) {
            service_->OnStop();
        }
    }

    void FuzzCpuTimeReader(const uint8_t* data, size_t size)
    {
        if (data == nullptr || size == 0) {
            return;
        }

        FuzzedDataProvider reader(data, size);
        CpuTimeReader cpuTimeReader;
        bool canReadCpuFiles = false;
        if (service_ != nullptr) {
            canReadCpuFiles = service_->GetBatteryStatsCore() != nullptr &&
                service_->GetBatteryStatsParser() != nullptr;
        }
        uint8_t operation = reader.ConsumeIntegral<uint8_t>();
        if (canReadCpuFiles && (operation & BOOLEAN_MASK) != 0) {
            cpuTimeReader.Init();
        }
        if (canReadCpuFiles) {
            cpuTimeReader.UpdateCpuTime();
        }

        size_t queryCount = MIN_ITEM_COUNT + (reader.ConsumeIntegral<uint8_t>() % MAX_QUERIES_PER_INPUT);
        for (size_t queryIndex = 0; queryIndex < queryCount; ++queryIndex) {
            uint32_t cluster = PickCluster(reader.ConsumeIntegral<uint8_t>());
            uint32_t speed = PickSpeed(reader.ConsumeIntegral<uint8_t>());
            cpuTimeReader.GetUidCpuActiveTimeMs(ConsumeOr<int32_t>(reader, StatsUtils::INVALID_VALUE));
            cpuTimeReader.GetUidCpuClusterTimeMs(ConsumeOr<int32_t>(reader, StatsUtils::INVALID_VALUE), cluster);
            cpuTimeReader.GetUidCpuFreqTimeMs(ConsumeOr<int32_t>(reader, StatsUtils::INVALID_VALUE), cluster, speed);
            cpuTimeReader.GetUidCpuTimeMs(ConsumeOr<int32_t>(reader, StatsUtils::INVALID_VALUE));

            std::string dumpResult;
            cpuTimeReader.DumpInfo(dumpResult, ConsumeOr<int32_t>(reader, StatsUtils::INVALID_VALUE));
        }

        if (service_ != nullptr) {
            auto core = service_->GetBatteryStatsCore();
            if (core != nullptr) {
                core->Reset();
            }
        }
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
    static CpuTimeReaderFuzzer fuzzer;
    fuzzer.FuzzCpuTimeReader(data, size);
    return 0;
}

} // namespace PowerMgr
} // namespace OHOS
