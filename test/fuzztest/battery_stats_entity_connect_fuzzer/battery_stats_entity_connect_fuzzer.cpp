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

#define FUZZ_PROJECT_NAME "battery_stats_entity_connect_fuzzer"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <fuzzer/FuzzedDataProvider.h>

#include "entities/battery_stats_entity.h"
#include "entities/bluetooth_entity.h"
#include "entities/gnss_entity.h"
#include "entities/phone_entity.h"
#include "entities/wifi_entity.h"
#include "stats_utils.h"

using namespace OHOS::PowerMgr;

namespace {
constexpr int32_t STATS_TYPE_MIN = static_cast<int32_t>(StatsUtils::STATS_TYPE_INVALID);
constexpr int32_t STATS_TYPE_MAX = static_cast<int32_t>(StatsUtils::STATS_TYPE_ALARM);
constexpr size_t MAX_STRING_BYTES = 32;

struct EntityParams {
    int32_t uid = StatsUtils::INVALID_VALUE;
    int16_t level = StatsUtils::INVALID_VALUE;
    StatsUtils::StatsType statsType = StatsUtils::STATS_TYPE_INVALID;
};

// Entities are created once: building them per input would repeat /proc parsing and slow the fuzzer down
const std::vector<std::shared_ptr<BatteryStatsEntity>>& GetEntities()
{
    static const std::vector<std::shared_ptr<BatteryStatsEntity>> entities = {
        std::make_shared<BluetoothEntity>(),
        std::make_shared<WifiEntity>(),
        std::make_shared<PhoneEntity>(),
        std::make_shared<GnssEntity>()
    };
    return entities;
}

EntityParams ConsumeParams(FuzzedDataProvider& fdp)
{
    EntityParams params;
    params.uid = fdp.ConsumeIntegral<int32_t>();
    params.level = fdp.ConsumeIntegral<int16_t>();
    params.statsType = static_cast<StatsUtils::StatsType>(
        fdp.ConsumeIntegralInRange<int32_t>(STATS_TYPE_MIN, STATS_TYPE_MAX));
    return params;
}

void FuzzConnectivityEntity(BatteryStatsEntity& entity, const EntityParams& params, FuzzedDataProvider& fdp)
{
    std::string deviceId = fdp.ConsumeRandomLengthString(MAX_STRING_BYTES);

    (void)entity.GetEntityPowerMah(params.uid);
    (void)entity.GetStatsPowerMah(params.statsType, params.uid);
    (void)entity.GetActiveTimeMs(params.statsType, params.level);
    (void)entity.GetActiveTimeMs(params.uid, params.statsType, params.level);
    (void)entity.GetTrafficByte(params.statsType, params.uid);
    (void)entity.GetConsumptionCount(params.statsType, params.uid);
    (void)entity.GetOrCreateTimer(params.statsType, params.level);
    (void)entity.GetOrCreateTimer(deviceId, params.uid, params.statsType, params.level);
    (void)entity.GetOrCreateCounter(params.statsType, params.uid);
    entity.Calculate(params.uid);
    std::string dumpResult;
    entity.DumpInfo(dumpResult, params.uid);
    entity.Reset();
}
} // namespace

class BatteryStatsEntityConnectFuzzer {
public:
    void Fuzz(FuzzedDataProvider& fdp)
    {
        BatteryStatsEntity::ResetStatsEntity();
        EntityParams params = ConsumeParams(fdp);
        for (const auto& entity : GetEntities()) {
            if (entity == nullptr) {
                continue;
            }
            FuzzConnectivityEntity(*entity, params, fdp);
        }
        BatteryStatsEntity::ResetStatsEntity();
    }
};

namespace {
BatteryStatsEntityConnectFuzzer& GetFuzzer()
{
    static BatteryStatsEntityConnectFuzzer fuzzer;
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
