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

#include "stats_display_test.h"
#include "stats_log.h"

#include <display_power_info.h>
#include <hisysevent.h>

#include "stats_log.h"
#include "stats_hisysevent.h"
#include "battery_stats_client.h"
#include "power_mgr_client.h"

using namespace testing::ext;
using namespace OHOS::HiviewDFX;
using namespace OHOS::PowerMgr;
using namespace std;


static void SetLastBrightness(int32_t lastBrightness)
{
    int32_t stateOn = static_cast<int32_t>(OHOS::DisplayPowerMgr::DisplayState::DISPLAY_ON);
    int32_t stateOff = static_cast<int32_t>(OHOS::DisplayPowerMgr::DisplayState::DISPLAY_OFF);

    GTEST_LOG_(INFO) << __func__ << ": Set last screen brightness value = " << lastBrightness;
    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::SCREEN_STATE,
        HiSysEvent::EventType::STATISTIC, "STATE", stateOn);
    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::BRIGHTNESS_NIT,
        HiSysEvent::EventType::STATISTIC, "BRIGHTNESS", lastBrightness);
    usleep(StatsTest::POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::SCREEN_STATE,
        HiSysEvent::EventType::STATISTIC, "STATE", stateOff);

    auto& statsClient = BatteryStatsClient::GetInstance();
    GTEST_LOG_(INFO) << __func__ << ": Battery stats client reset";
    statsClient.Reset();
}

void StatsDisplayTest::SetUpTestCase()
{
    ParserAveragePowerFile();
    system("hidumper -s 3302 -a -u");
    auto& pms = PowerMgrClient::GetInstance();
    pms.SuspendDevice();
}

void StatsDisplayTest::TearDownTestCase()
{
    system("hidumper -s 3302 -a -r");
}

void StatsDisplayTest::SetUp()
{
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.SetOnBattery(true);
}

void StatsDisplayTest::TearDown()
{
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.SetOnBattery(false);
}

namespace {
/**
 * @tc.name: StatsDisplayTest_001
 * @tc.desc: test Reset function(Screen)
 * @tc.type: FUNC
 */
HWTEST_F (StatsDisplayTest, StatsDisplayTest_001, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsDisplayTest_001 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    int32_t stateOn = static_cast<int32_t>(OHOS::DisplayPowerMgr::DisplayState::DISPLAY_ON);
    int32_t stateOff = static_cast<int32_t>(OHOS::DisplayPowerMgr::DisplayState::DISPLAY_OFF);
    int32_t brightness = 100;

    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::SCREEN_STATE,
        HiSysEvent::EventType::STATISTIC, "STATE", stateOn);
    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::BRIGHTNESS_NIT,
        HiSysEvent::EventType::STATISTIC, "BRIGHTNESS", brightness);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::SCREEN_STATE,
        HiSysEvent::EventType::STATISTIC, "STATE", stateOff);

    double powerMahBefore = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_SCREEN);
    statsClient.Reset();
    double powerMahAfter = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_SCREEN);
    GTEST_LOG_(INFO) << __func__ << ": before consumption = " << powerMahBefore << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": after consumption = " << powerMahAfter << " mAh";
    EXPECT_TRUE(powerMahBefore >= StatsUtils::DEFAULT_VALUE && powerMahAfter == StatsUtils::DEFAULT_VALUE);
    STATS_HILOGI(LABEL_TEST, "StatsDisplayTest_001 end");
}

/**
 * @tc.name: StatsDisplayTest_002
 * @tc.desc: test SetOnBattery function(Screen)
 * @tc.type: FUNC
 */
HWTEST_F (StatsDisplayTest, StatsDisplayTest_002, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsDisplayTest_002 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();
    statsClient.SetOnBattery(false);

    int32_t stateOn = static_cast<int32_t>(OHOS::DisplayPowerMgr::DisplayState::DISPLAY_ON);
    int32_t stateOff = static_cast<int32_t>(OHOS::DisplayPowerMgr::DisplayState::DISPLAY_OFF);
    int32_t brightness = 120;

    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::SCREEN_STATE,
        HiSysEvent::EventType::STATISTIC, "STATE", stateOn);
    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::BRIGHTNESS_NIT,
        HiSysEvent::EventType::STATISTIC, "BRIGHTNESS", brightness);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::SCREEN_STATE,
        HiSysEvent::EventType::STATISTIC, "STATE", stateOff);

    double expectedPower = StatsUtils::DEFAULT_VALUE;
    double actualPower = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_SCREEN);
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_EQ(expectedPower, actualPower);
    statsClient.SetOnBattery(true);
    STATS_HILOGI(LABEL_TEST, "StatsDisplayTest_002 end");
}

/**
 * @tc.name: StatsDisplayTest_003
 * @tc.desc: test GetPartStatsMah function(Screen)
 * @tc.type: FUNC
 */
HWTEST_F (StatsDisplayTest, StatsDisplayTest_003, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsDisplayTest_003 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    int32_t stateOn = static_cast<int32_t>(OHOS::DisplayPowerMgr::DisplayState::DISPLAY_ON);
    int32_t stateOff = static_cast<int32_t>(OHOS::DisplayPowerMgr::DisplayState::DISPLAY_OFF);
    int32_t brightness = 100;
    double screenOnAverage = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_SCREEN_ON);
    double screenBrightnessAverage = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_SCREEN_BRIGHTNESS);

    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::SCREEN_STATE,
        HiSysEvent::EventType::STATISTIC, "STATE", stateOn);
    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::BRIGHTNESS_NIT,
        HiSysEvent::EventType::STATISTIC, "BRIGHTNESS", brightness);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::SCREEN_STATE,
        HiSysEvent::EventType::STATISTIC, "STATE", stateOff);

    double average = screenBrightnessAverage * brightness + screenOnAverage;

    double expectedPower = average * POWER_CONSUMPTION_DURATION_US / US_PER_HOUR;
    double actualPower = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_SCREEN);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsDisplayTest_003 end");
}

/**
 * @tc.name: StatsDisplayTest_004
 * @tc.desc: test GetPartStatsPercent function(Screen)
 * @tc.type: FUNC
 */
HWTEST_F (StatsDisplayTest, StatsDisplayTest_004, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsDisplayTest_004 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    int32_t stateOn = static_cast<int32_t>(OHOS::DisplayPowerMgr::DisplayState::DISPLAY_ON);
    int32_t stateOff = static_cast<int32_t>(OHOS::DisplayPowerMgr::DisplayState::DISPLAY_OFF);
    int32_t brightness = 100;
    double fullPercent = 1;
    double zeroPercent = 0;

    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::SCREEN_STATE,
        HiSysEvent::EventType::STATISTIC, "STATE", stateOn);
    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::BRIGHTNESS_NIT,
        HiSysEvent::EventType::STATISTIC, "BRIGHTNESS", brightness);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::SCREEN_STATE,
        HiSysEvent::EventType::STATISTIC, "STATE", stateOff);

    double actualPercent = statsClient.GetPartStatsPercent(BatteryStatsInfo::CONSUMPTION_TYPE_SCREEN);
    GTEST_LOG_(INFO) << __func__ << ": actual percent = " << actualPercent;
    EXPECT_TRUE(actualPercent >= zeroPercent && actualPercent <= fullPercent);
    STATS_HILOGI(LABEL_TEST, "StatsDisplayTest_004 end");
}

/**
 * @tc.name: StatsDisplayTest_005
 * @tc.desc: test GetBatteryStats function(Screen)
 * @tc.type: FUNC
 */
HWTEST_F (StatsDisplayTest, StatsDisplayTest_005, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsDisplayTest_005 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    int32_t stateOn = static_cast<int32_t>(OHOS::DisplayPowerMgr::DisplayState::DISPLAY_ON);
    int32_t stateOff = static_cast<int32_t>(OHOS::DisplayPowerMgr::DisplayState::DISPLAY_OFF);
    int32_t brightness = 100;
    double screenOnAverage = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_SCREEN_ON);
    double screenBrightnessAverage = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_SCREEN_BRIGHTNESS);

    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::SCREEN_STATE,
        HiSysEvent::EventType::STATISTIC, "STATE", stateOn);
    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::BRIGHTNESS_NIT,
        HiSysEvent::EventType::STATISTIC, "BRIGHTNESS", brightness);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::SCREEN_STATE,
        HiSysEvent::EventType::STATISTIC, "STATE", stateOff);

    double average = screenBrightnessAverage * brightness + screenOnAverage;

    double expectedPower = average * POWER_CONSUMPTION_DURATION_US / US_PER_HOUR;
    double actualPower = StatsUtils::DEFAULT_VALUE;
    auto list = statsClient.GetBatteryStats();
    for (auto it : list) {
        if ((*it).GetConsumptionType() == BatteryStatsInfo::CONSUMPTION_TYPE_SCREEN) {
            actualPower = (*it).GetPower();
        }
    }
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsDisplayTest_005 end");
}

/**
 * @tc.name: StatsDisplayTest_006
 * @tc.desc: test Last brightness value exists, but no BRIGHTNESS_NIT event notification, screen power consumption
 * @tc.type: FUNC
 */
HWTEST_F (StatsDisplayTest, StatsDisplayTest_006, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsDisplayTest_006 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    int32_t stateOn = static_cast<int32_t>(OHOS::DisplayPowerMgr::DisplayState::DISPLAY_ON);
    int32_t stateOff = static_cast<int32_t>(OHOS::DisplayPowerMgr::DisplayState::DISPLAY_OFF);
    int32_t lastBrightness = 100;
    double screenOnAverage = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_SCREEN_ON);
    double screenBrightnessAverage = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_SCREEN_BRIGHTNESS);

    SetLastBrightness(lastBrightness);

    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::SCREEN_STATE,
        HiSysEvent::EventType::STATISTIC, "STATE", stateOn);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::SCREEN_STATE,
        HiSysEvent::EventType::STATISTIC, "STATE", stateOff);

    double average = screenBrightnessAverage * lastBrightness + screenOnAverage;

    double expectedPower = average * POWER_CONSUMPTION_DURATION_US / US_PER_HOUR;
    double actualPower = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_SCREEN);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsDisplayTest_006 end");
}

/**
 * @tc.name: StatsDisplayTest_007
 * @tc.desc: test SCREEN_STATE event are sent repeatedly, screen power consumption
 * @tc.type: FUNC
 */
HWTEST_F (StatsDisplayTest, StatsDisplayTest_007, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsDisplayTest_007 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    int32_t stateOn = static_cast<int32_t>(OHOS::DisplayPowerMgr::DisplayState::DISPLAY_ON);
    int32_t stateOff = static_cast<int32_t>(OHOS::DisplayPowerMgr::DisplayState::DISPLAY_OFF);
    int32_t lastBrightness = 100;
    double screenOnAverage = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_SCREEN_ON);
    double screenBrightnessAverage = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_SCREEN_BRIGHTNESS);

    SetLastBrightness(lastBrightness);

    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::SCREEN_STATE,
        HiSysEvent::EventType::STATISTIC, "STATE", stateOn);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::SCREEN_STATE,
        HiSysEvent::EventType::STATISTIC, "STATE", stateOn);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::SCREEN_STATE,
        HiSysEvent::EventType::STATISTIC, "STATE", stateOff);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::SCREEN_STATE,
        HiSysEvent::EventType::STATISTIC, "STATE", stateOff);

    double average = screenBrightnessAverage * lastBrightness + screenOnAverage;

    double expectedPower = average * 2 * POWER_CONSUMPTION_DURATION_US / US_PER_HOUR;
    double actualPower = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_SCREEN);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsDisplayTest_007 end");
}

/**
 * @tc.name: StatsDisplayTest_008
 * @tc.desc: test Screen is off, BRIGHTNESS_NIT event is invalid
 * @tc.type: FUNC
 */
HWTEST_F (StatsDisplayTest, StatsDisplayTest_008, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsDisplayTest_008 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    int32_t brightness = 100;

    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::BRIGHTNESS_NIT,
        HiSysEvent::EventType::STATISTIC, "BRIGHTNESS", brightness);
    usleep(POWER_CONSUMPTION_DURATION_US);

    double expectedPower = StatsUtils::DEFAULT_VALUE;
    double actualPower = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_SCREEN);
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_EQ(expectedPower, actualPower);
    STATS_HILOGI(LABEL_TEST, "StatsDisplayTest_008 end");
}

/**
 * @tc.name: StatsDisplayTest_009
 * @tc.desc: test Screen is off, BRIGHTNESS_NIT event is invalid
 * @tc.type: FUNC
 */
HWTEST_F (StatsDisplayTest, StatsDisplayTest_009, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsDisplayTest_009 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    int32_t stateOn = static_cast<int32_t>(OHOS::DisplayPowerMgr::DisplayState::DISPLAY_ON);
    int32_t stateOff = static_cast<int32_t>(OHOS::DisplayPowerMgr::DisplayState::DISPLAY_OFF);
    int32_t lastBrightness = 100;
    int32_t currentBrightness = 200;
    double screenOnAverage = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_SCREEN_ON);
    double screenBrightnessAverage = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_SCREEN_BRIGHTNESS);

    SetLastBrightness(lastBrightness);

    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::BRIGHTNESS_NIT,
        HiSysEvent::EventType::STATISTIC, "BRIGHTNESS", currentBrightness);
    usleep(POWER_CONSUMPTION_DURATION_US);

    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::SCREEN_STATE,
        HiSysEvent::EventType::STATISTIC, "STATE", stateOn);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::SCREEN_STATE,
        HiSysEvent::EventType::STATISTIC, "STATE", stateOff);

    double average = screenBrightnessAverage * lastBrightness + screenOnAverage;

    double expectedPower = average * POWER_CONSUMPTION_DURATION_US / US_PER_HOUR;
    double actualPower = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_SCREEN);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsDisplayTest_009 end");
}

/**
 * @tc.name: StatsDisplayTest_010
 * @tc.desc: test Screen is on, BRIGHTNESS_NIT event are sent delay
 * @tc.type: FUNC
 */
HWTEST_F (StatsDisplayTest, StatsDisplayTest_010, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsDisplayTest_010 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    int32_t stateOn = static_cast<int32_t>(OHOS::DisplayPowerMgr::DisplayState::DISPLAY_ON);
    int32_t stateOff = static_cast<int32_t>(OHOS::DisplayPowerMgr::DisplayState::DISPLAY_OFF);
    int32_t lastBrightness = 100;
    int32_t currentBrightness = 200;
    double screenOnAverage = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_SCREEN_ON);
    double screenBrightnessAverage = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_SCREEN_BRIGHTNESS);

    SetLastBrightness(lastBrightness);

    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::SCREEN_STATE,
        HiSysEvent::EventType::STATISTIC, "STATE", stateOn);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::BRIGHTNESS_NIT,
        HiSysEvent::EventType::STATISTIC, "BRIGHTNESS", currentBrightness);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::SCREEN_STATE,
        HiSysEvent::EventType::STATISTIC, "STATE", stateOff);

    double screenOnPower = screenOnAverage * 2 * POWER_CONSUMPTION_DURATION_US;
    double lastBrightnessPower = screenBrightnessAverage * lastBrightness * POWER_CONSUMPTION_DURATION_US;
    double curBrightnessPower = screenBrightnessAverage * currentBrightness * POWER_CONSUMPTION_DURATION_US;

    double expectedPower = (screenOnPower + lastBrightnessPower + curBrightnessPower) / US_PER_HOUR;
    double actualPower = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_SCREEN);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsDisplayTest_010 end");
}

/**
 * @tc.name: StatsDisplayTest_011
 * @tc.desc: test Screen is on, BRIGHTNESS_NIT event are sent with invalid value
 * @tc.type: FUNC
 */
HWTEST_F (StatsDisplayTest, StatsDisplayTest_011, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsDisplayTest_011 start");
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    int32_t stateOn = static_cast<int32_t>(OHOS::DisplayPowerMgr::DisplayState::DISPLAY_ON);
    int32_t stateOff = static_cast<int32_t>(OHOS::DisplayPowerMgr::DisplayState::DISPLAY_OFF);
    int32_t lastBrightness = 100;
    int32_t invalidBrightness1 = -1;
    int32_t invalidBrightness2 = 300;
    double screenOnAverage = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_SCREEN_ON);
    double screenBrightnessAverage = g_statsParser->GetAveragePowerMa(StatsUtils::CURRENT_SCREEN_BRIGHTNESS);

    SetLastBrightness(lastBrightness);

    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::SCREEN_STATE,
        HiSysEvent::EventType::STATISTIC, "STATE", stateOn);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::BRIGHTNESS_NIT,
        HiSysEvent::EventType::STATISTIC, "BRIGHTNESS", invalidBrightness1);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::BRIGHTNESS_NIT,
        HiSysEvent::EventType::STATISTIC, "BRIGHTNESS", invalidBrightness2);
    usleep(POWER_CONSUMPTION_DURATION_US);
    HiSysEventWrite(HiSysEvent::Domain::DISPLAY, StatsHiSysEvent::SCREEN_STATE,
        HiSysEvent::EventType::STATISTIC, "STATE", stateOff);

    double average = screenBrightnessAverage * lastBrightness + screenOnAverage;

    double expectedPower = average * 3 * POWER_CONSUMPTION_DURATION_US / US_PER_HOUR;
    double actualPower = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_SCREEN);
    double devPrecent = abs(expectedPower - actualPower) / expectedPower;
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(devPrecent, DEVIATION_PERCENT_THRESHOLD);
    STATS_HILOGI(LABEL_TEST, "StatsDisplayTest_011 end");
}
}