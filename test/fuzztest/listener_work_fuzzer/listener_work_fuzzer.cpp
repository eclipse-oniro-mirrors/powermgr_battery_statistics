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

#include "listener_work_fuzzer.h"
#include "../battery_listener_fuzz_support.h"

#include "stats_hisysevent.h"

namespace OHOS {
namespace PowerMgr {
namespace {
constexpr uint8_t DIRECT_WORKSCHEDULER_PARSE = 0;
constexpr uint8_t DIRECT_POWER_WORKSCHEDULER_PARSE = 1;
constexpr uint8_t DIRECT_OTHERS_WORKSCHEDULER_PARSE = 2;
constexpr uint8_t DIRECT_DISTRIBUTED_PARSE = 3;
constexpr uint8_t DIRECT_ALARM_PARSE = 4;
constexpr uint8_t PARSER_COUNT = 5;


void Exercise(BatteryStatsListener& listener, const std::string&,
    const cJSON* root, FuzzedDataProvider& reader)
{
    StatsUtils::StatsData statsData;
    switch (reader.ConsumeIntegral<uint8_t>() % PARSER_COUNT) {
        case DIRECT_WORKSCHEDULER_PARSE:
            listener.ProcessWorkschedulerEvent(statsData, root);
            break;
        case DIRECT_POWER_WORKSCHEDULER_PARSE:
            listener.ProcessPowerWorkschedulerEvent(statsData, root);
            break;
        case DIRECT_OTHERS_WORKSCHEDULER_PARSE:
            listener.ProcessOthersWorkschedulerEvent(statsData, root);
            break;
        case DIRECT_DISTRIBUTED_PARSE:
            listener.ProcessDistributedSchedulerEvent(statsData, root);
            break;
        case DIRECT_ALARM_PARSE:
            listener.ProcessAlarmEvent(statsData, root);
            break;
        default:
            break;
    }
}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    RunListenerFuzzCase(data, size, Exercise);
    return 0;
}

} // namespace PowerMgr
} // namespace OHOS
