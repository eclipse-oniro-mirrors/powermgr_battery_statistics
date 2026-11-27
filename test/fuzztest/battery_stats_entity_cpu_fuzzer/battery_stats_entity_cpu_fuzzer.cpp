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

#define FUZZ_PROJECT_NAME "battery_stats_entity_cpu_fuzzer"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <fuzzer/FuzzedDataProvider.h>

#include "entities/battery_stats_entity.h"
#include "entities/camera_entity.h"
#include "entities/cpu_entity.h"
#include "entities/flashlight_entity.h"
#include "entities/idle_entity.h"
#include "entities/screen_entity.h"
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
        std::make_shared<CpuEntity>(),
        std::make_shared<IdleEntity>(),
        std::make_shared<ScreenEntity>(),
        std::make_shared<CameraEntity>(),
        std::make_shared<FlashlightEntity>()
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

void FuzzHardwareEntity(BatteryStatsEntity& entity, const EntityParams& params, FuzzedDataProvider& fdp)
{
    // The device id timer is the one CameraEntity keys its per device timers on
    std::string deviceId = fdp.ConsumeRandomLengthString(MAX_STRING_BYTES);

    (void)entity.GetEntityPowerMah(params.uid);
    (void)entity.GetStatsPowerMah(params.statsType, params.uid);
    (void)entity.GetActiveTimeMs(params.uid, params.statsType, params.level);
    (void)entity.GetCpuTimeMs(params.uid);
    (void)entity.GetOrCreateTimer(deviceId, params.uid, params.statsType, params.level);
    (void)entity.GetOrCreateCounter(params.statsType, params.uid);
    entity.UpdateCpuTime();
    entity.Calculate(params.uid);
    std::string dumpResult;
    entity.DumpInfo(dumpResult, params.uid);
    entity.Reset();
}
} // namespace

class BatteryStatsEntityCpuFuzzer {
public:
    void Fuzz(FuzzedDataProvider& fdp)
    {
        BatteryStatsEntity::ResetStatsEntity();
        EntityParams params = ConsumeParams(fdp);
        for (const auto& entity : GetEntities()) {
            if (entity == nullptr) {
                continue;
            }
            FuzzHardwareEntity(*entity, params, fdp);
        }
        BatteryStatsEntity::ResetStatsEntity();
    }
};

namespace {
BatteryStatsEntityCpuFuzzer& GetFuzzer()
{
    static BatteryStatsEntityCpuFuzzer fuzzer;
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
