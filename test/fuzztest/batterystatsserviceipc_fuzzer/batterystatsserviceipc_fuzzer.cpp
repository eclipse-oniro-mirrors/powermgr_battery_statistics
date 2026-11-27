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

#include "batterystatsserviceipc_fuzzer.h"

#include "../battery_fuzz_input.h"

#include "battery_stats_core.h"
#include "battery_stats_info.h"
#include "battery_stats_service.h"
#include "stats_utils.h"

#include "ibattery_stats.h"
#include "message_option.h"
#include "message_parcel.h"
#include "string_ex.h"

#include <array>
#include <vector>

namespace OHOS {
namespace PowerMgr {

namespace {
constexpr size_t MIN_ITEM_COUNT = 1;
constexpr size_t STRING_LENGTH_RANGE_OFFSET = 1;
constexpr uint8_t BOOLEAN_MASK = 1;
constexpr size_t MAX_ARG_COUNT = 8;
constexpr size_t MAX_ARG_LENGTH = 40;
constexpr size_t MAX_OPERATIONS_PER_INPUT = 16;
constexpr int32_t UNKNOWN_STATS_TYPE_VALUE = 30;
constexpr int32_t UNKNOWN_CONSUMPTION_TYPE_VALUE = 99;
constexpr std::array<IBatteryStatsIpcCode, 10> IPC_CODES {
    IBatteryStatsIpcCode::COMMAND_GET_BATTERY_STATS_IPC,
    IBatteryStatsIpcCode::COMMAND_GET_APP_STATS_MAH_IPC,
    IBatteryStatsIpcCode::COMMAND_GET_APP_STATS_PERCENT_IPC,
    IBatteryStatsIpcCode::COMMAND_GET_PART_STATS_MAH_IPC,
    IBatteryStatsIpcCode::COMMAND_GET_PART_STATS_PERCENT_IPC,
    IBatteryStatsIpcCode::COMMAND_GET_TOTAL_TIME_SECOND_IPC,
    IBatteryStatsIpcCode::COMMAND_GET_TOTAL_DATA_BYTES_IPC,
    IBatteryStatsIpcCode::COMMAND_SET_ON_BATTERY_IPC,
    IBatteryStatsIpcCode::COMMAND_RESET_IPC,
    IBatteryStatsIpcCode::COMMAND_SHELL_DUMP_IPC,
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

std::vector<std::string> BuildDumpArgs(FuzzedDataProvider& reader)
{
    static const std::vector<std::string> knownArgs = {
        "-h",
        "-batterystats",
        "-poweraverage",
        "-unknown",
    };
    size_t count = reader.ConsumeIntegral<uint8_t>() % (MAX_ARG_COUNT + STRING_LENGTH_RANGE_OFFSET);
    std::vector<std::string> args;
    args.reserve(count);
    for (size_t argIndex = 0; argIndex < count; ++argIndex) {
        uint8_t selector = reader.ConsumeIntegral<uint8_t>();
        if ((selector & BOOLEAN_MASK) == 0) {
            args.push_back(knownArgs[selector % knownArgs.size()]);
        } else {
            args.push_back(reader.ConsumeRandomLengthString(MAX_ARG_LENGTH));
        }
    }
    return args;
}

bool WriteDumpArgs(MessageParcel& request, FuzzedDataProvider& reader)
{
    const auto args = BuildDumpArgs(reader);
    const auto count = static_cast<int32_t>(args.size());
    if (!request.WriteInt32(count)) {
        return false;
    }
    for (const auto& arg : args) {
        if (!request.WriteString16(Str8ToStr16(arg))) {
            return false;
        }
    }
    return request.WriteUint32(static_cast<uint32_t>(count));
}

bool WriteRequestArgs(MessageParcel& request, IBatteryStatsIpcCode code, FuzzedDataProvider& reader)
{
    switch (code) {
        case IBatteryStatsIpcCode::COMMAND_GET_BATTERY_STATS_IPC:
        case IBatteryStatsIpcCode::COMMAND_RESET_IPC:
            return true;
        case IBatteryStatsIpcCode::COMMAND_GET_APP_STATS_MAH_IPC:
        case IBatteryStatsIpcCode::COMMAND_GET_APP_STATS_PERCENT_IPC:
            return request.WriteInt32(ConsumeOr<int32_t>(reader, StatsUtils::INVALID_VALUE));
        case IBatteryStatsIpcCode::COMMAND_GET_PART_STATS_MAH_IPC:
        case IBatteryStatsIpcCode::COMMAND_GET_PART_STATS_PERCENT_IPC:
            return request.WriteInt32(static_cast<int32_t>(PickConsumptionType(reader.ConsumeIntegral<uint8_t>())));
        case IBatteryStatsIpcCode::COMMAND_GET_TOTAL_TIME_SECOND_IPC:
        case IBatteryStatsIpcCode::COMMAND_GET_TOTAL_DATA_BYTES_IPC:
            return request.WriteInt32(static_cast<int32_t>(PickStatsType(reader.ConsumeIntegral<uint8_t>()))) &&
                request.WriteInt32(ConsumeOr<int32_t>(reader, StatsUtils::INVALID_VALUE));
        case IBatteryStatsIpcCode::COMMAND_SET_ON_BATTERY_IPC:
            return request.WriteInt32(reader.ConsumeBool() ? 1 : 0);
        case IBatteryStatsIpcCode::COMMAND_SHELL_DUMP_IPC:
            return WriteDumpArgs(request, reader);
        default:
            return false;
    }
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

        FuzzedDataProvider reader(data, size);
        size_t operationCount = MIN_ITEM_COUNT + (reader.ConsumeIntegral<uint8_t>() % MAX_OPERATIONS_PER_INPUT);
        for (size_t operationIndex = 0; operationIndex < operationCount; ++operationIndex) {
            const auto code = IPC_CODES[reader.ConsumeIntegral<uint8_t>() % IPC_CODES.size()];
            DispatchOperation(code, reader);
        }

        auto core = service_->GetBatteryStatsCore();
        if (core != nullptr) {
            core->Reset();
        }
    }

private:
    void DispatchOperation(IBatteryStatsIpcCode code, FuzzedDataProvider& reader)
    {
        MessageParcel request;
        MessageParcel reply;
        MessageOption option(MessageOption::TF_SYNC);
        if (!request.WriteInterfaceToken(BatteryStatsService::GetDescriptor()) ||
            !WriteRequestArgs(request, code, reader)) {
            return;
        }
        service_->OnRemoteRequest(static_cast<uint32_t>(code), request, reply, option);
    }

    OHOS::sptr<BatteryStatsService> service_ = nullptr;
};
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    static BatteryStatsServiceIpcFuzzer fuzzer;
    fuzzer.FuzzServiceIpc(data, size);
    return 0;
}

} // namespace PowerMgr
} // namespace OHOS
