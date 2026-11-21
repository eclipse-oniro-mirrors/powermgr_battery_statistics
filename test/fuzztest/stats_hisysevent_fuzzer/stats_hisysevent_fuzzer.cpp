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

#include <cstddef>
#include <cstdint>
#include <string>

#include <fuzzer/FuzzedDataProvider.h>

#include "stats_hisysevent.h"

using namespace OHOS::PowerMgr;

namespace {
constexpr size_t MAX_EVENT_NAME_LEN = 64;

void CheckOneEvent(const std::string& eventName)
{
    (void)StatsHiSysEvent::CheckHiSysEvent(eventName);
}

// Cover the empty name and every registered HiSysEvent name
void CheckAllRegisteredEvents()
{
    CheckOneEvent(std::string());
    for (int32_t i = 0; i < StatsHiSysEvent::HISYSEVENT_TYPE_END; i++) {
        CheckOneEvent(StatsHiSysEvent::HISYSEVENT_LIST[i]);
    }
}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }

    FuzzedDataProvider fdp(data, size);
    CheckOneEvent(fdp.ConsumeRandomLengthString(MAX_EVENT_NAME_LEN));
    CheckAllRegisteredEvents();
    return 0;
}
