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

#include "statsjsonutils_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

#include "stats_cjson_utils.h"

using namespace OHOS::PowerMgr;

namespace OHOS {
namespace PowerMgr {
bool StatsJsonUtilsFuzzTest(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return false;
    }

    // Create a null-terminated string from fuzz data
    std::string jsonStr(reinterpret_cast<const char*>(data), size);

    // Parse the string as JSON
    cJSON* root = cJSON_Parse(jsonStr.c_str());

    // Test IsValidJsonString
    StatsJsonUtils::IsValidJsonString(root);
    StatsJsonUtils::IsValidJsonString(nullptr);

    // Test IsValidJsonStringAndNoEmpty
    StatsJsonUtils::IsValidJsonStringAndNoEmpty(root);
    StatsJsonUtils::IsValidJsonStringAndNoEmpty(nullptr);

    // Test IsValidJsonBool
    StatsJsonUtils::IsValidJsonBool(root);
    StatsJsonUtils::IsValidJsonBool(nullptr);

    // Test IsValidJsonNumber
    StatsJsonUtils::IsValidJsonNumber(root);
    StatsJsonUtils::IsValidJsonNumber(nullptr);

    // Test IsValidJsonObject
    StatsJsonUtils::IsValidJsonObject(root);
    StatsJsonUtils::IsValidJsonObject(nullptr);

    // Test IsValidJsonArray
    StatsJsonUtils::IsValidJsonArray(root);
    StatsJsonUtils::IsValidJsonArray(nullptr);

    // Test IsValidJsonObjectOrJsonArray
    StatsJsonUtils::IsValidJsonObjectOrJsonArray(root);
    StatsJsonUtils::IsValidJsonObjectOrJsonArray(nullptr);

    if (root != nullptr) {
        cJSON_Delete(root);
    }

    return true;
}
} // namespace PowerMgr
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    OHOS::PowerMgr::StatsJsonUtilsFuzzTest(data, size);
    return 0;
}

