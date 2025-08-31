/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "batterystats_sys_test.h"
#include "stats_log.h"

#include <cmath>
#include <csignal>
#include <iostream>
#include <unistd.h>

#include <bluetooth_def.h>
#include <call_manager_inner_type.h>
#include <display_power_info.h>
#include <hisysevent.h>
#include <if_system_ability_manager.h>
#include <ipc_skeleton.h>
#include <iservice_registry.h>
#include <running_lock_info.h>
#include <string_ex.h>
#include <system_ability_definition.h>
#include "wifi_msg.h"

#include "battery_stats_client.h"
#include "battery_stats_parser.h"
#include "stats_common.h"
#include "stats_hisysevent.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::HiviewDFX;
using namespace OHOS::PowerMgr;
using namespace OHOS::Telephony;

namespace {
constexpr int64_t US_PER_HOUR = 3600000000;
constexpr int32_t US_PER_SECOND = 1000000;
constexpr int32_t POWER_CONSUMPTION_DURATION_US = 300000;
constexpr double DEVIATION_PERCENT_THRESHOLD = 1;
static std::vector<std::string> dumpArgs;
static std::shared_ptr<BatteryStatsParser> g_statsParser = nullptr;
}

static void ParserAveragePowerFile()
{
    if (g_statsParser == nullptr) {
        g_statsParser = std::make_shared<BatteryStatsParser>();
        if (!g_statsParser->Init()) {
            GTEST_LOG_(INFO) << __func__ << ": Battery stats parser initialization failed";
        }
    }
}

void BatterystatsSysTest::SetUpTestCase()
{
    ParserAveragePowerFile();
    dumpArgs.push_back("-batterystats");
    system("hidumper -s 3302 -a -u");
}

void BatterystatsSysTest::TearDownTestCase()
{
    system("hidumper -s 3302 -a -r");
}

void BatterystatsSysTest::SetUp()
{
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.SetOnBattery(true);
}

void BatterystatsSysTest::TearDown()
{
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.SetOnBattery(false);
}

/**
 *
 * @tc.name: BatteryStatsSysTest_001
 * @tc.desc: test Wakelock consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_001, TestSize.Level0)
{
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_001 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    int32_t uid = 10001;
    int32_t pid = 3456;
    int32_t stateLock = 1;
    int32_t stateUnlock = 0;
    double wakelockAverage = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_CPU_AWAKE);
    int32_t type = static_cast<int32_t>(RunningLockType::RUNNINGLOCK_SCREEN);
    std::string name = " BatteryStatsSysTest_001";

    HiSysEventWrite(HiSysEvent::Domain::POWER, StatsHiSysEvent::POWER_RUNNINGLOCK,
        HiSysEvent::EventType::STATISTIC, "PID", pid,
        "UID", uid, "STATE", stateLock, "TYPE", type, "NAME", name);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::POWER, StatsHiSysEvent::POWER_RUNNINGLOCK,
        HiSysEvent::EventType::STATISTIC, "PID", pid,
        "UID", uid, "STATE", stateUnlock, "TYPE", type, "NAME", name);

    double expectedPowerMah = wakelockAverage * POWER_CONSUMPTION_DURATION_US / US_PER_HOUR;
    double actualPowerMah = statsClient.GetAppStatsMah(uid);
    double devPrecent = abs(expectedPowerMah - actualPowerMah) / expectedPowerMah;
    long expectedTimeSec = round(POWER_CONSUMPTION_DURATION_US / US_PER_SECOND);
    long actualTimeSec = statsClient.GetTotalTimeSecond(StatsUtils::STATS_TYPE_WAKELOCK_HOLD, uid);

    std::string expectedDebugInfo;
    expectedDebugInfo.append("UID = ")
        .append(ToString(uid))
        .append(", PID = ")
        .append(ToString(pid))
        .append(", wakelock type = ")
        .append(ToString(type))
        .append(", wakelock name = ")
        .append(name);

    std::string actualDebugInfo = statsClient.Dump(dumpArgs);

    auto index = actualDebugInfo.find(expectedDebugInfo);

    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPowerMah << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPowerMah << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": expected time = " << expectedTimeSec << " seconds";
    GTEST_LOG_(INFO) << __func__ << ": actual time = " <<  actualTimeSec << " seconds";

    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    EXPECT_EQ(expectedTimeSec,  actualTimeSec);
    EXPECT_TRUE(index != string::npos);
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_001 end");
}

/**
 *
 * @tc.name: BatteryStatsSysTest_002
 * @tc.desc: test Screen consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_002, TestSize.Level0)
{
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_002 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    int32_t stateOn = static_cast<int32_t>(DisplayPowerMgr::DisplayState::DISPLAY_ON);
    int32_t stateOff = static_cast<int32_t>(DisplayPowerMgr::DisplayState::DISPLAY_OFF);
    int32_t brightness = 150;
    double screenOnAverage = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_SCREEN_ON);
    double screenBrightnessAverage = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_SCREEN_BRIGHTNESS);

    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::SCREEN_STATE,
        HiviewDFX::HiSysEvent::EventType::STATISTIC, "STATE", stateOn);
    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::BRIGHTNESS_NIT,
        HiviewDFX::HiSysEvent::EventType::STATISTIC, "BRIGHTNESS", brightness);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::SCREEN_STATE,
        HiviewDFX::HiSysEvent::EventType::STATISTIC, "STATE", stateOff);

    double average = screenBrightnessAverage * brightness + screenOnAverage;

    double expectedPowerMah = average * POWER_CONSUMPTION_DURATION_US / US_PER_HOUR;
    double actualPowerMah = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_SCREEN);
    double devPrecent = abs(expectedPowerMah - actualPowerMah) / expectedPowerMah;
    long expectedTimeSec = round(POWER_CONSUMPTION_DURATION_US / US_PER_SECOND);
    long actualTimeSec = statsClient.GetTotalTimeSecond(StatsUtils::STATS_TYPE_SCREEN_ON);

    std::string expectedDebugInfo;
    expectedDebugInfo.append("Additional debug info: ")
        .append("Event name = ")
        .append(StatsHiSysEvent::SCREEN_STATE);

    std::string actualDebugInfo = statsClient.Dump(dumpArgs);

    auto index = actualDebugInfo.find(expectedDebugInfo);

    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPowerMah << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPowerMah << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": expected time = " << expectedTimeSec << " seconds";
    GTEST_LOG_(INFO) << __func__ << ": actual time = " <<  actualTimeSec << " seconds";

    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    EXPECT_EQ(expectedTimeSec,  actualTimeSec);
    EXPECT_TRUE(index != string::npos);
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_002 end");
}

/**
 *
 * @tc.name: BatteryStatsSysTest_003
 * @tc.desc: test Battery stats event
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_003, TestSize.Level0)
{
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_003 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    int32_t batteryLevel = 60;
    int32_t batteryChargerType = 2;

    HiSysEventWrite(HiSysEvent::Domain::BATTERY, StatsHiSysEvent::BATTERY_CHANGED,
        HiSysEvent::EventType::STATISTIC, "LEVEL", batteryLevel, "CHARGER", batteryChargerType);
    usleep(POWER_CONSUMPTION_DURATION_US);

    std::string expectedDebugInfo;
    expectedDebugInfo.append("Battery level = ")
        .append(ToString(batteryLevel))
        .append(", Charger type = ")
        .append(ToString(batteryChargerType));

    std::string actualDebugInfo = statsClient.Dump(dumpArgs);
    auto index = actualDebugInfo.find(expectedDebugInfo);
    EXPECT_TRUE(index != string::npos);
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_003 end");
}

/**
 *
 * @tc.name: BatteryStatsSysTest_004
 * @tc.desc: test Thermal stats event
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_004, TestSize.Level0)
{
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_004 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    std::string partName = "Battery";
    int32_t temperature = 40;

    HiSysEventWrite(HiSysEvent::Domain::THERMAL, StatsHiSysEvent::POWER_TEMPERATURE,
        HiSysEvent::EventType::STATISTIC, "NAME", partName, "TEMPERATURE", temperature);
    usleep(POWER_CONSUMPTION_DURATION_US);

    std::string expectedDebugInfo;
    expectedDebugInfo.append("Additional debug info: ")
        .append("Event name = POWER_TEMPERATURE")
        .append(" Name = ")
        .append(partName);

    std::string actualDebugInfo = statsClient.Dump(dumpArgs);
    auto index = actualDebugInfo.find(expectedDebugInfo);
    EXPECT_TRUE(index != string::npos);
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_004 end");
}

/**
 *
 * @tc.name: BatteryStatsSysTest_005
 * @tc.desc: test WorkScheduler stats event
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_005, TestSize.Level0)
{
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_005 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    int32_t pid = 3457;
    int32_t uid = 10002;
    int32_t type = 1;
    int32_t interval = 30000;
    int32_t state = 5;

    HiSysEventWrite(HiSysEvent::Domain::STATS, StatsHiSysEvent::POWER_WORKSCHEDULER,
        HiSysEvent::EventType::STATISTIC, "PID", pid, "UID", uid, "TYPE", type,
        "INTERVAL", interval, "STATE", state);
    usleep(POWER_CONSUMPTION_DURATION_US);

    std::string expectedDebugInfo;
    expectedDebugInfo.append("UID = ")
        .append(ToString(uid))
        .append(", PID = ")
        .append(ToString(pid))
        .append(", work type = ")
        .append(ToString(type))
        .append(", work interval = ")
        .append(ToString(interval))
        .append(", work state = ")
        .append(ToString(state));

    std::string actualDebugInfo = statsClient.Dump(dumpArgs);
    auto index = actualDebugInfo.find(expectedDebugInfo);
    EXPECT_TRUE(index != string::npos);
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_005 end");
}

/**
 *
 * @tc.name: BatteryStatsSysTest_006
 * @tc.desc: test Dump function
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_006, TestSize.Level0)
{
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_006 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    std::string result;
    result.clear();
    result = statsClient.Dump(dumpArgs);
    EXPECT_TRUE(result != "");
    statsClient.Reset();
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_006 end");
}

/**
 *
 * @tc.name: BatteryStatsSysTest_007
 * @tc.desc: test Bluetooth consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_007, TestSize.Level0)
{
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_007 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double bluetoothBrOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_BLUETOOTH_BR_ON);
    int32_t stateOn = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_ON);
    int32_t stateOff = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_OFF);
    int32_t uid = 10003;
    int32_t pid = 3458;
    HiSysEventWrite(HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BR_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BR_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);

    double expectedPower = POWER_CONSUMPTION_DURATION_US * bluetoothBrOnAverageMa / US_PER_HOUR;
    double actualPower = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_007 end");
}

/**
 *
 * @tc.name: BatteryStatsSysTest_008
 * @tc.desc: test Wifi consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_008, TestSize.Level0)
{
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_008 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double wifiOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_WIFI_ON);
    int32_t stateOn = static_cast<int32_t>(Wifi::ConnState::CONNECTED);
    int32_t stateOff = static_cast<int32_t>(Wifi::ConnState::DISCONNECTED);
    HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_CONNECTION,
        HiSysEvent::EventType::STATISTIC, "TYPE", stateOn);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_CONNECTION,
        HiSysEvent::EventType::STATISTIC, "TYPE", stateOff);

    double expectedPower = POWER_CONSUMPTION_DURATION_US * wifiOnAverageMa / US_PER_HOUR;
    double actualPower = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_WIFI);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_008 end");
}

/**
 *
 * @tc.name: BatteryStatsSysTest_009
 * @tc.desc: test Phone Call consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_009, TestSize.Level0)
{
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_009 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    int32_t stateOn = static_cast<int32_t>(TelCallState::CALL_STATUS_ACTIVE);
    int32_t stateOff = static_cast<int32_t>(TelCallState::CALL_STATUS_DISCONNECTED);
    int16_t level = 0;
    double phoneOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_RADIO_ON, level);

    HiSysEventWrite(HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOn);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::CALL_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOff);

    double expectedPower = POWER_CONSUMPTION_DURATION_US * phoneOnAverageMa / US_PER_HOUR;
    double actualPower = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_PHONE);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_009 end");
}

/**
 *
 * @tc.name: BatteryStatsSysTest_010
 * @tc.desc: test Idle consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_010, TestSize.Level0)
{
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_010 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double actualPower = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_IDLE);
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_TRUE(actualPower >= StatsUtils::DEFAULT_VALUE);
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_010 end");
}

/**
 *
 * @tc.name: BatteryStatsSysTest_011
 * @tc.desc: test User consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_011, TestSize.Level0)
{
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_011 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    int32_t uerId = 20003;

    double actualPower = statsClient.GetAppStatsMah(uerId);
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_TRUE(actualPower >= StatsUtils::DEFAULT_VALUE);
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_011 end");
}

/**
 *
 * @tc.name: BatteryStatsSysTest_012
 * @tc.desc: test Audio consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_012, TestSize.Level0)
{
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_012 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double audioOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_AUDIO_ON);
    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateRunning = 2;
    int32_t stateStopped = 3;

    HiSysEventWrite(HiSysEvent::Domain::AUDIO, StatsHiSysEvent::STREAM_CHANGE,
        HiSysEvent::EventType::BEHAVIOR, "PID", pid, "UID", uid, "STATE", stateRunning);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::AUDIO, StatsHiSysEvent::STREAM_CHANGE,
        HiSysEvent::EventType::BEHAVIOR, "PID", pid, "UID", uid, "STATE", stateStopped);

    double expectedPower = POWER_CONSUMPTION_DURATION_US * audioOnAverageMa / US_PER_HOUR;
    double actualPower = statsClient.GetAppStatsMah(uid);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_012 end");
}

/**
 *
 * @tc.name: BatteryStatsSysTest_013
 * @tc.desc: test GNSS consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_013, TestSize.Level0)
{
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_013 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double gnssOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_GNSS_ON);
    int32_t uid = 10003;
    int32_t pid = 3458;
    std::string stateOn = "start";
    std::string stateOff = "stop";

    HiSysEventWrite(HiSysEvent::Domain::LOCATION, StatsHiSysEvent::GNSS_STATE,
        HiSysEvent::EventType::STATISTIC, "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::LOCATION, StatsHiSysEvent::GNSS_STATE, HiSysEvent::EventType::STATISTIC, "PID",
        pid, "UID", uid, "STATE", stateOff);

    double expectedPower = POWER_CONSUMPTION_DURATION_US * gnssOnAverageMa / US_PER_HOUR;
    double actualPower = statsClient.GetAppStatsMah(uid);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_013 end");
}

/**
 *
 * @tc.name: BatteryStatsSysTest_014
 * @tc.desc: test Sensor consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_014, TestSize.Level0)
{
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_014 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double sensorGravityOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_SENSOR_GRAVITY);
    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = 1;
    int32_t stateOff = 0;

    HiSysEventWrite(HiSysEvent::Domain::STATS, StatsHiSysEvent::POWER_SENSOR_GRAVITY,
        HiSysEvent::EventType::STATISTIC, "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::STATS, StatsHiSysEvent::POWER_SENSOR_GRAVITY,
        HiSysEvent::EventType::STATISTIC, "PID", pid, "UID", uid, "STATE", stateOff);

    double expectedPower = POWER_CONSUMPTION_DURATION_US * sensorGravityOnAverageMa / US_PER_HOUR;
    double actualPower = statsClient.GetAppStatsMah(uid);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_014 end");
}

/**
 *
 * @tc.name: BatteryStatsSysTest_015
 * @tc.desc: test Camera consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_015, TestSize.Level0)
{
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_015 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double cameraOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_CAMERA_ON);
    int32_t uid = 10003;
    int32_t pid = 3458;
    std::string cameraId = "Camera0";

    HiSysEventWrite(HiSysEvent::Domain::CAMERA, StatsHiSysEvent::CAMERA_CONNECT, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "ID", cameraId);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::CAMERA, StatsHiSysEvent::CAMERA_DISCONNECT, HiSysEvent::EventType::STATISTIC,
        "ID", cameraId);

    double expectedPower = POWER_CONSUMPTION_DURATION_US * cameraOnAverageMa / US_PER_HOUR;
    double actualPower = statsClient.GetAppStatsMah(uid);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_015 end");
}

/**
 *
 * @tc.name: BatteryStatsSysTest_016
 * @tc.desc: test Flashlight consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_016, TestSize.Level0)
{
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_016 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double flashlightOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_FLASHLIGHT_ON);
    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = 1;
    int32_t stateOff = 0;

    HiSysEventWrite(HiSysEvent::Domain::CAMERA, StatsHiSysEvent::TORCH_STATE, HiSysEvent::EventType::STATISTIC, "PID",
        pid, "UID", uid, "STATE", stateOn);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::CAMERA, StatsHiSysEvent::TORCH_STATE, HiSysEvent::EventType::STATISTIC, "PID",
        pid, "UID", uid, "STATE", stateOff);

    double expectedPower = POWER_CONSUMPTION_DURATION_US * flashlightOnAverageMa / US_PER_HOUR;
    double actualPower = statsClient.GetAppStatsMah(uid);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_016 end");
}

/**
 *
 * @tc.name: BatteryStatsSysTest_017
 * @tc.desc: test Bluetooth and Wifi consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_017, TestSize.Level0)
{
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_017 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double bluetoothBleOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_BLUETOOTH_BLE_ON);
    int32_t stateOn = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_ON);
    int32_t stateOff = static_cast<int32_t>(Bluetooth::BTStateID::STATE_TURN_OFF);
    int32_t uid = 10003;
    int32_t pid = 3458;
    HiSysEventWrite(HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::BT_SERVICE, StatsHiSysEvent::BLE_SWITCH_STATE, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "STATE", stateOff);

    double expectedPower = POWER_CONSUMPTION_DURATION_US * bluetoothBleOnAverageMa / US_PER_HOUR;
    double actualPower = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);

    double wifiOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_WIFI_ON);
    stateOn = static_cast<int32_t>(Wifi::ConnState::CONNECTED);
    stateOff = static_cast<int32_t>(Wifi::ConnState::DISCONNECTED);
    HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_CONNECTION,
        HiSysEvent::EventType::STATISTIC, "TYPE", stateOn);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_CONNECTION,
        HiSysEvent::EventType::STATISTIC, "TYPE", stateOff);

    expectedPower = POWER_CONSUMPTION_DURATION_US * wifiOnAverageMa / US_PER_HOUR;
    actualPower = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_WIFI);
    devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_017 end");
}

/**
 *
 * @tc.name: BatteryStatsSysTest_018
 * @tc.desc: test Flashlight and Camera consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_018, TestSize.Level0)
{
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_018 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double flashlightOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_FLASHLIGHT_ON);
    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = 1;
    int32_t stateOff = 0;

    HiSysEventWrite(HiSysEvent::Domain::CAMERA, StatsHiSysEvent::TORCH_STATE, HiSysEvent::EventType::STATISTIC, "PID",
        pid, "UID", uid, "STATE", stateOn);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::CAMERA, StatsHiSysEvent::TORCH_STATE, HiSysEvent::EventType::STATISTIC, "PID",
        pid, "UID", uid, "STATE", stateOff);

    double expectedPower = POWER_CONSUMPTION_DURATION_US * flashlightOnAverageMa / US_PER_HOUR;
    double actualPower = statsClient.GetAppStatsMah(uid);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);

    double cameraOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_CAMERA_ON);
    uid = 10004;
    pid = 3459;
    std::string deviceId = "Camera0";

    HiSysEventWrite(HiSysEvent::Domain::CAMERA, StatsHiSysEvent::CAMERA_CONNECT, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "ID", deviceId);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::CAMERA, StatsHiSysEvent::CAMERA_DISCONNECT, HiSysEvent::EventType::STATISTIC,
        "ID", deviceId);

    expectedPower = POWER_CONSUMPTION_DURATION_US * cameraOnAverageMa / US_PER_HOUR;
    actualPower = statsClient.GetAppStatsMah(uid);
    devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_018 end");
}

/**
 *
 * @tc.name: BatteryStatsSysTest_019
 * @tc.desc: test Audio, Sensor and Gnss consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_019, TestSize.Level0)
{
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_019 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double audioOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_AUDIO_ON);
    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = 1;
    int32_t stateOff = 0;
    int32_t stateRunning = 2;
    int32_t stateStopped = 3;
    HiSysEventWrite(HiSysEvent::Domain::AUDIO, StatsHiSysEvent::STREAM_CHANGE, HiSysEvent::EventType::BEHAVIOR, "PID",
        pid, "UID", uid, "STATE", stateRunning);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::AUDIO, StatsHiSysEvent::STREAM_CHANGE, HiSysEvent::EventType::BEHAVIOR, "PID",
        pid, "UID", uid, "STATE", stateStopped);

    double expectedPower = POWER_CONSUMPTION_DURATION_US * audioOnAverageMa / US_PER_HOUR;
    double actualPower = statsClient.GetAppStatsMah(uid);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);

    double gnssOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_GNSS_ON);
    uid = 10004;
    pid = 3459;
    std::string gnssStateOn = "start";
    std::string gnssStateOff = "stop";
    HiSysEventWrite(HiSysEvent::Domain::LOCATION, StatsHiSysEvent::GNSS_STATE, HiSysEvent::EventType::STATISTIC, "PID",
        pid, "UID", uid, "STATE", gnssStateOn);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::LOCATION, StatsHiSysEvent::GNSS_STATE, HiSysEvent::EventType::STATISTIC, "PID",
        pid, "UID", uid, "STATE", gnssStateOff);

    expectedPower = POWER_CONSUMPTION_DURATION_US * gnssOnAverageMa / US_PER_HOUR;
    actualPower = statsClient.GetAppStatsMah(uid);
    devPrecent = abs(expectedPower - actualPower) / expectedPower;
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    double sensorGravityOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_SENSOR_GRAVITY);
    uid = 10005;
    pid = 3457;
    HiSysEventWrite(HiSysEvent::Domain::STATS, StatsHiSysEvent::POWER_SENSOR_GRAVITY,
        HiSysEvent::EventType::STATISTIC, "PID", pid, "UID", uid, "STATE", stateOn);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::STATS, StatsHiSysEvent::POWER_SENSOR_GRAVITY,
        HiSysEvent::EventType::STATISTIC, "PID", pid, "UID", uid, "STATE", stateOff);

    expectedPower = POWER_CONSUMPTION_DURATION_US * sensorGravityOnAverageMa / US_PER_HOUR;
    actualPower = statsClient.GetAppStatsMah(uid);
    devPrecent = abs(expectedPower - actualPower) / expectedPower;
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_019 end");
}

/**
 *
 * @tc.name: BatteryStatsSysTest_020
 * @tc.desc: test Phone data and Audio consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_020, TestSize.Level0)
{
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_020 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    int32_t stateOn = 1;
    int32_t stateOff = 0;
    int16_t level = 0;
    double phoneDataAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_RADIO_DATA, level);

    HiSysEventWrite(HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOn);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::TELEPHONY, StatsHiSysEvent::DATA_CONNECTION_STATE,
        HiSysEvent::EventType::BEHAVIOR, "STATE", stateOff);

    double expectedPower = POWER_CONSUMPTION_DURATION_US * phoneDataAverageMa / US_PER_HOUR;
    double actualPower = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_PHONE);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";

    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);

    double audioOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_AUDIO_ON);
    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateRunning = 2;
    int32_t stateStopped = 3;

    HiSysEventWrite(HiSysEvent::Domain::AUDIO, StatsHiSysEvent::STREAM_CHANGE, HiSysEvent::EventType::BEHAVIOR, "PID",
        pid, "UID", uid, "STATE", stateRunning);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::AUDIO, StatsHiSysEvent::STREAM_CHANGE, HiSysEvent::EventType::BEHAVIOR, "PID",
        pid, "UID", uid, "STATE", stateStopped);

    expectedPower = POWER_CONSUMPTION_DURATION_US * audioOnAverageMa / US_PER_HOUR;
    actualPower = statsClient.GetAppStatsMah(uid);
    devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_020 end");
}

/**
 *
 * @tc.name: BatteryStatsSysTest_021
 * @tc.desc: test Idle and consumption, Dump function
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_021, TestSize.Level0)
{
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_021 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double actualPower = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_IDLE);
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";

    EXPECT_TRUE(actualPower >= StatsUtils::DEFAULT_VALUE);

    int32_t uerId = 20003;

    actualPower = statsClient.GetAppStatsMah(uerId);
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_TRUE(actualPower >= StatsUtils::DEFAULT_VALUE);

    std::string result;
    result.clear();
    result = statsClient.Dump(dumpArgs);
    EXPECT_TRUE(result != "");
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_021 end");
}

/**
 *
 * @tc.name: BatteryStatsSysTest_022
 * @tc.desc: test Camera and Camera Flashlight consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_022, TestSize.Level0)
{
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_022 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double cameraOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_CAMERA_ON);
    double flashlightOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_FLASHLIGHT_ON);
    int32_t uid = 10003;
    int32_t pid = 3458;
    std::string cameraId = "Camera0";

    HiSysEventWrite(HiSysEvent::Domain::CAMERA, StatsHiSysEvent::CAMERA_CONNECT, HiSysEvent::EventType::STATISTIC,
        "PID", pid, "UID", uid, "ID", cameraId);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::CAMERA, StatsHiSysEvent::FLASHLIGHT_ON, HiSysEvent::EventType::STATISTIC);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::CAMERA, StatsHiSysEvent::FLASHLIGHT_OFF, HiSysEvent::EventType::STATISTIC);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::CAMERA, StatsHiSysEvent::CAMERA_DISCONNECT, HiSysEvent::EventType::STATISTIC,
        "ID", cameraId);

    double expectedPower = (3 * POWER_CONSUMPTION_DURATION_US * cameraOnAverageMa / US_PER_HOUR) +
        (POWER_CONSUMPTION_DURATION_US * flashlightOnAverageMa / US_PER_HOUR);
    double actualPower = statsClient.GetAppStatsMah(uid);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_022 end");
}

/**
 *
 * @tc.name: BatteryStatsSysTest_023
 * @tc.desc: test GetLastError interface
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_023, TestSize.Level0)
{
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_023 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    StatsError error = statsClient.GetLastError();
    EXPECT_TRUE(error == StatsError::ERR_OK) << "last error is " << static_cast<int32_t>(error);
    STATS_HILOGD(LABEL_TEST, "BatteryStatsSysTest_023 end");
}
