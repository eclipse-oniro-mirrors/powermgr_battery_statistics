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

#include "stats_service_audio_test.h"
#include "stats_log.h"

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
using namespace std;

namespace {
static sptr<BatteryStatsService> g_statsService = nullptr;
static std::shared_ptr<StatsServiceTestProxy> g_statsServiceProxy = nullptr;
} // namespace

void StatsServiceAudioTest::SetUpTestCase()
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

void StatsServiceAudioTest::TearDownTestCase()
{
    g_statsService->listenerPtr_ = nullptr;
    g_statsService->OnStop();
}

void StatsServiceAudioTest::SetUp()
{
    auto statsService = BatteryStatsService::GetInstance();
    statsService->SetOnBattery(true);
}

void StatsServiceAudioTest::TearDown()
{
    auto statsService = BatteryStatsService::GetInstance();
    statsService->SetOnBattery(false);
}

namespace {
/**
 * @tc.name: StatsServiceAudioTest_001
 * @tc.desc: test Reset function(Audio)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceAudioTest, StatsServiceAudioTest_001, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceAudioTest_001 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

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
    g_statsServiceProxy->ResetIpc();
    double powerMahAfter;
    g_statsServiceProxy->GetAppStatsMahIpc(uid, powerMahAfter, tempError);
    GTEST_LOG_(INFO) << __func__ << ": before consumption = " << powerMahBefore << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": after consumption = " << powerMahAfter << " mAh";
    EXPECT_TRUE(powerMahBefore >= StatsUtils::DEFAULT_VALUE && powerMahAfter == StatsUtils::DEFAULT_VALUE);
    STATS_HILOGI(LABEL_TEST, "StatsServiceAudioTest_001 end");
}

/**
 * @tc.name: StatsServiceAudioTest_002
 * @tc.desc: test GetAppStatsMah function(Audio)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceAudioTest, StatsServiceAudioTest_002, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceAudioTest_002 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

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
    int32_t tempError;
    double expectedPower = SERVICE_POWER_CONSUMPTION_DURATION_US * audioOnAverageMa / US_PER_HOUR;
    double actualPower;
    g_statsServiceProxy->GetAppStatsMahIpc(uid, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServiceAudioTest_002 end");
}

/**
 * @tc.name: StatsServiceAudioTest_003
 * @tc.desc: test GetAppStatsPercent function(Audio)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceAudioTest, StatsServiceAudioTest_003, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceAudioTest_003 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateRunning = 2;
    int32_t stateStopped = 3;
    double fullPercent = 1;
    double zeroPercent = 0;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::AUDIO, StatsHiSysEvent::STREAM_CHANGE, HiSysEvent::EventType::BEHAVIOR, "PID", pid,
        "UID", uid, "STATE", stateRunning);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::AUDIO, StatsHiSysEvent::STREAM_CHANGE, HiSysEvent::EventType::BEHAVIOR, "PID", pid,
        "UID", uid, "STATE", stateStopped);
    int32_t tempError;
    double actualPercent;
    g_statsServiceProxy->GetAppStatsPercentIpc(uid, actualPercent, tempError);
    GTEST_LOG_(INFO) << __func__ << ": actual percent = " << actualPercent;
    EXPECT_TRUE(actualPercent >= zeroPercent && actualPercent <= fullPercent);
    STATS_HILOGI(LABEL_TEST, "StatsServiceAudioTest_003 end");
}

/**
 * @tc.name: StatsServiceAudioTest_004
 * @tc.desc: test GetAppStatsMah function, Audio state composite test
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceAudioTest, StatsServiceAudioTest_004, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceAudioTest_004 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    double audioOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_AUDIO_ON);
    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateRunning = 2;
    int32_t stateStopped = 3;
    int32_t stateReleased = 4;
    int32_t statePaused = 5;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::AUDIO, StatsHiSysEvent::STREAM_CHANGE, HiSysEvent::EventType::BEHAVIOR, "PID", pid,
        "UID", uid, "STATE", stateRunning);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::AUDIO, StatsHiSysEvent::STREAM_CHANGE, HiSysEvent::EventType::BEHAVIOR, "PID", pid,
        "UID", uid, "STATE", stateStopped);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::AUDIO, StatsHiSysEvent::STREAM_CHANGE, HiSysEvent::EventType::BEHAVIOR, "PID", pid,
        "UID", uid, "STATE", stateRunning);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::AUDIO, StatsHiSysEvent::STREAM_CHANGE, HiSysEvent::EventType::BEHAVIOR, "PID", pid,
        "UID", uid, "STATE", stateReleased);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::AUDIO, StatsHiSysEvent::STREAM_CHANGE, HiSysEvent::EventType::BEHAVIOR, "PID", pid,
        "UID", uid, "STATE", stateRunning);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::AUDIO, StatsHiSysEvent::STREAM_CHANGE, HiSysEvent::EventType::BEHAVIOR, "PID", pid,
        "UID", uid, "STATE", statePaused);
    int32_t tempError;
    double expectedPower = 3 * SERVICE_POWER_CONSUMPTION_DURATION_US * audioOnAverageMa / US_PER_HOUR;
    double actualPower;
    g_statsServiceProxy->GetAppStatsMahIpc(uid, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServiceAudioTest_004 end");
}

/**
 * @tc.name: StatsServiceAudioTest_005
 * @tc.desc: test GetAppStatsMah function, Audio abnormal state test
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceAudioTest, StatsServiceAudioTest_005, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceAudioTest_005 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    double audioOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_AUDIO_ON);
    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateRunning = 2;
    int32_t stateStopped = 3;
    int32_t stateInvalid = -1;
    int32_t stateAbnormal = 101;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::AUDIO, StatsHiSysEvent::STREAM_CHANGE, HiSysEvent::EventType::BEHAVIOR, "PID", pid,
        "UID", uid, "STATE", stateInvalid);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::AUDIO, StatsHiSysEvent::STREAM_CHANGE, HiSysEvent::EventType::BEHAVIOR, "PID", pid,
        "UID", uid, "STATE", stateRunning);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::AUDIO, StatsHiSysEvent::STREAM_CHANGE, HiSysEvent::EventType::BEHAVIOR, "PID", pid,
        "UID", uid, "STATE", stateAbnormal);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::AUDIO, StatsHiSysEvent::STREAM_CHANGE, HiSysEvent::EventType::BEHAVIOR, "PID", pid,
        "UID", uid, "STATE", stateStopped);
    int32_t tempError;
    double expectedPower = 2 * SERVICE_POWER_CONSUMPTION_DURATION_US * audioOnAverageMa / US_PER_HOUR;
    double actualPower;
    g_statsServiceProxy->GetAppStatsMahIpc(uid, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServiceAudioTest_005 end");
}

/**
 * @tc.name: StatsServiceAudioTest_006
 * @tc.desc: test GetAppStatsMah(Audio) and GetAppStatsPercent(Sensor) function
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceAudioTest, StatsServiceAudioTest_006, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceAudioTest_006 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

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
    int32_t tempError;
    double expectedPower = SERVICE_POWER_CONSUMPTION_DURATION_US * audioOnAverageMa / US_PER_HOUR;
    double actualPower;
    g_statsServiceProxy->GetAppStatsMahIpc(uid, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);

    uid = 10004;
    pid = 3459;
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

    double actualPercent;
    g_statsServiceProxy->GetAppStatsPercentIpc(uid, actualPercent, tempError);
    GTEST_LOG_(INFO) << __func__ << ": actual percent = " << actualPercent;
    EXPECT_TRUE(actualPercent >= zeroPercent && actualPercent <= fullPercent);
    STATS_HILOGI(LABEL_TEST, "StatsServiceAudioTest_006 end");
}

/**
 * @tc.name: StatsServiceAudioTest_007
 * @tc.desc: test audio entity GetPartStatsMah function(Audio)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceAudioTest, StatsServiceAudioTest_007, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceAudioTest_007 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

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

    auto statsCore = statsService->GetBatteryStatsCore();
    auto audioEntity = statsCore->GetEntity(BatteryStatsInfo::CONSUMPTION_TYPE_AUDIO);
    statsCore->ComputePower();

    double expectedPower = SERVICE_POWER_CONSUMPTION_DURATION_US * audioOnAverageMa / US_PER_HOUR;
    double actualPower = audioEntity->GetStatsPowerMah(StatsUtils::STATS_TYPE_AUDIO_ON, uid);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);

    EXPECT_EQ(StatsUtils::DEFAULT_VALUE, audioEntity->GetStatsPowerMah(StatsUtils::STATS_TYPE_AUDIO_ON));
    EXPECT_EQ(StatsUtils::DEFAULT_VALUE, audioEntity->GetStatsPowerMah(StatsUtils::STATS_TYPE_INVALID, uid));
    STATS_HILOGI(LABEL_TEST, "StatsServiceAudioTest_007 end");
}

/**
 * @tc.name: StatsServiceAudioTest_008
 * @tc.desc: test send hisysevent with missing information(Audio)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceAudioTest, StatsServiceAudioTest_008, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceAudioTest_008 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t uid = 10003;
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::AUDIO, StatsHiSysEvent::STREAM_CHANGE, HiSysEvent::EventType::BEHAVIOR);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::AUDIO, StatsHiSysEvent::STREAM_CHANGE, HiSysEvent::EventType::BEHAVIOR);

    double expectedPower = StatsUtils::DEFAULT_VALUE;
    int32_t tempError;
    double actualPower;
    g_statsServiceProxy->GetAppStatsMahIpc(uid, actualPower, tempError);
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_EQ(expectedPower, actualPower);
    STATS_HILOGI(LABEL_TEST, "StatsServiceAudioTest_008 end");
}
}