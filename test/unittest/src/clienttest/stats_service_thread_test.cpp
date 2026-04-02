/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "stats_service_thread_test.h"
#include "stats_log.h"

#include <if_system_ability_manager.h>
#include <iservice_registry.h>
#include <system_ability_definition.h>
#include "battery_stats_parser.h"
#define private public
#define protected public
#include "battery_stats_service.h"
#undef private
#undef protected
#include "config_policy_utils.h"
#include "gtest/gtest-message.h"
#include "gtest/gtest-test-part.h"
#include "gtest/gtest.h"
#include "gtest/hwext/gtest-ext.h"
#include "gtest/hwext/gtest-tag.h"
#include "iremote_object.h"
#include "ostream"
#include "refbase.h"
#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <random>
#include <memory>

using namespace testing::ext;
using namespace OHOS::PowerMgr;
using namespace OHOS;

static sptr<BatteryStatsService> g_batteryStatsService = nullptr;

struct TestResult {
    std::atomic<int32_t> success{0};
    std::atomic<int32_t> failure{0};
    std::atomic<int32_t> total{0};
    uint64_t startTime{0};
    uint64_t endTime{0};
    uint64_t totalBytes{0};
};

static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_int_distribution<> dis_type(0, 3);
static std::uniform_int_distribution<> dis_uid(1000, 9999);

void StatsServiceThreadTest::SetUpTestCase()
{
    GTEST_LOG_(INFO) << __func__;
}

void StatsServiceThreadTest::TearDownTestCase()
{
    GTEST_LOG_(INFO) << __func__;
}

void StatsServiceThreadTest::SetUp()
{
    GTEST_LOG_(INFO) << __func__;
}

void StatsServiceThreadTest::TearDown()
{
    GTEST_LOG_(INFO) << __func__;
}

namespace {
/**
 * @tc.name: StatsServiceThreadTest_001
 * @tc.desc: test GetTotalDataBytes, GetTotalTimeSecond, UpdataStats(StatsStat), UpdataStats(time,data) concurrent calls
 * @tc.type: FUNC
 */
HWTEST_F (StatsServiceThreadTest, StatsServiceThreadTest_001, TestSize.Level0)
{
    STATS_HILOGI(LABEL_TEST, "StatsServiceThreadTest_001 start");
    
    g_batteryStatsService = OHOS::PowerMgr::BatteryStatsService::GetInstance();
    ASSERT_NE(g_batteryStatsService, nullptr) << "g_batteryStatsService instance is null";

    g_batteryStatsService->OnStart();

    // 3. 设置并发参数
    const int32_t totalWorkers = 100;
    const int32_t callsPerWorker = 100;
    const int32_t totalCalls = totalWorkers*callsPerWorker;

    // 4. 测试GetTotalDataBytes并发调用
    {
        STATS_HILOGI(LABEL_TEST, "Starting GetTotalDataBytes concurrent test...");

        TestResult dataResult;
        dataResult.total = totalCalls;
        dataResult.startTime = std::chrono::steady_clock::now().time_since_epoch().count();

        std::vector<std::thread> dataThreads;
        dataThreads.reserve(totalWorkers);

        for (int i = 0; i < totalWorkers; ++i) {
            dataThreads.emplace_back([&, i](int32_t workerId) {
                for (int j = 0; j < callsPerWorker; ++j) {
                    int32_t statsType = dis_type(gen);
                    int32_t uid = dis_uid(gen);

                    int32_t ret = g_batteryStatsService->GetTotalDataBytes(
                        static_cast<OHOS::PowerMgr::StatsUtils::StatsType>(statsType),
                        uid
                    );

                    if (ret == 0) {
                        dataResult.success++;
                    } else {
                        dataResult.failure++;
                    }
                }
            }, i);
        }

        // 等待所有线程完成
        for (auto& t : dataThreads) {
            if (t.joinable()) {
                t.join();
            }
        }

        dataResult.endTime = std::chrono::steady_clock::now().time_since_epoch().count();

        EXPECT_EQ(dataResult.failure, 0) << "Expected 0 failures in GetTotalDataBytes, but got " << dataResult.failure;
        EXPECT_GT(dataResult.success, 0) << "Expected at least one successful call in GetTotalDataBytes";
        STATS_HILOGI(LABEL_TEST, "GetTotalDataBytes concurrent test completed.");
    }

    // 5. 测试GetTotalTimeSecond并发调用
    {
        STATS_HILOGI(LABEL_TEST, "Starting GetTotalTimeSecond concurrent test...");

        TestResult timeResult;
        timeResult.total = totalCalls;
        timeResult.startTime = std::chrono::steady_clock::now().time_since_epoch().count();

        std::vector<std::thread> timeThreads;
        timeThreads.reserve(totalWorkers);

        for (int i = 0; i < totalWorkers; ++i) {
            timeThreads.emplace_back([&, i](int32_t workerId) {
                for (int j = 0; j < callsPerWorker; ++j) {
                    int32_t statsType = dis_type(gen);
                    int32_t uid = dis_uid(gen);

                    int32_t ret = g_batteryStatsService->GetTotalTimeSecond(
                        static_cast<OHOS::PowerMgr::StatsUtils::StatsType>(statsType),
                        uid
                    );

                    if (ret == 0) {
                        timeResult.success++;
                    } else {
                        timeResult.failure++;
                    }
                }
            }, i);
        }

        // 等待所有线程完成
        for (auto& t : timeThreads) {
            if (t.joinable()) {
                t.join();
            }
        }

        timeResult.endTime = std::chrono::steady_clock::now().time_since_epoch().count();

        EXPECT_EQ(timeResult.failure, 0) << "Expected 0 failures in GetTotalTimeSecond, but got " << timeResult.failure;
        EXPECT_GT(timeResult.success, 0) << "Expected at least one successful call in GetTotalTimeSecond";
        STATS_HILOGI(LABEL_TEST, "GetTotalTimeSecond concurrent test completed.");
    }

    // 6. 测试 UpdateStats(StatsType, StatsState, level, uid, deviceId) 并发调用
    {
        STATS_HILOGI(LABEL_TEST, "Starting UpdateStats(StatsState) concurrent test...");

        std::vector<std::thread> stateThreads;
        stateThreads.reserve(totalWorkers);

        for (int i = 0; i < totalWorkers; ++i) {
            stateThreads.emplace_back([&, i](int32_t workerId) {
                for (int j = 0; j < callsPerWorker; ++j) {
                    int32_t statsType = StatsUtils::STATS_TYPE_THERMAL;
                    int32_t uid = dis_uid(gen);
                    int16_t level = static_cast<int16_t>(dis_uid(gen) % 100);
                    int32_t deviceUid = dis_uid(gen);

                    StatsUtils::StatsState state = StatsUtils::STATS_STATE_ACTIVATED;

                    std::string deviceId = "device_" + std::to_string(deviceUid);

                    g_batteryStatsService->UpdateStats(
                        static_cast<OHOS::PowerMgr::StatsUtils::StatsType>(statsType),
                        state,
                        level,
                        uid,
                        deviceId
                    );
                }
            }, i);
        }

        // 等待所有线程完成
        for (auto& t : stateThreads) {
            if (t.joinable()) {
                t.join();
            }
        }
        STATS_HILOGI(LABEL_TEST, "UpdateStats(StatsState) concurrent test completed.");
    }

    // 7. 测试 UpdateStats(StatsType, time, data, uid) 并发调用
    {
        STATS_HILOGI(LABEL_TEST, "Starting UpdateStats(time, data) concurrent test...");

        std::vector<std::thread> timeDataThreads;
        timeDataThreads.reserve(totalWorkers);

        for (int i = 0; i < totalWorkers; ++i) {
            timeDataThreads.emplace_back([&, i](int32_t workerId) {
                for (int j = 0; j < callsPerWorker; ++j) {
                    int32_t statsType = StatsUtils::STATS_TYPE_THERMAL;
                    int64_t time = static_cast<int64_t>(dis_uid(gen) % 10000);
                    int64_t data = static_cast<int64_t>(dis_uid(gen) % 100000);
                    int32_t uid = dis_uid(gen);

                    g_batteryStatsService->UpdateStats(
                        static_cast<OHOS::PowerMgr::StatsUtils::StatsType>(statsType),
                        time,
                        data,
                        uid
                    );
                }
            }, i);
        }

        // 等待所有线程完成
        for (auto& t : timeDataThreads) {
            if (t.joinable()) {
                t.join();
            }
        }

        STATS_HILOGI(LABEL_TEST, "UpdateStats(time, data) concurrent test completed.");
    }

    g_batteryStatsService->OnStop();

    STATS_HILOGI(LABEL_TEST, "StatsServiceThreadTest_001 end");
}
} // namespace