/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "battery_stats_client_test.h"

#include <csignal>
#include <iostream>

#include "hisysevent.h"
#include <if_system_ability_manager.h>
#include <ipc_skeleton.h>
#include <iservice_registry.h>
#include <string_ex.h>
#include <system_ability_definition.h>

#include "battery_stats_client.h"
#include "battery_stats_service.h"
#include "stats_common.h"

using namespace testing::ext;
using namespace OHOS::PowerMgr;
using namespace OHOS;
using namespace std;

void BatteryStatsClientTest::SetUpTestCase(void)
{
}

void BatteryStatsClientTest::TearDownTestCase(void)
{
}

void BatteryStatsClientTest::SetUp(void)
{
}

void BatteryStatsClientTest::TearDown(void)
{
}

/**
 * @tc.name: BatteryStatsClientTest_001
 * @tc.desc: Test BatteryStatsClient ready.
 * @tc.type: FUNC
 */
HWTEST_F (BatteryStatsClientTest, BatteryStatsClientTest_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << " BatteryStatsClientTest_001: test start";
    auto& statsClient = BatteryStatsClient::GetInstance();
    ASSERT_TRUE(statsClient != nullptr) << "BatteryStatsClientTest_001 fail to get BatteryStatsClient";
    GTEST_LOG_(INFO) << " BatteryStatsClientTest_001: test end";
}

/**
 * @tc.name: BatteryStatsClientTest_002
 * @tc.desc: Test Reset Function.
 * @tc.type: FUNC
 */
HWTEST_F (BatteryStatsClientTest, BatteryStatsClientTest_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << " BatteryStatsClientTest_002: test start";
    auto& statsClient = BatteryStatsClient::GetInstance();
    ASSERT_TRUE(statsClient != nullptr) << "BatteryStatsClientTest_002 fail to get BatteryStatsClient";
    GTEST_LOG_(INFO) << " BatteryStatsClientTest_002: test end";
}