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

#ifndef BATTERY_STATS_SERVICE_H
#define BATTERY_STATS_SERVICE_H

#include <atomic>
#include "common_event_subscriber.h"
#include "hisysevent_listener.h"
#include "system_ability.h"

#include "battery_stats_core.h"
#include "battery_stats_detector.h"
#include "battery_stats_errors.h"
#include "battery_stats_info.h"
#include "battery_stats_parser.h"
#include "battery_stats_stub.h"

namespace OHOS {
namespace PowerMgr {
class BatteryStatsService final : public SystemAbility, public BatteryStatsStub {
    DECLARE_SYSTEM_ABILITY(BatteryStatsService);
    DISALLOW_COPY_AND_MOVE(BatteryStatsService);

public:
    BatteryStatsService();
    virtual ~BatteryStatsService();
    virtual void OnStart() override;
    virtual void OnStop() override;
    virtual void OnAddSystemAbility(int32_t systemAbilityId, const std::string& deviceId) override;

    bool IsServiceReady() const;
    int32_t Dump(int32_t fd, const std::vector<std::u16string>& args) override;

    int32_t GetBatteryStatsIpc(ParcelableBatteryStatsList& batteryStats, int32_t& tempError) override;
    int32_t SetOnBatteryIpc(bool isOnBattery) override;
    int32_t GetAppStatsMahIpc(int32_t uid, double& appStatsMah, int32_t& tempError) override;
    int32_t GetAppStatsPercentIpc(int32_t uid, double& appStatsPercent, int32_t& tempError) override;
    int32_t GetPartStatsMahIpc(int32_t type, double& partStatsMah, int32_t& tempError) override;
    int32_t GetPartStatsPercentIpc(int32_t type, double& partStatsPercent, int32_t& tempError) override;
    int32_t GetTotalTimeSecondIpc(int32_t statsType, int32_t uid, uint64_t& totalTimeSecond) override;
    int32_t GetTotalDataBytesIpc(int32_t statsType, int32_t uid, uint64_t& totalDataBytes) override;
    int32_t ResetIpc() override;
    int32_t ShellDumpIpc(const std::vector<std::string>& args, uint32_t argc, std::string& dumpShell) override;

    BatteryStatsInfoList GetBatteryStats();
    double GetAppStatsMah(const int32_t& uid);
    double GetAppStatsPercent(const int32_t& uid);
    double GetPartStatsMah(const BatteryStatsInfo::ConsumptionType& type);
    double GetPartStatsPercent(const BatteryStatsInfo::ConsumptionType& type);
    uint64_t GetTotalTimeSecond(const StatsUtils::StatsType& statsType, const int32_t& uid = StatsUtils::INVALID_VALUE);
    uint64_t GetTotalDataBytes(const StatsUtils::StatsType& statsType, const int32_t& uid = StatsUtils::INVALID_VALUE);
    void UpdateStats(StatsUtils::StatsType statsType, StatsUtils::StatsState state,
        int16_t level = StatsUtils::INVALID_VALUE, int32_t uid = StatsUtils::INVALID_VALUE,
        const std::string& deviceId = "");
    void UpdateStats(StatsUtils::StatsType statsType, int64_t time, int64_t data,
        int32_t uid = StatsUtils::INVALID_VALUE);
    void Reset();
    void SetOnBattery(bool isOnBattery);
    std::string ShellDump(const std::vector<std::string>& args, uint32_t argc);
    std::shared_ptr<BatteryStatsCore> GetBatteryStatsCore() const;
    std::shared_ptr<BatteryStatsParser> GetBatteryStatsParser() const;
    std::shared_ptr<BatteryStatsDetector> GetBatteryStatsDetector() const;

    static sptr<BatteryStatsService> GetInstance();
    static void DestroyInstance();

#ifndef STATS_SERVICE_UT_TEST
private:
#endif
    static constexpr int32_t DEPENDENCY_CHECK_DELAY_MS = 2000;
    bool Init();
    std::shared_ptr<BatteryStatsCore> core_;
    std::shared_ptr<BatteryStatsParser> parser_;
    std::shared_ptr<BatteryStatsDetector> detector_;
    std::shared_ptr<EventFwk::CommonEventSubscriber> subscriberPtr_;
    std::shared_ptr<HiviewDFX::HiSysEventListener> listenerPtr_;
    bool ready_ = false;
    static std::atomic_bool isBootCompleted_;
    std::mutex mutex_;
    std::atomic_int32_t lastError_ {static_cast<int32_t>(StatsError::ERR_OK)};
    bool SubscribeCommonEvent();
    bool AddHiSysEventListener();
    void RegisterBootCompletedCallback();
    static sptr<BatteryStatsService> instance_;
    static std::mutex singletonMutex_;
};
} // namespace PowerMgr
} // namespace OHOS
#endif // BATTERY_STATS_SERVICE_H