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

#include "entity_timer_fuzzer.h"
#include "../battery_entity_fuzz_support.h"

namespace OHOS {
namespace PowerMgr {
namespace {
constexpr uint8_t TIMER_START_OPERATION = 0;
constexpr uint8_t TIMER_STOP_OPERATION = 1;
constexpr uint8_t TIMER_ADD_OPERATION = 2;
constexpr uint8_t TIMER_QUERY_OPERATION = 3;
constexpr uint8_t TIMER_RESET_OPERATION = 4;
constexpr uint8_t TIMER_OPERATION_COUNT = TIMER_RESET_OPERATION + 1;
constexpr size_t MAX_DEVICE_ID_LENGTH = 48;

void ExerciseTimer(const std::shared_ptr<StatsHelper::ActiveTimer>& timer, FuzzedDataProvider& reader)
{
    if (timer == nullptr) {
        return;
    }
    switch (reader.ConsumeIntegral<uint8_t>() % TIMER_OPERATION_COUNT) {
        case TIMER_START_OPERATION:
            timer->StartRunning();
            break;
        case TIMER_STOP_OPERATION:
            timer->StopRunning();
            break;
        case TIMER_ADD_OPERATION:
            timer->AddRunningTimeMs(reader.ConsumeIntegral<int64_t>());
            break;
        case TIMER_QUERY_OPERATION:
            timer->GetRunningTimeMs();
            break;
        default:
            timer->Reset();
            break;
    }
}

void ExerciseCounter(const std::shared_ptr<StatsHelper::Counter>& counter, FuzzedDataProvider& reader)
{
    if (counter == nullptr) {
        return;
    }
    if (!reader.ConsumeBool()) {
        counter->AddCount(reader.ConsumeIntegral<int64_t>());
    } else {
        counter->GetCount();
    }
}

void Exercise(BatteryStatsEntity& entity, FuzzedDataProvider& reader)
{
    const auto globalType = PickEntityStatsType(reader.ConsumeIntegral<uint8_t>());
    const auto globalLevel = ConsumeOr<int16_t>(reader, StatsUtils::INVALID_VALUE);
    ExerciseTimer(entity.GetOrCreateTimer(globalType, globalLevel), reader);

    const auto uid = ConsumeOr<int32_t>(reader, StatsUtils::INVALID_VALUE);
    const auto statsType = PickEntityStatsType(reader.ConsumeIntegral<uint8_t>());
    const auto level = ConsumeOr<int16_t>(reader, StatsUtils::INVALID_VALUE);
    ExerciseTimer(entity.GetOrCreateTimer(uid, statsType, level), reader);

    const auto deviceId = reader.ConsumeRandomLengthString(MAX_DEVICE_ID_LENGTH);
    const auto deviceUid = ConsumeOr<int32_t>(reader, StatsUtils::INVALID_VALUE);
    const auto deviceStatsType = PickEntityStatsType(reader.ConsumeIntegral<uint8_t>());
    const auto deviceLevel = ConsumeOr<int16_t>(reader, StatsUtils::INVALID_VALUE);
    ExerciseTimer(entity.GetOrCreateTimer(deviceId, deviceUid, deviceStatsType, deviceLevel), reader);

    const auto counterType = PickEntityStatsType(reader.ConsumeIntegral<uint8_t>());
    const auto counterUid = ConsumeOr<int32_t>(reader, StatsUtils::INVALID_VALUE);
    ExerciseCounter(entity.GetOrCreateCounter(counterType, counterUid), reader);
}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    RunEntityFuzzCase(data, size, Exercise);
    return 0;
}

} // namespace PowerMgr
} // namespace OHOS
