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

#ifndef BATTERY_BATTERY_STATS_ENTITY_H
#define BATTERY_BATTERY_STATS_ENTITY_H

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <mutex>
#include <vector>
#include "stats_utils.h"
#include "stats_helper.h"
#include "battery_stats_info.h"

namespace OHOS {
namespace PowerMgr {
class BatteryStatsEntity {
public:
    BatteryStatsEntity() = default;
    virtual ~BatteryStatsEntity() = default;
    virtual double GetEntityPowerMah(int32_t uidOrUserId = StatsUtils::INVALID_VALUE) = 0;
    virtual void Reset() = 0;
    virtual void Calculate(int32_t uid = StatsUtils::INVALID_VALUE) = 0;
    virtual int64_t GetActiveTimeMs(StatsUtils::StatsType statsType, int16_t level = StatsUtils::INVALID_VALUE);
    virtual int64_t GetActiveTimeMs(int32_t uid, StatsUtils::StatsType statsType,
        int16_t level = StatsUtils::INVALID_VALUE);
    virtual int64_t GetTrafficByte(StatsUtils::StatsType statsType, int32_t uid = StatsUtils::INVALID_VALUE);
    virtual int64_t GetConsumptionCount(StatsUtils::StatsType statsType, int32_t uid = StatsUtils::INVALID_VALUE);
    virtual double GetStatsPowerMah(StatsUtils::StatsType statsType, int32_t uid = StatsUtils::INVALID_VALUE);
    virtual std::shared_ptr<StatsHelper::ActiveTimer> GetOrCreateTimer(StatsUtils::StatsType statsType,
        int16_t level = StatsUtils::INVALID_VALUE);
    virtual std::shared_ptr<StatsHelper::ActiveTimer> GetOrCreateTimer(int32_t uid, StatsUtils::StatsType statsType,
        int16_t level = StatsUtils::INVALID_VALUE);
    virtual std::shared_ptr<StatsHelper::ActiveTimer> GetOrCreateTimer(const std::string& deviceId, int32_t uid,
        StatsUtils::StatsType statsType, int16_t level = StatsUtils::INVALID_VALUE);
    virtual std::shared_ptr<StatsHelper::Counter> GetOrCreateCounter(StatsUtils::StatsType statsType,
        int32_t uid = StatsUtils::INVALID_VALUE);
    virtual void AggregateUserPowerMah(int32_t userId, double power);
    virtual void UpdateUidMap(int32_t uid);
    virtual int64_t GetCpuTimeMs(int32_t uid);
    virtual void UpdateCpuTime();
    virtual std::vector<int32_t> GetUids();
    virtual void DumpInfo(std::string& result, int32_t uid = StatsUtils::INVALID_VALUE);
    BatteryStatsInfo::ConsumptionType GetConsumptionType();
    static double GetTotalPowerMah();
    static void ResetStatsEntity();
    static BatteryStatsInfoList GetStatsInfoList();
    static void UpdateStatsInfoList(std::shared_ptr<BatteryStatsInfo> info);
protected:
    static double totalPowerMah_;
    static BatteryStatsInfoList statsInfoList_;
    // Protects the static members statsInfoList_ and totalPowerMah_, which are shared by all entity
    // instances and accessed from both CommonEvent and binder threads.
    static std::mutex statsMutex_;
    BatteryStatsInfo::ConsumptionType consumptionType_ = BatteryStatsInfo::CONSUMPTION_TYPE_INVALID;
};
} // namespace PowerMgr
} // namespace OHOS
#endif // BATTERY_BATTERY_STATS_ENTITY_H