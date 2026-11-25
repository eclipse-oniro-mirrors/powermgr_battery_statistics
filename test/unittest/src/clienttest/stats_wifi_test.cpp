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

#include "stats_wifi_test.h"
#include "stats_log.h"

#include <hisysevent.h>
#include "wifi_msg.h"
#include "stats_log.h"

#include "battery_stats_client.h"
#include "stats_hisysevent.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::PowerMgr;
using namespace OHOS;
using namespace std;


void StatsWifiTest::SetUpTestCase()
{
    ParserAveragePowerFile();
    system("hidumper -s 3302 -a -u");
}

void StatsWifiTest::TearDownTestCase()
{
    system("hidumper -s 3302 -a -r");
}

void StatsWifiTest::SetUp()
{
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.SetOnBattery(true);
}

void StatsWifiTest::TearDown()
{
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.SetOnBattery(false);
}

namespace {
/**
 * @tc.name: StatsWifiTest_001
 * @tc.desc: test Reset function(Wifi connection)
 * @tc.type: FUNC
 * @tc.require: issueI5HWJK
 */
HWTEST_F (StatsWifiTest, StatsWifiTest_001, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_001 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    int32_t stateOn = static_cast<int32_t>(Wifi::ConnState::CONNECTED);
    int32_t stateOff = static_cast<int32_t>(Wifi::ConnState::DISCONNECTED);
    HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_CONNECTION,
        HiSysEvent::EventType::STATISTIC, "TYPE", stateOn);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_CONNECTION,
        HiSysEvent::EventType::STATISTIC, "TYPE", stateOff);

    double powerMahBefore = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_WIFI);
    statsClient.Reset();
    double powerMahAfter = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_WIFI);
    GTEST_LOG_(INFO) << __func__ << ": before consumption = " << powerMahBefore << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": after consumption = " << powerMahAfter << " mAh";
    EXPECT_TRUE(powerMahBefore >= StatsUtils::DEFAULT_VALUE && powerMahAfter == StatsUtils::DEFAULT_VALUE);
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_001 end");
}

/**
 * @tc.name: StatsWifiTest_002
 * @tc.desc: test GetPartStatsMah function(Wifi connection)
 * @tc.type: FUNC
 * @tc.require: issueI5HWJK
 */
HWTEST_F (StatsWifiTest, StatsWifiTest_002, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_002 start");
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
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_002 is end");
}

/**
 * @tc.name: StatsWifiTest_003
 * @tc.desc: test GetPartStatsPercent function(Wifi connection)
 * @tc.type: FUNC
 * @tc.require: issueI5HWJK
 */
HWTEST_F (StatsWifiTest, StatsWifiTest_003, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_003 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    int32_t stateOn = static_cast<int32_t>(Wifi::ConnState::CONNECTED);
    int32_t stateOff = static_cast<int32_t>(Wifi::ConnState::DISCONNECTED);
    double fullPercent = 1;
    double zeroPercent = 0;

    HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_CONNECTION,
        HiSysEvent::EventType::STATISTIC, "TYPE", stateOn);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_CONNECTION,
        HiSysEvent::EventType::STATISTIC, "TYPE", stateOff);

    double actualPercent = statsClient.GetPartStatsPercent(BatteryStatsInfo::CONSUMPTION_TYPE_WIFI);
    GTEST_LOG_(INFO) << __func__ << ": actual percent = " << actualPercent;
    EXPECT_TRUE(actualPercent >= zeroPercent && actualPercent <= fullPercent);
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_003 is end");
}

/**
 * @tc.name: StatsWifiTest_004
 * @tc.desc: test GetBatteryStats function
 * @tc.type: FUNC
 * @tc.require: issueI5HWJK
 */
HWTEST_F (StatsWifiTest, StatsWifiTest_004, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_004 start");
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
    double actualPower = StatsUtils::DEFAULT_VALUE;
    auto list = statsClient.GetBatteryStats();
    for (auto it : list) {
        if ((*it).GetConsumptionType() == BatteryStatsInfo::CONSUMPTION_TYPE_WIFI) {
            actualPower = (*it).GetPower();
        }
    }
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_004 is end");
}

/**
 * @tc.name: StatsWifiTest_005
 * @tc.desc: test WIFI_CONNECTION event are sent repeatedly, wifi on power consumption(Wifi connection)
 * @tc.type: FUNC
 * @tc.require: issueI5HWJK
 */
HWTEST_F (StatsWifiTest, StatsWifiTest_005, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_005 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double wifiOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_WIFI_ON);
    int32_t stateOn = static_cast<int32_t>(Wifi::ConnState::CONNECTED);
    int32_t stateOff = static_cast<int32_t>(Wifi::ConnState::DISCONNECTED);

    HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_CONNECTION,
        HiSysEvent::EventType::STATISTIC, "TYPE", stateOn);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_CONNECTION,
        HiSysEvent::EventType::STATISTIC, "TYPE", stateOn);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_CONNECTION,
        HiSysEvent::EventType::STATISTIC, "TYPE", stateOff);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_CONNECTION,
        HiSysEvent::EventType::STATISTIC, "TYPE", stateOff);

    double expectedPower = 2 * POWER_CONSUMPTION_DURATION_US * wifiOnAverageMa / US_PER_HOUR;
    double actualPower = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_WIFI);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_005 is end");
}

/**
 * @tc.name: StatsWifiTest_006
 * @tc.desc: test test GetAppStatsMah function, Wifi connection abnormal state test(Wifi is off)
 * @tc.type: FUNC
 * @tc.require: issueI5HWJK
 */
HWTEST_F (StatsWifiTest, StatsWifiTest_006, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_006 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    int32_t stateOn = 3;
    int32_t stateOff = static_cast<int32_t>(Wifi::ConnState::DISCONNECTED);

    HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_CONNECTION,
        HiSysEvent::EventType::STATISTIC, "TYPE", stateOn);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_CONNECTION,
        HiSysEvent::EventType::STATISTIC, "TYPE", stateOff);

    double expectedPower = StatsUtils::DEFAULT_VALUE;
    double actualPower = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_WIFI);
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_EQ(expectedPower, actualPower);
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_006 is end");
}

/**
 * @tc.name: StatsWifiTest_007
 * @tc.desc: test test GetAppStatsMah function, Wifi connection abnormal state test(Wifi is on)
 * @tc.type: FUNC
 * @tc.require: issueI5HWJK
 */
HWTEST_F (StatsWifiTest, StatsWifiTest_007, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_007 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double wifiOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_WIFI_ON);
    int32_t stateOn = static_cast<int32_t>(Wifi::ConnState::CONNECTED);
    int32_t stateOff = static_cast<int32_t>(Wifi::ConnState::DISCONNECTED);
    int32_t stateInvaildOn = 5;
    int32_t stateInvaildOff = -1;

    HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_CONNECTION,
        HiSysEvent::EventType::STATISTIC, "TYPE", stateOn);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_CONNECTION,
        HiSysEvent::EventType::STATISTIC, "TYPE", stateInvaildOn);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_CONNECTION,
        HiSysEvent::EventType::STATISTIC, "TYPE", stateInvaildOff);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_CONNECTION,
        HiSysEvent::EventType::STATISTIC, "TYPE", stateOff);

    double expectedPower = 3 * POWER_CONSUMPTION_DURATION_US * wifiOnAverageMa / US_PER_HOUR;
    double actualPower = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_WIFI);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_007 is end");
}

/**
 * @tc.name: StatsWifiTest_008
 * @tc.desc: test GetTotalTimeSecond function(Wifi connection)
 * @tc.type: FUNC
 * @tc.require: issueI5HWJK
 */
HWTEST_F (StatsWifiTest, StatsWifiTest_008, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_008 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    int32_t stateOn = static_cast<int32_t>(Wifi::ConnState::CONNECTED);
    int32_t stateOff = static_cast<int32_t>(Wifi::ConnState::DISCONNECTED);

    HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_CONNECTION,
        HiSysEvent::EventType::STATISTIC, "TYPE", stateOn);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_CONNECTION,
        HiSysEvent::EventType::STATISTIC, "TYPE", stateOff);

    long expectedTime = round(POWER_CONSUMPTION_DURATION_US / US_PER_SECOND);
    long actualTime = statsClient.GetTotalTimeSecond(StatsUtils::STATS_TYPE_WIFI_ON);
    GTEST_LOG_(INFO) << __func__ << ": expected time = " << expectedTime << " seconds";
    GTEST_LOG_(INFO) << __func__ << ": actual time = " <<  actualTime << " seconds";
    EXPECT_EQ(expectedTime, actualTime);
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_008 is end");
}

/**
 * @tc.name: StatsWifiTest_09
 * @tc.desc: test Reset function(Wifi scan)
 * @tc.type: FUNC
 * @tc.require: issueI5HWJK
 */
HWTEST_F (StatsWifiTest, StatsWifiTest_09, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_009 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    int16_t count = 2;

    for (int16_t i = 0; i < count; i++) {
        HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_SCAN,
            HiSysEvent::EventType::STATISTIC);
        usleep(POWER_CONSUMPTION_TRIGGERED_US);
    }

    double powerMahBefore = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_WIFI);
    statsClient.Reset();
    double powerMahAfter = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_WIFI);
    GTEST_LOG_(INFO) << __func__ << ": before consumption = " << powerMahBefore << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": after consumption = " << powerMahAfter << " mAh";
    EXPECT_TRUE(powerMahBefore >= StatsUtils::DEFAULT_VALUE && powerMahAfter >= StatsUtils::DEFAULT_VALUE);
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_009 is end");
}

/**
 * @tc.name: StatsWifiTest_010
 * @tc.desc: test GetPartStatsMah function(Wifi scan)
 * @tc.type: FUNC
 * @tc.require: issueI5HWJK
 */
HWTEST_F (StatsWifiTest, StatsWifiTest_010, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_010 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double wifiScanAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_WIFI_SCAN);
    int16_t count = 2;

    for (int16_t i = 0; i < count; i++) {
        HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_SCAN,
            HiSysEvent::EventType::STATISTIC);
        usleep(POWER_CONSUMPTION_TRIGGERED_US);
    }

    double expectedPower = count * wifiScanAverageMa;
    double actualPower = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_WIFI);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_010 is end");
}

/**
 * @tc.name: StatsWifiTest_011
 * @tc.desc: test GetPartStatsPercent function(Wifi scan)
 * @tc.type: FUNC
 * @tc.require: issueI5HWJK
 */
HWTEST_F (StatsWifiTest, StatsWifiTest_011, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_011 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double fullPercent = 1;
    double zeroPercent = 0;
    int16_t count = 2;

    for (int16_t i = 0; i < count; i++) {
        HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_SCAN,
            HiSysEvent::EventType::STATISTIC);
        usleep(POWER_CONSUMPTION_TRIGGERED_US);
    }

    double actualPercent = statsClient.GetPartStatsPercent(BatteryStatsInfo::CONSUMPTION_TYPE_WIFI);
    GTEST_LOG_(INFO) << __func__ << ": actual percent = " << actualPercent;
    EXPECT_TRUE(actualPercent >= zeroPercent && actualPercent <= fullPercent);
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_011 is end");
}

/**
 * @tc.name: StatsWifiTest_012
 * @tc.desc: test GetBatteryStats function(Wifi scan)
 * @tc.type: FUNC
 * @tc.require: issueI5HWJK
 */
HWTEST_F (StatsWifiTest, StatsWifiTest_012, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_012 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double wifiScanAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_WIFI_SCAN);
    int16_t count = 2;

    for (int16_t i = 0; i < count; i++) {
        HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_SCAN,
            HiSysEvent::EventType::STATISTIC);
        usleep(POWER_CONSUMPTION_TRIGGERED_US);
    }

    double expectedPower = count * wifiScanAverageMa;
    double actualPower = StatsUtils::DEFAULT_VALUE;
    auto list = statsClient.GetBatteryStats();
    for (auto it : list) {
        if ((*it).GetConsumptionType() == BatteryStatsInfo::CONSUMPTION_TYPE_WIFI) {
            actualPower = (*it).GetPower();
        }
    }
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_012 is end");
}

/**
 * @tc.name: StatsWifiTest_013
 * @tc.desc: test GetTotalDataBytes function (Wifi scan)
 * @tc.type: FUNC
 * @tc.require: issueI5HWJK
 */
HWTEST_F (StatsWifiTest, StatsWifiTest_013, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_013 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    int16_t count = 2;
    for (int16_t i = 0; i < count; i++) {
        HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_SCAN,
            HiSysEvent::EventType::STATISTIC);
        usleep(POWER_CONSUMPTION_TRIGGERED_US);
    }

    long expectValue = StatsUtils::DEFAULT_VALUE;
    long data = statsClient.GetTotalDataBytes(StatsUtils::STATS_TYPE_WIFI_SCAN);
    EXPECT_EQ(data, expectValue);
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_013 is end");
}

/**
 * @tc.name: StatsWifiTest_014
 * @tc.desc: test GetPartStatsMah function(Wifi connection & Wifi scan)
 * @tc.type: FUNC
 * @tc.require: issueI5HWJK
 */
HWTEST_F (StatsWifiTest, StatsWifiTest_014, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_014 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double wifiOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_WIFI_ON);
    double wifiScanAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_WIFI_SCAN);
    int32_t stateOn = static_cast<int32_t>(Wifi::ConnState::CONNECTED);
    int32_t stateOff = static_cast<int32_t>(Wifi::ConnState::DISCONNECTED);
    int16_t count = 2;

    HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_CONNECTION,
        HiSysEvent::EventType::STATISTIC, "TYPE", stateOn);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_CONNECTION,
        HiSysEvent::EventType::STATISTIC, "TYPE", stateOff);

    for (int16_t i = 0; i < count; i++) {
        HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_SCAN,
            HiSysEvent::EventType::STATISTIC);
        usleep(POWER_CONSUMPTION_TRIGGERED_US);
    }

    double wifiOnPower = POWER_CONSUMPTION_DURATION_US * wifiOnAverageMa / US_PER_HOUR;
    double wifiScanPower = count * wifiScanAverageMa;

    double expectedPower = wifiOnPower + wifiScanPower;
    double actualPower = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_WIFI);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_014 is end");
}


/**
 * @tc.name: StatsWifiTest_015
 * @tc.desc: test SetOnBattery function(Wifi connection & Wifi scan)
 * @tc.type: FUNC
 * @tc.require: issueI5HWJK
 */
HWTEST_F (StatsWifiTest, StatsWifiTest_015, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_015 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();
    statsClient.SetOnBattery(false);

    int32_t stateOn = static_cast<int32_t>(Wifi::ConnState::CONNECTED);
    int32_t stateOff = static_cast<int32_t>(Wifi::ConnState::DISCONNECTED);
    int16_t count = 2;

    HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_CONNECTION,
        HiSysEvent::EventType::STATISTIC, "TYPE", stateOn);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_CONNECTION,
        HiSysEvent::EventType::STATISTIC, "TYPE", stateOff);

    for (int16_t i = 0; i < count; i++) {
        HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_SCAN,
            HiSysEvent::EventType::STATISTIC);
        usleep(POWER_CONSUMPTION_TRIGGERED_US);
    }

    double expectedPower = StatsUtils::DEFAULT_VALUE;
    double actualPower = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_WIFI);
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_EQ(expectedPower, actualPower);
    statsClient.SetOnBattery(true);
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_015 is end");
}

/**
 * @tc.name: StatsWifiTest_016
 * @tc.desc: test GetPartStatsMah function with battery changed(Wifi connection)
 * @tc.type: FUNC
 * @tc.require: issueI5HWJK
 */
HWTEST_F (StatsWifiTest, StatsWifiTest_016, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_016 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double wifiOnAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_WIFI_ON);
    int32_t stateOn = static_cast<int32_t>(Wifi::ConnState::CONNECTED);
    int32_t stateOff = static_cast<int32_t>(Wifi::ConnState::DISCONNECTED);

    HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_CONNECTION,
        HiSysEvent::EventType::STATISTIC, "TYPE", stateOn);
    usleep(POWER_CONSUMPTION_DURATION_US);
    statsClient.SetOnBattery(false);
    usleep(POWER_CONSUMPTION_DURATION_US);
    statsClient.SetOnBattery(true);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_CONNECTION,
        HiSysEvent::EventType::STATISTIC, "TYPE", stateOff);

    double expectedPower = 2 * POWER_CONSUMPTION_DURATION_US * wifiOnAverageMa / US_PER_HOUR;
    double actualPower = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_WIFI);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_016 is end");
}

/**
 * @tc.name: StatsWifiTest_017
 * @tc.desc: test GetPartStatsMah function with battery changed(Wifi scan)
 * @tc.type: FUNC
 * @tc.require: issueI5HWJK
 */
HWTEST_F (StatsWifiTest, StatsWifiTest_017, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_017 is start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double wifiScanAverageMa = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_WIFI_SCAN);
    int16_t totalCount = 3;
    int16_t delayCount = 1;
    int16_t startDelayPos = 1;

    for (int16_t i = 0; i < totalCount; i++) {
        auto ret =  HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_SCAN,
            HiSysEvent::EventType::STATISTIC);
        STATS_HILOGI(LABEL_TEST, "HiSysEventWrite return: %{public}d", ret);
        usleep(POWER_CONSUMPTION_TRIGGERED_US);
        if (i == startDelayPos) {
            statsClient.SetOnBattery(false);
            usleep(POWER_CONSUMPTION_TRIGGERED_US);
        } else if (i == startDelayPos + delayCount)
        {
            statsClient.SetOnBattery(true);
            usleep(POWER_CONSUMPTION_TRIGGERED_US);
        }
    }

    double expectedPower = (totalCount - delayCount) * wifiScanAverageMa;
    double actualPower = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_WIFI);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_017 is end");
}

/**
 * @tc.name: StatsWifiTest_018
 * @tc.desc: test GetPartStatsPercent(Wifi on) and GetAppStatsPercent(Camera) function
 * @tc.type: FUNC
 * @tc.require: issueI5HWJK
 */
HWTEST_F (StatsWifiTest, StatsWifiTest_018, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_018 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    int32_t stateOn = static_cast<int32_t>(Wifi::ConnState::CONNECTED);
    int32_t stateOff = static_cast<int32_t>(Wifi::ConnState::DISCONNECTED);
    double fullPercent = 1;
    double zeroPercent = 0;

    HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_CONNECTION,
        HiSysEvent::EventType::STATISTIC, "TYPE", stateOn);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::COMMUNICATION, StatsHiSysEvent::WIFI_CONNECTION,
        HiSysEvent::EventType::STATISTIC, "TYPE", stateOff);

    double actualPercent = statsClient.GetPartStatsPercent(BatteryStatsInfo::CONSUMPTION_TYPE_WIFI);
    GTEST_LOG_(INFO) << __func__ << ": actual percent = " << actualPercent;
    EXPECT_TRUE(actualPercent >= zeroPercent && actualPercent <= fullPercent);

    int32_t uid = 10003;
    int32_t pid = 3458;
    std::string deviceId = "Camera0";

    HiSysEventWrite(HiSysEvent::Domain::CAMERA, StatsHiSysEvent::CAMERA_CONNECT,
        HiSysEvent::EventType::STATISTIC, "PID", pid, "UID", uid, "ID", deviceId);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::CAMERA, StatsHiSysEvent::CAMERA_DISCONNECT,
        HiSysEvent::EventType::STATISTIC, "ID", deviceId);
    actualPercent = statsClient.GetAppStatsPercent(uid);
    GTEST_LOG_(INFO) << __func__ << ": actual percent = " << actualPercent;
    EXPECT_TRUE(actualPercent >= zeroPercent && actualPercent <= fullPercent);
    STATS_HILOGI(LABEL_TEST, "StatsWifiTest_018 is end");
}
}