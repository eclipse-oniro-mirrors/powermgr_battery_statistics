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

#define FUZZ_PROJECT_NAME "battery_stats_entity_app_fuzzer"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <fuzzer/FuzzedDataProvider.h>

#include "battery_stats_info.h"
#include "entities/alarm_entity.h"
#include "entities/audio_entity.h"
#include "entities/battery_stats_entity.h"
#include "entities/sensor_entity.h"
#include "entities/uid_entity.h"
#include "entities/user_entity.h"
#include "entities/wakelock_entity.h"
#include "stats_utils.h"

using namespace OHOS::PowerMgr;

namespace {
constexpr int32_t STATS_TYPE_MIN = static_cast<int32_t>(StatsUtils::STATS_TYPE_INVALID);
constexpr int32_t STATS_TYPE_MAX = static_cast<int32_t>(StatsUtils::STATS_TYPE_ALARM);
constexpr int32_t CONSUMPTION_TYPE_MIN = static_cast<int32_t>(BatteryStatsInfo::CONSUMPTION_TYPE_INVALID);
constexpr int32_t CONSUMPTION_TYPE_MAX = static_cast<int32_t>(BatteryStatsInfo::CONSUMPTION_TYPE_ALARM);
constexpr size_t MAX_STRING_BYTES = 32;

struct EntityParams {
    int32_t uid = StatsUtils::INVALID_VALUE;
    int32_t userId = StatsUtils::INVALID_VALUE;
    int16_t level = StatsUtils::INVALID_VALUE;
    int64_t power = StatsUtils::DEFAULT_VALUE;
    StatsUtils::StatsType statsType = StatsUtils::STATS_TYPE_INVALID;
};

// Entities are created once: building them per input would repeat /proc parsing and slow the fuzzer down
const std::vector<std::shared_ptr<BatteryStatsEntity>>& GetEntities()
{
    static const std::vector<std::shared_ptr<BatteryStatsEntity>> entities = {
        std::make_shared<AudioEntity>(),
        std::make_shared<SensorEntity>(),
        std::make_shared<WakelockEntity>(),
        std::make_shared<AlarmEntity>(),
        std::make_shared<UidEntity>(),
        std::make_shared<UserEntity>()
    };
    return entities;
}

EntityParams ConsumeParams(FuzzedDataProvider& fdp)
{
    EntityParams params;
    params.uid = fdp.ConsumeIntegral<int32_t>();
    params.userId = fdp.ConsumeIntegral<int32_t>();
    params.level = fdp.ConsumeIntegral<int16_t>();
    params.power = fdp.ConsumeIntegral<int64_t>();
    params.statsType = static_cast<StatsUtils::StatsType>(
        fdp.ConsumeIntegralInRange<int32_t>(STATS_TYPE_MIN, STATS_TYPE_MAX));
    return params;
}

void FuzzAppEntity(BatteryStatsEntity& entity, const EntityParams& params)
{
    (void)entity.GetEntityPowerMah(params.uid);
    (void)entity.GetActiveTimeMs(params.uid, params.statsType, params.level);
    (void)entity.GetOrCreateTimer(params.uid, params.statsType, params.level);
    entity.AggregateUserPowerMah(params.userId, static_cast<double>(params.power));
    entity.UpdateUidMap(params.uid);
    (void)entity.GetUids();
    entity.Calculate(params.uid);
    std::string dumpResult;
    entity.DumpInfo(dumpResult, params.uid);
    entity.Reset();
}

void FuzzStatsInfoList(const EntityParams& params, FuzzedDataProvider& fdp)
{
    std::shared_ptr<BatteryStatsInfo> info = std::make_shared<BatteryStatsInfo>();
    info->SetUid(params.uid);
    info->SetUserId(params.userId);
    info->SetConsumptioType(static_cast<BatteryStatsInfo::ConsumptionType>(
        fdp.ConsumeIntegralInRange<int32_t>(CONSUMPTION_TYPE_MIN, CONSUMPTION_TYPE_MAX)));
    info->SetPower(static_cast<double>(params.power));
    BatteryStatsEntity::UpdateStatsInfoList(info);
    (void)BatteryStatsEntity::GetStatsInfoList();
    (void)BatteryStatsEntity::GetTotalPowerMah();
}
} // namespace

class BatteryStatsEntityAppFuzzer {
public:
    void Fuzz(FuzzedDataProvider& fdp)
    {
        BatteryStatsEntity::ResetStatsEntity();
        EntityParams params = ConsumeParams(fdp);
        for (const auto& entity : GetEntities()) {
            if (entity == nullptr) {
                continue;
            }
            FuzzAppEntity(*entity, params);
        }
        FuzzStatsInfoList(params, fdp);
        BatteryStatsEntity::ResetStatsEntity();
    }
};

namespace {
BatteryStatsEntityAppFuzzer& GetFuzzer()
{
    static BatteryStatsEntityAppFuzzer fuzzer;
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
