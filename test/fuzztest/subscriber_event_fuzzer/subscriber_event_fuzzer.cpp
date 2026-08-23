/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
constexpr size_t BYTE_BITS = 8;
constexpr size_t MIN_ITEM_COUNT = 1;
constexpr size_t STRING_LENGTH_RANGE_OFFSET = 1;
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

class FuzzReader {
public:
    FuzzReader(const uint8_t* data, size_t size) : data_(data), size_(size) {}

    template <typename T>
    T Read(T fallback = T())
    {
        if (Remaining() < sizeof(T)) {
            return fallback;
        }
        uint64_t value = 0;
        for (size_t index = 0; index < sizeof(T); ++index) {
            value |= static_cast<uint64_t>(data_[offset_ + index]) << (BYTE_BITS * index);
        }
        offset_ += sizeof(T);
        return static_cast<T>(value);
    }

    std::string ReadString(size_t maxLength)
    {
        if (Remaining() == 0) {
            return "";
        }
        size_t length = Read<uint8_t>(0) % (maxLength + STRING_LENGTH_RANGE_OFFSET);
        length = std::min(length, Remaining());

        std::string value;
        value.reserve(length);
        for (size_t index = 0; index < length; ++index) {
            value.push_back(static_cast<char>(data_[offset_++]));
        }
        return value;
    }

private:
    size_t Remaining() const
    {
        return size_ - offset_;
    }

    const uint8_t* data_ = nullptr;
    size_t size_ = 0;
    size_t offset_ = 0;
};

std::string PickAction(FuzzReader& reader)
{
    static const std::vector<std::string> actions = {
        EventFwk::CommonEventSupport::COMMON_EVENT_SHUTDOWN,
        EventFwk::CommonEventSupport::COMMON_EVENT_BATTERY_CHANGED,
        "usual.event.BATTERY_CHANGED",
        "usual.event.SHUTDOWN",
        "usual.event.UNKNOWN_BATTERY_STATS",
    };
    uint8_t selector = reader.Read<uint8_t>(0);
    if ((selector % (actions.size() + CUSTOM_ACTION_SELECTOR_SLOT)) == actions.size()) {
        return reader.ReadString(MAX_STRING_LENGTH);
    }
    return actions[selector % actions.size()];
}

int32_t PickPluggedType(FuzzReader& reader)
{
    static constexpr int32_t pluggedTypes[] = {
        static_cast<int32_t>(BatteryPluggedType::PLUGGED_TYPE_NONE),
        static_cast<int32_t>(BatteryPluggedType::PLUGGED_TYPE_USB),
        static_cast<int32_t>(BatteryPluggedType::PLUGGED_TYPE_AC),
        static_cast<int32_t>(BatteryPluggedType::PLUGGED_TYPE_BUTT),
        StatsUtils::INVALID_VALUE,
        INT32_MAX_EDGE_VALUE,
    };
    return pluggedTypes[reader.Read<uint8_t>(0) % (sizeof(pluggedTypes) / sizeof(pluggedTypes[0]))];
}

int32_t PickCapacity(FuzzReader& reader)
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
    return capacities[reader.Read<uint8_t>(0) % (sizeof(capacities) / sizeof(capacities[0]))];
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

        FuzzReader reader(data, size);
        size_t eventCount = MIN_ITEM_COUNT + (reader.Read<uint8_t>(0) % MAX_EVENTS_PER_INPUT);
        for (size_t eventIndex = 0; eventIndex < eventCount; ++eventIndex) {
            std::string action = PickAction(reader);
            AAFwk::Want want;
            want.SetAction(action);
            want.SetParam(BatteryInfo::COMMON_EVENT_KEY_CAPACITY, PickCapacity(reader));
            want.SetParam(BatteryInfo::COMMON_EVENT_KEY_PLUGGED_TYPE, PickPluggedType(reader));
            want.SetParam("userId", reader.Read<int32_t>(StatsUtils::INVALID_VALUE));
            want.SetParam("uid", reader.Read<int32_t>(StatsUtils::INVALID_VALUE));
            want.SetParam("packageName", reader.ReadString(MAX_STRING_LENGTH));

            EventFwk::CommonEventData eventData;
            eventData.SetWant(want);
            eventData.SetCode(reader.Read<int32_t>(StatsUtils::INVALID_VALUE));

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

SubscriberEventFuzzer g_fuzzer;
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    g_fuzzer.FuzzSubscriberEvent(data, size);
    return 0;
}

} // namespace PowerMgr
} // namespace OHOS
