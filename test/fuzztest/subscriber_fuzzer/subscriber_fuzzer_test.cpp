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

#define FUZZ_PROJECT_NAME "subscriber_fuzzer"

#include "battery_stats_service.h"
#include "battery_stats_subscriber.h"
#include "battery_stats_detector.h"
#include "common_event_data.h"
#include "common_event_subscribe_info.h"
#include "want.h"
#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <string>
#include <vector>

using namespace OHOS::PowerMgr;
using namespace OHOS::EventFwk;
using namespace OHOS::AAFwk;

namespace
{
constexpr size_t MIN_SUBSCRIBER_DATA_SIZE = 16;
constexpr size_t MAX_ACTION_STRING_SLICE = 32;
constexpr size_t EVENT_PARAM_BYTES = sizeof(int32_t) * 3; // userId, uid, code
constexpr size_t PACKAGE_PARAM_BYTES = 16;
constexpr size_t INT32_BYTES = sizeof(int32_t);

class SubscriberFuzzer {
public:
    SubscriberFuzzer()
    {
        service_ = BatteryStatsService::GetInstance();
        service_->OnStart();
    }

    ~SubscriberFuzzer()
    {
        if (service_ != nullptr) {
            service_->OnStop();
        }
    }

    void FuzzSubscriber(const uint8_t* data, size_t size)
    {
        if (data == nullptr || size < MIN_SUBSCRIBER_DATA_SIZE) {
            return;
        }

        // Create fuzzed event actions
        std::vector<std::string> actions = {
            "usual.event.USER_SWITCHED",
            "usual.event.USER_REMOVED",
            "usual.event.SCREEN_ON",
            "usual.event.SCREEN_OFF",
            "usual.event.PACKAGE_REMOVED",
            "usual.event.PACKAGE_ADDED",
            std::string(reinterpret_cast<const char*>(data), std::min(MAX_ACTION_STRING_SLICE, size))
        };

        size_t offset = 0;

        for (const auto& action : actions) {
            if (offset + EVENT_PARAM_BYTES > size) {
                break;
            }

            // Create Want with fuzzed data
            Want want;
            want.SetAction(action);

            // Add fuzzed parameters
            int32_t userId = *reinterpret_cast<const int32_t*>(&data[offset]);
            offset += INT32_BYTES;

            int32_t uid = *reinterpret_cast<const int32_t*>(&data[offset]);
            offset += INT32_BYTES;

            int32_t code = *reinterpret_cast<const int32_t*>(&data[offset]);
            offset += INT32_BYTES;

            want.SetParam("userId", userId);
            want.SetParam("uid", uid);

            // Add string parameter
            if (offset + PACKAGE_PARAM_BYTES <= size) {
                size_t copyLen = std::min(PACKAGE_PARAM_BYTES, size - offset);
                std::string strParam(reinterpret_cast<const char*>(&data[offset]), copyLen);
                want.SetParam("packageName", strParam);
                offset += PACKAGE_PARAM_BYTES;
            }

            // Create CommonEventData
            CommonEventData eventData;
            eventData.SetWant(want);
            eventData.SetCode(code);

            // Create subscriber
            MatchingSkills matchingSkills;
            matchingSkills.AddEvent(action);
            CommonEventSubscribeInfo subscribeInfo(matchingSkills);
            auto subscriber = std::make_shared<BatteryStatsSubscriber>(subscribeInfo);

            // Test OnReceiveEvent
            subscriber->OnReceiveEvent(eventData);
        }

        auto detector = service_->GetBatteryStatsDetector();
        if (detector != nullptr) {
            // Detector is tested indirectly through event processing.
        }
    }

private:
    OHOS::sptr<BatteryStatsService> service_ = nullptr;
};

SubscriberFuzzer g_fuzzer;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    g_fuzzer.FuzzSubscriber(data, size);
    return 0;
}
