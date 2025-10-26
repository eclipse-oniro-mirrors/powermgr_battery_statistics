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
#include <string>
#include <vector>

using namespace OHOS::PowerMgr;

namespace {
    class ListenerFuzzer {
    public:
        ListenerFuzzer() {
            service_ = BatteryStatsService::GetInstance();
            service_->OnStart();
        }

        ~ListenerFuzzer() {
            if (service_ != nullptr) {
                service_->OnStop();
            }
        }

        void FuzzListenerAndDumper(const uint8_t* data, size_t size) {
            if (data == nullptr || size < 4) {
                return;
            }

            size_t offset = 0;

            // Fuzz dumper with various argument combinations
            // BatteryStatsDumper::Dump is a static method

            // Generate fuzzed dump arguments
            std::vector<std::string> args;
            size_t numArgs = (data[offset] % 10);
            offset++;

            std::vector<std::string> predefinedArgs = {
                "-batterystats",
                "-u",
                "-a",
                "-p",
                "-r",
                "-reset",
                "-help",
                "-c",
                "-checkereset"
            };

            for (size_t i = 0; i < numArgs && offset < size; i++) {
                if (data[offset] % 2 == 0 && !predefinedArgs.empty()) {
                    // Use predefined args
                    args.push_back(predefinedArgs[data[offset] % predefinedArgs.size()]);
                    offset++;
                } else {
                    // Use fuzzed args
                    size_t argLen = std::min(size_t(data[offset] % 32), size - offset - 1);
                    offset++;

                    if (offset + argLen <= size) {
                        std::string arg(reinterpret_cast<const char*>(&data[offset]), argLen);
                        args.push_back(arg);
                        offset += argLen;
                    }
                }
            }

            std::string result;
            BatteryStatsDumper::Dump(args, result);

            // Fuzz SetOnBattery state transitions
            offset = 0;
            while (offset < size) {
                bool onBattery = data[offset++] & 0x01;
                service_->SetOnBattery(onBattery);

                if (offset + 4 <= size) {
                    // Add some stats updates between state changes
                    int32_t uid = *reinterpret_cast<const int32_t*>(&data[offset]);
                    offset += 4;

                    auto core = service_->GetBatteryStatsCore();
                    if (core != nullptr) {
                        core->UpdateStats(StatsUtils::STATS_TYPE_WAKELOCK_HOLD,
                                        StatsUtils::STATS_STATE_ACTIVATED, 0, uid);
                    }
                }

                if (offset >= size) break;
            }

            // Fuzz service lifecycle
            // Test ready callback
            if (size > 4) {
                service_->OnStart();
                service_->OnStop();
                service_->OnStart();
            }

            // Fuzz debug info
            if (offset + 16 <= size) {
                auto core = service_->GetBatteryStatsCore();
                if (core != nullptr) {
                    std::string debugInfo(reinterpret_cast<const char*>(&data[offset]),
                                        std::min(size_t(64), size - offset));
                    core->UpdateDebugInfo(debugInfo);

                    std::string dumpResult;
                    core->GetDebugInfo(dumpResult);
                }
            }
        }

    private:
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
