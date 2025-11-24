/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#include "battery_stats_service.h"

#include <file_ex.h>
#include <cmath>
#include <ipc_skeleton.h>

#include "common_event_data.h"
#include "common_event_manager.h"
#include "common_event_subscribe_info.h"
#include "common_event_support.h"
#include "hisysevent.h"
#include "hisysevent_manager.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "permission.h"
#include "sysparam.h"
#include "system_ability_definition.h"
#include "xcollie/watchdog.h"

#include "battery_stats_dumper.h"
#include "battery_stats_listener.h"
#include "battery_stats_subscriber.h"
#include "stats_common.h"
#include "stats_hisysevent.h"
#include "stats_xcollie.h"

namespace OHOS {
namespace PowerMgr {
sptr<BatteryStatsService> BatteryStatsService::instance_ = nullptr;
std::mutex BatteryStatsService::singletonMutex_;
namespace {
auto g_statsService = BatteryStatsService::GetInstance();
const bool G_REGISTER_RESULT = SystemAbility::MakeAndRegisterAbility(g_statsService.GetRefPtr());
SysParam::BootCompletedCallback g_bootCompletedCallback;
}
std::atomic_bool BatteryStatsService::isBootCompleted_ = false;

BatteryStatsService::BatteryStatsService() : SystemAbility(POWER_MANAGER_BATT_STATS_SERVICE_ID, true) {}

BatteryStatsService::~BatteryStatsService() {}

sptr<BatteryStatsService> BatteryStatsService::GetInstance()
{
    if (instance_ == nullptr) {
        std::lock_guard<std::mutex> lock(singletonMutex_);
        if (instance_ == nullptr) {
            instance_ = new BatteryStatsService();
        }
    }
    return instance_;
}

void BatteryStatsService::OnStart()
{
    if (ready_) {
        STATS_HILOGI(COMP_SVC, "OnStart is ready, nothing to do");
        return;
    }
    if (!(Init())) {
        STATS_HILOGE(COMP_SVC, "Call init failed");
        return;
    }
    AddSystemAbilityListener(DFX_SYS_EVENT_SERVICE_ABILITY_ID);
    AddSystemAbilityListener(COMMON_EVENT_SERVICE_ID);
    if (!Publish(BatteryStatsService::GetInstance())) {
        STATS_HILOGE(COMP_SVC, "OnStart register to system ability manager failed");
        return;
    }
    RegisterBootCompletedCallback();
    ready_ = true;
}

void BatteryStatsService::OnStop()
{
    if (!ready_) {
        STATS_HILOGI(COMP_SVC, "OnStop is not ready, nothing to do");
        return;
    }
    ready_ = false;
    isBootCompleted_ = false;
    RemoveSystemAbilityListener(DFX_SYS_EVENT_SERVICE_ABILITY_ID);
    RemoveSystemAbilityListener(COMMON_EVENT_SERVICE_ID);
    HiviewDFX::HiSysEventManager::RemoveListener(listenerPtr_);
    if (!OHOS::EventFwk::CommonEventManager::UnSubscribeCommonEvent(subscriberPtr_)) {
        STATS_HILOGE(COMP_SVC, "OnStart unregister to commonevent manager failed");
    }
}

void BatteryStatsService::RegisterBootCompletedCallback()
{
    g_bootCompletedCallback = []() {
        isBootCompleted_ = true;
    };
    SysParam::RegisterBootCompletedCallback(g_bootCompletedCallback);
}


void BatteryStatsService::OnAddSystemAbility(int32_t systemAbilityId, const std::string& deviceId)
{
    STATS_HILOGI(COMP_SVC, "systemAbilityId=%{public}d, deviceId=%{private}s", systemAbilityId, deviceId.c_str());
    if (systemAbilityId == DFX_SYS_EVENT_SERVICE_ABILITY_ID) {
        AddHiSysEventListener();
    }
    if (systemAbilityId == COMMON_EVENT_SERVICE_ID) {
        SubscribeCommonEvent();
    }
}

bool BatteryStatsService::Init()
{
    if (parser_ == nullptr) {
        parser_ = std::make_shared<BatteryStatsParser>();
        if (!parser_->Init()) {
            STATS_HILOGE(COMP_SVC, "Battery stats parser initialization failed");
            return false;
        }
    }

    if (core_ == nullptr) {
        core_ = std::make_shared<BatteryStatsCore>();
        if (!core_->Init()) {
            STATS_HILOGE(COMP_SVC, "Battery stats core initialization failed");
            return false;
        }
    }

    if (detector_ == nullptr) {
        detector_ = std::make_shared<BatteryStatsDetector>();
    }

    return true;
}

bool BatteryStatsService::SubscribeCommonEvent()
{
    using namespace OHOS::EventFwk;
    bool result = false;
    MatchingSkills matchingSkills;
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_SHUTDOWN);
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_BATTERY_CHANGED);
    CommonEventSubscribeInfo subscribeInfo(matchingSkills);
    subscribeInfo.SetThreadMode(CommonEventSubscribeInfo::ThreadMode::COMMON);
    if (!subscriberPtr_) {
        subscriberPtr_ = std::make_shared<BatteryStatsSubscriber>(subscribeInfo);
    }
    result = CommonEventManager::SubscribeCommonEvent(subscriberPtr_);
    if (!result) {
        STATS_HILOGE(COMP_SVC, "Subscribe CommonEvent failed");
    }
    return result;
}

bool BatteryStatsService::AddHiSysEventListener()
{
    if (!listenerPtr_) {
        OHOS::EventFwk::CommonEventSubscribeInfo info;
        listenerPtr_ = std::make_shared<BatteryStatsListener>();
    }
    OHOS::HiviewDFX::ListenerRule statsRule("PowerStats");
    OHOS::HiviewDFX::ListenerRule distSchedRule("DISTSCHEDULE", StatsHiSysEvent::START_REMOTE_ABILITY);
    std::vector<OHOS::HiviewDFX::ListenerRule> sysRules;
    sysRules.push_back(statsRule);
    sysRules.push_back(distSchedRule);
    auto res = HiviewDFX::HiSysEventManager::AddListener(listenerPtr_, sysRules);
    if (res != 0) {
        STATS_HILOGE(COMP_SVC, "Listener added failed");
    }
    return res;
}

bool BatteryStatsService::IsServiceReady() const
{
    return ready_;
}

BatteryStatsInfoList BatteryStatsService::GetBatteryStats()
{
    std::lock_guard lock(mutex_);
    BatteryStatsInfoList statsInfoList = {};
    if (!Permission::IsSystem()) {
        lastError_ = static_cast<int32_t>(StatsError::ERR_SYSTEM_API_DENIED);
        return statsInfoList;
    }
    core_->ComputePower();
    statsInfoList = core_->GetBatteryStats();
    return statsInfoList;
}

int32_t BatteryStatsService::Dump(int32_t fd, const std::vector<std::u16string>& args)
{
    if (!isBootCompleted_) {
        return ERR_NO_INIT;
    }
    if (!Permission::IsSystem()) {
        return ERR_PERMISSION_DENIED;
    }
    std::lock_guard lock(mutex_);
    std::vector<std::string> argsInStr;
    std::transform(args.begin(), args.end(), std::back_inserter(argsInStr),
        [](const std::u16string &arg) {
        std::string ret = Str16ToStr8(arg);
        STATS_HILOGD(COMP_SVC, "arg: %{public}s", ret.c_str());
        return ret;
    });
    std::string result;
    BatteryStatsDumper::Dump(argsInStr, result);
    if (!SaveStringToFd(fd, result)) {
        STATS_HILOGE(COMP_SVC, "Dump save to fd failed, %{public}s", result.c_str());
        return ERR_OK;
    }
    return ERR_OK;
}

double BatteryStatsService::GetAppStatsMah(const int32_t& uid)
{
    std::lock_guard lock(mutex_);
    if (!Permission::IsSystem()) {
        lastError_ = static_cast<int32_t>(StatsError::ERR_SYSTEM_API_DENIED);
        return StatsUtils::DEFAULT_VALUE;
    }
    core_->ComputePower();
    return core_->GetAppStatsMah(uid);
}

double BatteryStatsService::GetAppStatsPercent(const int32_t& uid)
{
    std::lock_guard lock(mutex_);
    if (!Permission::IsSystem()) {
        lastError_ = static_cast<int32_t>(StatsError::ERR_SYSTEM_API_DENIED);
        return StatsUtils::DEFAULT_VALUE;
    }
    core_->ComputePower();
    return core_->GetAppStatsPercent(uid);
}

double BatteryStatsService::GetPartStatsMah(const BatteryStatsInfo::ConsumptionType& type)
{
    std::lock_guard lock(mutex_);
    if (!Permission::IsSystem()) {
        lastError_ = static_cast<int32_t>(StatsError::ERR_SYSTEM_API_DENIED);
        return StatsUtils::DEFAULT_VALUE;
    }
    core_->ComputePower();
    return core_->GetPartStatsMah(type);
}

double BatteryStatsService::GetPartStatsPercent(const BatteryStatsInfo::ConsumptionType& type)
{
    std::lock_guard lock(mutex_);
    if (!Permission::IsSystem()) {
        lastError_ = static_cast<int32_t>(StatsError::ERR_SYSTEM_API_DENIED);
        return StatsUtils::DEFAULT_VALUE;
    }
    core_->ComputePower();
    return core_->GetPartStatsPercent(type);
}

uint64_t BatteryStatsService::GetTotalTimeSecond(const StatsUtils::StatsType& statsType, const int32_t& uid)
{
    if (!Permission::IsSystem()) {
        lastError_ = static_cast<int32_t>(StatsError::ERR_SYSTEM_API_DENIED);
        return ERR_OK;
    }
    STATS_HILOGD(COMP_SVC, "statsType: %{public}d, uid: %{public}d", statsType, uid);
    uint64_t timeSecond;
    if (uid > StatsUtils::INVALID_VALUE) {
        double timeMs = static_cast<double>(core_->GetTotalTimeMs(uid, statsType));
        timeSecond = round(timeMs / StatsUtils::MS_IN_SECOND);
    } else {
        double timeMs = static_cast<double>(core_->GetTotalTimeMs(statsType));
        timeSecond = round(timeMs / StatsUtils::MS_IN_SECOND);
    }
    return timeSecond;
}

uint64_t BatteryStatsService::GetTotalDataBytes(const StatsUtils::StatsType& statsType, const int32_t& uid)
{
    if (!Permission::IsSystem()) {
        lastError_ = static_cast<int32_t>(StatsError::ERR_SYSTEM_API_DENIED);
        return ERR_OK;
    }
    return core_->GetTotalDataCount(statsType, uid);
}

void BatteryStatsService::Reset()
{
    if (!Permission::IsSystem()) {
        return;
    }
    core_->Reset();
}

std::shared_ptr<BatteryStatsCore> BatteryStatsService::GetBatteryStatsCore() const
{
    return core_;
}

std::shared_ptr<BatteryStatsParser> BatteryStatsService::GetBatteryStatsParser() const
{
    return parser_;
}

std::shared_ptr<BatteryStatsDetector> BatteryStatsService::GetBatteryStatsDetector() const
{
    return detector_;
}

void BatteryStatsService::SetOnBattery(bool isOnBattery)
{
    if (!Permission::IsSystem()) {
        return;
    }
    StatsHelper::SetOnBattery(isOnBattery);
}

std::string BatteryStatsService::ShellDump(const std::vector<std::string>& args, uint32_t argc)
{
    if (!Permission::IsSystem()|| !isBootCompleted_) {
        return "";
    }
    std::lock_guard lock(mutex_);
    pid_t pid = IPCSkeleton::GetCallingPid();
    std::string result;
    bool ret = BatteryStatsDumper::Dump(args, result);
    STATS_HILOGI(COMP_SVC, "PID: %{public}d, Dump result :%{public}d", pid, ret);
    return result;
}

int32_t BatteryStatsService::GetBatteryStatsIpc(ParcelableBatteryStatsList& batteryStats, int32_t& tempError)
{
    StatsXCollie statsXCollie("BatteryStatsService::GetBatteryStatsIpc", false);
    batteryStats.statsList_ = GetBatteryStats();
    tempError = lastError_.load();
    lastError_ = static_cast<int32_t>(StatsError::ERR_OK);
    return ERR_OK;
}

int32_t BatteryStatsService::GetAppStatsMahIpc(int32_t uid, double& appStatsMah, int32_t& tempError)
{
    StatsXCollie statsXCollie("BatteryStatsService::GetAppStatsMahIpc", false);
    appStatsMah = GetAppStatsMah(uid);
    tempError = lastError_.load();
    lastError_ = static_cast<int32_t>(StatsError::ERR_OK);
    return ERR_OK;
}

int32_t BatteryStatsService::GetAppStatsPercentIpc(int32_t uid, double& appStatsPercent, int32_t& tempError)
{
    StatsXCollie statsXCollie("BatteryStatsService::GetAppStatsPercentIpc", false);
    appStatsPercent = GetAppStatsPercent(uid);
    tempError = lastError_.load();
    lastError_ = static_cast<int32_t>(StatsError::ERR_OK);
    return ERR_OK;
}

int32_t BatteryStatsService::GetPartStatsMahIpc(int32_t type, double& partStatsMah, int32_t& tempError)
{
    StatsXCollie statsXCollie("BatteryStatsService::GetPartStatsMahIpc", false);
    partStatsMah = GetPartStatsMah(static_cast<BatteryStatsInfo::ConsumptionType>(type));
    tempError = lastError_.load();
    lastError_ = static_cast<int32_t>(StatsError::ERR_OK);
    return ERR_OK;
}

int32_t BatteryStatsService::GetPartStatsPercentIpc(int32_t type, double& partStatsPercent, int32_t& tempError)
{
    StatsXCollie statsXCollie("BatteryStatsService::GetPartStatsPercentIpc", false);
    partStatsPercent = GetPartStatsPercent(static_cast<BatteryStatsInfo::ConsumptionType>(type));
    tempError = lastError_.load();
    lastError_ = static_cast<int32_t>(StatsError::ERR_OK);
    return ERR_OK;
}

int32_t BatteryStatsService::GetTotalTimeSecondIpc(int32_t statsType, int32_t uid, uint64_t& totalTimeSecond)
{
    StatsXCollie statsXCollie("BatteryStatsService::GetTotalTimeSecondIpc", false);
    totalTimeSecond = GetTotalTimeSecond(static_cast<StatsUtils::StatsType>(statsType), uid);
    return ERR_OK;
}

int32_t BatteryStatsService::GetTotalDataBytesIpc(int32_t statsType, int32_t uid, uint64_t& totalDataBytes)
{
    StatsXCollie statsXCollie("BatteryStatsService::GetTotalDataBytesIpc", false);
    totalDataBytes = GetTotalDataBytes(static_cast<StatsUtils::StatsType>(statsType), uid);
    return ERR_OK;
}

int32_t BatteryStatsService::ResetIpc()
{
    StatsXCollie statsXCollie("BatteryStatsService::ResetIpc", false);
    Reset();
    return ERR_OK;
}

int32_t BatteryStatsService::SetOnBatteryIpc(bool isOnBattery)
{
    StatsXCollie statsXCollie("BatteryStatsService::SetOnBatteryIpc", false);
    SetOnBattery(isOnBattery);
    return ERR_OK;
}

int32_t BatteryStatsService::ShellDumpIpc(const std::vector<std::string>& args, uint32_t argc, std::string& dumpShell)
{
    StatsXCollie statsXCollie("BatteryStatsService::ShellDumpIpc", false);
    dumpShell = ShellDump(args, argc);
    return ERR_OK;
}

void BatteryStatsService::DestroyInstance()
{
    std::lock_guard<std::mutex> lock(singletonMutex_);
    if (instance_) {
        instance_.clear();
        instance_ = nullptr;
    }
}
} // namespace PowerMgr
} // namespace OHOS
