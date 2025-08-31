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

#include "stats_service_powermgr_test.h"
#include "stats_log.h"
#include <call_manager_inner_type.h>
#include <hisysevent.h>
#include <running_lock_info.h>

#include "battery_stats_listener.h"
#include "battery_stats_service.h"
#include "hisysevent_operation.h"
#include "stats_hisysevent.h"
#include "stats_service_test_proxy.h"
#include "stats_service_write_event.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::PowerMgr;
using namespace OHOS::Telephony;
using namespace OHOS;
using namespace std;

namespace {
static sptr<BatteryStatsService> g_statsService = nullptr;
static std::shared_ptr<StatsServiceTestProxy> g_statsServiceProxy = nullptr;
} // namespace

void StatsServicePowerMgrTest::SetUpTestCase()
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

void StatsServicePowerMgrTest::TearDownTestCase()
{
    g_statsService->listenerPtr_ = nullptr;
    g_statsService->OnStop();
}

void StatsServicePowerMgrTest::SetUp()
{
    auto statsService = BatteryStatsService::GetInstance();
    statsService->SetOnBattery(true);
}

void StatsServicePowerMgrTest::TearDown()
{
    auto statsService = BatteryStatsService::GetInstance();
    statsService->SetOnBattery(false);
}

namespace {
/**
 * @tc.name: StatsServicePowerMgrTest_001
 * @tc.desc: test GetTotalTimeSecond function(Sensor Gravity)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePowerMgrTest, StatsServicePowerMgrTest_001, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePowerMgrTest_001 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t stateOn = 1;
    int32_t stateOff = 0;
    int32_t uid = 10003;
    int32_t pid = 3458;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::STATS, StatsHiSysEvent::POWER_SENSOR_GRAVITY,
        HiSysEvent::EventType::STATISTIC, "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::STATS, StatsHiSysEvent::POWER_SENSOR_GRAVITY,
        HiSysEvent::EventType::STATISTIC, "PID", pid, "UID", uid, "STATE", stateOff);

    uint64_t expectedTime = round(SERVICE_POWER_CONSUMPTION_DURATION_US / US_PER_SECOND);
    uint64_t actualTime;
    g_statsServiceProxy->GetTotalTimeSecondIpc(StatsUtils::STATS_TYPE_SENSOR_GRAVITY_ON, uid, actualTime);
    GTEST_LOG_(INFO) << __func__ << ": expected time = " << expectedTime << " seconds";
    GTEST_LOG_(INFO) << __func__ << ": actual time = " <<  actualTime << " seconds";
    EXPECT_EQ(expectedTime, actualTime);
    STATS_HILOGI(LABEL_TEST, "StatsServicePowerMgrTest_001 end");
}

/**
 * @tc.name: StatsServicePowerMgrTest_002
 * @tc.desc: test GetAppStatsMah function(Sensor Gravity)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePowerMgrTest, StatsServicePowerMgrTest_002, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePowerMgrTest_002 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    double sensorGravityOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_SENSOR_GRAVITY);
    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = 1;
    int32_t stateOff = 0;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::STATS, StatsHiSysEvent::POWER_SENSOR_GRAVITY, HiSysEvent::EventType::STATISTIC, "PID",
        pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::STATS, StatsHiSysEvent::POWER_SENSOR_GRAVITY, HiSysEvent::EventType::STATISTIC, "PID",
        pid, "UID", uid, "STATE", stateOff);

    double expectedPower = SERVICE_POWER_CONSUMPTION_DURATION_US * sensorGravityOnAverageMa / US_PER_HOUR;
    int32_t tempError;
    double actualPower;
    g_statsServiceProxy->GetAppStatsMahIpc(uid, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServicePowerMgrTest_002 end");
}

/**
 * @tc.name: StatsServicePowerMgrTest_003
 * @tc.desc: test GetAppStatsPercent function(Sensor Gravity)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePowerMgrTest, StatsServicePowerMgrTest_003, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePowerMgrTest_003 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = 1;
    int32_t stateOff = 0;
    double fullPercent = 1;
    double zeroPercent = 0;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::STATS, StatsHiSysEvent::POWER_SENSOR_GRAVITY, HiSysEvent::EventType::STATISTIC, "PID",
        pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::STATS, StatsHiSysEvent::POWER_SENSOR_GRAVITY, HiSysEvent::EventType::STATISTIC, "PID",
        pid, "UID", uid, "STATE", stateOff);
    int32_t tempError;
    double actualPercent;
    g_statsServiceProxy->GetAppStatsPercentIpc(uid, actualPercent, tempError);
    GTEST_LOG_(INFO) << __func__ << ": actual percent = " << actualPercent;
    EXPECT_TRUE(actualPercent >= zeroPercent && actualPercent <= fullPercent);
    STATS_HILOGI(LABEL_TEST, "StatsServicePowerMgrTest_003 end");
}

/**
 * @tc.name: StatsServicePowerMgrTest_004
 * @tc.desc: test GetAppStatsMah(Sensor Gravity) and GetAppStatsPercent(Sensor Proximity) function
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePowerMgrTest, StatsServicePowerMgrTest_004, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePowerMgrTest_004 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    double sensorGravityOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_SENSOR_GRAVITY);
    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = 1;
    int32_t stateOff = 0;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::STATS, StatsHiSysEvent::POWER_SENSOR_GRAVITY, HiSysEvent::EventType::STATISTIC, "PID",
        pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::STATS, StatsHiSysEvent::POWER_SENSOR_GRAVITY, HiSysEvent::EventType::STATISTIC, "PID",
        pid, "UID", uid, "STATE", stateOff);

    double expectedPower = SERVICE_POWER_CONSUMPTION_DURATION_US * sensorGravityOnAverageMa / US_PER_HOUR;
    int32_t tempError;
    double actualPower;
    g_statsServiceProxy->GetAppStatsMahIpc(uid, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);

    uid = 10004;
    pid = 3459;
    stateOn = 1;
    stateOff = 0;
    double fullPercent = 1;
    double zeroPercent = 0;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::STATS, StatsHiSysEvent::POWER_SENSOR_PROXIMITY, HiSysEvent::EventType::STATISTIC, "PID",
        pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::STATS, StatsHiSysEvent::POWER_SENSOR_PROXIMITY, HiSysEvent::EventType::STATISTIC, "PID",
        pid, "UID", uid, "STATE", stateOff);
    double actualPercent;
    g_statsServiceProxy->GetAppStatsPercentIpc(uid, actualPercent, tempError);
    GTEST_LOG_(INFO) << __func__ << ": actual percent = " << actualPercent;
    EXPECT_TRUE(actualPercent >= zeroPercent && actualPercent <= fullPercent);
    STATS_HILOGI(LABEL_TEST, "StatsServicePowerMgrTest_004 end");
}

/**
 * @tc.name: StatsServicePowerMgrTest_005
 * @tc.desc: test GetTotalTimeSecond function(Sensor Proximity)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePowerMgrTest, StatsServicePowerMgrTest_005, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePowerMgrTest_005 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t stateOn = 1;
    int32_t stateOff = 0;
    int32_t uid = 10003;
    int32_t pid = 3458;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::STATS, StatsHiSysEvent::POWER_SENSOR_PROXIMITY, HiSysEvent::EventType::STATISTIC, "PID",
        pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::STATS, StatsHiSysEvent::POWER_SENSOR_PROXIMITY, HiSysEvent::EventType::STATISTIC, "PID",
        pid, "UID", uid, "STATE", stateOff);

    uint64_t expectedPower = StatsUtils::DEFAULT_VALUE;
    uint64_t actualPower;
    g_statsServiceProxy->GetTotalTimeSecondIpc(StatsUtils::STATS_TYPE_SENSOR_PROXIMITY_ON, uid, actualPower);
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_EQ(expectedPower, actualPower);
    STATS_HILOGI(LABEL_TEST, "StatsServicePowerMgrTest_005 end");
}

/**
 * @tc.name: StatsServicePowerMgrTest_006
 * @tc.desc: test GetAppStatsMah function(Sensor Proximity)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePowerMgrTest, StatsServicePowerMgrTest_006, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePowerMgrTest_006 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    double sensorProximityOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_SENSOR_PROXIMITY);
    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = 1;
    int32_t stateOff = 0;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::STATS, StatsHiSysEvent::POWER_SENSOR_PROXIMITY, HiSysEvent::EventType::STATISTIC, "PID",
        pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::STATS, StatsHiSysEvent::POWER_SENSOR_PROXIMITY, HiSysEvent::EventType::STATISTIC, "PID",
        pid, "UID", uid, "STATE", stateOff);

    double expectedPower = SERVICE_POWER_CONSUMPTION_DURATION_US * sensorProximityOnAverageMa / US_PER_HOUR;
    int32_t tempError;
    double actualPower;
    g_statsServiceProxy->GetAppStatsMahIpc(uid, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServicePowerMgrTest_006 end");
}

/**
 * @tc.name: StatsServicePowerMgrTest_007
 * @tc.desc: test GetAppStatsPercent function(Sensor Proximity)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePowerMgrTest, StatsServicePowerMgrTest_007, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePowerMgrTest_007 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = 1;
    int32_t stateOff = 0;
    double fullPercent = 1;
    double zeroPercent = 0;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::STATS, StatsHiSysEvent::POWER_SENSOR_PROXIMITY, HiSysEvent::EventType::STATISTIC, "PID",
        pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::STATS, StatsHiSysEvent::POWER_SENSOR_PROXIMITY, HiSysEvent::EventType::STATISTIC, "PID",
        pid, "UID", uid, "STATE", stateOff);
    int32_t tempError;
    double actualPercent;
    g_statsServiceProxy->GetAppStatsPercentIpc(uid, actualPercent, tempError);
    GTEST_LOG_(INFO) << __func__ << ": actual percent = " << actualPercent;
    EXPECT_TRUE(actualPercent >= zeroPercent && actualPercent <= fullPercent);
    STATS_HILOGI(LABEL_TEST, "StatsServicePowerMgrTest_007 end");
}

/**
 * @tc.name: StatsServicePowerMgrTest_008
 * @tc.desc: test GetAppStatsMah(Sensor Proximity) and GetAppStatsPercent(Torch) function
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePowerMgrTest, StatsServicePowerMgrTest_008, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePowerMgrTest_008 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    double sensorProximityOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_SENSOR_PROXIMITY);
    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = 1;
    int32_t stateOff = 0;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::STATS, StatsHiSysEvent::POWER_SENSOR_PROXIMITY, HiSysEvent::EventType::STATISTIC, "PID",
        pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::STATS, StatsHiSysEvent::POWER_SENSOR_PROXIMITY, HiSysEvent::EventType::STATISTIC, "PID",
        pid, "UID", uid, "STATE", stateOff);

    double expectedPower = SERVICE_POWER_CONSUMPTION_DURATION_US * sensorProximityOnAverageMa / US_PER_HOUR;
    int32_t tempError;
    double actualPower;
    g_statsServiceProxy->GetAppStatsMahIpc(uid, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);

    uid = 10004;
    pid = 3459;
    stateOn = 1;
    stateOff = 0;
    double fullPercent = 1;
    double zeroPercent = 0;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::CAMERA, StatsHiSysEvent::TORCH_STATE, HiSysEvent::EventType::STATISTIC, "PID", pid,
        "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::CAMERA, StatsHiSysEvent::TORCH_STATE, HiSysEvent::EventType::STATISTIC, "PID", pid,
        "UID", uid, "STATE", stateOff);
    double actualPercent;
    g_statsServiceProxy->GetAppStatsPercentIpc(uid, actualPercent, tempError);
    GTEST_LOG_(INFO) << __func__ << ": actual percent = " << actualPercent;
    EXPECT_TRUE(actualPercent >= zeroPercent && actualPercent <= fullPercent);
    STATS_HILOGI(LABEL_TEST, "StatsServicePowerMgrTest_008 end");
}

/**
 * @tc.name: StatsServicePowerMgrTest_009
 * @tc.desc: test sensor entity GetPartStatsMah function(Sensor)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePowerMgrTest, StatsServicePowerMgrTest_009, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePowerMgrTest_009 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    double sensorGravityOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_SENSOR_GRAVITY);
    double sensorProximityOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_SENSOR_PROXIMITY);
    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = 1;
    int32_t stateOff = 0;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::STATS, StatsHiSysEvent::POWER_SENSOR_GRAVITY, HiSysEvent::EventType::STATISTIC, "PID",
        pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::STATS, StatsHiSysEvent::POWER_SENSOR_GRAVITY, HiSysEvent::EventType::STATISTIC, "PID",
        pid, "UID", uid, "STATE", stateOff);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::STATS, StatsHiSysEvent::POWER_SENSOR_PROXIMITY, HiSysEvent::EventType::STATISTIC, "PID",
        pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::STATS, StatsHiSysEvent::POWER_SENSOR_PROXIMITY, HiSysEvent::EventType::STATISTIC, "PID",
        pid, "UID", uid, "STATE", stateOff);

    auto statsCore = statsService->GetBatteryStatsCore();
    auto sensorEntity = statsCore->GetEntity(BatteryStatsInfo::CONSUMPTION_TYPE_SENSOR);
    statsCore->ComputePower();

    double expectedPower = SERVICE_POWER_CONSUMPTION_DURATION_US *
        (sensorGravityOnAverageMa + sensorProximityOnAverageMa)/ US_PER_HOUR;
    double actualPower = sensorEntity->GetStatsPowerMah(StatsUtils::STATS_TYPE_SENSOR_GRAVITY_ON, uid) +
        sensorEntity->GetStatsPowerMah(StatsUtils::STATS_TYPE_SENSOR_PROXIMITY_ON, uid);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);

    EXPECT_EQ(StatsUtils::DEFAULT_VALUE, sensorEntity->GetStatsPowerMah(StatsUtils::STATS_TYPE_SENSOR_GRAVITY_ON));
    EXPECT_EQ(StatsUtils::DEFAULT_VALUE, sensorEntity->GetStatsPowerMah(StatsUtils::STATS_TYPE_SENSOR_PROXIMITY_ON));
    EXPECT_EQ(StatsUtils::DEFAULT_VALUE, sensorEntity->GetStatsPowerMah(StatsUtils::STATS_TYPE_INVALID, uid));
    STATS_HILOGI(LABEL_TEST, "StatsServicePowerMgrTest_009 end");
}

/**
 *
 * @tc.name: StatsServicePowerMgrTest_010
 * @tc.desc: test wakelock entity GetPartStatsMah function(Wakelock)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePowerMgrTest,  StatsServicePowerMgrTest_010, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePowerMgrTest_010 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    double wakelockAverage = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_CPU_AWAKE);
    int32_t uid = 10001;
    int32_t pid = 3456;
    int32_t stateLock = 1;
    int32_t stateUnlock = 0;
    int32_t type = static_cast<int32_t>(RunningLockType::RUNNINGLOCK_SCREEN);
    std::string name = " StatsServicePowerMgrTest_010";

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::POWER, StatsHiSysEvent::POWER_RUNNINGLOCK, HiSysEvent::EventType::STATISTIC, "PID", pid,
        "UID", uid, "STATE", stateLock, "TYPE", type, "NAME", name);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::POWER, StatsHiSysEvent::POWER_RUNNINGLOCK, HiSysEvent::EventType::STATISTIC, "PID", pid,
        "UID", uid, "STATE", stateUnlock, "TYPE", type, "NAME", name);

    auto statsCore = statsService->GetBatteryStatsCore();
    auto wakelockEntity = statsCore->GetEntity(BatteryStatsInfo::CONSUMPTION_TYPE_WAKELOCK);
    statsCore->ComputePower();

    double expectedPower = wakelockAverage * SERVICE_POWER_CONSUMPTION_DURATION_US / US_PER_HOUR;
    double actualPower = wakelockEntity->GetStatsPowerMah(StatsUtils::STATS_TYPE_WAKELOCK_HOLD, uid);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);

    EXPECT_EQ(StatsUtils::DEFAULT_VALUE, wakelockEntity->GetStatsPowerMah(StatsUtils::STATS_TYPE_WAKELOCK_HOLD));
    EXPECT_EQ(StatsUtils::DEFAULT_VALUE, wakelockEntity->GetStatsPowerMah(StatsUtils::STATS_TYPE_INVALID, uid));
    STATS_HILOGI(LABEL_TEST, "StatsServicePowerMgrTest_010 end");
}

/**
 * @tc.name: StatsServicePowerMgrTest_011
 * @tc.desc: test send hisysevent with missing information(Sensor)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePowerMgrTest, StatsServicePowerMgrTest_011, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePowerMgrTest_011 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t uid = 10003;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::STATS, StatsHiSysEvent::POWER_SENSOR_GRAVITY, HiSysEvent::EventType::STATISTIC);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::STATS, StatsHiSysEvent::POWER_SENSOR_GRAVITY, HiSysEvent::EventType::STATISTIC);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::STATS, StatsHiSysEvent::POWER_SENSOR_PROXIMITY, HiSysEvent::EventType::STATISTIC);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::STATS, StatsHiSysEvent::POWER_SENSOR_PROXIMITY, HiSysEvent::EventType::STATISTIC);
    int32_t tempError;
    double expectedPower = StatsUtils::DEFAULT_VALUE;
    double actualPower;
    g_statsServiceProxy->GetAppStatsMahIpc(uid, actualPower, tempError);
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_EQ(expectedPower, actualPower);
    STATS_HILOGI(LABEL_TEST, "StatsServicePowerMgrTest_011 end");
}

/**
 * @tc.name: StatsServicePowerMgrTest_012
 * @tc.desc: test batteryStatics Start
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServicePowerMgrTest, StatsServicePowerMgrTest_012, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServicePowerMgrTest_012 start");
    auto statsService = BatteryStatsService::GetInstance();
    EXPECT_TRUE(statsService != nullptr);
    statsService->OnStart();
    statsService->RegisterBootCompletedCallback();
    statsService->OnStop();
    statsService->DestroyInstance();
    STATS_HILOGI(LABEL_TEST, "StatsServicePowerMgrTest_012 end");
}

}