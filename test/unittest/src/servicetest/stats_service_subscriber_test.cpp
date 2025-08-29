/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#include "stats_service_subscriber_test.h"
#include "stats_log.h"
#include <call_manager_inner_type.h>
#include <hisysevent.h>

#include "battery_stats_listener.h"
#include "battery_stats_subscriber.h"
#include "common_event_data.h"
#include "common_event_manager.h"
#include "common_event_support.h"
#include "hisysevent_operation.h"
#include "stats_hisysevent.h"
#include "stats_service_test_proxy.h"
#include "stats_service_write_event.h"

using namespace OHOS;
using namespace OHOS::AAFwk;
using namespace OHOS::EventFwk;
using namespace OHOS::HiviewDFX;
using namespace OHOS::PowerMgr;
using namespace OHOS::Telephony;
using namespace std;
using namespace testing::ext;

namespace {
static sptr<BatteryStatsService> g_statsService = nullptr;
static std::shared_ptr<StatsServiceTestProxy> g_statsServiceProxy = nullptr;
const int32_t BATTERY_LEVEL_FULL = 100;
} // namespace

void StatsServiceSubscriberTest::SetUpTestCase()
{
    ParserAveragePowerFile();
    g_statsService = BatteryStatsService::GetInstance();
    g_statsService->OnStart();

    if (g_statsService->listenerPtr_ == nullptr) {
        g_statsService->listenerPtr_ = std::make_shared<BatteryStatsListener>();
    }

    if (g_statsService->subscriberPtr_ == nullptr) {
        OHOS::EventFwk::MatchingSkills matchingSkills {};
        OHOS::EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);
        g_statsService->subscriberPtr_ = std::make_shared<BatteryStatsSubscriber>(subscribeInfo);
    }

    if (g_statsServiceProxy == nullptr) {
        g_statsServiceProxy = std::make_shared<StatsServiceTestProxy>(g_statsService);
    }
}

void StatsServiceSubscriberTest::TearDownTestCase()
{
    g_statsService->listenerPtr_ = nullptr;
    g_statsService->subscriberPtr_ = nullptr;
    g_statsService->OnStop();
}

void StatsServiceSubscriberTest::SetUp()
{
    auto statsService = BatteryStatsService::GetInstance();
    statsService->SetOnBattery(true);
}

void StatsServiceSubscriberTest::TearDown()
{
    auto statsService = BatteryStatsService::GetInstance();
    statsService->SetOnBattery(false);
}

void StatsServiceSubscriberTest::PublishChangedEvent(const sptr<BatteryStatsService>& service,
    const std::string& action)
{
    Want want;
    want.SetParam(BatteryInfo::COMMON_EVENT_KEY_CAPACITY, capacity_);
    want.SetParam(BatteryInfo::COMMON_EVENT_KEY_PLUGGED_TYPE, static_cast<int>(pluggedType_));
    want.SetAction(action);
    CommonEventData data;
    data.SetWant(want);
    if (service->subscriberPtr_ != nullptr) {
        g_statsService->subscriberPtr_->OnReceiveEvent(data);
    }
}

namespace {
/**
 * @tc.name: StatsServiceSubscriberTest_001
 * @tc.desc: test COMMON_EVENT_BATTERY_CHANGED with capacity is BATTERY_LEVEL_FULL(Alarm)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceSubscriberTest, StatsServiceSubscriberTest_001, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceSubscriberTest_001 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();
    BatteryInfoReset();

    int32_t uid = 10003;
    int32_t pid = 3458;
    int16_t count = 10;

    for (int16_t i = 0; i < count; i++) {
        StatsWriteHiSysEvent(statsService,
            HiSysEvent::Domain::TIME, StatsHiSysEvent::MISC_TIME_STATISTIC_REPORT, HiSysEvent::EventType::STATISTIC,
            "CALLER_PID", pid, "CALLER_UID", uid);
    }
    int32_t tempError;
    double powerMahBefore;
    g_statsServiceProxy->GetAppStatsMahIpc(uid, powerMahBefore, tempError);
    SetCapacity(BATTERY_LEVEL_FULL);
    PublishChangedEvent(statsService, CommonEventSupport::COMMON_EVENT_BATTERY_CHANGED);
    double powerMahAfter;
    g_statsServiceProxy->GetAppStatsMahIpc(uid, powerMahAfter, tempError);
    GTEST_LOG_(INFO) << __func__ << ": before consumption = " << powerMahBefore << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": after consumption = " << powerMahAfter << " mAh";
    EXPECT_TRUE(powerMahBefore >= StatsUtils::DEFAULT_VALUE && powerMahAfter == StatsUtils::DEFAULT_VALUE);
    STATS_HILOGI(LABEL_TEST, "StatsServiceSubscriberTest_001 end");
}

/**
 * @tc.name: StatsServiceSubscriberTest_002
 * @tc.desc: test COMMON_EVENT_BATTERY_CHANGED with BatteryPluggedType(Alarm)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceSubscriberTest, StatsServiceSubscriberTest_002, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceSubscriberTest_002 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();
    g_statsServiceProxy->SetOnBatteryIpc(false);
    BatteryInfoReset();

    double alarmOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_ALARM_ON);
    int32_t uid = 10003;
    int32_t pid = 3458;

    SetPluggedType(BatteryPluggedType::PLUGGED_TYPE_NONE);
    PublishChangedEvent(statsService, CommonEventSupport::COMMON_EVENT_BATTERY_CHANGED);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TIME, StatsHiSysEvent::MISC_TIME_STATISTIC_REPORT, HiSysEvent::EventType::STATISTIC,
        "CALLER_PID", pid, "CALLER_UID", uid);

    SetPluggedType(BatteryPluggedType::PLUGGED_TYPE_USB);
    PublishChangedEvent(statsService, CommonEventSupport::COMMON_EVENT_BATTERY_CHANGED);
    StatsWriteHiSysEvent(statsService,
            HiSysEvent::Domain::TIME, StatsHiSysEvent::MISC_TIME_STATISTIC_REPORT, HiSysEvent::EventType::STATISTIC,
            "CALLER_PID", pid, "CALLER_UID", uid);

    SetPluggedType(BatteryPluggedType::PLUGGED_TYPE_BUTT);
    PublishChangedEvent(statsService, CommonEventSupport::COMMON_EVENT_BATTERY_CHANGED);
    StatsWriteHiSysEvent(statsService,
            HiSysEvent::Domain::TIME, StatsHiSysEvent::MISC_TIME_STATISTIC_REPORT, HiSysEvent::EventType::STATISTIC,
            "CALLER_PID", pid, "CALLER_UID", uid);
    int32_t tempError;
    double expectedPower = 2 * alarmOnAverageMa;
    double actualPower;
    g_statsServiceProxy->GetAppStatsMahIpc(uid, actualPower, tempError);
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_EQ(expectedPower, actualPower);
    g_statsServiceProxy->SetOnBatteryIpc(true);
    STATS_HILOGI(LABEL_TEST, "StatsServiceSubscriberTest_002 end");
}

/**
 * @tc.name: StatsServiceSubscriberTest_003
 * @tc.desc: test COMMON_EVENT_BATTERY_CHANGED with capacity is BATTERY_LEVEL_FULL(Audio)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceSubscriberTest, StatsServiceSubscriberTest_003, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceSubscriberTest_003 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();
    BatteryInfoReset();

    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateRunning = 2;
    int32_t stateStopped = 3;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::AUDIO, StatsHiSysEvent::STREAM_CHANGE, HiSysEvent::EventType::BEHAVIOR, "PID", pid,
        "UID", uid, "STATE", stateRunning);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::AUDIO, StatsHiSysEvent::STREAM_CHANGE, HiSysEvent::EventType::BEHAVIOR, "PID", pid,
        "UID", uid, "STATE", stateStopped);
    int32_t tempError;
    double powerMahBefore;
    g_statsServiceProxy->GetAppStatsMahIpc(uid, powerMahBefore, tempError);
    SetCapacity(BATTERY_LEVEL_FULL);
    PublishChangedEvent(statsService, CommonEventSupport::COMMON_EVENT_BATTERY_CHANGED);
    double powerMahAfter;
    g_statsServiceProxy->GetAppStatsMahIpc(uid, powerMahAfter, tempError);
    GTEST_LOG_(INFO) << __func__ << ": before consumption = " << powerMahBefore << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": after consumption = " << powerMahAfter << " mAh";
    EXPECT_TRUE(powerMahBefore >= StatsUtils::DEFAULT_VALUE && powerMahAfter == StatsUtils::DEFAULT_VALUE);
    STATS_HILOGI(LABEL_TEST, "StatsServiceSubscriberTest_003 end");
}

/**
 * @tc.name: StatsServiceSubscriberTest_004
 * @tc.desc: test COMMON_EVENT_BATTERY_CHANGED with BatteryPluggedType(Audio)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceSubscriberTest, StatsServiceSubscriberTest_004, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceSubscriberTest_004 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();
    g_statsServiceProxy->SetOnBatteryIpc(false);
    BatteryInfoReset();

    double audioOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_AUDIO_ON);
    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateRunning = 2;
    int32_t stateStopped = 3;

    SetPluggedType(BatteryPluggedType::PLUGGED_TYPE_NONE);
    PublishChangedEvent(statsService, CommonEventSupport::COMMON_EVENT_BATTERY_CHANGED);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::AUDIO, StatsHiSysEvent::STREAM_CHANGE, HiSysEvent::EventType::BEHAVIOR, "PID", pid,
        "UID", uid, "STATE", stateRunning);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::AUDIO, StatsHiSysEvent::STREAM_CHANGE, HiSysEvent::EventType::BEHAVIOR, "PID", pid,
        "UID", uid, "STATE", stateStopped);

    SetPluggedType(BatteryPluggedType::PLUGGED_TYPE_AC);
    PublishChangedEvent(statsService, CommonEventSupport::COMMON_EVENT_BATTERY_CHANGED);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::AUDIO, StatsHiSysEvent::STREAM_CHANGE, HiSysEvent::EventType::BEHAVIOR, "PID", pid,
        "UID", uid, "STATE", stateRunning);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::AUDIO, StatsHiSysEvent::STREAM_CHANGE, HiSysEvent::EventType::BEHAVIOR, "PID", pid,
        "UID", uid, "STATE", stateStopped);

    SetPluggedType(BatteryPluggedType::PLUGGED_TYPE_BUTT);
    PublishChangedEvent(statsService, CommonEventSupport::COMMON_EVENT_BATTERY_CHANGED);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::AUDIO, StatsHiSysEvent::STREAM_CHANGE, HiSysEvent::EventType::BEHAVIOR, "PID", pid,
        "UID", uid, "STATE", stateRunning);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::AUDIO, StatsHiSysEvent::STREAM_CHANGE, HiSysEvent::EventType::BEHAVIOR, "PID", pid,
        "UID", uid, "STATE", stateStopped);

    double expectedPower = 2 * SERVICE_POWER_CONSUMPTION_DURATION_US * audioOnAverageMa / US_PER_HOUR;
    int32_t tempError;
    double actualPower;
    g_statsServiceProxy->GetAppStatsMahIpc(uid, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServiceSubscriberTest_004 end");
}

/**
 * @tc.name: StatsServiceSubscriberTest_005
 * @tc.desc: test COMMON_EVENT_BATTERY_LOW is no use(Audio)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceSubscriberTest, StatsServiceSubscriberTest_005, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceSubscriberTest_005 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();
    BatteryInfoReset();

    double audioOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_AUDIO_ON);
    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateRunning = 2;
    int32_t stateStopped = 3;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::AUDIO, StatsHiSysEvent::STREAM_CHANGE, HiSysEvent::EventType::BEHAVIOR, "PID", pid,
        "UID", uid, "STATE", stateRunning);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::AUDIO, StatsHiSysEvent::STREAM_CHANGE, HiSysEvent::EventType::BEHAVIOR, "PID", pid,
        "UID", uid, "STATE", stateStopped);

    SetPluggedType(BatteryPluggedType::PLUGGED_TYPE_AC);
    PublishChangedEvent(statsService, CommonEventSupport::COMMON_EVENT_BATTERY_LOW);
    double expectedPower = SERVICE_POWER_CONSUMPTION_DURATION_US * audioOnAverageMa / US_PER_HOUR;
    int32_t tempError;
    double actualPower;
    g_statsServiceProxy->GetAppStatsMahIpc(uid, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServiceSubscriberTest_005 end");
}

/**
 * @tc.name: StatsServiceSubscriberTest_006
 * @tc.desc: test COMMON_EVENT_SHUTDOWN(Phone)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceSubscriberTest, StatsServiceSubscriberTest_006, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceSubscriberTest_006 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t stateOn = static_cast<int32_t>(TelCallState::CALL_STATUS_ACTIVE);
    int32_t stateOff = static_cast<int32_t>(TelCallState::CALL_STATUS_DISCONNECTED);
    int16_t level = 0;
    double phoneOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_RADIO_ON, level);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE, HiSysEvent::EventType::BEHAVIOR, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE, HiSysEvent::EventType::BEHAVIOR, "STATE", stateOff);

    PublishChangedEvent(statsService, CommonEventSupport::COMMON_EVENT_SHUTDOWN);
    g_statsServiceProxy->ResetIpc();

    auto statsCore = statsService->GetBatteryStatsCore();
    statsCore->Init();
    BatteryStatsInfoList statsInfoList {};
    statsInfoList = statsCore->GetBatteryStats();

    double expectedPower = SERVICE_POWER_CONSUMPTION_DURATION_US * phoneOnAverageMa / US_PER_HOUR;
    double actualPower = StatsUtils::DEFAULT_VALUE;
    for (auto it : statsInfoList) {
        if ((*it).GetConsumptionType() == BatteryStatsInfo::CONSUMPTION_TYPE_PHONE) {
            actualPower = (*it).GetPower();
        }
    }

    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServiceSubscriberTest_006 end");
}
}