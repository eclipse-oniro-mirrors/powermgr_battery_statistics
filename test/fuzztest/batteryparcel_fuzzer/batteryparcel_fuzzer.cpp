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

#include "batteryparcel_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <limits>
#include <algorithm>

#include "battery_stats_info.h"
#include "parcel.h"
#include "ibattery_stats.h"

using namespace OHOS::PowerMgr;
namespace OHOS {
namespace {
constexpr size_t K_MIN_INPUT_SIZE = 10;
constexpr int32_t K_CORRUPTED_VALUE = -1;
constexpr int32_t K_OVERSIZE_USER_ID = 999999;
constexpr size_t K_MIN_PARTIAL_THRESHOLD = 4;
constexpr uint8_t K_MASK_LIST_PARCEL = 0x01;
constexpr uint8_t K_MASK_INFO_MARSHALLING = 0x02;
constexpr uint8_t K_MASK_READ_FROM_PARCEL = 0x04;
constexpr uint8_t K_MASK_LARGE_PARCEL = 0x08;
constexpr int32_t K_FIELD_COUNT_INT32 = 3;

template<typename T>
T ConsumeData(const uint8_t** ptr, size_t* remain)
{
    if (*remain < sizeof(T)) {
        return T();
    }
    T value;
    std::copy_n(*ptr, sizeof(T), reinterpret_cast<char*>(&value));
    *ptr += sizeof(T);
    *remain -= sizeof(T);
    return value;
}

void HandleListParcel(const uint8_t** ptr, size_t* remain)
{
    size_t chunkSize = *remain / 4;
    if (chunkSize == 0) {
        OHOS::Parcel emptyParcel;
        auto emptyListPtr = std::unique_ptr<ParcelableBatteryStatsList>(
            ParcelableBatteryStatsList::Unmarshalling(emptyParcel));
        (void)emptyListPtr;
        return;
    }
    OHOS::Parcel inParcel;
    inParcel.WriteBuffer(*ptr, chunkSize);
    inParcel.RewindRead(0);
    auto listPtr = std::unique_ptr<ParcelableBatteryStatsList>(
        ParcelableBatteryStatsList::Unmarshalling(inParcel));
    if (listPtr) {
        OHOS::Parcel outParcel;
        listPtr->Marshalling(outParcel);

        for (const auto& info : listPtr->statsList_) {
            if (!info) {
                continue;
            }
            (void)info->GetUid();
            (void)info->GetUserId();
            (void)info->GetConsumptionType();
            (void)info->GetPower();
        }
    }
    *ptr += chunkSize;
    *remain -= chunkSize;
    OHOS::Parcel emptyParcel;
    auto emptyListPtr = std::unique_ptr<ParcelableBatteryStatsList>(
        ParcelableBatteryStatsList::Unmarshalling(emptyParcel));
    (void)emptyListPtr;
}

void HandleInfoMarshalling(const uint8_t** ptr, size_t* remain)
{
    const size_t need = sizeof(int32_t) * static_cast<size_t>(K_FIELD_COUNT_INT32) + sizeof(double);
    if (*remain < need) {
        return;
    }

    BatteryStatsInfo info;
    info.SetUid(ConsumeData<int32_t>(ptr, remain));
    info.SetUserId(ConsumeData<int32_t>(ptr, remain));
    int32_t rawType = ConsumeData<int32_t>(ptr, remain);
    info.SetConsumptioType(static_cast<BatteryStatsInfo::ConsumptionType>(rawType));
    double power = ConsumeData<double>(ptr, remain);
    info.SetPower(power);

    info.SetUid(0);
    info.SetUid(-1);
    info.SetPower(0.0);
    info.SetPower(-1.0);
    info.SetPower(std::numeric_limits<double>::quiet_NaN());

    OHOS::Parcel parcel;
    info.Marshalling(parcel);
    parcel.RewindRead(0);
    BatteryStatsInfo info2;
    info2.ReadFromParcel(parcel);
    (void)info2.GetUid();
    (void)info2.GetUserId();
    (void)info2.GetConsumptionType();
    (void)info2.GetPower();
}

void HandleReadFromParcel(const uint8_t** ptr, size_t* remain)
{
    OHOS::Parcel emptyParcel;
    BatteryStatsInfo info;
    info.ReadFromParcel(emptyParcel);
    if (*remain > K_MIN_PARTIAL_THRESHOLD) {
        OHOS::Parcel partialParcel;
        size_t partialSize = (*remain / 2) > 0 ? (*remain / 2) : 1;
        partialParcel.WriteBuffer(*ptr, partialSize);
        partialParcel.RewindRead(0);
        BatteryStatsInfo info2;
        info2.ReadFromParcel(partialParcel);
        *ptr += partialSize;
        *remain -= partialSize;
    }
    OHOS::Parcel corruptedParcel;
    corruptedParcel.WriteInt32(K_CORRUPTED_VALUE);
    corruptedParcel.WriteInt32(K_OVERSIZE_USER_ID);
    BatteryStatsInfo info3;
    info3.ReadFromParcel(corruptedParcel);
}

void HandleLargeParcel(const uint8_t* ptr, size_t remain)
{
    if (remain == 0) {
        return;
    }
    OHOS::Parcel largeParcel;
    largeParcel.WriteBuffer(ptr, remain);
    largeParcel.RewindRead(0);
    auto listPtr = std::unique_ptr<ParcelableBatteryStatsList>(
        ParcelableBatteryStatsList::Unmarshalling(largeParcel));
    if (!listPtr || listPtr->statsList_.empty()) {
        return;
    }
    for (const auto& info : listPtr->statsList_) {
        if (!info) {
            continue;
        }
        (void)info->GetPower();
        (void)BatteryStatsInfo::ConvertConsumptionType(info->GetConsumptionType());
    }
    auto front = listPtr->statsList_.front();
    if (front) {
        (void)front->GetUid();
    }
    auto back = listPtr->statsList_.back();
    if (back) {
        (void)back->GetPower();
    }
}
} // namespace

bool FuzzBatteryParcel(const uint8_t* data, size_t size)
{
    if (data == nullptr || size < K_MIN_INPUT_SIZE) {
        return false;
    }
    const uint8_t* ptr = data;
    size_t remain = size;
    uint8_t testMask = ConsumeData<uint8_t>(&ptr, &remain);
    if (testMask & K_MASK_LIST_PARCEL) {
        HandleListParcel(&ptr, &remain);
    }
    if (testMask & K_MASK_INFO_MARSHALLING) {
        HandleInfoMarshalling(&ptr, &remain);
    }
    if (testMask & K_MASK_READ_FROM_PARCEL) {
        HandleReadFromParcel(&ptr, &remain);
    }
    if (testMask & K_MASK_LARGE_PARCEL) {
        HandleLargeParcel(ptr, remain);
    }
    return true;
}

} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    return OHOS::FuzzBatteryParcel(data, size) ? 0 : 0;
}