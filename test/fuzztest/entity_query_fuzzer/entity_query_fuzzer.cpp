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

#include "entity_query_fuzzer.h"
#include "../battery_entity_fuzz_support.h"

namespace OHOS {
namespace PowerMgr {
namespace {
void Exercise(BatteryStatsEntity& entity, FuzzedDataProvider& reader)
{
    const auto statsType = PickEntityStatsType(reader.ConsumeIntegral<uint8_t>());
    const auto uid = ConsumeOr<int32_t>(reader, StatsUtils::INVALID_VALUE);
    entity.GetStatsPowerMah(statsType, uid);

    const auto type = PickEntityStatsType(reader.ConsumeIntegral<uint8_t>());
    entity.GetActiveTimeMs(type, ConsumeOr<int16_t>(reader, StatsUtils::INVALID_VALUE));
    const auto activeUid = ConsumeOr<int32_t>(reader, StatsUtils::INVALID_VALUE);
    const auto activeType = PickEntityStatsType(reader.ConsumeIntegral<uint8_t>());
    entity.GetActiveTimeMs(activeUid, activeType, ConsumeOr<int16_t>(reader, StatsUtils::INVALID_VALUE));

    const auto trafficType = PickEntityStatsType(reader.ConsumeIntegral<uint8_t>());
    entity.GetTrafficByte(trafficType, ConsumeOr<int32_t>(reader, StatsUtils::INVALID_VALUE));
    const auto countType = PickEntityStatsType(reader.ConsumeIntegral<uint8_t>());
    entity.GetConsumptionCount(countType, ConsumeOr<int32_t>(reader, StatsUtils::INVALID_VALUE));

    entity.GetCpuTimeMs(ConsumeOr<int32_t>(reader, StatsUtils::INVALID_VALUE));
    entity.UpdateCpuTime();
    entity.GetUids();
    std::string dump;
    entity.DumpInfo(dump, ConsumeOr<int32_t>(reader, StatsUtils::INVALID_VALUE));
}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    RunEntityFuzzCase(data, size, Exercise);
    return 0;
}

} // namespace PowerMgr
} // namespace OHOS
