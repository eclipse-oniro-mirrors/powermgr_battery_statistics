/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "async_callback_info.h"

#include "battery_stats_client.h"
#include "napi_utils.h"
#include "stats_log.h"

namespace OHOS {
namespace PowerMgr {
void AsyncCallbackInfo::CallFunction(napi_env& env, napi_value results)
{
    napi_value callback = nullptr;
    if (napi_ok != napi_get_reference_value(env, callbackRef_, &callback)) {
        STATS_HILOGW(COMP_FWK, "Failed to get a callback reference.");
        return;
    }
    const int32_t maxArgc = 2;
    napi_value argv[] = {error_.GetNapiError(env), results};
    napi_value result;
    if (napi_ok != napi_call_function(env, nullptr, callback, maxArgc, argv, &result)) {
        STATS_HILOGW(COMP_FWK, "Failed to call the callback function");
    }
}

void AsyncCallbackInfo::Release(napi_env& env)
{
    NapiUtils::ReleaseReference(env, callbackRef_);
    if (asyncWork_ != nullptr) {
        napi_delete_async_work(env, asyncWork_);
        asyncWork_ = nullptr;
    }
    deferred_ = nullptr;
}

void AsyncCallbackInfo::CreatePromise(napi_env& env, napi_value& promise)
{
    if (napi_ok != napi_create_promise(env, &deferred_, &promise)) {
        STATS_HILOGW(COMP_FWK, "napi_create_promise failure");
    }
}

void AsyncCallbackInfo::CreateCallback(napi_env& env, napi_value& callback)
{
    callbackRef_ = NapiUtils::CreateReference(env, callback);
}

StatsError AsyncCallbackInfo::AsyncData::GetBatteryStatsInfo()
{
    statsInfos_.clear();
    BatteryStatsInfoList nativeStatsInfos = BatteryStatsClient::GetInstance().GetBatteryStats();
    StatsError code = BatteryStatsClient::GetInstance().GetLastError();
    std::transform(
        nativeStatsInfos.begin(), nativeStatsInfos.end(), std::back_inserter(statsInfos_), [](const auto& item) {
            StatsInfo statsInfo = {
                .uid_ = item->GetUid(),
                .type_ = item->GetConsumptionType(),
                .power_ = item->GetPower()};
            return statsInfo;
        });
    return code;
}

void AsyncCallbackInfo::AsyncData::CreateArrayValue(napi_env& env, napi_value& arrRes)
{
    if (napi_ok != napi_create_array_with_length(env, statsInfos_.size(), &arrRes)) {
        STATS_HILOGW(COMP_FWK, "napi creates array error");
        return;
    }
    int32_t index = 0;
    for (auto& item : statsInfos_) {
        napi_value result = nullptr;
        napi_create_object(env, &result);

        NapiUtils::SetIntValue(env, "uid", item.uid_, result);
        NapiUtils::SetIntValue(env, "type", item.type_, result);
        NapiUtils::SetDoubleValue(env, "power", item.power_, result);

        napi_set_element(env, arrRes, index, result);
        index++;
    }
}
} // namespace PowerMgr
} // namespace OHOS