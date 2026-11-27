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

#include "subscriber_event_fuzzer.h"

#include "../battery_fuzz_input.h"

#include "battery_info.h"
#include "battery_stats_core.h"
#include "battery_stats_service.h"
#include "battery_stats_subscriber.h"
#include "common_event_data.h"
#include "common_event_subscribe_info.h"
#include "common_event_support.h"
#include "stats_utils.h"
#include "want.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <vector>

namespace OHOS {
namespace PowerMgr {

namespace {
constexpr size_t MIN_ITEM_COUNT = 1;
constexpr size_t MAX_EVENTS_PER_INPUT = 8;
constexpr size_t MAX_STRING_LENGTH = 48;
constexpr int32_t BATTERY_LEVEL_FULL = 100;
constexpr int32_t BATTERY_LEVEL_EMPTY = 0;
constexpr int32_t BATTERY_LEVEL_MIN = 1;
constexpr int32_t BATTERY_LEVEL_HALF = 50;
constexpr int32_t BATTERY_LEVEL_NEAR_FULL = 99;
constexpr int32_t BATTERY_LEVEL_OVER_FULL = 101;
constexpr int32_t INT32_MAX_EDGE_VALUE = std::numeric_limits<int32_t>::max();
constexpr size_t CUSTOM_ACTION_SELECTOR_SLOT = 1;

std::string PickAction(FuzzedDataProvider& reader)
{
    static const std::vector<std::string> actions = {
        EventFwk::CommonEventSupport::COMMON_EVENT_SHUTDOWN,
        EventFwk::CommonEventSupport::COMMON_EVENT_BATTERY_CHANGED,
        "usual.event.BATTERY_CHANGED",
        "usual.event.SHUTDOWN",
        "usual.event.UNKNOWN_BATTERY_STATS",
    };
    uint8_t selector = reader.ConsumeIntegral<uint8_t>();
    if ((selector % (actions.size() + CUSTOM_ACTION_SELECTOR_SLOT)) == actions.size()) {
        return reader.ConsumeRandomLengthString(MAX_STRING_LENGTH);
    }
    return actions[selector % actions.size()];
}

int32_t PickPluggedType(FuzzedDataProvider& reader)
{
    static constexpr int32_t pluggedTypes[] = {
        static_cast<int32_t>(BatteryPluggedType::PLUGGED_TYPE_NONE),
        static_cast<int32_t>(BatteryPluggedType::PLUGGED_TYPE_USB),
        static_cast<int32_t>(BatteryPluggedType::PLUGGED_TYPE_AC),
        static_cast<int32_t>(BatteryPluggedType::PLUGGED_TYPE_BUTT),
        StatsUtils::INVALID_VALUE,
        INT32_MAX_EDGE_VALUE,
    };
    return pluggedTypes[reader.ConsumeIntegral<uint8_t>() % (sizeof(pluggedTypes) / sizeof(pluggedTypes[0]))];
}

int32_t PickCapacity(FuzzedDataProvider& reader)
{
    static constexpr int32_t capacities[] = {
        StatsUtils::INVALID_VALUE,
        BATTERY_LEVEL_EMPTY,
        BATTERY_LEVEL_MIN,
        BATTERY_LEVEL_HALF,
        BATTERY_LEVEL_NEAR_FULL,
        BATTERY_LEVEL_FULL,
        BATTERY_LEVEL_OVER_FULL,
        INT32_MAX_EDGE_VALUE,
    };
    return capacities[reader.ConsumeIntegral<uint8_t>() % (sizeof(capacities) / sizeof(capacities[0]))];
}

class SubscriberEventFuzzer {
public:
    SubscriberEventFuzzer()
    {
        service_ = BatteryStatsService::GetInstance();
        if (service_ != nullptr) {
            service_->OnStart();
        }
    }

    ~SubscriberEventFuzzer()
    {
        if (service_ != nullptr) {
            service_->OnStop();
        }
    }

    void FuzzSubscriberEvent(const uint8_t* data, size_t size)
    {
        if (data == nullptr || size == 0 || service_ == nullptr) {
            return;
        }

        FuzzedDataProvider reader(data, size);
        size_t eventCount = MIN_ITEM_COUNT + (reader.ConsumeIntegral<uint8_t>() % MAX_EVENTS_PER_INPUT);
        for (size_t eventIndex = 0; eventIndex < eventCount; ++eventIndex) {
            std::string action = PickAction(reader);
            AAFwk::Want want;
            want.SetAction(action);
            want.SetParam(BatteryInfo::COMMON_EVENT_KEY_CAPACITY, PickCapacity(reader));
            want.SetParam(BatteryInfo::COMMON_EVENT_KEY_PLUGGED_TYPE, PickPluggedType(reader));
            want.SetParam("userId", ConsumeOr<int32_t>(reader, StatsUtils::INVALID_VALUE));
            want.SetParam("uid", ConsumeOr<int32_t>(reader, StatsUtils::INVALID_VALUE));
            want.SetParam("packageName", reader.ConsumeRandomLengthString(MAX_STRING_LENGTH));

            EventFwk::CommonEventData eventData;
            eventData.SetWant(want);
            eventData.SetCode(ConsumeOr<int32_t>(reader, StatsUtils::INVALID_VALUE));

            EventFwk::MatchingSkills matchingSkills;
            matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_SHUTDOWN);
            matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_BATTERY_CHANGED);
            matchingSkills.AddEvent(action);
            EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);
            auto subscriber = std::make_shared<BatteryStatsSubscriber>(subscribeInfo);
            subscriber->OnReceiveEvent(eventData);
        }

        auto core = service_->GetBatteryStatsCore();
        if (core != nullptr) {
            core->Reset();
        }
    }

private:
    OHOS::sptr<BatteryStatsService> service_ = nullptr;
};
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    static SubscriberEventFuzzer fuzzer;
    fuzzer.FuzzSubscriberEvent(data, size);
    return 0;
}

} // namespace PowerMgr
} // namespace OHOS
