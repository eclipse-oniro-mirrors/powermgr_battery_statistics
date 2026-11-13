/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "entities/user_entity.h"

#include "ohos_account_kits_impl.h"
#include "stats_log.h"

#include "battery_stats_parser.h"
#include "battery_stats_service.h"

namespace OHOS {
namespace PowerMgr {
namespace {
}

UserEntity::UserEntity()
{
    consumptionType_ = BatteryStatsInfo::CONSUMPTION_TYPE_USER;
}

double UserEntity::GetEntityPowerMah(int32_t uidOrUserId)
{
    double power = StatsUtils::DEFAULT_VALUE;
    auto iter = userPowerMap_.find(uidOrUserId);
    if (iter != userPowerMap_.end()) {
        power = iter->second;
        STATS_HILOGD(COMP_SVC, "Get user power consumption: %{public}lfmAh for user id: %{public}d",
            power, uidOrUserId);
    } else {
        STATS_HILOGD(COMP_SVC,
            "No user power consumption related with user id: %{public}d found, return 0", uidOrUserId);
    }
    return power;
}

void UserEntity::AggregateUserPowerMah(int32_t userId, double power)
{
    auto iter = userPowerMap_.find(userId);
    if (iter != userPowerMap_.end()) {
        iter->second += power;
        STATS_HILOGD(COMP_SVC, "Add user power consumption: %{public}lfmAh for user id: %{public}d",
            power, userId);
    } else {
        STATS_HILOGD(COMP_SVC, "Create user power consumption: %{public}lfmAh for user id: %{public}d",
            power, userId);
        userPowerMap_.insert(std::pair<int32_t, double>(userId, power));
    }
}

void UserEntity::Calculate(int32_t uid)
{
    for (auto& iter : userPowerMap_) {
        std::shared_ptr<BatteryStatsInfo> statsInfo = std::make_shared<BatteryStatsInfo>();
        statsInfo->SetConsumptioType(BatteryStatsInfo::CONSUMPTION_TYPE_USER);
        statsInfo->SetUserId(iter.first);
        statsInfo->SetPower(iter.second);
        UpdateStatsInfoList(statsInfo);
    }
}

void UserEntity::Reset()
{
    // Reset app user total power consumption
    for (auto& iter : userPowerMap_) {
        iter.second = StatsUtils::DEFAULT_VALUE;
    }
}
} // namespace PowerMgr
} // namespace OHOS

