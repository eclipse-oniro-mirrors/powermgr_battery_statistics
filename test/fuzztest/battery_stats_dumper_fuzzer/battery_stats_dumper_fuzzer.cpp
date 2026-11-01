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

#include "battery_stats_dumper.h"
#include "battery_stats_service.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

using namespace OHOS::PowerMgr;

namespace {
constexpr size_t MIN_INPUT_SIZE = 1;
constexpr size_t MAX_STRING_BYTES = 32;
constexpr size_t MAX_ARG_COUNT = 8;

std::vector<std::string> BuildArgs(const uint8_t* data, size_t size)
{
    std::vector<std::string> args = {
        "-h",
        "-batterystats",
        "-poweraverage",
        "-unknown"
    };
    if (data == nullptr || size < MIN_INPUT_SIZE) {
        return args;
    }

    size_t offset = 0;
    while (offset < size && args.size() < MAX_ARG_COUNT) {
        size_t length = std::min(static_cast<size_t>(data[offset] % MAX_STRING_BYTES), size - offset - 1);
        offset++;
        if (length == 0 || offset + length > size) {
            continue;
        }
        args.emplace_back(reinterpret_cast<const char*>(data + offset), length);
        offset += length;
    }
    return args;
}

std::vector<std::vector<std::string>> BuildArgSets(const uint8_t* data, size_t size)
{
    std::vector<std::vector<std::string>> argSets;
    argSets.push_back({});
    argSets.push_back({"-h"});
    argSets.push_back({"-batterystats"});
    argSets.push_back({"-poweraverage"});
    argSets.push_back({"-batterystats", "-poweraverage"});

    auto fuzzArgs = BuildArgs(data, size);
    if (!fuzzArgs.empty()) {
        argSets.push_back(fuzzArgs);
    }
    return argSets;
}
} // namespace

class BatteryStatsDumperFuzzer {
public:
    BatteryStatsDumperFuzzer()
    {
        service_ = BatteryStatsService::GetInstance();
        service_->OnStart();
    }

    ~BatteryStatsDumperFuzzer()
    {
        if (service_ != nullptr) {
            service_->OnStop();
        }
    }

    void Fuzz(const uint8_t* data, size_t size)
    {
        if (data == nullptr || size < MIN_INPUT_SIZE) {
            return;
        }

        for (const auto& args : BuildArgSets(data, size)) {
            std::string result;
            (void)BatteryStatsDumper::Dump(args, result);
        }
    }

private:
    OHOS::sptr<BatteryStatsService> service_ = nullptr;
};

namespace {
BatteryStatsDumperFuzzer g_fuzzer;
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    g_fuzzer.Fuzz(data, size);
    return 0;
}
