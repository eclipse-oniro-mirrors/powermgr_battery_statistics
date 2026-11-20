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

#include "battery_entity_fuzz_support.h"

#include "battery_stats_core.h"
#include "battery_stats_service.h"

namespace OHOS {
namespace PowerMgr {
namespace {
constexpr size_t MIN_ITEM_COUNT = 1;
constexpr size_t STATE_INPUT_DIVISOR = 2;
constexpr uint8_t BOOLEAN_MASK = 1;
constexpr size_t MAX_STATE_EVENTS = 10;
constexpr size_t MAX_ENTITY_QUERIES = 12;
constexpr size_t MAX_STRING_LENGTH = 48;
constexpr int32_t UNKNOWN_CONSUMPTION_TYPE_VALUE = 100;
constexpr int32_t UNKNOWN_STATS_STATE_VALUE = 100;
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

void SeedCoreState(const std::shared_ptr<BatteryStatsCore>& core, FuzzedDataProvider& reader)
{
    size_t eventCount = MIN_ITEM_COUNT + (reader.ConsumeIntegral<uint8_t>() % MAX_STATE_EVENTS);
    for (size_t index = 0; index < eventCount; ++index) {
        auto statsType = PickEntityStatsType(reader.ConsumeIntegral<uint8_t>());
        int32_t uid = ConsumeOr<int32_t>(reader, StatsUtils::INVALID_VALUE);
        int16_t level = ConsumeOr<int16_t>(reader, StatsUtils::INVALID_VALUE);
        if ((reader.ConsumeIntegral<uint8_t>() & BOOLEAN_MASK) == 0) {
            core->UpdateStats(statsType, PickStatsState(reader.ConsumeIntegral<uint8_t>()), level, uid,
                reader.ConsumeRandomLengthString(MAX_STRING_LENGTH));
        } else {
            const auto time = reader.ConsumeIntegral<int64_t>();
            const auto bytes = reader.ConsumeIntegral<int64_t>();
            core->UpdateStats(statsType, time, bytes, uid);
        }
    }
}

class BatteryEntityFuzzer {
public:
    BatteryEntityFuzzer()
    {
        service_ = BatteryStatsService::GetInstance();
        if (service_ != nullptr) {
            service_->OnStart();
        }
    }

    ~BatteryEntityFuzzer()
    {
        if (service_ != nullptr) {
            service_->OnStop();
        }
    }

    void FuzzEntityPower(const uint8_t* data, size_t size, EntityFuzzCase exercise)
    {
        if (data == nullptr || size == 0 || service_ == nullptr) {
            return;
        }
        auto core = service_->GetBatteryStatsCore();
        if (core == nullptr) {
            return;
        }

        FuzzedDataProvider reader(data, size);
        StatsHelper::SetOnBattery((reader.ConsumeIntegral<uint8_t>() & BOOLEAN_MASK) != 0);
        auto seed = reader.ConsumeBytes<uint8_t>(reader.remaining_bytes() / STATE_INPUT_DIVISOR);
        FuzzedDataProvider seedReader(seed.data(), seed.size());
        SeedCoreState(core, seedReader);

        size_t entityCount = MIN_ITEM_COUNT + (reader.ConsumeIntegral<uint8_t>() % MAX_ENTITY_QUERIES);
        for (size_t index = 0; index < entityCount; ++index) {
            auto entity = core->GetEntity(PickConsumptionType(reader.ConsumeIntegral<uint8_t>()));
            if (entity != nullptr) {
                exercise(*entity, reader);
            }
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
} // namespace

StatsUtils::StatsType PickEntityStatsType(uint8_t selector)
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


void RunEntityFuzzCase(const uint8_t* data, size_t size, EntityFuzzCase exercise)
{
    if (data == nullptr || size == 0) {
        return;
    }
    static BatteryEntityFuzzer fuzzer;
    fuzzer.FuzzEntityPower(data, size, exercise);
}

} // namespace PowerMgr
} // namespace OHOS
