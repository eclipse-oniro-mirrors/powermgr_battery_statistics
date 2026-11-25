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

#include "BatteryStatsIpc_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <vector>

#include <fuzzer/FuzzedDataProvider.h>

#include "battery_stats_service.h"
#include "ibattery_stats.h"
#include "message_parcel.h"

using namespace OHOS;
using namespace OHOS::PowerMgr;

namespace {
constexpr uint32_t CODE_MAX = static_cast<uint32_t>(IBatteryStatsIpcCode::COMMAND_SHELL_DUMP_IPC) + 1;

sptr<BatteryStatsService> g_batteryStatsService = BatteryStatsService::GetInstance();
} // namespace

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;
    if (g_batteryStatsService != nullptr) {
        g_batteryStatsService->OnStart();
    }
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr || g_batteryStatsService == nullptr) {
        return 0;
    }

    FuzzedDataProvider fdp(data, size);
    // FuzzedDataProvider consumes integral values from the end, so the last byte
    // selects the request code and the leading bytes are the marshalled parameters.
    uint32_t code = fdp.ConsumeIntegral<uint8_t>() % CODE_MAX;
    std::vector<uint8_t> payload = fdp.ConsumeRemainingBytes<uint8_t>();

    MessageParcel dataParcel;
    if (!dataParcel.WriteInterfaceToken(BatteryStatsService::GetDescriptor())) {
        return 0;
    }
    if (!payload.empty() && !dataParcel.WriteUnpadBuffer(payload.data(), payload.size())) {
        return 0;
    }
    dataParcel.RewindRead(0);

    MessageParcel replyParcel;
    MessageOption option;
    (void)g_batteryStatsService->OnRemoteRequest(code, dataParcel, replyParcel, option);
    return 0;
}
