/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "my_fuzzer.h"

#include <stddef.h>
#include <stdint.h>
#include <cstring>
#include <vector>
#include <string>

#include "battery_stats_client.h"
#include "battery_stats_info.h"
#include "stats_utils.h"

using namespace OHOS::PowerMgr;

namespace {
// Minimum size needed to extract a valid int32_t UID
constexpr size_t MIN_SIZE = sizeof(int32_t);

// UID boundary constants for testing
constexpr int32_t MIN_UID = 0;
constexpr int32_t MAX_UID = 99999;
constexpr int32_t SPECIAL_UID_ROOT = 0;
constexpr int32_t SPECIAL_UID_SYSTEM = 1000;
constexpr int32_t SPECIAL_UID_SHELL = 2000;
constexpr int32_t FIRST_APP_UID = 10000;
constexpr int32_t INVALID_UID = -1;

// Test case constants
constexpr uint8_t TEST_CASES_UID = 8;
constexpr uint8_t TEST_CASES_CONSUMPTION = 16;
constexpr uint8_t TEST_CASES_STATS = 8;
constexpr uint8_t TEST_CASES_DATA = 6;
constexpr uint8_t TEST_CASES_DUMP = 5;

// Minimum data size constants
constexpr size_t MIN_SIZE_FOR_CONSUMPTION = 1;
constexpr size_t MIN_SIZE_FOR_STATS = 2;
constexpr size_t MIN_SIZE_FOR_EXTRA_TESTS = 3;

// Case index constants
enum TestCaseIndex {
    CASE_0 = 0,
    CASE_1 = 1,
    CASE_2 = 2,
    CASE_3 = 3,
    CASE_4 = 4,
    CASE_5 = 5,
    CASE_6 = 6,
    CASE_7 = 7,
    CASE_8 = 8,
    CASE_9 = 9,
    CASE_10 = 10,
    CASE_11 = 11,
    CASE_12 = 12,
    CASE_13 = 13,
    CASE_14 = 14,
    CASE_15 = 15
};

/**
 * @brief Extract int32_t UID from fuzzer data
 * @param data Input data from fuzzer
 * @param size Size of input data
 * @return Extracted UID value
 */
int32_t ExtractUid(const uint8_t* data, size_t size)
{
    if (size < MIN_SIZE) {
        return MIN_UID;
    }

    int32_t uid;
    std::memcpy(&uid, data, sizeof(int32_t));
    return uid;
}

/**
 * @brief Test BatteryStatsClient::GetAppStatsMah with various UID inputs
 * @param data Input data from fuzzer
 * @param size Size of input data
 */
void TestGetAppStatsMah(const uint8_t* data, size_t size)
{
    if (size < MIN_SIZE) {
        return;
    }

    // Extract UID from fuzzer data
    int32_t uid = ExtractUid(data, size);

    // Get BatteryStatsClient instance and test GetAppStatsMah
    auto& client = BatteryStatsClient::GetInstance();
    
    // Test with extracted UID (may be random/invalid)
    client.GetAppStatsMah(uid);
    
    // Test with boundary values if we have extra data
    if (size >= MIN_SIZE + 1) {
        uint8_t testCase = data[MIN_SIZE];
        
        switch (testCase % TEST_CASES_UID) {
            case CASE_0:
                client.GetAppStatsMah(MIN_UID);
                break;
            case CASE_1:
                client.GetAppStatsMah(MAX_UID);
                break;
            case CASE_2:
                client.GetAppStatsMah(SPECIAL_UID_ROOT);
                break;
            case CASE_3:
                client.GetAppStatsMah(SPECIAL_UID_SYSTEM);
                break;
            case CASE_4:
                client.GetAppStatsMah(SPECIAL_UID_SHELL);
                break;
            case CASE_5:
                client.GetAppStatsMah(FIRST_APP_UID);
                break;
            case CASE_6:
                client.GetAppStatsMah(INVALID_UID);  // Test negative UID
                break;
            case CASE_7:
                client.GetAppStatsMah(INT32_MAX);  // Test maximum int32_t
                break;
            default:
                break;
        }
    }
}

/**
 * @brief Test BatteryStatsClient::GetAppStatsPercent with various UID inputs
 * @param data Input data from fuzzer
 * @param size Size of input data
 */
void TestGetAppStatsPercent(const uint8_t* data, size_t size)
{
    if (size < MIN_SIZE) {
        return;
    }

    // Extract UID from fuzzer data
    int32_t uid = ExtractUid(data, size);

    // Get BatteryStatsClient instance and test GetAppStatsPercent
    auto& client = BatteryStatsClient::GetInstance();
    
    // Test with extracted UID (may be random/invalid)
    client.GetAppStatsPercent(uid);
    
    // Test with boundary values if we have extra data
    if (size >= MIN_SIZE + 1) {
        uint8_t testCase = data[MIN_SIZE];
        
        switch (testCase % TEST_CASES_UID) {
            case CASE_0:
                client.GetAppStatsPercent(MIN_UID);
                break;
            case CASE_1:
                client.GetAppStatsPercent(MAX_UID);
                break;
            case CASE_2:
                client.GetAppStatsPercent(SPECIAL_UID_ROOT);
                break;
            case CASE_3:
                client.GetAppStatsPercent(SPECIAL_UID_SYSTEM);
                break;
            case CASE_4:
                client.GetAppStatsPercent(SPECIAL_UID_SHELL);
                break;
            case CASE_5:
                client.GetAppStatsPercent(FIRST_APP_UID);
                break;
            case CASE_6:
                client.GetAppStatsPercent(INVALID_UID);  // Test negative UID
                break;
            case CASE_7:
                client.GetAppStatsPercent(INT32_MAX);  // Test maximum int32_t
                break;
            default:
                break;
        }
    }
}
}

/**
 * @brief Test predefined consumption types
 * @param testCase Test case selector
 */
void TestPredefinedConsumptionTypes(uint8_t testCase)
{
    auto& client = BatteryStatsClient::GetInstance();
    
    switch (testCase % TEST_CASES_CONSUMPTION) {
        case CASE_0:
            client.GetPartStatsPercent(BatteryStatsInfo::CONSUMPTION_TYPE_INVALID);
            break;
        case CASE_1:
            client.GetPartStatsPercent(BatteryStatsInfo::CONSUMPTION_TYPE_APP);
            break;
        case CASE_2:
            client.GetPartStatsPercent(BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH);
            break;
        case CASE_3:
            client.GetPartStatsPercent(BatteryStatsInfo::CONSUMPTION_TYPE_IDLE);
            break;
        case CASE_4:
            client.GetPartStatsPercent(BatteryStatsInfo::CONSUMPTION_TYPE_PHONE);
            break;
        case CASE_5:
            client.GetPartStatsPercent(BatteryStatsInfo::CONSUMPTION_TYPE_RADIO);
            break;
        case CASE_6:
            client.GetPartStatsPercent(BatteryStatsInfo::CONSUMPTION_TYPE_SCREEN);
            break;
        case CASE_7:
            client.GetPartStatsPercent(BatteryStatsInfo::CONSUMPTION_TYPE_USER);
            break;
        case CASE_8:
            client.GetPartStatsPercent(BatteryStatsInfo::CONSUMPTION_TYPE_WIFI);
            break;
        case CASE_9:
            client.GetPartStatsPercent(BatteryStatsInfo::CONSUMPTION_TYPE_CAMERA);
            break;
        case CASE_10:
            client.GetPartStatsPercent(BatteryStatsInfo::CONSUMPTION_TYPE_FLASHLIGHT);
            break;
        case CASE_11:
            client.GetPartStatsPercent(BatteryStatsInfo::CONSUMPTION_TYPE_AUDIO);
            break;
        case CASE_12:
            client.GetPartStatsPercent(BatteryStatsInfo::CONSUMPTION_TYPE_SENSOR);
            break;
        case CASE_13:
            client.GetPartStatsPercent(BatteryStatsInfo::CONSUMPTION_TYPE_GNSS);
            break;
        case CASE_14:
            client.GetPartStatsPercent(BatteryStatsInfo::CONSUMPTION_TYPE_CPU);
            break;
        case CASE_15:
            client.GetPartStatsPercent(BatteryStatsInfo::CONSUMPTION_TYPE_WAKELOCK);
            break;
        default:
            break;
    }
}

/**
 * @brief Test BatteryStatsClient::GetPartStatsPercent with various ConsumptionType inputs
 * @param data Input data from fuzzer
 * @param size Size of input data
 */
void TestGetPartStatsPercent(const uint8_t* data, size_t size)
{
    if (size < MIN_SIZE_FOR_CONSUMPTION) {
        return;
    }

    // Get BatteryStatsClient instance
    auto& client = BatteryStatsClient::GetInstance();
    
    // Extract consumption type from fuzzer data
    int32_t typeValue = static_cast<int32_t>(data[0]);
    BatteryStatsInfo::ConsumptionType type = static_cast<BatteryStatsInfo::ConsumptionType>(typeValue);
    
    // Test with extracted type
    client.GetPartStatsPercent(type);
    
    // Test with predefined consumption types if we have extra data
    if (size >= MIN_SIZE_FOR_STATS) {
        TestPredefinedConsumptionTypes(data[1]);
    }
}

/**
 * @brief Test BatteryStatsClient::GetLastError
 */
void TestGetLastError()
{
    // Get BatteryStatsClient instance and test GetLastError
    auto& client = BatteryStatsClient::GetInstance();
    client.GetLastError();
}

/**
 * @brief Test BatteryStatsClient::SetOnBattery with various boolean inputs
 * @param data Input data from fuzzer
 * @param size Size of input data
 */
void TestSetOnBattery(const uint8_t* data, size_t size)
{
    if (size < MIN_SIZE_FOR_CONSUMPTION) {
        return;
    }

    // Get BatteryStatsClient instance
    auto& client = BatteryStatsClient::GetInstance();
    
    // Extract boolean value from fuzzer data
    bool isOnBattery = (data[0] % MIN_SIZE_FOR_STATS) == 0;
    
    // Test with extracted boolean value
    client.SetOnBattery(isOnBattery);
    
    // Test with both true and false if we have extra data
    if (size >= MIN_SIZE_FOR_STATS) {
        client.SetOnBattery(true);
        client.SetOnBattery(false);
    }
}

/**
 * @brief Test BatteryStatsClient::Reset
 */
void TestReset()
{
    // Get BatteryStatsClient instance and test Reset
    auto& client = BatteryStatsClient::GetInstance();
    client.Reset();
}

/**
 * @brief Test BatteryStatsClient::GetTotalTimeSecond with various StatsType and UID inputs
 * @param data Input data from fuzzer
 * @param size Size of input data
 */
void TestGetTotalTimeSecond(const uint8_t* data, size_t size)
{
    if (size < MIN_SIZE_FOR_STATS) {
        return;
    }

    // Get BatteryStatsClient instance
    auto& client = BatteryStatsClient::GetInstance();
    
    // Extract stats type and uid from fuzzer data
    int32_t statsTypeValue = static_cast<int32_t>(data[0]);
    StatsUtils::StatsType statsType = static_cast<StatsUtils::StatsType>(statsTypeValue);
    int32_t uid = static_cast<int32_t>(data[1]);
    
    // Test with extracted parameters
    client.GetTotalTimeSecond(statsType, uid);
    
    // Test with predefined stats types if we have extra data
    if (size >= MIN_SIZE_FOR_EXTRA_TESTS) {
        uint8_t testCase = data[MIN_SIZE_FOR_STATS];
        
        switch (testCase % TEST_CASES_STATS) {
            case CASE_0:
                client.GetTotalTimeSecond(StatsUtils::STATS_TYPE_INVALID);
                break;
            case CASE_1:
                client.GetTotalTimeSecond(StatsUtils::STATS_TYPE_WIFI_ON, FIRST_APP_UID);
                break;
            case CASE_2:
                client.GetTotalTimeSecond(StatsUtils::STATS_TYPE_BLUETOOTH_BR_ON, SPECIAL_UID_SYSTEM);
                break;
            case CASE_3:
                client.GetTotalTimeSecond(StatsUtils::STATS_TYPE_SCREEN_ON);
                break;
            case CASE_4:
                client.GetTotalTimeSecond(StatsUtils::STATS_TYPE_CAMERA_ON, MIN_UID);
                break;
            case CASE_5:
                client.GetTotalTimeSecond(StatsUtils::STATS_TYPE_GNSS_ON, INVALID_UID);
                break;
            case CASE_6:
                client.GetTotalTimeSecond(StatsUtils::STATS_TYPE_CPU_ACTIVE);
                break;
            case CASE_7:
                client.GetTotalTimeSecond(StatsUtils::STATS_TYPE_ALARM, INT32_MAX);
                break;
            default:
                break;
        }
    }
}

/**
 * @brief Test BatteryStatsClient::GetTotalDataBytes with various StatsType and UID inputs
 * @param data Input data from fuzzer
 * @param size Size of input data
 */
void TestGetTotalDataBytes(const uint8_t* data, size_t size)
{
    if (size < MIN_SIZE_FOR_STATS) {
        return;
    }

    // Get BatteryStatsClient instance
    auto& client = BatteryStatsClient::GetInstance();
    
    // Extract stats type and uid from fuzzer data
    int32_t statsTypeValue = static_cast<int32_t>(data[0]);
    StatsUtils::StatsType statsType = static_cast<StatsUtils::StatsType>(statsTypeValue);
    int32_t uid = static_cast<int32_t>(data[1]);
    
    // Test with extracted parameters
    client.GetTotalDataBytes(statsType, uid);
    
    // Test with predefined stats types if we have extra data
    if (size >= MIN_SIZE_FOR_EXTRA_TESTS) {
        uint8_t testCase = data[MIN_SIZE_FOR_STATS];
        
        switch (testCase % TEST_CASES_DATA) {
            case CASE_0:
                client.GetTotalDataBytes(StatsUtils::STATS_TYPE_INVALID);
                break;
            case CASE_1:
                client.GetTotalDataBytes(StatsUtils::STATS_TYPE_WIFI_ON, FIRST_APP_UID);
                break;
            case CASE_2:
                client.GetTotalDataBytes(StatsUtils::STATS_TYPE_BLUETOOTH_BR_ON, SPECIAL_UID_SYSTEM);
                break;
            case CASE_3:
                client.GetTotalDataBytes(StatsUtils::STATS_TYPE_PHONE_DATA, MIN_UID);
                break;
            case CASE_4:
                client.GetTotalDataBytes(StatsUtils::STATS_TYPE_PHONE_DATA, INVALID_UID);
                break;
            case CASE_5:
                client.GetTotalDataBytes(StatsUtils::STATS_TYPE_PHONE_DATA, INT32_MAX);
                break;
            default:
                break;
        }
    }
}

/**
 * @brief Test BatteryStatsClient::Dump with various arguments
 * @param data Input data from fuzzer
 * @param size Size of input data
 */
void TestDump(const uint8_t* data, size_t size)
{
    // Get BatteryStatsClient instance
    auto& client = BatteryStatsClient::GetInstance();
    
    // Test with empty args
    std::vector<std::string> emptyArgs;
    client.Dump(emptyArgs);
    
    // Test with various dump arguments if we have data
    if (size >= MIN_SIZE_FOR_CONSUMPTION) {
        uint8_t testCase = data[0];
        
        switch (testCase % TEST_CASES_DUMP) {
            case CASE_0: {
                std::vector<std::string> args1 = {"-batterystats"};
                client.Dump(args1);
                break;
            }
            case CASE_1: {
                std::vector<std::string> args2 = {"-u", "10000"};
                client.Dump(args2);
                break;
            }
            case CASE_2: {
                std::vector<std::string> args3 = {"-reset"};
                client.Dump(args3);
                break;
            }
            case CASE_3: {
                std::vector<std::string> args4 = {"-invalid"};
                client.Dump(args4);
                break;
            }
            case CASE_4: {
                std::vector<std::string> args5 = {"-batterystats", "-u", "1000"};
                client.Dump(args5);
                break;
            }
            default:
                break;
        }
    }
}

/**
 * @brief Test BatteryStatsClient::GetBatteryStats
 */
void TestGetBatteryStats()
{
    // Get BatteryStatsClient instance and test GetBatteryStats
    auto& client = BatteryStatsClient::GetInstance();
    client.GetBatteryStats();
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    TestGetAppStatsMah(data, size);
    TestGetAppStatsPercent(data, size);
    TestGetPartStatsPercent(data, size);
    TestGetLastError();
    TestSetOnBattery(data, size);
    TestReset();
    TestGetTotalTimeSecond(data, size);
    TestGetTotalDataBytes(data, size);
    TestDump(data, size);
    TestGetBatteryStats();
    return 0;
}

