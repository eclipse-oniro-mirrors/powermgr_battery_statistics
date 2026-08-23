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

#include "entity_power_matrix_fuzzer.h"

#include "battery_stats_core.h"
#include "battery_stats_info.h"
#include "battery_stats_service.h"
#include "entities/battery_stats_entity.h"
#include "stats_helper.h"
#include "stats_utils.h"

#include <algorithm>
#include <memory>
#include <string>

namespace OHOS {
namespace PowerMgr {

namespace {
constexpr size_t BYTE_BITS = 8;
constexpr size_t MIN_ITEM_COUNT = 1;
constexpr size_t STRING_LENGTH_RANGE_OFFSET = 1;
constexpr uint8_t BOOLEAN_MASK = 1;
constexpr size_t MAX_STATE_EVENTS = 10;
constexpr size_t MAX_ENTITY_QUERIES = 12;
constexpr size_t MAX_STRING_LENGTH = 48;
constexpr int32_t UNKNOWN_CONSUMPTION_TYPE_VALUE = 100;
constexpr int32_t UNKNOWN_STATS_STATE_VALUE = 100;
constexpr uint8_t TIMER_START_OPERATION = 0;
constexpr uint8_t TIMER_STOP_OPERATION = 1;
constexpr uint8_t TIMER_ADD_OPERATION = 2;
constexpr uint8_t TIMER_QUERY_OPERATION = 3;
constexpr uint8_t TIMER_RESET_OPERATION = 4;
constexpr uint8_t TIMER_OPERATION_COUNT = TIMER_RESET_OPERATION + MIN_ITEM_COUNT;
constexpr double POWER_VALUE_SCALE = 1000.0;

class FuzzReader {
public:
    FuzzReader(const uint8_t* data, size_t size) : data_(data), size_(size) {}

    template <typename T>
    T Read(T fallback = T())
    {
        if (Remaining() < sizeof(T)) {
            return fallback;
        }
        uint64_t value = 0;
        for (size_t index = 0; index < sizeof(T); ++index) {
            value |= static_cast<uint64_t>(data_[offset_ + index]) << (BYTE_BITS * index);
        }
        offset_ += sizeof(T);
        return static_cast<T>(value);
    }

    std::string ReadString(size_t maxLength)
    {
        if (Remaining() == 0) {
            return "";
        }
        size_t length = Read<uint8_t>(0) % (maxLength + STRING_LENGTH_RANGE_OFFSET);
        length = std::min(length, Remaining());
        std::string value;
        value.reserve(length);
        for (size_t index = 0; index < length; ++index) {
            value.push_back(static_cast<char>(data_[offset_++]));
        }
        return value;
    }

private:
    size_t Remaining() const
    {
        return size_ - offset_;
    }

    const uint8_t* data_ = nullptr;
    size_t size_ = 0;
    size_t offset_ = 0;
};

BatteryStatsInfo::ConsumptionType PickConsumptionType(uint8_t selector)
{
    static constexpr BatteryStatsInfo::ConsumptionType types[] = {
        BatteryStatsInfo::CONSUMPTION_TYPE_APP,
        BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH,
        BatteryStatsInfo::CONSUMPTION_TYPE_IDLE,
        BatteryStatsInfo::CONSUMPTION_TYPE_PHONE,
        BatteryStatsInfo::CONSUMPTION_TYPE_RADIO,
        BatteryStatsInfo::CONSUMPTION_TYPE_SCREEN,
        BatteryStatsInfo::CONSUMPTION_TYPE_USER,
        BatteryStatsInfo::CONSUMPTION_TYPE_WIFI,
        BatteryStatsInfo::CONSUMPTION_TYPE_CAMERA,
        BatteryStatsInfo::CONSUMPTION_TYPE_FLASHLIGHT,
        BatteryStatsInfo::CONSUMPTION_TYPE_AUDIO,
        BatteryStatsInfo::CONSUMPTION_TYPE_SENSOR,
        BatteryStatsInfo::CONSUMPTION_TYPE_GNSS,
        BatteryStatsInfo::CONSUMPTION_TYPE_CPU,
        BatteryStatsInfo::CONSUMPTION_TYPE_WAKELOCK,
        BatteryStatsInfo::CONSUMPTION_TYPE_ALARM,
        BatteryStatsInfo::CONSUMPTION_TYPE_INVALID,
        static_cast<BatteryStatsInfo::ConsumptionType>(UNKNOWN_CONSUMPTION_TYPE_VALUE),
    };
    return types[selector % (sizeof(types) / sizeof(types[0]))];
}

StatsUtils::StatsType PickStatsType(uint8_t selector)
{
    static constexpr StatsUtils::StatsType types[] = {
        StatsUtils::STATS_TYPE_BLUETOOTH_BR_ON,
        StatsUtils::STATS_TYPE_BLUETOOTH_BR_SCAN,
        StatsUtils::STATS_TYPE_BLUETOOTH_BLE_ON,
        StatsUtils::STATS_TYPE_BLUETOOTH_BLE_SCAN,
        StatsUtils::STATS_TYPE_WIFI_ON,
        StatsUtils::STATS_TYPE_WIFI_SCAN,
        StatsUtils::STATS_TYPE_PHONE_ACTIVE,
        StatsUtils::STATS_TYPE_PHONE_DATA,
        StatsUtils::STATS_TYPE_CAMERA_ON,
        StatsUtils::STATS_TYPE_CAMERA_FLASHLIGHT_ON,
        StatsUtils::STATS_TYPE_FLASHLIGHT_ON,
        StatsUtils::STATS_TYPE_GNSS_ON,
        StatsUtils::STATS_TYPE_SENSOR_GRAVITY_ON,
        StatsUtils::STATS_TYPE_SENSOR_PROXIMITY_ON,
        StatsUtils::STATS_TYPE_AUDIO_ON,
        StatsUtils::STATS_TYPE_SCREEN_ON,
        StatsUtils::STATS_TYPE_SCREEN_BRIGHTNESS,
        StatsUtils::STATS_TYPE_WAKELOCK_HOLD,
        StatsUtils::STATS_TYPE_PHONE_IDLE,
        StatsUtils::STATS_TYPE_CPU_CLUSTER,
        StatsUtils::STATS_TYPE_CPU_SPEED,
        StatsUtils::STATS_TYPE_CPU_ACTIVE,
        StatsUtils::STATS_TYPE_CPU_SUSPEND,
        StatsUtils::STATS_TYPE_ALARM,
        StatsUtils::STATS_TYPE_INVALID,
    };
    return types[selector % (sizeof(types) / sizeof(types[0]))];
}

StatsUtils::StatsState PickStatsState(uint8_t selector)
{
    static constexpr StatsUtils::StatsState states[] = {
        StatsUtils::STATS_STATE_ACTIVATED,
        StatsUtils::STATS_STATE_DEACTIVATED,
        StatsUtils::STATS_STATE_WORKSCHEDULER_SCHEDULED,
        StatsUtils::STATS_STATE_WORKSCHEDULER_EXECUTED,
        StatsUtils::STATS_STATE_INVALID,
        static_cast<StatsUtils::StatsState>(UNKNOWN_STATS_STATE_VALUE),
    };
    return states[selector % (sizeof(states) / sizeof(states[0]))];
}

void SeedCoreState(const std::shared_ptr<BatteryStatsCore>& core, FuzzReader& reader)
{
    size_t eventCount = MIN_ITEM_COUNT + (reader.Read<uint8_t>(0) % MAX_STATE_EVENTS);
    for (size_t index = 0; index < eventCount; ++index) {
        auto statsType = PickStatsType(reader.Read<uint8_t>(0));
        int32_t uid = reader.Read<int32_t>(StatsUtils::INVALID_VALUE);
        int16_t level = reader.Read<int16_t>(StatsUtils::INVALID_VALUE);
        if ((reader.Read<uint8_t>(0) & BOOLEAN_MASK) == 0) {
            core->UpdateStats(statsType, PickStatsState(reader.Read<uint8_t>(0)), level, uid,
                reader.ReadString(MAX_STRING_LENGTH));
        } else {
            core->UpdateStats(statsType, reader.Read<int64_t>(0), reader.Read<int64_t>(0), uid);
        }
    }
}

void ExerciseTimer(const std::shared_ptr<StatsHelper::ActiveTimer>& timer, FuzzReader& reader)
{
    if (timer == nullptr) {
        return;
    }
    switch (reader.Read<uint8_t>(0) % TIMER_OPERATION_COUNT) {
        case TIMER_START_OPERATION:
            timer->StartRunning();
            break;
        case TIMER_STOP_OPERATION:
            timer->StopRunning();
            break;
        case TIMER_ADD_OPERATION:
            timer->AddRunningTimeMs(reader.Read<int64_t>(0));
            break;
        case TIMER_QUERY_OPERATION:
            timer->GetRunningTimeMs();
            break;
        default:
            timer->Reset();
            break;
    }
}

void ExerciseCounter(const std::shared_ptr<StatsHelper::Counter>& counter, FuzzReader& reader)
{
    if (counter == nullptr) {
        return;
    }
    if ((reader.Read<uint8_t>(0) & BOOLEAN_MASK) == 0) {
        counter->AddCount(reader.Read<int64_t>(0));
    } else {
        counter->GetCount();
    }
}

void ExerciseEntity(const std::shared_ptr<BatteryStatsEntity>& entity, FuzzReader& reader)
{
    if (entity == nullptr) {
        return;
    }
    size_t queryCount = MIN_ITEM_COUNT + (reader.Read<uint8_t>(0) % MAX_ENTITY_QUERIES);
    for (size_t index = 0; index < queryCount; ++index) {
        int32_t uid = reader.Read<int32_t>(StatsUtils::INVALID_VALUE);
        int32_t userId = reader.Read<int32_t>(StatsUtils::INVALID_VALUE);
        int16_t level = reader.Read<int16_t>(StatsUtils::INVALID_VALUE);
        auto statsType = PickStatsType(reader.Read<uint8_t>(0));
        std::string deviceId = reader.ReadString(MAX_STRING_LENGTH);

        entity->GetConsumptionType();
        entity->UpdateUidMap(uid);
        entity->AggregateUserPowerMah(userId, static_cast<double>(reader.Read<int64_t>(0)) / POWER_VALUE_SCALE);
        entity->Calculate(uid);
        entity->GetEntityPowerMah(uid);
        entity->GetEntityPowerMah(userId);
        entity->GetStatsPowerMah(statsType, uid);
        entity->GetActiveTimeMs(statsType, level);
        entity->GetActiveTimeMs(uid, statsType, level);
        entity->GetTrafficByte(statsType, uid);
        entity->GetConsumptionCount(statsType, uid);
        ExerciseTimer(entity->GetOrCreateTimer(statsType, level), reader);
        ExerciseTimer(entity->GetOrCreateTimer(uid, statsType, level), reader);
        ExerciseTimer(entity->GetOrCreateTimer(deviceId, uid, statsType, level), reader);
        ExerciseCounter(entity->GetOrCreateCounter(statsType, uid), reader);
        entity->GetCpuTimeMs(uid);
        auto refreshEntityCpuSnapshot = &BatteryStatsEntity::UpdateCpuTime;
        ((*entity).*refreshEntityCpuSnapshot)();
        entity->GetUids();
        std::string dump;
        entity->DumpInfo(dump, uid);
        if ((reader.Read<uint8_t>(0) & BOOLEAN_MASK) != 0) {
            entity->Reset();
        }
    }
}

class EntityPowerMatrixFuzzer {
public:
    EntityPowerMatrixFuzzer()
    {
        service_ = BatteryStatsService::GetInstance();
        if (service_ != nullptr) {
            service_->OnStart();
        }
    }

    ~EntityPowerMatrixFuzzer()
    {
        if (service_ != nullptr) {
            service_->OnStop();
        }
    }

    void FuzzEntityPower(const uint8_t* data, size_t size)
    {
        if (data == nullptr || size == 0 || service_ == nullptr) {
            return;
        }
        auto core = service_->GetBatteryStatsCore();
        if (core == nullptr) {
            return;
        }

        FuzzReader reader(data, size);
        StatsHelper::SetOnBattery((reader.Read<uint8_t>(0) & BOOLEAN_MASK) != 0);
        SeedCoreState(core, reader);

        size_t entityCount = MIN_ITEM_COUNT + (reader.Read<uint8_t>(0) % MAX_ENTITY_QUERIES);
        for (size_t index = 0; index < entityCount; ++index) {
            auto entity = core->GetEntity(PickConsumptionType(reader.Read<uint8_t>(0)));
            ExerciseEntity(entity, reader);
        }

        core->ComputePower();
        core->GetBatteryStats();
        BatteryStatsEntity::GetTotalPowerMah();
        BatteryStatsEntity::GetStatsInfoList();
        core->Reset();
    }

private:
    OHOS::sptr<BatteryStatsService> service_ = nullptr;
};

EntityPowerMatrixFuzzer g_fuzzer;
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    g_fuzzer.FuzzEntityPower(data, size);
    return 0;
}

} // namespace PowerMgr
} // namespace OHOS
