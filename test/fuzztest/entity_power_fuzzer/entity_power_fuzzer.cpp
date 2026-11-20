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

#include "entity_power_fuzzer.h"
#include "../battery_entity_fuzz_support.h"

namespace OHOS {
namespace PowerMgr {
namespace {
constexpr double POWER_VALUE_SCALE = 1000.0;

void Exercise(BatteryStatsEntity& entity, FuzzedDataProvider& reader)
{
    entity.GetConsumptionType();
    entity.UpdateUidMap(ConsumeOr<int32_t>(reader, StatsUtils::INVALID_VALUE));
    const auto userId = ConsumeOr<int32_t>(reader, StatsUtils::INVALID_VALUE);
    const auto power = static_cast<double>(reader.ConsumeIntegral<int64_t>()) / POWER_VALUE_SCALE;
    entity.AggregateUserPowerMah(userId, power);
    entity.Calculate(ConsumeOr<int32_t>(reader, StatsUtils::INVALID_VALUE));
    entity.GetEntityPowerMah(ConsumeOr<int32_t>(reader, StatsUtils::INVALID_VALUE));
    entity.GetEntityPowerMah(ConsumeOr<int32_t>(reader, StatsUtils::INVALID_VALUE));
    if (reader.ConsumeBool()) {
        entity.Reset();
    }
}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    RunEntityFuzzCase(data, size, Exercise);
    return 0;
}

} // namespace PowerMgr
} // namespace OHOS
