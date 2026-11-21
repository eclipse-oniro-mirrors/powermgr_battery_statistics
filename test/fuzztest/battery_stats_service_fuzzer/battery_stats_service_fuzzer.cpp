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

#define FUZZ_PROJECT_NAME "battery_stats_service_fuzzer"

#include <fcntl.h>
#include <unistd.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <fuzzer/FuzzedDataProvider.h>

#include "battery_stats_core.h"
#include "battery_stats_detector.h"
#include "battery_stats_parser.h"
#include "battery_stats_service.h"
#include "system_ability_definition.h"

using namespace OHOS::PowerMgr;

namespace {
constexpr size_t MAX_ARG_COUNT = 4;
constexpr size_t MAX_ARG_BYTES = 32;
constexpr char NULL_DEVICE_PATH[] = "/dev/null";

// Only the system abilities that BatteryStatsService handles in OnAddSystemAbility() are injected,
// an unknown id would only walk the logging path and register nothing.
const std::vector<int32_t>& GetListenedSystemAbilityIds()
{
    static const std::vector<int32_t> ids = {
        OHOS::DFX_SYS_EVENT_SERVICE_ABILITY_ID,
        OHOS::COMMON_EVENT_SERVICE_ID
    };
    return ids;
}

const std::vector<std::string>& GetDumpArgPool()
{
    static const std::vector<std::string> args = {
        "-h",
        "-batterystats",
        "-poweraverage"
    };
    return args;
}

std::vector<std::string> ConsumeDumpArgs(FuzzedDataProvider& fdp)
{
    std::vector<std::string> args = GetDumpArgPool();
    while (fdp.remaining_bytes() > 0 && args.size() < MAX_ARG_COUNT) {
        args.emplace_back(fdp.ConsumeRandomLengthString(MAX_ARG_BYTES));
    }
    return args;
}

std::vector<std::u16string> ToU16Args(const std::vector<std::string>& args)
{
    std::vector<std::u16string> result;
    result.reserve(args.size());
    for (const auto& arg : args) {
        result.emplace_back(arg.begin(), arg.end());
    }
    return result;
}
} // namespace

class BatteryStatsServiceFuzzer {
public:
    BatteryStatsServiceFuzzer()
    {
        service_ = BatteryStatsService::GetInstance();
        if (service_ != nullptr) {
            service_->OnStart();
        }
    }

    ~BatteryStatsServiceFuzzer()
    {
        if (service_ != nullptr) {
            service_->OnStop();
        }
        service_ = nullptr;
    }

    void Fuzz(FuzzedDataProvider& fdp)
    {
        if (service_ == nullptr) {
            return;
        }

        FuzzSystemAbilityListener(fdp);
        (void)service_->IsServiceReady();
        (void)service_->GetBatteryStatsCore();
        (void)service_->GetBatteryStatsParser();
        (void)service_->GetBatteryStatsDetector();
        FuzzShellDump(fdp);
        FuzzDump(fdp);
    }

private:
    void FuzzSystemAbilityListener(FuzzedDataProvider& fdp)
    {
        for (int32_t systemAbilityId : GetListenedSystemAbilityIds()) {
            std::string deviceId = fdp.ConsumeRandomLengthString(MAX_ARG_BYTES);
            service_->OnAddSystemAbility(systemAbilityId, deviceId);
        }
    }

    void FuzzShellDump(FuzzedDataProvider& fdp)
    {
        std::vector<std::string> args = ConsumeDumpArgs(fdp);
        std::string shellDump = service_->ShellDump(args, static_cast<uint32_t>(args.size()));
        (void)shellDump;
    }

    void FuzzDump(FuzzedDataProvider& fdp)
    {
        int32_t fd = open(NULL_DEVICE_PATH, O_WRONLY);
        if (fd < 0) {
            return;
        }
        std::vector<std::u16string> u16Args = ToU16Args(ConsumeDumpArgs(fdp));
        (void)service_->Dump(fd, u16Args);
        close(fd);
    }

    OHOS::sptr<BatteryStatsService> service_ = nullptr;
};

namespace {
BatteryStatsServiceFuzzer& GetFuzzer()
{
    static BatteryStatsServiceFuzzer fuzzer;
    return fuzzer;
}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }

    FuzzedDataProvider fdp(data, size);
    GetFuzzer().Fuzz(fdp);
    return 0;
}
