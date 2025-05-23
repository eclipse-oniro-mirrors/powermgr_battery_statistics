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

#include "stats_service_bluetooth_test.h"
#include "stats_log.h"

#include <bluetooth_def.h>
#include <hisysevent.h>

#include "battery_stats_listener.h"
#include "battery_stats_service.h"
#include "hisysevent_operation.h"
#include "stats_hisysevent.h"
#include "stats_service_test_proxy.h"
#include "stats_service_write_event.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::PowerMgr;
using namespace OHOS;
using namespace std;

namespace {
static sptr<BatteryStatsService> g_statsService = nullptr;
static std::shared_ptr<StatsServiceTestProxy> g_statsServiceProxy = nullptr;
} // namespace

static void WriteBluetoothEvent(int32_t pid, int32_t uid, long time)
{
    auto statsService = BatteryStatsService::GetInstance();
    int32_t stateOn = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_ON);
    int32_t stateOff = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_OFF);
    int32_t stateScanOn = static_cast<int32_t>(Bluetooth::DISCOVERY_STARTED);
    int32_t stateScanOff = static_cast<int32_t>(Bluetooth::DISCOVERY_STOPED);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BR_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(time * StatsTest::US_PER_MS);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BR_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SCAN_START, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid);
    usleep(time * StatsTest::US_PER_MS);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SCAN_STOP, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(time * StatsTest::US_PER_MS);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::DISCOVERY_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateScanOn);
    usleep(time * StatsTest::US_PER_MS);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::DISCOVERY_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateScanOff);
}

void StatsServiceBluetoothTest::SetUpTestCase()
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

void StatsServiceBluetoothTest::TearDownTestCase()
{
    g_statsService->listenerPtr_ = nullptr;
    g_statsService->OnStop();
}

void StatsServiceBluetoothTest::SetUp()
{
    auto statsService = BatteryStatsService::GetInstance();
    statsService->SetOnBattery(true);
}

void StatsServiceBluetoothTest::TearDown()
{
    auto statsService = BatteryStatsService::GetInstance();
    statsService->SetOnBattery(false);
}

namespace {
/**
 * @tc.name: StatsServiceBluetoothTest_001
 * @tc.desc: test Reset function(Bluetooth BR on)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_001, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_001 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_ON);
    int32_t stateOff = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_OFF);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BR_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BR_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);
    int32_t tempError;
    double powerMahBefore;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH, powerMahBefore, tempError);
    g_statsServiceProxy->ResetIpc();
    double powerMahAfter;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH, powerMahAfter, tempError);
    GTEST_LOG_(INFO) << __func__ << ": before consumption = " << powerMahBefore << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": after consumption = " << powerMahAfter << " mAh";
    EXPECT_TRUE(powerMahBefore >= StatsUtils::DEFAULT_VALUE && powerMahAfter == StatsUtils::DEFAULT_VALUE);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_001 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_002
 * @tc.desc: test GetPartStatsMah function(Bluetooth BR on)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_002, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_002 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    double bluetoothBrOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_BLUETOOTH_BR_ON);
    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_ON);
    int32_t stateOff = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_OFF);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BR_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BR_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);
    int32_t tempError;
    double expectedPower = SERVICE_POWER_CONSUMPTION_DURATION_US * bluetoothBrOnAverageMa / US_PER_HOUR;
    double actualPower;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_002 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_003
 * @tc.desc: test GetPartStatsPercent function(Bluetooth BR on)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_003, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_003 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t stateOn = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_ON);
    int32_t stateOff = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_OFF);
    int32_t uid = 10003;
    int32_t pid = 3458;
    double fullPercent = 1;
    double zeroPercent = 0;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BR_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BR_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);
    int32_t tempError;
    double actualPercent;
    g_statsServiceProxy->GetPartStatsPercentIpc(BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH, actualPercent, tempError);
    GTEST_LOG_(INFO) << __func__ << ": actual percent = " << actualPercent;
    EXPECT_TRUE(actualPercent >= zeroPercent && actualPercent <= fullPercent);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_003 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_004
 * @tc.desc: test GetBatteryStats function(Bluetooth BR on)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_004, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_004 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    double bluetoothBrOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_BLUETOOTH_BR_ON);
    int32_t stateOn = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_ON);
    int32_t stateOff = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_OFF);
    int32_t uid = 10003;
    int32_t pid = 3458;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BR_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BR_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);

    double expectedPower = SERVICE_POWER_CONSUMPTION_DURATION_US * bluetoothBrOnAverageMa / US_PER_HOUR;
    double actualPower = StatsUtils::DEFAULT_VALUE;
    ParcelableBatteryStatsList parcelableEntityList;
    int32_t tempError;
    g_statsServiceProxy->GetBatteryStatsIpc(parcelableEntityList, tempError);
    auto list = parcelableEntityList.statsList_;
    for (auto it : list) {
        if ((*it).GetConsumptionType() == BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH) {
            actualPower = (*it).GetPower();
        }
    }
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_004 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_005
 * @tc.desc: test BR_SWITCH_STATE event are sent repeatedly, Bluetooth BR power consumption(Bluetooth BR on)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_005, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_005 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    double bluetoothBrOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_BLUETOOTH_BR_ON);
    int32_t stateOn = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_ON);
    int32_t stateOff = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_OFF);
    int32_t uid = 10003;
    int32_t pid = 3458;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BR_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BR_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BR_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BR_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);
    int32_t tempError;
    double expectedPower = 2 * SERVICE_POWER_CONSUMPTION_DURATION_US * bluetoothBrOnAverageMa / US_PER_HOUR;
    double actualPower;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_005 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_006
 * @tc.desc: test GetAppStatsMah function, Bluetooth BR on state composite test
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_006, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_006 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    double bluetoothBrOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_BLUETOOTH_BR_ON);
    int32_t stateOn = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_ON);
    int32_t stateOff = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_OFF);
    int32_t stateTurningOn = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURNING_ON);
    int32_t stateTurningOff = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURNING_OFF);
    int32_t uid = 10003;
    int32_t pid = 3458;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BR_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateTurningOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BR_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BR_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateTurningOff);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BR_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);

    int32_t tempError;
    double expectedPower = 2 * SERVICE_POWER_CONSUMPTION_DURATION_US * bluetoothBrOnAverageMa / US_PER_HOUR;
    double actualPower;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_006 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_007
 * @tc.desc: test test GetAppStatsMah function, Bluetooth BR on abnormal state test(Bluetooth BR is off)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_007, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_007 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = 10;
    int32_t stateOff = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_OFF);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BR_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BR_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);

    int32_t tempError;
    double expectedPower = StatsUtils::DEFAULT_VALUE;
    double actualPower;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH, actualPower, tempError);
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_EQ(expectedPower, actualPower);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_007 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_008
 * @tc.desc: test test GetAppStatsMah function, Bluetooth BR on abnormal state test(Bluetooth BR is on)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_008, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_008 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    double bluetoothBrOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_BLUETOOTH_BR_ON);
    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_ON);
    int32_t stateOff = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_OFF);
    int32_t stateInvaildOn = 5;
    int32_t stateInvaildOff = -1;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BR_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BR_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateInvaildOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BR_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateInvaildOff);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BR_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);

    int32_t tempError;
    double expectedPower = 3 * SERVICE_POWER_CONSUMPTION_DURATION_US * bluetoothBrOnAverageMa / US_PER_HOUR;
    double actualPower;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_008 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_009
 * @tc.desc: test GetTotalTimeSecond function(Bluetooth BR on)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_009, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_009 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_ON);
    int32_t stateOff = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_OFF);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BR_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BR_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);

    uint64_t expectedTime = round(SERVICE_POWER_CONSUMPTION_DURATION_US / US_PER_SECOND);
    uint64_t actualTime;
    g_statsServiceProxy->GetTotalTimeSecondIpc(StatsUtils::STATS_TYPE_BLUETOOTH_BR_ON, -1, actualTime);
    GTEST_LOG_(INFO) << __func__ << ": expected time = " << expectedTime << " seconds";
    GTEST_LOG_(INFO) << __func__ << ": actual time = " <<  actualTime << " seconds";
    EXPECT_EQ(expectedTime, actualTime);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_009 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_010
 * @tc.desc: test Reset function(Bluetooth BLE on)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_010, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_010 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_ON);
    int32_t stateOff = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_OFF);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);

    int32_t tempError;
    double powerMahBefore;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH, powerMahBefore, tempError);
    g_statsServiceProxy->ResetIpc();
    double powerMahAfter;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH, powerMahAfter, tempError);
    GTEST_LOG_(INFO) << __func__ << ": before consumption = " << powerMahBefore << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": after consumption = " << powerMahAfter << " mAh";
    EXPECT_TRUE(powerMahBefore >= StatsUtils::DEFAULT_VALUE && powerMahAfter == StatsUtils::DEFAULT_VALUE);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_010 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_011
 * @tc.desc: test GetPartStatsMah function(Bluetooth BLE on)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_011, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_011 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    double bluetoothBleOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_BLUETOOTH_BLE_ON);

    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_ON);
    int32_t stateOff = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_OFF);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);
    int32_t tempError;
    double expectedPower = SERVICE_POWER_CONSUMPTION_DURATION_US * bluetoothBleOnAverageMa / US_PER_HOUR;
    double actualPower;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_011 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_012
 * @tc.desc: test GetPartStatsPercent function(Bluetooth BLE on)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_012, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_012 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t stateOn = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_ON);
    int32_t stateOff = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_OFF);
    int32_t uid = 10003;
    int32_t pid = 3458;
    double fullPercent = 1;
    double zeroPercent = 0;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);
    int32_t tempError;
    double actualPercent;
    g_statsServiceProxy->GetPartStatsPercentIpc(BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH, actualPercent, tempError);
    GTEST_LOG_(INFO) << __func__ << ": actual percent = " << actualPercent;
    EXPECT_TRUE(actualPercent >= zeroPercent && actualPercent <= fullPercent);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_012 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_013
 * @tc.desc: test GetBatteryStats function(Bluetooth BLE on)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_013, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_013 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    double bluetoothBleOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_BLUETOOTH_BLE_ON);
    int32_t stateOn = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_ON);
    int32_t stateOff = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_OFF);
    int32_t uid = 10003;
    int32_t pid = 3458;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);

    double expectedPower = SERVICE_POWER_CONSUMPTION_DURATION_US * bluetoothBleOnAverageMa / US_PER_HOUR;
    double actualPower = StatsUtils::DEFAULT_VALUE;
    ParcelableBatteryStatsList parcelableEntityList;
    int32_t tempError;
    g_statsServiceProxy->GetBatteryStatsIpc(parcelableEntityList, tempError);
    auto list = parcelableEntityList.statsList_;
    for (auto it : list) {
        if ((*it).GetConsumptionType() == BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH) {
            actualPower = (*it).GetPower();
        }
    }
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_013 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_014
 * @tc.desc: test BLE_SWITCH_STATE event are sent repeatedly, Bluetooth BLE power consumption(Bluetooth BLE on)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_014, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_014 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    double bluetoothBleOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_BLUETOOTH_BLE_ON);
    int32_t stateOn = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_ON);
    int32_t stateOff = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_OFF);
    int32_t uid = 10003;
    int32_t pid = 3458;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);

    double expectedPower = 2 * SERVICE_POWER_CONSUMPTION_DURATION_US * bluetoothBleOnAverageMa / US_PER_HOUR;
    int32_t tempError;
    double actualPower;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_014 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_015
 * @tc.desc: test GetAppStatsMah function, Bluetooth BLE on state composite test
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_015, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_015 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    double bluetoothBleOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_BLUETOOTH_BLE_ON);
    int32_t stateOn = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_ON);
    int32_t stateOff = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_OFF);
    int32_t stateTurningOn = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURNING_ON);
    int32_t stateTurningOff = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURNING_OFF);
    int32_t uid = 10003;
    int32_t pid = 3458;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateTurningOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateTurningOff);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);

    double expectedPower = 2 * SERVICE_POWER_CONSUMPTION_DURATION_US * bluetoothBleOnAverageMa / US_PER_HOUR;
    int32_t tempError;
    double actualPower;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_015 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_016
 * @tc.desc: test test GetAppStatsMah function, Bluetooth BLE on abnormal state test(Bluetooth BLE is off)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_016, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_016 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = 10;
    int32_t stateOff = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_OFF);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);

    double expectedPower = StatsUtils::DEFAULT_VALUE;
    int32_t tempError;
    double actualPower;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH, actualPower, tempError);
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_EQ(expectedPower, actualPower);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_016 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_017
 * @tc.desc: test test GetAppStatsMah function, Bluetooth BLE on abnormal state test(Bluetooth BLE is on)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_017, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_017 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    double bluetoothBleOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_BLUETOOTH_BLE_ON);
    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_ON);
    int32_t stateOff = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_OFF);
    int32_t stateInvaildOn = 5;
    int32_t stateInvaildOff = -1;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateInvaildOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateInvaildOff);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);

    double expectedPower = 3 * SERVICE_POWER_CONSUMPTION_DURATION_US * bluetoothBleOnAverageMa / US_PER_HOUR;
    int32_t tempError;
    double actualPower;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_017 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_018
 * @tc.desc: test GetTotalTimeSecond function(Bluetooth BLE on)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_018, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_018 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_ON);
    int32_t stateOff = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_OFF);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);

    uint64_t expectedTime = round(SERVICE_POWER_CONSUMPTION_DURATION_US / US_PER_SECOND);
    uint64_t actualTime;
    g_statsServiceProxy->GetTotalTimeSecondIpc(StatsUtils::STATS_TYPE_BLUETOOTH_BR_ON, -1, actualTime);
    GTEST_LOG_(INFO) << __func__ << ": expected time = " << expectedTime << " seconds";
    GTEST_LOG_(INFO) << __func__ << ": actual time = " <<  actualTime << " seconds";
    EXPECT_EQ(expectedTime, actualTime);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_018 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_019
 * @tc.desc: test Reset function(Bluetooth BR scan)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_019, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_019 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = static_cast<int32_t>(Bluetooth::DISCOVERY_STARTED);
    int32_t stateOff = static_cast<int32_t>(Bluetooth::DISCOVERY_STOPED);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::DISCOVERY_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::DISCOVERY_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);
    int32_t tempError;
    double powerMahBefore;
    g_statsServiceProxy->GetAppStatsMahIpc(uid, powerMahBefore, tempError);
    g_statsServiceProxy->ResetIpc();
    double powerMahAfter;
    g_statsServiceProxy->GetAppStatsMahIpc(uid, powerMahAfter, tempError);
    GTEST_LOG_(INFO) << __func__ << ": before consumption = " << powerMahBefore << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": after consumption = " << powerMahAfter << " mAh";
    EXPECT_TRUE(powerMahBefore >= StatsUtils::DEFAULT_VALUE && powerMahAfter == StatsUtils::DEFAULT_VALUE);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_019 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_020
 * @tc.desc: test GetAppStatsMah function(Bluetooth BR scan)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_020, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_020 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    double bluetoothBrScanAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_BLUETOOTH_BR_SCAN);
    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = static_cast<int32_t>(Bluetooth::DISCOVERY_STARTED);
    int32_t stateOff = static_cast<int32_t>(Bluetooth::DISCOVERY_STOPED);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::DISCOVERY_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::DISCOVERY_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);

    double expectedPower = SERVICE_POWER_CONSUMPTION_DURATION_US * bluetoothBrScanAverageMa / US_PER_HOUR;
    int32_t tempError;
    double actualPower;
    g_statsServiceProxy->GetAppStatsMahIpc(uid, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_020 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_021
 * @tc.desc: test GetAppStatsPercent function(Bluetooth BR scan)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_021, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_021 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = static_cast<int32_t>(Bluetooth::DISCOVERY_STARTED);
    int32_t stateOff = static_cast<int32_t>(Bluetooth::DISCOVERY_STOPED);
    double fullPercent = 1;
    double zeroPercent = 0;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::DISCOVERY_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::DISCOVERY_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);
    int32_t tempError;
    double actualPercent;
    g_statsServiceProxy->GetAppStatsPercentIpc(uid, actualPercent, tempError);
    GTEST_LOG_(INFO) << __func__ << ": actual percent = " << actualPercent;
    EXPECT_TRUE(actualPercent >= zeroPercent && actualPercent <= fullPercent);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_021 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_022
 * @tc.desc: test DISCOVERY_STATE event are sent repeatedly, BR scan power consumption(Bluetooth BR scan)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_022, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_022 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    double bluetoothBrScanAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_BLUETOOTH_BR_SCAN);
    int32_t stateOn = static_cast<int32_t>(Bluetooth::DISCOVERY_STARTED);
    int32_t stateOff = static_cast<int32_t>(Bluetooth::DISCOVERY_STOPED);
    int32_t uid = 10003;
    int32_t pid = 3458;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::DISCOVERY_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::DISCOVERY_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::DISCOVERY_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::DISCOVERY_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);

    double expectedPower = 2 * SERVICE_POWER_CONSUMPTION_DURATION_US * bluetoothBrScanAverageMa / US_PER_HOUR;
    int32_t tempError;
    double actualPower;
    g_statsServiceProxy->GetAppStatsMahIpc(uid, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_022 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_023
 * @tc.desc: test GetAppStatsMah function, Bluetooth BR scan state composite test
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_023, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_023 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    double bluetoothBrScanAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_BLUETOOTH_BR_SCAN);
    int32_t stateOn = static_cast<int32_t>(Bluetooth::DISCOVERY_STARTED);
    int32_t stateOff = static_cast<int32_t>(Bluetooth::DISCOVERY_STOPED);
    int32_t stateScan = static_cast<int32_t>(Bluetooth::DISCOVERYING);
    int32_t uid = 10003;
    int32_t pid = 3458;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::DISCOVERY_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateScan);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::DISCOVERY_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::DISCOVERY_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateScan);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::DISCOVERY_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);

    double expectedPower = 2 * SERVICE_POWER_CONSUMPTION_DURATION_US * bluetoothBrScanAverageMa / US_PER_HOUR;
    int32_t tempError;
    double actualPower;
    g_statsServiceProxy->GetAppStatsMahIpc(uid, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_023 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_024
 * @tc.desc: test test GetAppStatsMah function, Bluetooth BR scan abnormal state test(Bluetooth BR scan is off)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_024, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_024 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = 10;
    int32_t stateOff = static_cast<int32_t>(Bluetooth::DISCOVERY_STOPED);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::DISCOVERY_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::DISCOVERY_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);

    double expectedPower = StatsUtils::DEFAULT_VALUE;
    int32_t tempError;
    double actualPower;
    g_statsServiceProxy->GetAppStatsMahIpc(uid, actualPower, tempError);
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_EQ(expectedPower, actualPower);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_024 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_025
 * @tc.desc: test test GetAppStatsMah function, Bluetooth BR scan abnormal state test(Bluetooth BR scan is on)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_025, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_025 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    double bluetoothBleOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_BLUETOOTH_BR_SCAN);
    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = static_cast<int32_t>(Bluetooth::DISCOVERY_STARTED);
    int32_t stateOff = static_cast<int32_t>(Bluetooth::DISCOVERY_STOPED);
    int32_t stateInvaildOn = 5;
    int32_t stateInvaildOff = -1;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::DISCOVERY_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::DISCOVERY_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateInvaildOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::DISCOVERY_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateInvaildOff);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::DISCOVERY_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);

    double expectedPower = 3 * SERVICE_POWER_CONSUMPTION_DURATION_US * bluetoothBleOnAverageMa / US_PER_HOUR;
    int32_t tempError;
    double actualPower;
    g_statsServiceProxy->GetAppStatsMahIpc(uid, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_025 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_026
 * @tc.desc: test GetTotalTimeSecond function(Bluetooth BR scan)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_026, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_026 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = static_cast<int32_t>(Bluetooth::DISCOVERY_STARTED);
    int32_t stateOff = static_cast<int32_t>(Bluetooth::DISCOVERY_STOPED);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::DISCOVERY_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::DISCOVERY_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);

    uint64_t expectedTime = round(SERVICE_POWER_CONSUMPTION_DURATION_US / US_PER_SECOND);
    uint64_t actualTime;
    g_statsServiceProxy->GetTotalTimeSecondIpc(StatsUtils::STATS_TYPE_BLUETOOTH_BR_ON, uid, actualTime);
    GTEST_LOG_(INFO) << __func__ << ": expected time = " << expectedTime << " seconds";
    GTEST_LOG_(INFO) << __func__ << ": actual time = " <<  actualTime << " seconds";
    EXPECT_EQ(expectedTime, actualTime);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_026 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_027
 * @tc.desc: test GetAppStatsMah function with different UID(Bluetooth BR scan)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_027, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_027 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    double bluetoothBrScanAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_BLUETOOTH_BR_SCAN);
    int32_t uidOne = 10003;
    int32_t pidOne = 3458;
    int32_t uidTwo = 10004;
    int32_t pidTwo = 3459;
    int32_t stateOn = static_cast<int32_t>(Bluetooth::DISCOVERY_STARTED);
    int32_t stateOff = static_cast<int32_t>(Bluetooth::DISCOVERY_STOPED);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::DISCOVERY_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pidOne, "UID", uidOne, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::DISCOVERY_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pidTwo, "UID", uidTwo, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::DISCOVERY_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pidTwo, "UID", uidTwo, "STATE", stateOff);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::DISCOVERY_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pidOne, "UID", uidOne, "STATE", stateOff);
    int32_t tempError;
    double expectedPowerOne = 3 * SERVICE_POWER_CONSUMPTION_DURATION_US * bluetoothBrScanAverageMa / US_PER_HOUR;
    double actualPowerOne;
    g_statsServiceProxy->GetAppStatsMahIpc(uidOne, actualPowerOne, tempError);
    double devPrecentOne = abs(expectedPowerOne - actualPowerOne) / expectedPowerOne;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption One = " << expectedPowerOne << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption One = " << actualPowerOne << " mAh";
    EXPECT_LE(devPrecentOne, DEVIATION_PERCENT_THRESHOLD);

    double expectedPowerTwo = SERVICE_POWER_CONSUMPTION_DURATION_US * bluetoothBrScanAverageMa / US_PER_HOUR;
    double actualPowerTwo;
    g_statsServiceProxy->GetAppStatsMahIpc(uidTwo, actualPowerTwo, tempError);
    double devPrecentTwo = abs(expectedPowerTwo - actualPowerTwo) / expectedPowerTwo;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption Two = " << expectedPowerTwo << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption Two = " << actualPowerTwo << " mAh";
    EXPECT_LE(devPrecentTwo, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_027 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_028
 * @tc.desc: test Reset function(Bluetooth BLE scan)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_028, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_028 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t uid = 10003;
    int32_t pid = 3458;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SCAN_START, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SCAN_STOP, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid);
    int32_t tempError;
    double powerMahBefore;
    g_statsServiceProxy->GetAppStatsMahIpc(uid, powerMahBefore, tempError);
    g_statsServiceProxy->ResetIpc();
    double powerMahAfter;
    g_statsServiceProxy->GetAppStatsMahIpc(uid, powerMahAfter, tempError);
    GTEST_LOG_(INFO) << __func__ << ": before consumption = " << powerMahBefore << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": after consumption = " << powerMahAfter << " mAh";
    EXPECT_TRUE(powerMahBefore >= StatsUtils::DEFAULT_VALUE && powerMahAfter == StatsUtils::DEFAULT_VALUE);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_028 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_029
 * @tc.desc: test GetAppStatsMah function(Bluetooth BLE scan)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_029, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_029 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    double bluetoothBleScanAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_BLUETOOTH_BLE_SCAN);
    int32_t uid = 10003;
    int32_t pid = 3458;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SCAN_START, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SCAN_STOP, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid);

    double expectedPower = SERVICE_POWER_CONSUMPTION_DURATION_US * bluetoothBleScanAverageMa / US_PER_HOUR;
    int32_t tempError;
    double actualPower;
    g_statsServiceProxy->GetAppStatsMahIpc(uid, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_029 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_030
 * @tc.desc: test GetAppStatsPercent function(Bluetooth BLE scan)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_030, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_030 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t uid = 10003;
    int32_t pid = 3458;
    double fullPercent = 1;
    double zeroPercent = 0;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SCAN_START, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SCAN_STOP, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid);

    int32_t tempError;
    double actualPercent;
    g_statsServiceProxy->GetAppStatsPercentIpc(uid, actualPercent, tempError);
    GTEST_LOG_(INFO) << __func__ << ": actual percent = " << actualPercent;
    EXPECT_TRUE(actualPercent >= zeroPercent && actualPercent <= fullPercent);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_030 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_031
 * @tc.desc: test Bluetooth BLE scan event are sent repeatedly, Bluetooth BLE power consumption(Bluetooth BLE scan)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_031, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_031 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    double bluetoothBleScanAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_BLUETOOTH_BLE_SCAN);
    int32_t uid = 10003;
    int32_t pid = 3458;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SCAN_START, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SCAN_START, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SCAN_STOP, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SCAN_STOP, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid);

    double expectedPower = 2 * SERVICE_POWER_CONSUMPTION_DURATION_US * bluetoothBleScanAverageMa / US_PER_HOUR;
    int32_t tempError;
    double actualPower;
    g_statsServiceProxy->GetAppStatsMahIpc(uid, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_031 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_032
 * @tc.desc: test GetTotalTimeSecond function(Bluetooth BLE scan)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_032, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_032 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    int32_t uid = 10003;
    int32_t pid = 3458;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SCAN_START, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SCAN_STOP, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid);

    uint64_t expectedTime = round(SERVICE_POWER_CONSUMPTION_DURATION_US / US_PER_SECOND);
    uint64_t actualTime;
    g_statsServiceProxy->GetTotalTimeSecondIpc(StatsUtils::STATS_TYPE_BLUETOOTH_BR_ON, uid, actualTime);
    GTEST_LOG_(INFO) << __func__ << ": expected time = " << expectedTime << " seconds";
    GTEST_LOG_(INFO) << __func__ << ": actual time = " <<  actualTime << " seconds";
    EXPECT_EQ(expectedTime, actualTime);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_032 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_033
 * @tc.desc: test GetAppStatsMah function with different UID(Bluetooth BLE scan)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_033, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_033 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    double bluetoothBleScanAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_BLUETOOTH_BLE_SCAN);
    int32_t uidOne = 10003;
    int32_t pidOne = 3458;
    int32_t uidTwo = 10004;
    int32_t pidTwo = 3459;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SCAN_START, HiSysEvent::EventType::STATISTIC,
        "PID", pidOne, "UID", uidOne);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SCAN_START, HiSysEvent::EventType::STATISTIC,
        "PID", pidTwo, "UID", uidTwo);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SCAN_STOP, HiSysEvent::EventType::STATISTIC,
        "PID", pidTwo, "UID", uidTwo);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SCAN_STOP, HiSysEvent::EventType::STATISTIC,
        "PID", pidOne, "UID", uidOne);
    int32_t tempError;
    double expectedPowerOne = 3 * SERVICE_POWER_CONSUMPTION_DURATION_US * bluetoothBleScanAverageMa / US_PER_HOUR;
    double actualPowerOne;
    g_statsServiceProxy->GetAppStatsMahIpc(uidOne, actualPowerOne, tempError);
    double devPrecentOne = abs(expectedPowerOne - actualPowerOne) / expectedPowerOne;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption One = " << expectedPowerOne << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption One = " << actualPowerOne << " mAh";
    EXPECT_LE(devPrecentOne, DEVIATION_PERCENT_THRESHOLD);

    double expectedPowerTwo = SERVICE_POWER_CONSUMPTION_DURATION_US * bluetoothBleScanAverageMa / US_PER_HOUR;
    double actualPowerTwo;
    g_statsServiceProxy->GetAppStatsMahIpc(uidTwo, actualPowerTwo, tempError);
    double devPrecentTwo = abs(expectedPowerTwo - actualPowerTwo) / expectedPowerTwo;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption Two = " << expectedPowerTwo << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption Two = " << actualPowerTwo << " mAh";
    EXPECT_LE(devPrecentTwo, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_033 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_034
 * @tc.desc: test GetPartStatsMah function(Bluetooth BR on & BLE on & BR scan & BLE scan)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_034, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_034 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    double bluetoothBrOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_BLUETOOTH_BR_ON);
    double bluetoothBleOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_BLUETOOTH_BLE_ON);
    double bluetoothBrScanAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_BLUETOOTH_BR_SCAN);
    double bluetoothBleScanAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_BLUETOOTH_BLE_SCAN);
    long testTimeMs = 200;
    int32_t uid = 10003;
    int32_t pid = 3458;

    WriteBluetoothEvent(pid, uid, testTimeMs);
    int32_t tempError;
    double expectedPartPower = testTimeMs * (bluetoothBrOnAverageMa + bluetoothBleOnAverageMa) / MS_PER_HOUR;
    double actualPartPower;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH, actualPartPower, tempError);
    double devPrecentPart = abs(expectedPartPower - actualPartPower) / expectedPartPower;
    GTEST_LOG_(INFO) << __func__ << ": expected part consumption = " << expectedPartPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual part consumption = " << actualPartPower << " mAh";
    EXPECT_LE(devPrecentPart, DEVIATION_PERCENT_THRESHOLD);

    double expectedSoftPower = testTimeMs * (bluetoothBrScanAverageMa + bluetoothBleScanAverageMa) / MS_PER_HOUR;
    double actualSoftPower;
    g_statsServiceProxy->GetAppStatsMahIpc(uid, actualSoftPower, tempError);
    double devPrecentSoft = abs(expectedSoftPower - actualSoftPower) / expectedSoftPower;
    GTEST_LOG_(INFO) << __func__ << ": expected soft consumption = " << expectedSoftPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual soft consumption = " << actualSoftPower << " mAh";
    EXPECT_LE(devPrecentSoft, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_034 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_035
 * @tc.desc: test SetOnBattery function(Bluetooth BR on & BLE on & BR scan & BLE scan)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_035, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_035 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();
    g_statsServiceProxy->SetOnBatteryIpc(false);

    long testTimeMs = 200;
    int32_t uid = 10003;
    int32_t pid = 3458;

    WriteBluetoothEvent(pid, uid, testTimeMs);
    int32_t tempError;
    double expectedPower = StatsUtils::DEFAULT_VALUE;
    double actualPartPower;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH, actualPartPower, tempError);
    GTEST_LOG_(INFO) << __func__ << ": expected part consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual part consumption = " << actualPartPower << " mAh";
    EXPECT_EQ(expectedPower, actualPartPower);

    double actualSoftPower;
    g_statsServiceProxy->GetAppStatsMahIpc(uid, actualSoftPower, tempError);
    GTEST_LOG_(INFO) << __func__ << ": expected soft consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual soft consumption = " << actualSoftPower << " mAh";
    EXPECT_EQ(expectedPower, actualSoftPower);
    g_statsServiceProxy->SetOnBatteryIpc(true);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_035 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_036
 * @tc.desc: test GetPartStatsMah function with battery changed(Bluetooth BR on)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_036, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_036 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    double bluetoothBrOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_BLUETOOTH_BR_ON);
    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_ON);
    int32_t stateOff = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_OFF);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BR_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    g_statsServiceProxy->SetOnBatteryIpc(false);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    g_statsServiceProxy->SetOnBatteryIpc(true);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BR_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);

    double expectedPower = 2 * SERVICE_POWER_CONSUMPTION_DURATION_US * bluetoothBrOnAverageMa / US_PER_HOUR;
    int32_t tempError;
    double actualPower;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH, actualPower, tempError);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_036 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_037
 * @tc.desc: test bluetooth entity GetPartStatsMah function(Bluetooth BR on & BLE on & BR scan & BLE scan)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_037, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_037 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();

    double bluetoothBrOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_BLUETOOTH_BR_ON);
    double bluetoothBleOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_BLUETOOTH_BLE_ON);
    double bluetoothBrScanAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_BLUETOOTH_BR_SCAN);
    double bluetoothBleScanAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_BLUETOOTH_BLE_SCAN);
    long testTimeMs = 200;
    int32_t uid = 10003;
    int32_t pid = 3458;

    WriteBluetoothEvent(pid, uid, testTimeMs);

    auto statsCore = statsService->GetBatteryStatsCore();
    auto bluetoothEntity = statsCore->GetEntity(BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH);
    statsCore->ComputePower();

    double expectedPartPower = testTimeMs * (bluetoothBrOnAverageMa + bluetoothBleOnAverageMa) / MS_PER_HOUR;
    double actualPartPower = bluetoothEntity->GetStatsPowerMah(StatsUtils::STATS_TYPE_BLUETOOTH_BR_ON) +
        bluetoothEntity->GetStatsPowerMah(StatsUtils::STATS_TYPE_BLUETOOTH_BLE_ON);
    double devPrecentPart = abs(expectedPartPower - actualPartPower) / expectedPartPower;
    GTEST_LOG_(INFO) << __func__ << ": expected part consumption = " << expectedPartPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual part consumption = " << actualPartPower << " mAh";
    EXPECT_LE(devPrecentPart, DEVIATION_PERCENT_THRESHOLD);

    double expectedSoftPower = testTimeMs * (bluetoothBrScanAverageMa + bluetoothBleScanAverageMa) / MS_PER_HOUR;
    double actualSoftPower = bluetoothEntity->GetStatsPowerMah(StatsUtils::STATS_TYPE_BLUETOOTH_BR_SCAN, uid) +
        bluetoothEntity->GetStatsPowerMah(StatsUtils::STATS_TYPE_BLUETOOTH_BLE_SCAN, uid);
    double devPrecentSoft = abs(expectedSoftPower - actualSoftPower) / expectedSoftPower;
    GTEST_LOG_(INFO) << __func__ << ": expected soft consumption = " << expectedSoftPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual soft consumption = " << actualSoftPower << " mAh";
    EXPECT_LE(devPrecentSoft, DEVIATION_PERCENT_THRESHOLD);

    EXPECT_EQ(StatsUtils::DEFAULT_VALUE, bluetoothEntity->GetStatsPowerMah(StatsUtils::STATS_TYPE_BLUETOOTH_BR_SCAN));
    EXPECT_EQ(StatsUtils::DEFAULT_VALUE, bluetoothEntity->GetStatsPowerMah(StatsUtils::STATS_TYPE_BLUETOOTH_BLE_SCAN));
    EXPECT_EQ(StatsUtils::DEFAULT_VALUE, bluetoothEntity->GetStatsPowerMah(StatsUtils::STATS_TYPE_INVALID, uid));
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_037 end");
}

/**
 * @tc.name: StatsServiceBluetoothTest_038
 * @tc.desc: test send hisysevent with missing information(Bluetooth)
 * @tc.type: FUNC
 * @tc.require: issueI663DX
 */
HWTEST_F (StatsServiceBluetoothTest, StatsServiceBluetoothTest_038, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_038 start");
    ASSERT_NE(g_statsServiceProxy, nullptr);
    auto statsService = BatteryStatsService::GetInstance();
    g_statsServiceProxy->ResetIpc();
    int32_t uid = 10003;

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BR_SWITCH_STATE, HiSysEvent::EventType::STATISTIC);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BR_SWITCH_STATE, HiSysEvent::EventType::STATISTIC);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SCAN_START, HiSysEvent::EventType::STATISTIC);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SCAN_STOP, HiSysEvent::EventType::STATISTIC);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SWITCH_STATE, HiSysEvent::EventType::STATISTIC);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SWITCH_STATE, HiSysEvent::EventType::STATISTIC);

    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::DISCOVERY_STATE, HiSysEvent::EventType::STATISTIC);
    usleep(SERVICE_POWER_CONSUMPTION_DURATION_US);
    StatsWriteHiSysEvent(statsService,
        HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::DISCOVERY_STATE, HiSysEvent::EventType::STATISTIC);
    int32_t tempError;
    double expectedPartPower = StatsUtils::DEFAULT_VALUE;
    double actualPartPower;
    g_statsServiceProxy->GetPartStatsMahIpc(BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH, actualPartPower, tempError);
    GTEST_LOG_(INFO) << __func__ << ": expected part consumption = " << expectedPartPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual part consumption = " << actualPartPower << " mAh";
    EXPECT_EQ(expectedPartPower, actualPartPower);

    double expectedSoftPower = StatsUtils::DEFAULT_VALUE;
    double actualSoftPower;
    g_statsServiceProxy->GetAppStatsMahIpc(uid, actualSoftPower, tempError);
    GTEST_LOG_(INFO) << __func__ << ": expected soft consumption = " << expectedSoftPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual soft consumption = " << actualSoftPower << " mAh";
    EXPECT_EQ(expectedSoftPower, actualSoftPower);
    STATS_HILOGI(LABEL_TEST, "StatsServiceBluetoothTest_038 end");
}
}