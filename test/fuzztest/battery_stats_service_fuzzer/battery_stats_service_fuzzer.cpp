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

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "battery_stats_core.h"
#include "battery_stats_info.h"
#include "battery_stats_service.h"
#include "stats_utils.h"

using namespace OHOS::PowerMgr;

namespace {
constexpr size_t MIN_INPUT_SIZE = sizeof(uint32_t);
constexpr size_t MAX_STRING_BYTES = 32;
constexpr size_t MAX_ARG_COUNT = 8;
constexpr uint32_t STATS_TYPE_COUNT = 30;
constexpr uint32_t CONSUMPTION_TYPE_COUNT = 17;
constexpr uint32_t ON_BATTERY_MASK = 0x01;

struct QueryParams {
    int32_t uid = StatsUtils::INVALID_VALUE;
    StatsUtils::StatsType statsType = StatsUtils::STATS_TYPE_INVALID;
    BatteryStatsInfo::ConsumptionType consumptionType = BatteryStatsInfo::CONSUMPTION_TYPE_INVALID;
    BatteryStatsInfo::ConsumptionType outOfRangeType = BatteryStatsInfo::CONSUMPTION_TYPE_INVALID;
};

template<typename T>
T ReadValue(const uint8_t* data, size_t size, size_t& offset)
{
    T value {};
    if (data == nullptr || offset + sizeof(T) > size) {
        return value;
    }
    value = *reinterpret_cast<const T*>(data + offset);
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

// Produces a value that is a legal enumerator, so the switch statement takes a real branch
BatteryStatsInfo::ConsumptionType ToValidConsumptionType(uint8_t raw)
{
    return static_cast<BatteryStatsInfo::ConsumptionType>(
        -static_cast<int32_t>(raw % CONSUMPTION_TYPE_COUNT) - 1);
}

// Produces a value outside the enumerator range, so the default branch is exercised
BatteryStatsInfo::ConsumptionType ToOutOfRangeConsumptionType(uint8_t raw)
{
    return static_cast<BatteryStatsInfo::ConsumptionType>(
        static_cast<int32_t>(raw % CONSUMPTION_TYPE_COUNT));
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
        std::vector<std::string> args = BuildArgs(data, size);
        std::vector<std::u16string> u16Args = BuildU16Args(args);

        FuzzServiceStub(data, size, offset, args, u16Args);
        FuzzIpcAndCore(data, size, offset, ReadString(data, size, offset));
    }

private:
    void FuzzServiceStub(const uint8_t* data, size_t size, size_t& offset, const std::vector<std::string>& args,
        const std::vector<std::u16string>& u16Args)
    {
        std::string deviceId = ReadString(data, size, offset);
        service_->OnAddSystemAbility(ReadValue<int32_t>(data, size, offset), deviceId);
        service_->OnAddSystemAbility(static_cast<int32_t>(data[0]), ReadString(data, size, offset));
        (void)service_->IsServiceReady();
        (void)service_->GetBatteryStatsCore();
        (void)service_->GetBatteryStatsParser();
        (void)service_->GetBatteryStatsDetector();

        std::vector<std::string> shellArgs = args;
        std::string shellDump = service_->ShellDump(shellArgs, static_cast<uint32_t>(shellArgs.size()));
        (void)shellDump;

        int32_t fd = -1;
        (void)service_->Dump(fd, u16Args);
    }

    void FuzzIpcAndCore(const uint8_t* data, size_t size, size_t& offset, const std::string& deviceId)
    {
        QueryParams params;
        params.uid = ReadValue<int32_t>(data, size, offset);
        params.statsType = ToStatsType(data[0]);
        params.consumptionType = ToValidConsumptionType(data[0]);
        params.outOfRangeType = ToOutOfRangeConsumptionType(data[0]);

        FuzzIpcGetters(params);
        FuzzCoreQueries(params, deviceId);
    }

    void FuzzIpcGetters(const QueryParams& params)
    {
        ParcelableBatteryStatsList batteryStats;
        int32_t tempError = 0;
        double appStatsMah = 0.0;
        double appStatsPercent = 0.0;
        double partStatsMah = 0.0;
        double partStatsPercent = 0.0;
        uint64_t totalTimeSecond = 0;
        uint64_t totalDataBytes = 0;

        (void)service_->GetBatteryStatsIpc(batteryStats, tempError);
        (void)service_->GetAppStatsMahIpc(params.uid, appStatsMah, tempError);
        (void)service_->GetAppStatsPercentIpc(params.uid, appStatsPercent, tempError);
        (void)service_->GetPartStatsMahIpc(static_cast<int32_t>(params.consumptionType), partStatsMah, tempError);
        (void)service_->GetPartStatsPercentIpc(
            static_cast<int32_t>(params.consumptionType), partStatsPercent, tempError);
        (void)service_->GetPartStatsMahIpc(static_cast<int32_t>(params.outOfRangeType), partStatsMah, tempError);
        (void)service_->GetPartStatsPercentIpc(
            static_cast<int32_t>(params.outOfRangeType), partStatsPercent, tempError);
        (void)service_->GetTotalTimeSecondIpc(static_cast<int32_t>(params.statsType), params.uid, totalTimeSecond);
        (void)service_->GetTotalDataBytesIpc(static_cast<int32_t>(params.statsType), params.uid, totalDataBytes);
        (void)service_->ResetIpc();
        (void)service_->SetOnBatteryIpc((static_cast<uint32_t>(params.uid) & ON_BATTERY_MASK) != 0);
    }

    void FuzzCoreQueries(const QueryParams& params, const std::string& deviceId)
    {
        std::shared_ptr<BatteryStatsCore> core = service_->GetBatteryStatsCore();
        if (core == nullptr) {
            return;
        }

        std::string result;
        core->ComputePower();
        core->DumpInfo(result);
        (void)core->GetBatteryStats();
        (void)core->GetAppStatsMah(params.uid);
        (void)core->GetAppStatsPercent(params.uid);
        (void)core->GetPartStatsMah(params.consumptionType);
        (void)core->GetPartStatsPercent(params.consumptionType);
        (void)core->GetPartStatsMah(params.outOfRangeType);
        (void)core->GetPartStatsPercent(params.outOfRangeType);
        (void)core->GetTotalTimeMs(params.statsType);
        (void)core->GetTotalTimeMs(params.uid, params.statsType);
        (void)core->GetTotalDataCount(params.statsType, params.uid);
        (void)core->GetTotalConsumptionCount(params.statsType, params.uid);
        core->UpdateDebugInfo(deviceId);
        core->GetDebugInfo(result);
        core->Reset();
    }

    OHOS::sptr<BatteryStatsService> service_ = nullptr;
};

namespace {
BatteryStatsServiceFuzzer g_fuzzer;
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    g_fuzzer.Fuzz(data, size);
    return 0;
}
