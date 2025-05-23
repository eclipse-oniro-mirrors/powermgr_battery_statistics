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

#include "stats_service_phone_test.h"
#include "stats_log.h"

#include <call_manager_inner_type.h>
#include <hisysevent.h>

#include "battery_stats_listener.h"
#include "battery_stats_service.h"
#include "hisysevent_operation.h"
#include "stats_hisysevent.h"
#include "stats_service_test_proxy.h"
#include "stats_service_write_event.h"

using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::PowerMgr;
using namespace OHOS::Telephony;
using namespace std;

namespace {
static sptr<BatteryStatsService> g_statsService = nullptr;
static std::shared_ptr<StatsServiceTestProxy> g_statsServiceProxy = nullptr;
} // namespace

void StatsServicePhoneTest::SetUpTestCase()
{
    ParserAveragePowerFile();
    g_statsService = BatteryStatsService::GetInstance();
    g_statsService->OnStart();

    if (g_statsService->listenerPtr_ == nullptr) {
        g_statsService->listenerPtr_ = std::make_shared<BatteryStatsListener>();
    }

    if (g_statsServiceProxy == nullptr) {
        g_statsServiceProxy = std::make_shared<StatsServiceTestProxy>(g_statsService);
    }
}

void StatsServicePhoneTest::TearDownTestCase()
{
    g_statsService->listenerPtr_ = nullptr;
    g_statsService->OnStop();
}

void StatsServicePhoneTest::SetUp()
{
    auto statsService = BatteryStatsService::GetInstance();
    statsService->SetOnBattery(true);
}

void StatsServicePhoneTest::TearDown()
{
    auto statsService = BatteryStatsService::GetInstance();
    statsService->SetOnBattery(false);
}

namespace {
/**
 * @tc.name: StatsServicePhoneTest_001
 * @tc.desc: test Reset function(Phone Call)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePhoneTest, StatsServicePhoneTest_001, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_001 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t stateOn = static_cast<int32_t>(TelCallState::CALL_STATUS_ACTIVE);
    int32_t stateOff = static_cast<int32_t>(TelCallState::CALL_STATUS_DISCONNECTED);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOff);
    int32_t tempError;
    double powerMahBefore;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_PHONE, powerMahBefore, tempError);
    g_statsServiceProxy->ResetIpc();
    double powerMahAfter;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_PHONE, powerMahAfter, tempError);
    GTEST_LOG_(INFO) << __func__ << ": before consumption = " << powerMahBefore << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": after consumption = " << powerMahAfter << " mAh";
    EXPECT_TRUE(powerMahBefore >= StatsUtils::DEFAULT_VALUE && powerMahAfter == StatsUtils::DEFAULT_VALUE);
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_001 end");
}

/**
 * @tc.name: StatsServicePhoneTest_002
 * @tc.desc: test GetPartStatsMah function(Phone Call)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePhoneTest, StatsServicePhoneTest_002, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_002 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t stateOn = static_cast<int32_t>(TelCallState::CALL_STATUS_ACTIVE);
    int32_t stateOff = static_cast<int32_t>(TelCallState::CALL_STATUS_DISCONNECTED);
    int16_t level = 0;
    double phoneOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_RADIO_ON, level);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOff);

    double expectedPower = SERVICE_POWER_CONSUMPTION_DURATION_US * phoneOnAverageMa / US_PER_HOUR;
    int32_t tempError;
    double actualPower;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_PHONE, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";

    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_002 end");
}

/**
 * @tc.name: StatsServicePhoneTest_003
 * @tc.desc: test GetPartStatsPercent function(Phone Call)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePhoneTest, StatsServicePhoneTest_003, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_003 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t stateOn = static_cast<int32_t>(TelCallState::CALL_STATUS_ACTIVE);
    int32_t stateOff = static_cast<int32_t>(TelCallState::CALL_STATUS_DISCONNECTED);
    double fullPercent = 1;
    double zeroPercent = 0;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOff);
    int32_t tempError;
    double actualPercent;
    g_statsServiceProxy->GetPartStatsPercentIpc(BatteryStatsInfo::CONSUMPTION_TYPE_PHONE, actualPercent, tempError);
    GTEST_LOG_(INFO) << __func__ << ": actual percent = " << actualPercent;
    EXPECT_TRUE(actualPercent >= zeroPercent && actualPercent <= fullPercent);
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_003 end");
}

/**
 * @tc.name: StatsServicePhoneTest_004
 * @tc.desc: test GetBatteryStats function(Phone Call)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePhoneTest, StatsServicePhoneTest_004, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_004 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t stateOn = static_cast<int32_t>(TelCallState::CALL_STATUS_ACTIVE);
    int32_t stateOff = static_cast<int32_t>(TelCallState::CALL_STATUS_DISCONNECTED);
    int16_t level = 0;
    double phoneOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_RADIO_ON, level);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOff);

    double expectedPower = SERVICE_POWER_CONSUMPTION_DURATION_US * phoneOnAverageMa / US_PER_HOUR;
    double actualPower = StatsUtils::DEFAULT_VALUE;
    ParcelableBatteryStatsList parcelableEntityList;
    int32_t tempError;
    g_statsServiceProxy->GetBatteryStatsIpc(parcelableEntityList, tempError);
    auto list = parcelableEntityList.statsList_;
    for (auto it : list) {
        if ((*it).GetConsumptionType() == BatteryStatsInfo::CONSUMPTION_TYPE_PHONE) {
            actualPower = (*it).GetPower();
        }
    }
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_004 end");
}

/**
 * @tc.name: StatsServicePhoneTest_005
 * @tc.desc: test CALL_STATE event are sent repeatedly, phone power consumption(Phone Call)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePhoneTest, StatsServicePhoneTest_005, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_005 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t stateOn = static_cast<int32_t>(TelCallState::CALL_STATUS_ACTIVE);
    int32_t stateOff = static_cast<int32_t>(TelCallState::CALL_STATUS_DISCONNECTED);
    int16_t level = 0;
    double phoneOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_RADIO_ON, level);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOff);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOff);

    double expectedPower = 2 * SERVICE_POWER_CONSUMPTION_DURATION_US * phoneOnAverageMa / US_PER_HOUR;
    int32_t tempError;
    double actualPower;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_PHONE, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_005 end");
}

/**
 * @tc.name: StatsServicePhoneTest_006
 * @tc.desc: test GetAppStatsMah function, Phone call on state composite test
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePhoneTest, StatsServicePhoneTest_006, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_006 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t stateOn = static_cast<int32_t>(TelCallState::CALL_STATUS_ACTIVE);
    int32_t stateOff = static_cast<int32_t>(TelCallState::CALL_STATUS_DISCONNECTED);
    int32_t stateHolding = static_cast<int32_t>(TelCallState::CALL_STATUS_HOLDING);
    int32_t stateDisconnecting = static_cast<int32_t>(TelCallState::CALL_STATUS_DISCONNECTING);
    int16_t level = 0;
    double phoneOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_RADIO_ON, level);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateHolding);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateDisconnecting);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOff);

    double expectedPower = 3 * SERVICE_POWER_CONSUMPTION_DURATION_US * phoneOnAverageMa / US_PER_HOUR;
    int32_t tempError;
    double actualPower;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_PHONE, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_006 end");
}

/**
 * @tc.name: StatsServicePhoneTest_007
 * @tc.desc: test test GetAppStatsMah function, Phone call on abnormal state test(Phone call is off)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePhoneTest, StatsServicePhoneTest_007, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_007 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t stateOn = 10;
    int32_t stateOff = static_cast<int32_t>(TelCallState::CALL_STATUS_DISCONNECTED);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOff);

    double expectedPower = StatsUtils::DEFAULT_VALUE;
    int32_t tempError;
    double actualPower;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_PHONE, actualPower, tempError);
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_EQ(expectedPower, actualPower);
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_007 end");
}

/**
 * @tc.name: StatsServicePhoneTest_008
 * @tc.desc: test test GetAppStatsMah function, Phone call on abnormal state test(Phone call is on)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePhoneTest, StatsServicePhoneTest_008, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_008 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t stateOn = static_cast<int32_t>(TelCallState::CALL_STATUS_ACTIVE);
    int32_t stateOff = static_cast<int32_t>(TelCallState::CALL_STATUS_DISCONNECTED);
    int32_t stateInvaildOn = 5;
    int32_t stateInvaildOff = -1;
    int16_t level = 0;
    double phoneOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_RADIO_ON, level);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateInvaildOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateInvaildOff);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOff);

    double expectedPower = 3 * SERVICE_POWER_CONSUMPTION_DURATION_US * phoneOnAverageMa / US_PER_HOUR;
    int32_t tempError;
    double actualPower;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_PHONE, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_008 end");
}

/**
 * @tc.name: StatsServicePhoneTest_009
 * @tc.desc: test Reset function(Phone Data)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePhoneTest, StatsServicePhoneTest_009, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_009 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t stateOn = 1;
    int32_t stateOff = 0;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOff);

    int32_t tempError;
    double powerMahBefore;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_PHONE, powerMahBefore, tempError);
    g_statsServiceProxy->ResetIpc();
    double powerMahAfter;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_PHONE, powerMahAfter, tempError);
    GTEST_LOG_(INFO) << __func__ << ": before consumption = " << powerMahBefore << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": after consumption = " << powerMahAfter << " mAh";
    EXPECT_TRUE(powerMahBefore >= StatsUtils::DEFAULT_VALUE && powerMahAfter == StatsUtils::DEFAULT_VALUE);
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_009 end");
}

/**
 * @tc.name: StatsServicePhoneTest_010
 * @tc.desc: test GetPartStatsMah function(Phone Data)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePhoneTest, StatsServicePhoneTest_010, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_010 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t stateOn = 1;
    int32_t stateOff = 0;
    int16_t level = 0;
    double phoneDataAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_RADIO_DATA, level);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOff);

    double expectedPower = SERVICE_POWER_CONSUMPTION_DURATION_US * phoneDataAverageMa / US_PER_HOUR;
    int32_t tempError;
    double actualPower;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_PHONE, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_010 end");
}

/**
 * @tc.name: StatsServicePhoneTest_011
 * @tc.desc: test GetPartStatsPercent function(Phone Data)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePhoneTest, StatsServicePhoneTest_011, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_011 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t stateOn = 1;
    int32_t stateOff = 0;
    double fullPercent = 1;
    double zeroPercent = 0;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOff);

    int32_t tempError;
    double actualPercent;
    g_statsServiceProxy->GetPartStatsPercentIpc(BatteryStatsInfo::CONSUMPTION_TYPE_PHONE, actualPercent, tempError);
    GTEST_LOG_(INFO) << __func__ << ": actual percent = " << actualPercent;
    EXPECT_TRUE(actualPercent >= zeroPercent && actualPercent <= fullPercent);
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_011 end");
}

/**
 * @tc.name: StatsServicePhoneTest_012
 * @tc.desc: test GetBatteryStats function(Phone Data)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePhoneTest, StatsServicePhoneTest_012, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_012 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t stateOn = 1;
    int32_t stateOff = 0;
    int16_t level = 0;
    double phoneDataAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_RADIO_DATA, level);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOff);

    double expectedPower = SERVICE_POWER_CONSUMPTION_DURATION_US * phoneDataAverageMa / US_PER_HOUR;
    double actualPower = StatsUtils::DEFAULT_VALUE;
    ParcelableBatteryStatsList parcelableEntityList;
    int32_t tempError;
    g_statsServiceProxy->GetBatteryStatsIpc(parcelableEntityList, tempError);
    auto list = parcelableEntityList.statsList_;
    for (auto it : list) {
        if ((*it).GetConsumptionType() == BatteryStatsInfo::CONSUMPTION_TYPE_PHONE) {
            actualPower = (*it).GetPower();
        }
    }
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_012 end");
}

/**
 * @tc.name: StatsServicePhoneTest_013
 * @tc.desc: test CALL_STATE event are sent repeatedly, phone power consumption(Phone Data)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePhoneTest, StatsServicePhoneTest_013, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_013 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t stateOn = 1;
    int32_t stateOff = 0;
    int16_t level = 0;
    double phoneDataAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_RADIO_DATA, level);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOff);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOff);

    double expectedPower = 2 * SERVICE_POWER_CONSUMPTION_DURATION_US * phoneDataAverageMa / US_PER_HOUR;
    int32_t tempError;
    double actualPower;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_PHONE, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_013 end");
}

/**
 * @tc.name: StatsServicePhoneTest_014
 * @tc.desc: test test GetAppStatsMah function, Phone data on abnormal state test(Phone data is off)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePhoneTest, StatsServicePhoneTest_014, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_014 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t stateOn = 5;
    int32_t stateOff = 0;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOff);

    double expectedPower = StatsUtils::DEFAULT_VALUE;
    int32_t tempError;
    double actualPower;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_PHONE, actualPower, tempError);
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_EQ(expectedPower, actualPower);
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_014 end");
}

/**
 * @tc.name: StatsServicePhoneTest_015
 * @tc.desc: test test GetAppStatsMah function, Phone data on abnormal state test(Phone data is on)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePhoneTest, StatsServicePhoneTest_015, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_015 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t stateOn = 1;
    int32_t stateOff = 0;
    int32_t stateInvaildOn = 5;
    int32_t stateInvaildOff = -1;
    int16_t level = 0;
    double phoneDataAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_RADIO_DATA, level);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE,
        HiSysEvent::EventType::BEHAVIOR,
        "STATE", stateInvaildOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE,
        HiSysEvent::EventType::BEHAVIOR,
        "STATE", stateInvaildOff);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOff);

    double expectedPower = 3 * SERVICE_POWER_CONSUMPTION_DURATION_US * phoneDataAverageMa / US_PER_HOUR;
    int32_t tempError;
    double actualPower;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_PHONE, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_015 end");
}

/**
 * @tc.name: StatsServicePhoneTest_016
 * @tc.desc: test CALL_STATE and TELEPHONY event are sent repeatedly, power consumption(Func:GetPartStatsMah)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePhoneTest, StatsServicePhoneTest_016, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_016 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t callStateOn = static_cast<int32_t>(TelCallState::CALL_STATUS_ACTIVE);
    int32_t callStateOff = static_cast<int32_t>(TelCallState::CALL_STATUS_DISCONNECTED);
    int32_t dataStateOn = 1;
    int32_t dataStateOff = 0;
    int16_t level = 0;
    double phoneOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_RADIO_ON, level);
    double phoneDataAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_RADIO_DATA, level);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", callStateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", dataStateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", callStateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", callStateOff);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", dataStateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", dataStateOff);

    double phoneOnPower = 3 * SERVICE_POWER_CONSUMPTION_DURATION_US * phoneOnAverageMa / US_PER_HOUR;
    double phoneDataPower = 4 * SERVICE_POWER_CONSUMPTION_DURATION_US * phoneDataAverageMa / US_PER_HOUR;
    double expectedPower = phoneOnPower + phoneDataPower;
    int32_t tempError;
    double actualPower;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_PHONE, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_016 end");
}

/**
 * @tc.name: StatsServicePhoneTest_017
 * @tc.desc: test CALL_STATE and TELEPHONY event are sent repeatedly, power consumption(Func:GetBatteryStats)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePhoneTest, StatsServicePhoneTest_017, TestSize.Level0)
{
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t callStateOn = static_cast<int32_t>(TelCallState::CALL_STATUS_ACTIVE);
    int32_t callStateOff = static_cast<int32_t>(TelCallState::CALL_STATUS_DISCONNECTED);
    int32_t dataStateOn = 1;
    int32_t dataStateOff = 0;
    int16_t level = 0;
    double phoneOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_RADIO_ON, level);
    double phoneDataAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_RADIO_DATA, level);

    StatsWriteHiSysEvent(statsService, HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", callStateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService, HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", dataStateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService, HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", callStateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService, HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", callStateOff);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService, HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", dataStateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService, HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", dataStateOff);

    double phoneOnPower = 3 * SERVICE_POWER_CONSUMPTION_DURATION_US * phoneOnAverageMa / US_PER_HOUR;
    double phoneDataPower = 4 * SERVICE_POWER_CONSUMPTION_DURATION_US * phoneDataAverageMa / US_PER_HOUR;
    double expectedPower = phoneOnPower + phoneDataPower;
    double actualPower = StatsUtils::DEFAULT_VALUE;
    ParcelableBatteryStatsList parcelableEntityList;
    int32_t tempError;
    g_statsServiceProxy->GetBatteryStatsIpc(parcelableEntityList, tempError);
    auto list = parcelableEntityList.statsList_;
    for (auto it : list) {
        if ((*it).GetConsumptionType() == BatteryStatsInfo::CONSUMPTION_TYPE_PHONE) {
            actualPower = (*it).GetPower();
        }
    }
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
}

/**
 * @tc.name: StatsServicePhoneTest_018
 * @tc.desc: test SetOnBattery function(Phone Call & Phone Data)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePhoneTest, StatsServicePhoneTest_018, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_018 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();
    g_statsServiceProxy->SetOnBatteryIpc(false);

    int32_t callStateOn = static_cast<int32_t>(TelCallState::CALL_STATUS_ACTIVE);
    int32_t callStateOff = static_cast<int32_t>(TelCallState::CALL_STATUS_DISCONNECTED);
    int32_t dataStateOn = 1;
    int32_t dataStateOff = 0;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", callStateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", callStateOff);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE, HiSysEvent::EventType::BEHAVIOR,
        "STATE", dataStateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE, HiSysEvent::EventType::BEHAVIOR,
        "STATE", dataStateOff);

    double expectedPower = StatsUtils::DEFAULT_VALUE;
    int32_t tempError;
    double actualPower;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_PHONE, actualPower, tempError);
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_EQ(expectedPower, actualPower);
    g_statsServiceProxy->SetOnBatteryIpc(true);
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_018 end");
}

/**
 * @tc.name: StatsServicePhoneTest_019
 * @tc.desc: test GetPartStatsMah function with battery changed(Phone call on)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePhoneTest, StatsServicePhoneTest_019, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_019 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t stateOn = static_cast<int32_t>(TelCallState::CALL_STATUS_ACTIVE);
    int32_t stateOff = static_cast<int32_t>(TelCallState::CALL_STATUS_DISCONNECTED);
    int16_t level = 0;
    double phoneOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_RADIO_ON, level);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    g_statsServiceProxy->SetOnBatteryIpc(false);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    g_statsServiceProxy->SetOnBatteryIpc(true);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOff);

    double expectedPower = 2 * SERVICE_POWER_CONSUMPTION_DURATION_US * phoneOnAverageMa / US_PER_HOUR;
    int32_t tempError;
    double actualPower;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_PHONE, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_019 end");
}

/**
 * @tc.name: StatsServicePhoneTest_020
 * @tc.desc: test GetPartStatsMah(Phone Data) and GetAppStatsMah(Audio) function
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePhoneTest, StatsServicePhoneTest_020, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_020 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t stateOn = 1;
    int32_t stateOff = 0;
    int16_t level = 0;
    double phoneDataAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_RADIO_DATA, level);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOff);

    double expectedPower = SERVICE_POWER_CONSUMPTION_DURATION_US * phoneDataAverageMa / US_PER_HOUR;
    int32_t tempError;
    double actualPower;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_PHONE, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);

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

    expectedPower = SERVICE_POWER_CONSUMPTION_DURATION_US * audioOnAverageMa / US_PER_HOUR;
    g_statsServiceProxy->GetPartStatsMahIpc(uid, actualPower, tempError);
    devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_020 end");
}

/**
 * @tc.name: StatsServicePhoneTest_021
 * @tc.desc: test GetTotalTimeSecond function(Phone Call)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePhoneTest, StatsServicePhoneTest_021, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_021 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t stateOn = static_cast<int32_t>(TelCallState::CALL_STATUS_ACTIVE);
    int32_t stateOff = static_cast<int32_t>(TelCallState::CALL_STATUS_DISCONNECTED);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOff);

    uint64_t expectedTime = round(SERVICE_POWER_CONSUMPTION_DURATION_US / US_PER_SECOND);
    uint64_t actualTime;
    g_statsServiceProxy->GetTotalTimeSecondIpc(StatsUtils::STATS_TYPE_PHONE_ACTIVE, -1, actualTime);
    GTEST_LOG_(INFO) << __func__ << ": expected time = " << expectedTime << " seconds";
    GTEST_LOG_(INFO) << __func__ << ": actual time = " <<  actualTime << " seconds";
    EXPECT_EQ(expectedTime, actualTime);
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_021 end");
}

/**
 * @tc.name: StatsServicePhoneTest_022
 * @tc.desc: test GetTotalTimeSecond function(Phone Data)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePhoneTest, StatsServicePhoneTest_022, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_022 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t stateOn = 1;
    int32_t stateOff = 0;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOff);

    uint64_t expectedTime = round(SERVICE_POWER_CONSUMPTION_DURATION_US / US_PER_SECOND);
    uint64_t actualTime;
    g_statsServiceProxy->GetTotalTimeSecondIpc(StatsUtils::STATS_TYPE_PHONE_DATA, -1, actualTime);
    GTEST_LOG_(INFO) << __func__ << ": expected time = " << expectedTime << " seconds";
    GTEST_LOG_(INFO) << __func__ << ": actual time = " <<  actualTime << " seconds";
    EXPECT_EQ(expectedTime, actualTime);
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_022 end");
}

/**
 * @tc.name: StatsServicePhoneTest_023
 * @tc.desc: test send hisysevent with missing information(Phone Call & Phone Data)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePhoneTest, StatsServicePhoneTest_023, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_023 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE, HiSysEvent::EventType::BEHAVIOR);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE, HiSysEvent::EventType::BEHAVIOR);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE, HiSysEvent::EventType::BEHAVIOR);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE, HiSysEvent::EventType::BEHAVIOR);

    double expectedPower = StatsUtils::DEFAULT_VALUE;
    int32_t tempError;
    double actualPower;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_PHONE, actualPower, tempError);
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_EQ(expectedPower, actualPower);
    STATS_HILOGI(LABEL_TEST, "StatsServicePhoneTest_023 end");
}
}