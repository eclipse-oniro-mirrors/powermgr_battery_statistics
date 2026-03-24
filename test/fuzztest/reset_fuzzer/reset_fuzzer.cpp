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

#include "reset_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <cstring>     // For memcpy_s

#include "battery_stats_client.h"
#include "stats_utils.h"

using namespace OHOS::PowerMgr;

namespace {
constexpr int32_t MIN_UID = 1000;
constexpr int32_t MAX_UID = 20000;
constexpr int32_t BIT_PER_BYTE = 8;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    auto& statsClient = BatteryStatsClient::GetInstance();
    
    /* Set on battery state with bounds check */
    if (size > 0) {
        bool onBattery = (data[0] % 2 == 0);
        statsClient.SetOnBattery(onBattery);
    }
    
    /* Generate UID safely without memcpy */
    int32_t uid = StatsUtils::INVALID_VALUE;
    if (size >= sizeof(int32_t) + 1) {
        uint32_t temp = 0;
        for (size_t i = 0; i < sizeof(uint32_t) && (i + 1) < size; ++i) {
            temp = (temp << BIT_PER_BYTE) | data[i + 1];
        }
        constexpr uint32_t range = MAX_UID - MIN_UID;
        uid = MIN_UID + static_cast<int32_t>(temp % range);
    }
        
    if (uid != StatsUtils::INVALID_VALUE) {
        (void)statsClient.GetAppStatsMah(uid);
        (void)statsClient.GetAppStatsPercent(uid);
    }
    
    statsClient.Reset();
    
    if (uid != StatsUtils::INVALID_VALUE) {
        (void)statsClient.GetAppStatsMah(uid);
        (void)statsClient.GetAppStatsPercent(uid);
    }
    
    (void)statsClient.GetBatteryStats();
    
    return 0;
}