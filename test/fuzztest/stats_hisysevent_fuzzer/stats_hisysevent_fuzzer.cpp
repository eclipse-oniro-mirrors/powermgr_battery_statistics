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

#define FUZZ_PROJECT_NAME "stats_hisysevent_fuzzer"

#include "stats_hisysevent.h"
#include <cstddef>
#include <cstdint>
#include <string>

using namespace OHOS::PowerMgr;

namespace {
constexpr size_t MAX_EVENT_NAME_LEN = 64;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }

    // Limit input length to avoid excessive memory usage
    size_t len = (size > MAX_EVENT_NAME_LEN) ? MAX_EVENT_NAME_LEN : size;
    std::string eventName(reinterpret_cast<const char*>(data), len);

    // Test CheckHiSysEvent with fuzzed event name
    StatsHiSysEvent::CheckHiSysEvent(eventName);

    // Also test with empty string and known valid names for coverage
    StatsHiSysEvent::CheckHiSysEvent("");
    StatsHiSysEvent::CheckHiSysEvent("RUNNINGLOCK");
    StatsHiSysEvent::CheckHiSysEvent("SCREEN_STATE");
    StatsHiSysEvent::CheckHiSysEvent("CHANGED");
    StatsHiSysEvent::CheckHiSysEvent("CALL_STATE");
    StatsHiSysEvent::CheckHiSysEvent("DATA_CONNECTION_STATE");

    return 0;
}
