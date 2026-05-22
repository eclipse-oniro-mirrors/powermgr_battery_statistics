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

#define FUZZ_PROJECT_NAME "battery_stats_entity_fuzzer"

#include "entities/alarm_entity.h"
#include "entities/audio_entity.h"
#include "entities/battery_stats_entity.h"
#include "entities/bluetooth_entity.h"
#include "entities/camera_entity.h"
#include "entities/cpu_entity.h"
#include "entities/flashlight_entity.h"
#include "entities/gnss_entity.h"
#include "entities/idle_entity.h"
#include "entities/phone_entity.h"
#include "entities/screen_entity.h"
#include "entities/sensor_entity.h"
#include "entities/uid_entity.h"
#include "entities/user_entity.h"
#include "entities/wakelock_entity.h"
#include "entities/wifi_entity.h"
#include "battery_stats_info.h"
#include "stats_helper.h"
#include "stats_utils.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

using namespace OHOS::PowerMgr;

namespace {
constexpr size_t MIN_INPUT_SIZE = sizeof(int32_t);
constexpr size_t MAX_STRING_BYTES = 32;
constexpr size_t STATS_TYPE_COUNT = 30;
constexpr size_t CONSUMPTION_TYPE_COUNT = 17;

template<typename T>
T ReadValue(const uint8_t* data, size_t size, size_t& offset)
{
    T value {};
    if (data == nullptr || offset + sizeof(T) > size) {
        return value;
    }
    (void)memcpy_s(&value, sizeof(T), data + offset, sizeof(T));
    offset += sizeof(T);
    return value;
}

std::string ReadString(const uint8_t* data, size_t size, size_t& offset)
{
    if (data == nullptr || offset >= size) {
        return {};
    }

    size_t length = std::min(static_cast<size_t>(data[offset] % MAX_STRING_BYTES), size - offset - 1);
    offset++;
    if (length == 0 || offset + length > size) {
        return {};
    }

    std::string value(reinterpret_cast<const char*>(data + offset), length);
    offset += length;
    return value;
}

StatsUtils::StatsType ToStatsType(uint8_t raw)
{
    return static_cast<StatsUtils::StatsType>(static_cast<int32_t>(raw % STATS_TYPE_COUNT) - 1);
}

BatteryStatsInfo::ConsumptionType ToConsumptionType(uint8_t raw)
{
    return static_cast<BatteryStatsInfo::ConsumptionType>(
        static_cast<int32_t>(raw % CONSUMPTION_TYPE_COUNT));
}

BatteryStatsInfo::ConsumptionType ToInvalidConsumptionType(uint8_t raw)
{
    return static_cast<BatteryStatsInfo::ConsumptionType>(
        -static_cast<int32_t>(raw % CONSUMPTION_TYPE_COUNT) - 1);
}

std::vector<std::shared_ptr<BatteryStatsEntity>> BuildEntities()
{
    return {
        std::make_shared<AudioEntity>(),
        std::make_shared<BluetoothEntity>(),
        std::make_shared<CameraEntity>(),
        std::make_shared<CpuEntity>(),
        std::make_shared<FlashlightEntity>(),
        std::make_shared<GnssEntity>(),
        std::make_shared<IdleEntity>(),
        std::make_shared<PhoneEntity>(),
        std::make_shared<ScreenEntity>(),
        std::make_shared<SensorEntity>(),
        std::make_shared<UidEntity>(),
        std::make_shared<UserEntity>(),
        std::make_shared<WakelockEntity>(),
        std::make_shared<WifiEntity>(),
        std::make_shared<AlarmEntity>()
    };
}

void ExerciseTimer(const std::shared_ptr<StatsHelper::ActiveTimer>& timer, int64_t delta)
{
    if (timer == nullptr) {
        return;
    }
    timer->StartRunning();
    timer->GetRunningTimeMs();
    timer->AddRunningTimeMs(delta);
    timer->StopRunning();
    timer->Reset();
}

void ExerciseCounter(const std::shared_ptr<StatsHelper::Counter>& counter, int64_t delta)
{
    if (counter == nullptr) {
        return;
    }
    counter->AddCount(delta);
    (void)counter->GetCount();
    counter->Reset();
}
} // namespace

class BatteryStatsEntityFuzzer {
public:
    void FuzzEntities(int32_t uid, int32_t userId, int16_t level, int64_t powerDelta,
        const std::string& deviceId, StatsUtils::StatsType statsType)
    {
        for (const auto& entity : BuildEntities()) {
            if (entity == nullptr) {
                continue;
            }

            std::string dumpResult;
            entity->Calculate(uid);
            (void)entity->GetEntityPowerMah(uid);
            (void)entity->GetStatsPowerMah(statsType, uid);
            (void)entity->GetActiveTimeMs(statsType, level);
            (void)entity->GetActiveTimeMs(uid, statsType, level);
            (void)entity->GetTrafficByte(statsType, uid);
            (void)entity->GetConsumptionCount(statsType, uid);
            (void)entity->GetOrCreateTimer(statsType, level);
            ExerciseTimer(entity->GetOrCreateTimer(uid, statsType, level), powerDelta);
            ExerciseTimer(entity->GetOrCreateTimer(deviceId, uid, statsType, level), powerDelta);
            ExerciseCounter(entity->GetOrCreateCounter(statsType, uid), powerDelta);
            entity->AggregateUserPowerMah(userId, static_cast<double>(powerDelta));
            entity->UpdateUidMap(uid);
            (void)entity->GetCpuTimeMs(uid);
            entity->UpdateCpuTime();
            (void)entity->GetUids();
            entity->DumpInfo(dumpResult, uid);
            (void)entity->GetConsumptionType();
            entity->Reset();
        }
    }

    void Fuzz(const uint8_t* data, size_t size)
    {
        if (data == nullptr || size < MIN_INPUT_SIZE) {
            return;
        }

        BatteryStatsEntity::ResetStatsEntity();

        size_t offset = 0;
        int32_t uid = ReadValue<int32_t>(data, size, offset);
        int32_t userId = ReadValue<int32_t>(data, size, offset);
        int16_t level = static_cast<int16_t>(ReadValue<uint8_t>(data, size, offset));
        int64_t powerDelta = ReadValue<int64_t>(data, size, offset);
        std::string deviceId = ReadString(data, size, offset);
        StatsUtils::StatsType statsType = ToStatsType(data[0]);
        BatteryStatsInfo::ConsumptionType consumptionType = ToConsumptionType(data[0]);
        BatteryStatsInfo::ConsumptionType invalidConsumptionType = ToInvalidConsumptionType(data[0]);

        auto info = std::make_shared<BatteryStatsInfo>();
        info->SetUid(uid);
        info->SetUserId(userId);
        info->SetConsumptioType(consumptionType);
        info->SetPower(static_cast<double>(powerDelta));
        BatteryStatsEntity::UpdateStatsInfoList(info);
        (void)BatteryStatsEntity::GetStatsInfoList();
        (void)BatteryStatsEntity::GetTotalPowerMah();
        (void)BatteryStatsInfo::ConvertConsumptionType(consumptionType);
        (void)BatteryStatsInfo::ConvertConsumptionType(invalidConsumptionType);

        FuzzEntities(uid, userId, level, powerDelta, deviceId, statsType);
        BatteryStatsEntity::ResetStatsEntity();
    }
};

namespace {
BatteryStatsEntityFuzzer g_fuzzer;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    g_fuzzer.Fuzz(data, size);
    return 0;
}
