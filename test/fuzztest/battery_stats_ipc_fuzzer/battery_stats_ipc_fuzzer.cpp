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

#define FUZZ_PROJECT_NAME "battery_stats_ipc_fuzzer"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <fuzzer/FuzzedDataProvider.h>

#include "battery_stats_service.h"
#include "message_parcel.h"
#include "stats_utils.h"
#include "string_ex.h"

using namespace OHOS::PowerMgr;

namespace {
// services/IBatteryStats.idl declares ten methods, their ipc codes are assigned from 0 in declaration order
constexpr uint32_t IPC_CODE_COUNT = 10;
constexpr uint32_t IPC_CODE_MAX = IPC_CODE_COUNT - 1;
constexpr uint32_t IPC_CODE_SHELL_DUMP = IPC_CODE_MAX;
constexpr uint32_t INVALID_IPC_CODE_SPAN = 8;
constexpr int32_t REWIND_READ_DATA = 0;
constexpr size_t MAX_ARG_BYTES = 32;
constexpr size_t MAX_ARG_COUNT = 4;
constexpr size_t MAX_RAW_PAYLOAD_BYTES = 128;
constexpr int32_t STATS_TYPE_MIN = static_cast<int32_t>(StatsUtils::STATS_TYPE_INVALID);
constexpr int32_t STATS_TYPE_MAX = static_cast<int32_t>(StatsUtils::STATS_TYPE_ALARM);

std::vector<std::u16string> ConsumeShellArgs(FuzzedDataProvider& fdp)
{
    std::vector<std::u16string> args;
    while (fdp.remaining_bytes() > 0 && args.size() < MAX_ARG_COUNT) {
        args.emplace_back(OHOS::Str8ToStr16(fdp.ConsumeRandomLengthString(MAX_ARG_BYTES)));
    }
    return args;
}
} // namespace

class BatteryStatsIpcFuzzer {
public:
    BatteryStatsIpcFuzzer()
    {
        service_ = BatteryStatsService::GetInstance();
        if (service_ != nullptr) {
            service_->OnStart();
        }
    }

    ~BatteryStatsIpcFuzzer()
    {
        if (service_ != nullptr) {
            service_->OnStop();
        }
        service_ = nullptr;
    }

    void Fuzz(FuzzedDataProvider& fdp)
    {
        if (service_ == nullptr) {
            return;
        }

        uint32_t code = fdp.ConsumeIntegralInRange<uint32_t>(0, IPC_CODE_MAX);
        // Raw payload: the whole parcel body is fuzz data, so truncated and over long arguments are covered too
        FuzzRawParcel(code, fdp);
        // Unknown code: exercised so that the dispatch fall back branch is reached as well
        uint32_t invalidCode = IPC_CODE_MAX + 1 + fdp.ConsumeIntegralInRange<uint32_t>(0, INVALID_IPC_CODE_SPAN);
        FuzzRawParcel(invalidCode, fdp);
        // Well formed payload: lets the handler body run with typed arguments instead of failing on read
        FuzzTypedParcel(code, fdp);
    }

private:
    void FuzzRawParcel(uint32_t code, FuzzedDataProvider& fdp)
    {
        MessageParcel datas;
        if (!datas.WriteInterfaceToken(BatteryStatsService::GetDescriptor())) {
            return;
        }
        size_t payloadSize = fdp.ConsumeIntegralInRange<size_t>(0, MAX_RAW_PAYLOAD_BYTES);
        std::vector<uint8_t> payload = fdp.ConsumeBytes<uint8_t>(payloadSize);
        if (!payload.empty() && !datas.WriteBuffer(payload.data(), payload.size())) {
            return;
        }
        datas.RewindRead(REWIND_READ_DATA);
        SendRequest(code, datas);
    }

    void FuzzTypedParcel(uint32_t code, FuzzedDataProvider& fdp)
    {
        MessageParcel datas;
        if (!datas.WriteInterfaceToken(BatteryStatsService::GetDescriptor())) {
            return;
        }
        if (code == IPC_CODE_SHELL_DUMP) {
            WriteShellDumpArgs(datas, fdp);
        } else {
            (void)datas.WriteInt32(fdp.ConsumeIntegral<int32_t>());
            (void)datas.WriteInt32(
                fdp.ConsumeIntegralInRange<int32_t>(STATS_TYPE_MIN, STATS_TYPE_MAX));
            (void)datas.WriteBool(fdp.ConsumeBool());
        }
        SendRequest(code, datas);
    }

    void WriteShellDumpArgs(MessageParcel& datas, FuzzedDataProvider& fdp)
    {
        std::vector<std::u16string> args = ConsumeShellArgs(fdp);
        (void)datas.WriteUint32(static_cast<uint32_t>(args.size()));
        for (const auto& arg : args) {
            (void)datas.WriteString16(arg);
        }
        (void)datas.WriteUint32(static_cast<uint32_t>(args.size()));
    }

    void SendRequest(uint32_t code, MessageParcel& datas)
    {
        MessageParcel reply;
        MessageOption option;
        (void)service_->OnRemoteRequest(code, datas, reply, option);
    }

    OHOS::sptr<BatteryStatsService> service_ = nullptr;
};

namespace {
BatteryStatsIpcFuzzer& GetFuzzer()
{
    static BatteryStatsIpcFuzzer fuzzer;
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
