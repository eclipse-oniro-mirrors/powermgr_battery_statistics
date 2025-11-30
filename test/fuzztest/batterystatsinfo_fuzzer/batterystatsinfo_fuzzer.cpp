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

#include "batterystatsinfo_fuzzer.h"

#include <cstddef>
#include <cstdint>

#include "message_parcel.h"
#include "battery_stats_info.h"

using namespace OHOS::PowerMgr;

namespace OHOS {
namespace PowerMgr {
bool BatteryStatsInfoFuzzTest(const uint8_t* data, size_t size)
{
    if (data == nullptr || size < sizeof(int32_t) * 2) {
        return false;
    }

    // Create BatteryStatsInfo object
    auto statsInfo = std::make_shared<BatteryStatsInfo>();

    // Test SetUid and GetUid
    int32_t uid = *reinterpret_cast<const int32_t*>(data);
    statsInfo->SetUid(uid);
    statsInfo->GetUid();

    // Test SetUserId and GetUserId
    int32_t userId = *reinterpret_cast<const int32_t*>(data + sizeof(int32_t));
    statsInfo->SetUserId(userId);
    statsInfo->GetUserId();

    // Test Marshalling and Unmarshalling
    MessageParcel parcel;
    statsInfo->Marshalling(parcel);
    parcel.RewindRead(0);
    BatteryStatsInfo::Unmarshalling(parcel);

    // Test ReadFromParcel
    MessageParcel parcel2;
    parcel2.WriteBuffer(data, size);
    parcel2.RewindRead(0);
    auto statsInfo2 = std::make_shared<BatteryStatsInfo>();
    statsInfo2->ReadFromParcel(parcel2);

    return true;
}

bool ParcelableBatteryStatsListFuzzTest(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return false;
    }

    // Test ParcelableBatteryStatsList Unmarshalling
    MessageParcel parcel;
    parcel.WriteBuffer(data, size);
    parcel.RewindRead(0);
    auto list = ParcelableBatteryStatsList::Unmarshalling(parcel);
    if (list != nullptr) {
        delete list;
    }

    // Test with valid data
    MessageParcel parcel2;
    parcel2.WriteInt32(1);  // size = 1
    parcel2.WriteInt32(1000);  // uid
    parcel2.WriteInt32(0);  // type
    parcel2.WriteDouble(10.5);  // power
    parcel2.RewindRead(0);
    auto list2 = ParcelableBatteryStatsList::Unmarshalling(parcel2);
    if (list2 != nullptr) {
        delete list2;
    }

    return true;
}
} // namespace PowerMgr
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    OHOS::PowerMgr::BatteryStatsInfoFuzzTest(data, size);
    OHOS::PowerMgr::ParcelableBatteryStatsListFuzzTest(data, size);
    return 0;
}

