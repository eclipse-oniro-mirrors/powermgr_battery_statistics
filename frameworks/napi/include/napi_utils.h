/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef NAPI_UTILS_H
#define NAPI_UTILS_H

#include "napi/native_api.h"
#include "napi/native_common.h"
#include "napi/native_node_api.h"

namespace OHOS {
namespace PowerMgr {
class NapiUtils {
public:
    static void GetCallbackInfo(napi_env& env, napi_callback_info& info, size_t& argc, napi_value argv[]);
    static napi_ref CreateReference(napi_env& env, napi_value& value);
    static void ReleaseReference(napi_env& env, napi_ref& ref);
    static bool CheckValueType(napi_env& env, napi_value& value, napi_valuetype checkType);
    static void SetIntValue(napi_env& env, const std::string fieldStr, const int32_t value, napi_value& result);
    static void SetDoubleValue(napi_env& env, const std::string fieldStr, const double value, napi_value& result);
};
} // PowerMgr
} // OHOS

#endif // NAPI_UTILS_H
