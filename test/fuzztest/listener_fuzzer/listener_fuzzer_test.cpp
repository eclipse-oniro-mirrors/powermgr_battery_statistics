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

#define FUZZ_PROJECT_NAME "listener_fuzzer"

#include "battery_stats_service.h"
#include "battery_stats_dumper.h"
#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <string>
#include <vector>

using namespace OHOS::PowerMgr;

namespace
{
constexpr size_t MIN_LISTENER_DATA_SIZE = sizeof(int32_t);
constexpr size_t MAX_DUMPER_ARGS = 10;
constexpr size_t PREDEFINED_ARG_SELECTOR = 2;
constexpr size_t MAX_FUZZ_ARG_LENGTH = 32;
constexpr size_t UID_BYTE_SIZE = sizeof(int32_t);
constexpr size_t DEBUG_INFO_INPUT_REQUIREMENT = 16;
constexpr size_t DEBUG_INFO_MAX_LENGTH = 64;
constexpr uint8_t BATTERY_STATE_MASK = 0x01;

class ListenerFuzzer {
public:
    ListenerFuzzer()
    {
        service_ = BatteryStatsService::GetInstance();
        service_->OnStart();
    }

    ~ListenerFuzzer()
    {
        if (service_ != nullptr) {
            service_->OnStop();
        }
    }

    void FuzzListenerAndDumper(const uint8_t* data, size_t size)
    {
        if (data == nullptr || size < MIN_LISTENER_DATA_SIZE) {
            return;
        }

        size_t offset = 0;
        auto args = GenerateDumpArgs(data, size, offset);

        std::string result;
        BatteryStatsDumper::Dump(args, result);

        FuzzStateTransitions(data, size);
        FuzzLifecycle(size);
        FuzzDebugInfo(data, size, offset);
    }

private:
    std::vector<std::string> GenerateDumpArgs(const uint8_t* data, size_t size, size_t& offset) const
    {
        std::vector<std::string> args;
        if (offset >= size) {
            return args;
        }

        static const std::vector<std::string> predefinedArgs = {
            "-batterystats", "-u", "-a", "-p", "-r", "-reset", "-help", "-c", "-checkereset"
        };

        size_t numArgs = data[offset] % MAX_DUMPER_ARGS;
        offset++;

        for (size_t i = 0; i < numArgs && offset < size; i++) {
            bool usePredefined = (data[offset] % PREDEFINED_ARG_SELECTOR == 0) && !predefinedArgs.empty();
            if (usePredefined) {
                args.push_back(predefinedArgs[data[offset] % predefinedArgs.size()]);
                offset++;
                continue;
            }

            size_t remaining = size - offset;
            if (remaining <= 1) {
                break;
            }

            size_t argLen = std::min(static_cast<size_t>(data[offset] % MAX_FUZZ_ARG_LENGTH), remaining - 1);
            offset++;

            if (offset + argLen <= size) {
                std::string arg(reinterpret_cast<const char*>(&data[offset]), argLen);
                args.push_back(arg);
                offset += argLen;
            }
        }

        return args;
    }

    void FuzzStateTransitions(const uint8_t* data, size_t size)
    {
        size_t offset = 0;
        while (offset < size) {
            bool onBattery = (data[offset++] & BATTERY_STATE_MASK) != 0;
            service_->SetOnBattery(onBattery);

            if (offset + UID_BYTE_SIZE <= size) {
                int32_t uid = *reinterpret_cast<const int32_t*>(&data[offset]);
                offset += UID_BYTE_SIZE;

                auto core = service_->GetBatteryStatsCore();
                if (core != nullptr) {
                    core->UpdateStats(
                        StatsUtils::STATS_TYPE_WAKELOCK_HOLD,
                        StatsUtils::STATS_STATE_ACTIVATED,
                        0,
                        uid);
                }
            }
        }
    }

    void FuzzLifecycle(size_t size)
    {
        if (size <= MIN_LISTENER_DATA_SIZE) {
            return;
        }

        service_->OnStart();
        service_->OnStop();
        service_->OnStart();
    }

    void FuzzDebugInfo(const uint8_t* data, size_t size, size_t offset)
    {
        if (offset + DEBUG_INFO_INPUT_REQUIREMENT > size) {
            return;
        }

        auto core = service_->GetBatteryStatsCore();
        if (core == nullptr) {
            return;
        }

        size_t copyLen = std::min(DEBUG_INFO_MAX_LENGTH, size - offset);
        std::string debugInfo(reinterpret_cast<const char*>(&data[offset]), copyLen);
        core->UpdateDebugInfo(debugInfo);

        std::string dumpResult;
        core->GetDebugInfo(dumpResult);
    }

    OHOS::sptr<BatteryStatsService> service_ = nullptr;
};

ListenerFuzzer g_fuzzer;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    g_fuzzer.FuzzListenerAndDumper(data, size);
    return 0;
}
