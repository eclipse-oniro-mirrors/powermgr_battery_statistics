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

#define FUZZ_PROJECT_NAME "battery_stats_service_fuzzer"

#include "battery_stats_service.h"
#include "battery_stats_info.h"
#include "stats_utils.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

using namespace OHOS::PowerMgr;

namespace {
constexpr size_t MIN_INPUT_SIZE = sizeof(uint32_t);
constexpr size_t MAX_STRING_BYTES = 32;
constexpr size_t MAX_ARG_COUNT = 8;
constexpr size_t STATS_TYPE_COUNT = 30;
constexpr size_t STATS_STATE_COUNT = 5;
constexpr size_t CONSUMPTION_TYPE_COUNT = 17;

template<typename T>
T ReadValue(const uint8_t* data, size_t size, size_t& offset)
{
    T value {};
    if (data == nullptr || offset + sizeof(T) > size) {
        return value;
    }
    (void)memcpy(&value, data + offset, sizeof(T));
    offset += sizeof(T);
    return value;
}

std::string ReadString(const uint8_t* data, size_t size, size_t& offset)
{
    if (data == nullptr || offset >= size) {
        return {};
    }

    size_t length = std::min(static_cast<size_t>(data[offset] % MAX_STRING_BYTES), size - offset - 1);
    offset++;
    if (length == 0 || offset + length > size) {
        return {};
    }

    std::string value(reinterpret_cast<const char*>(data + offset), length);
    offset += length;
    return value;
}

StatsUtils::StatsType ToStatsType(uint8_t raw)
{
    return static_cast<StatsUtils::StatsType>(static_cast<int32_t>(raw % STATS_TYPE_COUNT) - 1);
}

StatsUtils::StatsState ToStatsState(uint8_t raw)
{
    return static_cast<StatsUtils::StatsState>(static_cast<int32_t>(raw % STATS_STATE_COUNT) - 1);
}

BatteryStatsInfo::ConsumptionType ToConsumptionType(uint8_t raw)
{
    return static_cast<BatteryStatsInfo::ConsumptionType>(
        static_cast<int32_t>(raw % CONSUMPTION_TYPE_COUNT));
}

BatteryStatsInfo::ConsumptionType ToInvalidConsumptionType(uint8_t raw)
{
    return static_cast<BatteryStatsInfo::ConsumptionType>(
        -static_cast<int32_t>(raw % CONSUMPTION_TYPE_COUNT) - 1);
}

std::vector<std::string> BuildArgs(const uint8_t* data, size_t size)
{
    std::vector<std::string> args = {
        "-h",
        "-batterystats",
        "-poweraverage",
        "-reset"
    };
    if (data == nullptr || size == 0) {
        return args;
    }

    size_t offset = 0;
    while (offset < size && args.size() < MAX_ARG_COUNT) {
        size_t chunkLength = std::min(static_cast<size_t>(data[offset] % MAX_STRING_BYTES), size - offset - 1);
        offset++;
        if (chunkLength == 0 || offset + chunkLength > size) {
            continue;
        }
        args.emplace_back(reinterpret_cast<const char*>(data + offset), chunkLength);
        offset += chunkLength;
    }
    return args;
}

std::vector<std::u16string> BuildU16Args(const std::vector<std::string>& args)
{
    std::vector<std::u16string> result;
    result.reserve(args.size());
    for (const auto& arg : args) {
        result.emplace_back(arg.begin(), arg.end());
    }
    return result;
}
} // namespace

class BatteryStatsServiceFuzzer {
public:
    BatteryStatsServiceFuzzer()
    {
        service_ = BatteryStatsService::GetInstance();
        service_->OnStart();
    }

    ~BatteryStatsServiceFuzzer()
    {
        if (service_ != nullptr) {
            service_->OnStop();
        }
    }

    void Fuzz(const uint8_t* data, size_t size)
    {
        if (data == nullptr || size < MIN_INPUT_SIZE) {
            return;
        }

        size_t offset = 0;
        auto args = BuildArgs(data, size);
        auto u16Args = BuildU16Args(args);
        std::string deviceId = ReadString(data, size, offset);

        service_->OnAddSystemAbility(ReadValue<int32_t>(data, size, offset), deviceId);
        service_->OnAddSystemAbility(static_cast<int32_t>(data[0]), ReadString(data, size, offset));
        service_->IsServiceReady();
        service_->GetBatteryStatsCore();
        service_->GetBatteryStatsParser();
        service_->GetBatteryStatsDetector();

        std::vector<std::string> shellArgs = args;
        std::string shellDump = service_->ShellDump(shellArgs, static_cast<uint32_t>(shellArgs.size()));
        (void)shellDump;

        int32_t fd = -1;
        (void)service_->Dump(fd, u16Args);

        ParcelableBatteryStatsList batteryStats;
        int32_t tempError = 0;
        int32_t uid = ReadValue<int32_t>(data, size, offset);
        uint64_t totalTimeSecond = 0;
        uint64_t totalDataBytes = 0;
        double appStatsMah = 0.0;
        double appStatsPercent = 0.0;
        double partStatsMah = 0.0;
        double partStatsPercent = 0.0;
        StatsUtils::StatsType statsType = ToStatsType(data[0]);
        BatteryStatsInfo::ConsumptionType consumptionType = ToConsumptionType(data[0]);
        BatteryStatsInfo::ConsumptionType invalidConsumptionType = ToInvalidConsumptionType(data[0]);

        (void)service_->GetBatteryStatsIpc(batteryStats, tempError);
        (void)service_->GetAppStatsMahIpc(uid, appStatsMah, tempError);
        (void)service_->GetAppStatsPercentIpc(uid, appStatsPercent, tempError);
        (void)service_->GetPartStatsMahIpc(static_cast<int32_t>(consumptionType), partStatsMah, tempError);
        (void)service_->GetPartStatsPercentIpc(static_cast<int32_t>(consumptionType), partStatsPercent, tempError);
        (void)service_->GetPartStatsMahIpc(static_cast<int32_t>(invalidConsumptionType), partStatsMah, tempError);
        (void)service_->GetPartStatsPercentIpc(
            static_cast<int32_t>(invalidConsumptionType), partStatsPercent, tempError);
        (void)service_->GetTotalTimeSecondIpc(static_cast<int32_t>(statsType), uid, totalTimeSecond);
        (void)service_->GetTotalDataBytesIpc(static_cast<int32_t>(statsType), uid, totalDataBytes);
        (void)service_->ResetIpc();
        (void)service_->SetOnBatteryIpc((data[0] & 0x01) != 0);

        auto core = service_->GetBatteryStatsCore();
        if (core != nullptr) {
            core->ComputePower();
            std::string result;
            core->DumpInfo(result);
            core->GetBatteryStats();
            core->GetAppStatsMah(uid);
            core->GetAppStatsPercent(uid);
            core->GetPartStatsMah(consumptionType);
            core->GetPartStatsPercent(consumptionType);
            core->GetPartStatsMah(invalidConsumptionType);
            core->GetPartStatsPercent(invalidConsumptionType);
            core->GetTotalTimeMs(statsType);
            core->GetTotalTimeMs(uid, statsType);
            core->GetTotalDataCount(statsType, uid);
            core->GetTotalConsumptionCount(statsType, uid);
            core->UpdateDebugInfo(deviceId);
            core->GetDebugInfo(result);
            core->Reset();
        }
    }

private:
    OHOS::sptr<BatteryStatsService> service_ = nullptr;
};

namespace {
BatteryStatsServiceFuzzer g_fuzzer;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    g_fuzzer.Fuzz(data, size);
    return 0;
}
