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

#ifndef STATS_LOG_H
#define STATS_LOG_H

#define CONFIG_HILOG
#ifdef CONFIG_HILOG

#include <cstdint>

#include "hilog/log.h"

namespace OHOS {
namespace PowerMgr {

#ifdef STATS_HILOGF
#undef STATS_HILOGF
#endif

#ifdef STATS_HILOGE
#undef STATS_HILOGE
#endif

#ifdef STATS_HILOGW
#undef STATS_HILOGW
#endif

#ifdef STATS_HILOGI
#undef STATS_HILOGI
#endif

#ifdef STATS_HILOGD
#undef STATS_HILOGD
#endif

namespace {
// Battery stats reserved domain id range
constexpr uint32_t STATS_DOMAIN_ID_START = 0xD002960;
constexpr uint32_t STATS_DOMAIN_ID_END = STATS_DOMAIN_ID_START + 32;
constexpr uint32_t TEST_DOMAIN_ID = 0xD000F00;
}

enum BatteryStatsLogLabel {
    // Component labels, you can add if needed
    COMP_FWK = 0,
    COMP_SVC = 1,
    COMP_UTILS = 2,
    // Test label
    LABEL_TEST,
    // The end of labels, max to the domain id range length 32
    LABEL_END,
};

enum BatteryStatsLogDomain {
    DOMAIN_SERVICE = STATS_DOMAIN_ID_START + COMP_SVC, // 0xD002961
    DOMAIN_TEST = TEST_DOMAIN_ID, // 0xD000F00
    DOMAIN_END = STATS_DOMAIN_ID_END, // Max to 0xD002980, keep the sequence and length same as BatteryStatsLogLabel
};

struct BatteryStatsLogLabelTag {
    uint32_t logLabel;
    const char* tag;
};

// Keep the sequence same as BatteryStatsLogLabel
static constexpr BatteryStatsLogLabelTag STATS_LABEL_TAG[LABEL_END] = {
    {COMP_FWK,       "StatsFwk"},
    {COMP_SVC,       "StatsSvc"},
    {COMP_UTILS,     "StatsUtils"},
    {DOMAIN_TEST,    "StatsTest"},
};

struct BatteryStatsLogLabelDomain {
    uint32_t logLabel;
    uint32_t domainId;
};

static constexpr BatteryStatsLogLabelDomain STATS_LABEL_DOMAIN[LABEL_END] = {
    {COMP_FWK,              DOMAIN_SERVICE},
    {COMP_SVC,              DOMAIN_SERVICE},
    {COMP_UTILS,            DOMAIN_SERVICE},
    {LABEL_TEST,            DOMAIN_SERVICE},
};

#define STATS_HILOGF(domain, ...) \
    ((void)HILOG_IMPL(LOG_CORE, LOG_FATAL, STATS_LABEL_DOMAIN[domain].domainId, STATS_LABEL_TAG[domain].tag,   \
    ##__VA_ARGS__))
#define STATS_HILOGE(domain, ...) \
    ((void)HILOG_IMPL(LOG_CORE, LOG_ERROR, STATS_LABEL_DOMAIN[domain].domainId, STATS_LABEL_TAG[domain].tag,   \
    ##__VA_ARGS__))
#define STATS_HILOGW(domain, ...) \
    ((void)HILOG_IMPL(LOG_CORE, LOG_WARN, STATS_LABEL_DOMAIN[domain].domainId, STATS_LABEL_TAG[domain].tag,    \
    ##__VA_ARGS__))
#define STATS_HILOGI(domain, ...) \
    ((void)HILOG_IMPL(LOG_CORE, LOG_INFO, STATS_LABEL_DOMAIN[domain].domainId, STATS_LABEL_TAG[domain].tag,    \
    ##__VA_ARGS__))
#define STATS_HILOGD(domain, ...) \
    ((void)HILOG_IMPL(LOG_CORE, LOG_DEBUG, STATS_LABEL_DOMAIN[domain].domainId, STATS_LABEL_TAG[domain].tag,   \
    ##__VA_ARGS__))
} // namespace PowerMgr
} // namespace OHOS

#else

#define STATS_HILOGF(...)
#define STATS_HILOGE(...)
#define STATS_HILOGW(...)
#define STATS_HILOGI(...)
#define STATS_HILOGD(...)

#endif // CONFIG_HILOG

#endif // STATS_LOG_H