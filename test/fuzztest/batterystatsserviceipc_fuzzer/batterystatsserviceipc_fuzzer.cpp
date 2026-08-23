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

#include "batterystatsserviceipc_fuzzer.h"

#include "battery_stats_core.h"
#include "battery_stats_info.h"
#include "battery_stats_service.h"
#include "stats_utils.h"

#include <algorithm>
#include <vector>

namespace OHOS {
namespace PowerMgr {

namespace {
constexpr size_t BYTE_BITS = 8;
constexpr size_t MIN_ITEM_COUNT = 1;
constexpr size_t STRING_LENGTH_RANGE_OFFSET = 1;
constexpr uint8_t BOOLEAN_MASK = 1;
constexpr size_t MAX_ARG_COUNT = 8;
constexpr size_t MAX_ARG_LENGTH = 40;
constexpr size_t MAX_OPERATIONS_PER_INPUT = 16;
constexpr int32_t UNKNOWN_STATS_TYPE_VALUE = 30;
constexpr int32_t UNKNOWN_CONSUMPTION_TYPE_VALUE = 99;
constexpr uint8_t GET_BATTERY_STATS_OPERATION = 0;
constexpr uint8_t GET_APP_STATS_MAH_OPERATION = 1;
constexpr uint8_t GET_APP_STATS_PERCENT_OPERATION = 2;
constexpr uint8_t GET_PART_STATS_MAH_OPERATION = 3;
constexpr uint8_t GET_PART_STATS_PERCENT_OPERATION = 4;
constexpr uint8_t GET_TOTAL_TIME_OPERATION = 5;
constexpr uint8_t GET_TOTAL_DATA_OPERATION = 6;
constexpr uint8_t SET_ON_BATTERY_OPERATION = 7;
constexpr uint8_t RESET_OPERATION = 8;
constexpr uint8_t SHELL_DUMP_OPERATION = 9;
constexpr uint8_t IPC_OPERATION_COUNT = SHELL_DUMP_OPERATION + MIN_ITEM_COUNT;
constexpr double DEFAULT_DOUBLE_RESULT = 0.0;

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

StatsUtils::StatsType PickStatsType(uint8_t selector)
{
    static constexpr StatsUtils::StatsType types[] = {
        StatsUtils::STATS_TYPE_WIFI_SCAN,
        StatsUtils::STATS_TYPE_ALARM,
        StatsUtils::STATS_TYPE_WAKELOCK_HOLD,
        StatsUtils::STATS_TYPE_SCREEN_ON,
        StatsUtils::STATS_TYPE_SCREEN_BRIGHTNESS,
        StatsUtils::STATS_TYPE_PHONE_DATA,
        StatsUtils::STATS_TYPE_CPU_ACTIVE,
        StatsUtils::STATS_TYPE_INVALID,
        static_cast<StatsUtils::StatsType>(UNKNOWN_STATS_TYPE_VALUE),
    };
    return types[selector % (sizeof(types) / sizeof(types[0]))];
}

BatteryStatsInfo::ConsumptionType PickConsumptionType(uint8_t selector)
{
    static constexpr BatteryStatsInfo::ConsumptionType types[] = {
        BatteryStatsInfo::CONSUMPTION_TYPE_APP,
        BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH,
        BatteryStatsInfo::CONSUMPTION_TYPE_IDLE,
        BatteryStatsInfo::CONSUMPTION_TYPE_SCREEN,
        BatteryStatsInfo::CONSUMPTION_TYPE_WIFI,
        BatteryStatsInfo::CONSUMPTION_TYPE_CPU,
        BatteryStatsInfo::CONSUMPTION_TYPE_WAKELOCK,
        BatteryStatsInfo::CONSUMPTION_TYPE_INVALID,
        static_cast<BatteryStatsInfo::ConsumptionType>(UNKNOWN_CONSUMPTION_TYPE_VALUE),
    };
    return types[selector % (sizeof(types) / sizeof(types[0]))];
}

std::vector<std::string> BuildDumpArgs(FuzzReader& reader)
{
    static const std::vector<std::string> knownArgs = {
        "-h",
        "-batterystats",
        "-poweraverage",
        "-unknown",
    };
    size_t count = reader.Read<uint8_t>(0) % (MAX_ARG_COUNT + STRING_LENGTH_RANGE_OFFSET);
    std::vector<std::string> args;
    args.reserve(count);
    for (size_t argIndex = 0; argIndex < count; ++argIndex) {
        uint8_t selector = reader.Read<uint8_t>(0);
        if ((selector & BOOLEAN_MASK) == 0) {
            args.push_back(knownArgs[selector % knownArgs.size()]);
        } else {
            args.push_back(reader.ReadString(MAX_ARG_LENGTH));
        }
    }
    return args;
}

class BatteryStatsServiceIpcFuzzer {
public:
    BatteryStatsServiceIpcFuzzer()
    {
        service_ = BatteryStatsService::GetInstance();
        if (service_ != nullptr) {
            service_->OnStart();
        }
    }

    ~BatteryStatsServiceIpcFuzzer()
    {
        if (service_ != nullptr) {
            service_->OnStop();
        }
    }

    void FuzzServiceIpc(const uint8_t* data, size_t size)
    {
        if (data == nullptr || size == 0 || service_ == nullptr) {
            return;
        }

        FuzzReader reader(data, size);
        size_t operationCount = MIN_ITEM_COUNT + (reader.Read<uint8_t>(0) % MAX_OPERATIONS_PER_INPUT);
        for (size_t operationIndex = 0; operationIndex < operationCount; ++operationIndex) {
            DispatchOperation(reader.Read<uint8_t>(0), reader);
        }

        auto core = service_->GetBatteryStatsCore();
        if (core != nullptr) {
            core->Reset();
        }
    }

private:
    bool DispatchStatsOperation(uint8_t operation, FuzzReader& reader, int32_t& tempError, double& doubleResult)
    {
        switch (operation) {
            case GET_BATTERY_STATS_OPERATION: {
                ParcelableBatteryStatsList statsList;
                service_->GetBatteryStatsIpc(statsList, tempError);
                return true;
            }
            case GET_APP_STATS_MAH_OPERATION:
                service_->GetAppStatsMahIpc(
                    reader.Read<int32_t>(StatsUtils::INVALID_VALUE), doubleResult, tempError);
                return true;
            case GET_APP_STATS_PERCENT_OPERATION:
                service_->GetAppStatsPercentIpc(
                    reader.Read<int32_t>(StatsUtils::INVALID_VALUE), doubleResult, tempError);
                return true;
            case GET_PART_STATS_MAH_OPERATION:
                service_->GetPartStatsMahIpc(static_cast<int32_t>(PickConsumptionType(reader.Read<uint8_t>(0))),
                    doubleResult, tempError);
                return true;
            case GET_PART_STATS_PERCENT_OPERATION:
                service_->GetPartStatsPercentIpc(static_cast<int32_t>(PickConsumptionType(reader.Read<uint8_t>(0))),
                    doubleResult, tempError);
                return true;
            default:
                return false;
        }
    }

    bool DispatchTotalOperation(uint8_t operation, FuzzReader& reader, uint64_t& uint64Result)
    {
        switch (operation) {
            case GET_TOTAL_TIME_OPERATION:
                service_->GetTotalTimeSecondIpc(static_cast<int32_t>(PickStatsType(reader.Read<uint8_t>(0))),
                    reader.Read<int32_t>(StatsUtils::INVALID_VALUE), uint64Result);
                return true;
            case GET_TOTAL_DATA_OPERATION:
                service_->GetTotalDataBytesIpc(static_cast<int32_t>(PickStatsType(reader.Read<uint8_t>(0))),
                    reader.Read<int32_t>(StatsUtils::INVALID_VALUE), uint64Result);
                return true;
            default:
                return false;
        }
    }

    void DispatchControlOperation(uint8_t operation, FuzzReader& reader)
    {
        switch (operation) {
            case SET_ON_BATTERY_OPERATION:
                service_->SetOnBatteryIpc((reader.Read<uint8_t>(0) & BOOLEAN_MASK) != 0);
                break;
            case RESET_OPERATION:
                service_->ResetIpc();
                break;
            case SHELL_DUMP_OPERATION: {
                std::vector<std::string> args = BuildDumpArgs(reader);
                std::string dumpResult;
                service_->ShellDumpIpc(args, static_cast<uint32_t>(args.size()), dumpResult);
                break;
            }
            default:
                break;
        }
    }

    void DispatchOperation(uint8_t operation, FuzzReader& reader)
    {
        uint8_t ipcOperation = operation % IPC_OPERATION_COUNT;
        int32_t tempError = 0;
        double doubleResult = DEFAULT_DOUBLE_RESULT;
        uint64_t uint64Result = 0;
        if (DispatchStatsOperation(ipcOperation, reader, tempError, doubleResult)) {
            return;
        }
        if (DispatchTotalOperation(ipcOperation, reader, uint64Result)) {
            return;
        }
        DispatchControlOperation(ipcOperation, reader);
    }

    OHOS::sptr<BatteryStatsService> service_ = nullptr;
};

BatteryStatsServiceIpcFuzzer g_fuzzer;
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    g_fuzzer.FuzzServiceIpc(data, size);
    return 0;
}

} // namespace PowerMgr
} // namespace OHOS
