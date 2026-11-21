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

#define FUZZ_PROJECT_NAME "battery_stats_dumper_fuzzer"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <fuzzer/FuzzedDataProvider.h>

#include "battery_stats_dumper.h"
#include "battery_stats_service.h"

using namespace OHOS::PowerMgr;

namespace {
constexpr size_t MAX_STRING_BYTES = 32;
constexpr size_t MAX_ARG_COUNT = 4;

// The option sets BatteryStatsDumper actually handles, kept minimal on purpose
const std::vector<std::vector<std::string>>& GetSeedArgSets()
{
    static const std::vector<std::vector<std::string>> argSets = {
        {},
        {"-h"},
        {"-batterystats"},
        {"-poweraverage"},
        {"-batterystats", "-poweraverage"}
    };
    return argSets;
}

std::vector<std::string> ConsumeArgs(FuzzedDataProvider& fdp)
{
    std::vector<std::string> args;
    while (fdp.remaining_bytes() > 0 && args.size() < MAX_ARG_COUNT) {
        args.emplace_back(fdp.ConsumeRandomLengthString(MAX_STRING_BYTES));
    }
    return args;
}

std::vector<std::vector<std::string>> BuildArgSets(FuzzedDataProvider& fdp)
{
    std::vector<std::vector<std::string>> argSets = GetSeedArgSets();
    argSets.emplace_back(ConsumeArgs(fdp));
    return argSets;
}
} // namespace

class BatteryStatsDumperFuzzer {
public:
    BatteryStatsDumperFuzzer()
    {
        service_ = BatteryStatsService::GetInstance();
        if (service_ != nullptr) {
            service_->OnStart();
        }
    }

    ~BatteryStatsDumperFuzzer()
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

        std::vector<std::vector<std::string>> argSets = BuildArgSets(fdp);
        for (const auto& args : argSets) {
            std::string result;
            (void)BatteryStatsDumper::Dump(args, result);
        }
    }

private:
    OHOS::sptr<BatteryStatsService> service_ = nullptr;
};

namespace {
BatteryStatsDumperFuzzer& GetFuzzer()
{
    static BatteryStatsDumperFuzzer fuzzer;
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
