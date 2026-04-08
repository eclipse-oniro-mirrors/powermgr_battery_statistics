/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "batterystatsClientcallback_fuzzer.h"

#include <cstddef>
#include <cstdint>

#include "battery_stats_client.h"
#include "iremote_object.h"

using namespace OHOS::PowerMgr;
using namespace OHOS;

namespace OHOS {
namespace PowerMgr {

// Test BatteryStatsClient::GetLastError
void GetLastErrorFuzzTest(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return;
    }

    BatteryStatsClient::GetInstance().GetLastError();
}

// Note: BatteryStatsDeathRecipient is a private class, cannot be tested directly
// The OnRemoteDied callback is tested indirectly through client lifecycle

void ResetClientFuzzTest(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return;
    }

    // Test client reset functionality which indirectly tests death recipient
    BatteryStatsClient::GetInstance().Reset();
}

} // namespace PowerMgr
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    OHOS::PowerMgr::GetLastErrorFuzzTest(data, size);
    OHOS::PowerMgr::ResetClientFuzzTest(data, size);
    return 0;
}

