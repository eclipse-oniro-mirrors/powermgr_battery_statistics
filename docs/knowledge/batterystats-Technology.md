# 技术术语知识

battery_statistics 模块核心技术框架和机制的知识定义，涵盖价值认知、原理、使用方式和代码关联。路径均相对仓根（`base/powermgr/battery_statistics`）。数据与持有者见 [业务实体知识](batterystats-Entity.md)，术语释义见 [术语表](glossary.md)，分层与约束总览见 [架构设计及约束](architecture.md)。

## HiSysEvent 事件订阅与解析分发机制

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | 耗电归因需要所有子系统（电源/显示/相机/音频/传感器/蓝牙/WiFi/通话/热管理/分布式调度）的活动记录，逐一调用各子系统接口会产生大量跨服务调用与接口耦合，且无法获得"已发生"的历史事件 |
| 技术目标 | 以统一事件总线做单向数据采集：一次订阅、多源汇聚、JSON 自描述、非法事件可拒 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | `BatteryStatsService::AddHiSysEventListener` 向 `HiviewDFX::HiSysEventManager` 注册两条 `ListenerRule`：`("PowerStats")` 全域 + `("DISTSCHEDULE", START_REMOTE_ABILITY)` 单事件（`battery_stats_service.cpp:168-184`）。事件到达 `BatteryStatsListener::OnEvent`：`StatsHiSysEvent::CheckHiSysEvent` 白名单校验事件名 → `cJSON_Parse(AsJson())` → `ProcessHiSysEvent` 按事件名路由到 20+ 个 `Process*Event`（每个负责从 JSON 抽取 type/state/level/uid/pid/traffic 等字段填 `StatsUtils::StatsData`）→ `BatteryStatsDetector::HandleStatsChangedEvent`（`battery_stats_listener.cpp:49-89`） |
| 关键流程 | 事件名常量集中在 `utils/native/include/stats_hisysevent.h:66-94`（RUNNINGLOCK、SCREEN_STATE、BRIGHTNESS_NIT、BACKLIGHT_DISCOUNT、AMBIENT_LIGHT、CHANGED、POWER_TEMPERATURE、LEVEL_CHANGED、POWER_WORKSCHEDULER、WORK_ADD/REMOVE/START/STOP、TORCH_STATE、CAMERA_CONNECT/DISCONNECT、FLASHLIGHT_ON/OFF、STREAM_CHANGE、POWER_SENSOR_GRAVITY/PROXIMITY、GNSS_STATE、BR/BLE_SWITCH_STATE、DISCOVERY_STATE、BLE_SCAN_START/STOP、WIFI_CONNECTION、WIFI_SCAN、CALL_STATE、DATA_CONNECTION_STATE 等）；蓝芽/通话/显示相关解析代码由 `HAS_BATTERYSTATS_*_PART` 宏裁剪；thermal/display 类事件仅进调试信息不进统计 |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | 无对外 API。新增统计事件：在 `stats_hisysevent.h` 加事件名常量与 `CheckHiSysEvent` 白名单 → listener 加 `Process*Event` 解析 → detector 的 `IsStateRelated/IsDurationRelated` 分类 → core 加分发 case |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `BatteryStatsListener`（`services/native/include/battery_stats_listener.h`）、`StatsHiSysEvent`（`utils/native/include/stats_hisysevent.h`） |
| 核心成员 | 事件名常量表；`StatsData` 载荷（`utils/native/include/stats_utils.h:102-115`） |
| 核心方法 | `OnEvent`/`OnServiceDied`/`ProcessHiSysEvent`/`ProcessWakelockEvent`/`ProcessDisplayEvent`/`ProcessBatteryEvent`/`ProcessThermalEvent`/`ProcessWorkschedulerEvent`/`ProcessPhoneEvent`/`ProcessFlashlightEvent`/`ProcessCameraEvent`/`ProcessAudioEvent`/`ProcessSensorEvent`/`ProcessGnssEvent`/`ProcessBluetooth*(Br/Ble)Event`/`ProcessWifiEvent`/`ProcessDistributedSchedulerEvent`/`ProcessAlarmEvent`（`services/native/src/battery_stats_listener.cpp`） |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | 订阅依赖 DFX 事件服务上线（`OnAddSystemAbility(DFX_SYS_EVENT_SERVICE_ABILITY_ID)` 补做）；懒启动前的早期事件不可得，靠持久化恢复补偿 |
| 并发约束 | `OnEvent` 在 HiSysEvent 分发线程执行，只做解析与 timer/counter 更新，禁止阻塞（AGENTS.md 禁止事项） |
| 安全约束 | 事件 JSON 来自系统域（HiView 分发），uid 字段按值信任；deviceId 等敏感字段仅 `%{private}s` 日志输出 |

## CommonEvent 订阅机制（关机保存与供电状态维护）

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | 统计数据在内存中累加，掉电即失；"是否使用电池"决定耗电归因口径，必须与真实插拔精确同步 |
| 技术目标 | 在关键边界（关机、插拔、充满）执行持久化与状态切换，保证跨开关机连续与口径正确 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | `SubscribeCommonEvent` 订阅两事件（COMMON 线程模式，`battery_stats_service.cpp:149-166`）：`COMMON_EVENT_SHUTDOWN` → `SaveBatteryStatsData()`（先 ComputePower 再写 JSON）；`COMMON_EVENT_BATTERY_CHANGED` → 读 capacity/pluggedType 参数：capacity==100（`BATTERY_LEVEL_FULL`）→ `Core::Reset()` 开启新放电周期；pluggedType 为 NONE/BUTT → `StatsHelper::SetOnBattery(true)`，否则 false（`battery_stats_subscriber.cpp:34-63`） |
| 关键流程 | OnBattery 变化时 `StatsHelper` 换算电池供电时钟基线（`latestUnplug*` 与 `onBattery*`），使 ActiveTimer 的"供电时长"自动剔除充电区间 |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | 无对外 API；系统侧可经 IPC `SetOnBatteryIpc` 覆盖（用于测试/特殊产品形态） |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `BatteryStatsSubscriber`（`services/native/include/battery_stats_subscriber.h`） |
| 核心成员 | 订阅集合 {SHUTDOWN, BATTERY_CHANGED} |
| 核心方法 | `OnReceiveEvent`（实现 `services/native/src/battery_stats_subscriber.cpp:34-63`） |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | 订阅依赖 CES 上线（`OnAddSystemAbility(COMMON_EVENT_SERVICE_ID)` 补做）；SHUTDOWN 保存必须在该事件时限内完成（JSON 全量序列化） |
| 并发约束 | `OnReceiveEvent` 与查询线程并发，Save 内 ComputePower 持 Core 锁 |
| 安全约束 | 事件参数（capacity/pluggedType）来自 battery_manager 发布的公共事件，可信 |

## 耗电计算机制（ComputePower）

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | 时长/次数只是原始量，用户与策略需要的是 mAh 与占比；且各实体数据持续变化，需要一致的全量快照 |
| 技术目标 | 把"供电时段内的活动量 × 平均电流系数"换算为耗电值，并在每次查询前得到自洽结果集 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | `BatteryStatsService` 各查询先 `core_->ComputePower()`：持锁 → XCollie 60s 看护 → `ResetStatsEntity()`（清 totalPowerMah_ 与 statsInfoList_）→ 依次 `Calculate` 7 个实体：UidEntity（内部级联 camera/flashlight/audio/sensor/gnss/cpu/wakelock/alarm/bluetooth 的按 uid 计算，`uid_entity.cpp:64-110`）、Bluetooth、Idle、Phone、Screen、Wifi、User（`battery_stats_core.cpp:146-164`）。计算公式：mAh = Σ(时长h × 该项系数mA)，分档项（radio 信号档、屏幕亮度档、CPU 频点）逐档乘对应系数累加。percent = power / `GetTotalPowerMah()`（总耗电 ≤0 时返回 0） |
| 关键流程 | 结果写入双通道：静态 `statsInfoList_`（供 GetBatteryStats 全量）与各实体查询 map（供 GetAppStatsMah 等单点查询经列表检索，`battery_stats_core.cpp:718-782`）。Load 恢复的 Power 节直接进 statsInfoList_ 作上轮基线 |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | `BatteryStatsClient::GetBatteryStats/GetAppStatsMah/GetAppStatsPercent/GetPartStatsMah/GetPartStatsPercent`（均触发服务端重算）；NAPI 对应 `getBatteryStats/getAppPowerValue/getAppPowerPercent/getHardwareUnitPowerValue/getHardwareUnitPowerPercent` |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `BatteryStatsCore` + 15 实体 + `BatteryStatsParser` |
| 核心成员 | 静态 `totalPowerMah_`/`statsInfoList_`（`services/native/src/entities/battery_stats_entity.cpp:22`） |
| 核心方法 | `ComputePower`、各实体 `Calculate/GetEntityPowerMah`、`ResetStatsEntity`/`UpdateStatsInfoList`/`GetTotalPowerMah`；`CpuEntity::CalculateCpuActivePower/CalculateCpuClusterPower/CalculateCpuSpeedPower` |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | 全量重算的复杂度随 uid 数与实体数线性增长，由 XCollie 看护 60s 上限；系数缺失项返回 0 参与汇总 |
| 并发约束 | ComputePower 与查询互斥（服务锁）；中途不得插入 UpdateStats（Core 锁保证） |
| 安全约束 | 计算不涉权限（权限在 Service 入口已校验） |

## ActiveTimer/OnBattery 供电时间轴机制

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | 耗电归因必须只统计"电池供电"时段：充电期间的使用不应计入；且计时需要支持"仍在进行中"的读数与跨事件的区间累加 |
| 技术目标 | 提供供电敏感的单调时钟与幂等的区间计时/计数原语 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | `StatsHelper` 双时钟体系：`GetBootTimeMs`/`GetUpTimeMs` 为基准，`GetOnBatteryBootTimeMs`/`GetOnBatteryUpTimeMs` 只在 `onBattery_` 为真时前进（插拔时记录 `latestUnplug*` 基线换算，`utils/native/src/stats_helper.cpp`）。`ActiveTimer::StartRunning` 记供电时钟起点（已运行则幂等 false）；`StopRunning` 累加差值；`GetRunningTimeMs` 对运行中 timer 结算到当前并推进起点（读不中断计）；`AddRunningTimeMs` 注入外部时长（仅正值）。`Counter::AddCount` 仅 `IsOnBattery()` 时累加（`stats_helper.h:28-117`） |
| 关键流程 | 实体在 (uid, statsType[, level]) 维度 GetOrCreateTimer；Core 的 UpdateTimer 分发 Start/Stop；查询经 GetActiveTimeMs → GetRunningTimeMs |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | 内部原语（`utils/native/`，对 thermal_manager 可见）；外部经 `GetTotalTimeSecond` 间接消费（ms→s 四舍五入，`battery_stats_service.cpp:273-289`） |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `StatsHelper::ActiveTimer`、`StatsHelper::Counter`（`utils/native/include/stats_helper.h`） |
| 核心成员 | `onBattery_`、`screenOff_`、`onBatteryBootTimeMs_`、`latestUnplugBootTimeMs_` 等 6 个静态时钟字段 |
| 核心方法 | `SetOnBattery`/`IsOnBattery`/`GetOnBatteryBootTimeMs`/`GetOnBatteryUpTimeMs`/`SetScreenOff`/`IsOnBatteryScreenOff` |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | 时钟分辨率 ms，依赖 CLOCK_BOOTTIME 类系统时钟；进程重启后 ActiveTimer 内存态丢失（Power 值经 JSON 恢复但计时器归零） |
| 并发约束 | 静态全局态写路径集中在三个来源（Init 初值/Subscriber 事件/IPCIpc），当前无锁；timer 启停由事件线程串行 |
| 安全约束 | 无（纯时间计算） |

## 统计数据 JSON 持久化机制

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | 内存统计随关机/重启丢失，耗电排行需要跨开关机的连续视角 |
| 技术目标 | 关机边界落盘、启动恢复，且恢复值可作为新周期基线继续参与排行 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | **保存**：SHUTDOWN 事件 → `SaveBatteryStatsData`：先 `ComputePower()` → cJSON 构建三节：Power（键=uid（正数）或 ConsumptionType 值（负数），值=mAh；USER 类型不单独存）、Hardware（bluetooth_br/ble_on、screen_on、screen_brightness[SCREEN_BRIGHTNESS_BIN+1 数组]、wifi_on、wifi_scan、cpu_idle、radio_on/radio_data[RADIO_SIGNAL_BIN 数组]）、Software（每 uid：camera_on、flashlight_on、gnss_on、audio_on、cpu_awake、sensor_gravity、sensor_proximity、alarm、cpu_time、bluetooth_br/ble_scan）→ `fopen/fwrite` 写 `/data/service/el0/stats/battery_stats.json`（`battery_stats_core.cpp:784-1090`）。**恢复**：`Core::Init` → `LoadBatteryStatsData` 读文件 → cJSON 校验 → `UpdateStatsEntity`：重放 Power 节为 BatteryStatsInfo 列表（uid→APP 类型；负数键→硬件类型；并经 `OhosAccountKits::GetDeviceAccountIdByUID` 聚合 USER 条目），硬件/软件节读入对应实体 timer 基线（`battery_stats_core.cpp:1092-1183`） |
| 关键流程 | 文件损坏/缺失仅告警不阻断启动（`Init` 仍返回 true） |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | 无手动触发入口；自动于关机事件。调试可经 `hidumper -s 3304` 观察当前统计 |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `BatteryStatsCore`、`StatsJsonUtils`（`utils/native/include/stats_cjson_utils.h`） |
| 核心成员 | `BATTERY_STATS_JSON = "/data/service/el0/stats/battery_stats.json"`（`battery_stats_core.cpp:57`） |
| 核心方法 | `SaveBatteryStatsData`/`SaveForPower`/`SaveForHardware(Internal)`/`SaveForSoftware(Common/Internal/Connectivity)`/`LoadBatteryStatsData`/`UpdateStatsEntity` |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | 恢复的是"上轮 Power 结果 + 硬件/软件计时基线"，内存 timer 的运行态不恢复；写入在关机事件时限内完成，大 uid 数场景需关注序列化耗时 |
| 并发约束 | Save 前的 ComputePower 持 Core 锁 |
| 安全约束 | 文件位于 el0 服务目录，属系统信任域；schema 变更必须向后兼容（旧版本忽略新字段，AGENTS.md 架构不变量） |

## CPU 时间增量采集机制（CpuTimeReader）

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | 应用耗电的最大头是 CPU，而内核按 uid 维度导出的 CPU 时间是**单调累计值**，直接使用会把开机以来的全部时间计入当前统计周期 |
| 技术目标 | 以增量（差值）方式采集每 uid 的 active/cluster/freq/总 CPU 时间，供 CpuEntity 分路计算 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | `CpuTimeReader` 维护两套 map：`last*Map_`（上次快照）与累计 map（当前值）；`UpdateCpuTime` 依次读四个内核文件——`/proc/uid_concurrent_active_time`（active）、`/proc/uid_concurrent_policy_time`（cluster，按 CPU 策略簇）、`/proc/uid_time_in_state`（freq，按簇按频点）、`/proc/uid_cputime/show_uid_stat`（uid 总时间）——解析行格式为 `uid: t1 t2 ...`，计算与 last 的增量累加进累计 map（簇间时间按 `ReadPolicy` 解析的 policy 分布，频点时间经 `DistributeFreqTime/AddFreqTimeToUid` 归到 uid，`services/native/src/cpu_time_reader.cpp:29-32, 142-165`）。`CpuEntity` 构造时 `cpuReader_->Init()` 首刷；`CpuEntity::Calculate(uid)` 直接读 `GetUidCpuTimeMs(uid)` 缓存值计算 |
| 关键流程 | 注意：当前生产代码中 `CpuTimeReader::UpdateCpuTime` 的刷新仅发生在 `Init()`（实体构造/服务启动时），`CpuEntity::Calculate` 消费的是缓存快照；`CpuEntity::UpdateCpuTime()` 公开刷新接口现仅被单元测试调用。AGENTS.md 中"Detector 周期采集 CPU 时间"的描述与当前代码不符——生产路径无周期刷新调用（此为按代码如实记录的事实，进一步的产品预期需与维护者确认） |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | `GetUidCpuActiveTimeMs(uid)`、`GetUidCpuClusterTimeMs(uid, cluster)`、`GetUidCpuFreqTimeMs(uid, cluster, speed)`、`GetUidCpuTimeMs(uid)`（向量）、`UpdateCpuTime()` |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `CpuTimeReader`（`services/native/include/cpu_time_reader.h`）、消费者 `CpuEntity` |
| 核心成员 | `activeTimeMap_`、`clusterTimeMap_`、`freqTimeMap_`、`uidTimeMap_` 及对应 `last*Map_`、`clustersMap_` |
| 核心方法 | `Init`/`UpdateCpuTime`/`ReadUidCpuActiveTime`/`ReadUidCpuClusterTime`/`ReadUidCpuFreqTime`/`ReadUidCpuTime`/`GetUidCpuTimeMs`/`DumpInfo` |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | `/proc` 读取非原子（AGENTS.md 项目特定陷阱），高频读取可能得到不一致快照；文件不存在（内核未开 uid 统计）时读取失败仅告警 |
| 并发约束 | reader 无锁；生产写读路径当前为 Init 单次 + 查询线程读缓存；若引入周期刷新需评估与 Calculate 的并发 |
| 安全约束 | 读系统 proc 接口，无权限要求；uid 数据只进统计不外泄（dump 需系统权限） |

## 懒启动与依赖就绪时序机制

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | `run-on-create: false` 让服务在首次 IPC 才拉起，但事件订阅依赖 DFX 事件服务与 CES 先就绪；boot 早期调用 Dump 会拿到不完整状态 |
| 技术目标 | 用最少的常驻开销换取"订阅不漏、查询门控正确" |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | `OnStart`：Init（parser/core/detector，core Init 含插拔初值与 JSON 恢复）→ `AddSystemAbilityListener(DFX_SYS_EVENT_SERVICE_ABILITY_ID)` + `AddSystemAbilityListener(COMMON_EVENT_SERVICE_ID)` → `Publish` → `RegisterBootCompletedCallback` 置 `isBootCompleted_`。`OnAddSystemAbility` 按上线 SA 分别补做 `AddHiSysEventListener`（HiSysEvent 规则）与 `SubscribeCommonEvent`（`battery_stats_service.cpp:68-122`）。Dump/ShellDump 双门控 `isBootCompleted_` + `Permission::IsSystem()`。懒启动的丢失补偿：启动时主动查 `BatterySrvClient::GetPluggedType` 定 OnBattery 初值 + Load 恢复上轮 Power |
| 关键流程 | 客户端首次 `Connect()` 的 `GetSystemAbility(3304)` 即拉起触发点（非 `CheckSystemAbility`，与 battery_manager 的 Check 语义不同） |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | 无直接 API；系统侧 `SetOnBatteryIpc` 可在服务拉起后校正供电状态 |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `BatteryStatsService` |
| 核心成员 | `ready_`、`isBootCompleted_`（atomic_bool）、`instance_`/`singletonMutex_`、`DEPENDENCY_CHECK_DELAY_MS` 常量（2000） |
| 核心方法 | `OnStart`/`OnStop`/`OnAddSystemAbility`/`RegisterBootCompletedCallback`/`IsServiceReady`/`GetInstance`/`DestroyInstance` |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | `sa_profile/3304.json` 的 `run-on-create: false` 禁止改 true（AGENTS.md 禁止事项：增加首屏开销）；早于订阅生效的子系统事件不可在线获得 |
| 并发约束 | OnStart 单线程初始化；OnAddSystemAbility 可能与已到查询并发（订阅补做幂等：`if (!listenerPtr_)`/`if (!subscriberPtr_)`） |
| 安全约束 | 无 |

## IPC 通道与双错误码机制

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | 查询接口需要区分"IPC 送达失败"与"权限拒绝等业务失败"两类错误；跨进程返回复杂列表需要稳定序列化；服务懒启动意味着代理获取时机不确定 |
| 技术目标 | 送达与业务错误分离、列表可序列化、代理死亡自愈 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | **IDL 生成**：`services/IBatteryStats.idl` 10 方法经 `idl_gen_interface("batterystats_interface")` 生成 proxy/stub（`services/BUILD.gn:24-76`；`sequenceable ParcelableBatteryStatsList` 声明使列表可作 [out] 参数）。**双错误码**：`*Ipc` 方法返回值恒 ERR_OK（送达），业务错误服务端写 `lastError_`（atomic_int32_t）→ 经 `tempError` 出参带回 → 读清复位；客户端把 `tempError_` 存入 `lastError_` 供 `GetLastError()` 查询，连接失败时客户端自身置 `ERR_CONNECTION_FAIL`（`battery_stats_service.cpp:344-422`、`battery_stats_client.cpp`）。**死亡通知**：`BatteryStatsDeathRecipient::OnRemoteDied` → `ResetProxy` 清代理重连 |
| 关键流程 | `GetBatteryStatsIpc` 出参列表 + tempError；数值类接口出参 double/uint64 + tempError；`ShellDumpIpc` 出参字符串（供 CLI 远程 dump） |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | `BatteryStatsClient` 全部公共方法；新增接口流程：改 `.idl`（追加 ipccode）→ 构建重生成 → 同步 stub 实现与客户端封装 → 新增 fuzzer（AGENTS.md 陷阱"30+ fuzzer 维护成本"） |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `IBatteryStats`（生成物）、`BatteryStatsStub`（生成基类）、`BatteryStatsService`（实现）、`BatteryStatsClient` |
| 核心成员 | `lastError_`（服务端 atomic）、`tempError_`/`lastError_`（客户端）、`proxy_`/`deathRecipient_` |
| 核心方法 | `GetBatteryStatsIpc` 等 10 个 IDL 方法；客户端 `Connect`/`ResetProxy`/`GetLastError` |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | `[ipccode N]` 只能追加（ABI）；Parcelable 字段顺序不可变；懒启动首次调用有冷启动延迟 |
| 并发约束 | `lastError_` 为 atomic 但"写后读清"模式在并发查询下错误码可能被覆盖（单次语义，既有设计） |
| 安全约束 | 权限校验在服务端入口；`GetBatteryStats` 全量数据仅系统调用可得 |

## HIDump 耗电导出机制

### 解决的问题（Problem）
| 子维度 | 内容 |
|---|---|
| 业务痛点 | 耗电问题定位需要查看原始计时/计数与系数配置；数值类 API 只给聚合结果，不够诊断 |
| 技术目标 | 提供系统权限门控的全量快照导出，覆盖统计原始值与配置 |

### 原理（Principles）
| 子维度 | 内容 |
|---|---|
| 核心机制 | 两条通道：(1) `BatteryStatsService::Dump(fd, args)`——hidumper 标准 SA 入口：boot 完成 + 系统权限门控 → u16 参数转 string → `BatteryStatsDumper::Dump` 分派 → `SaveStringToFd` 输出；(2) `ShellDumpIpc(args, argc)`——IPC 通道（CLI 工具用），同样门控并额外记录调用 pid（`battery_stats_service.cpp:204-227, 331-342`）。`BatteryStatsDumper::Dump`（静态、delete 构造）：`-h` 帮助；`-batterystats` → `Core::DumpInfo`（依次 Bluetooth/Idle/Phone/Screen/Wifi/Uid 实体原始 dump + 调试信息）；`-poweraverage` → `Parser::DumpInfo`（系数配置全量）（`battery_stats_dumper.cpp:29-68`） |
| 关键流程 | 调试信息来自 `Detector` 聚合的 `Core::UpdateDebugInfo`（Thermal/Battery/Display/Wakelock/Workscheduler/Phone/Flashlight/DistributedScheduler 事件留痕） |

### 使用方式（Usage）
| 子维度 | 内容 |
|---|---|
| 核心 API | `hidumper -s 3304 -a "-batterystats"` / `-a "-poweraverage"` / `-a "-h"`；或经 CLI 的 ShellDumpIpc |

### 代码关联（Code）
| 子维度 | 内容 |
|---|---|
| 核心类 | `BatteryStatsDumper`（`services/native/include/battery_stats_dumper.h`，静态工具类） |
| 核心成员 | ARGS_HELP/ARGS_STATS/ARGS_POWER_AVERAGE 命令常量 |
| 核心方法 | `Dump`/`ShowUsage`；`BatteryStatsService::Dump`/`ShellDump`/`ShellDumpIpc` |

### 约束与限制（Constraints）
| 子维度 | 内容 |
|---|---|
| 系统限制 | boot 未完成返回 ERR_NO_INIT；非系统调用 Dump 返回 ERR_PERMISSION_DENIED / ShellDump 返回空串 |
| 并发约束 | ShellDump 持服务锁与查询互斥 |
| 安全约束 | 双权限门控；dump 内容含 uid 级统计，属系统诊断信息 |
