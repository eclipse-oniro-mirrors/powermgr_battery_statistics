# 业务实体知识

battery_statistics 模块核心业务实体的完整知识定义，涵盖静态概念、动态运行、规则约束与代码映射。路径均相对仓根（`base/powermgr/battery_statistics`）。术语释义见 [术语表](glossary.md)，实体依赖的机制见 [技术术语知识](batterystats-Technology.md)，整体边界见 [业务背景](business-context.md)。

## 耗电统计信息（BatteryStatsInfo / ParcelableBatteryStatsList）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | 耗电统计信息条目 / BatteryStatsInfo |
| 实体定义 | 单个耗电主体（应用 uid、硬件项、账户）在一个统计周期内的耗电结果单元；身份边界：它是"计算结果"的传输载体，不含计时/计数原始数据（那些在实体内） |
| 核心特征 | 字段 uid_、userId_、type_（`ConsumptionType` 17 值枚举，INVALID=-17 至 ALARM=-1）、totalPowerMah_；`Marshalling/ReadFromParcel/Unmarshalling` 跨进程序列化；`ConvertConsumptionType` 供日志/调试（`interfaces/inner_api/include/battery_stats_info.h:28-78`） |
| 类型/分类 | Parcelable 数据类；`BatteryStatsInfoList = std::list<shared_ptr<BatteryStatsInfo>>`，外层 `ParcelableBatteryStatsList` 是 IDL sequenceable 载体 |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | `ComputePower` 重算后各实体 `Calculate` 把结果写入静态 `statsInfoList_`（`BatteryStatsEntity::UpdateStatsInfoList`）→ 查询从列表取值 → IPC 打包 `ParcelableBatteryStatsList` → 客户端解包为 `BatteryStatsInfoList`；Load 恢复时由 `UpdateStatsEntity` 从 JSON 重建（`battery_stats_core.cpp:1092-1141`） |
| 状态流转 | 每轮 ComputePower 先 `ResetStatsEntity` 清空重建；无跨查询持久状态（持久化走 JSON 文件而非该对象） |

### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | APP 类型条目 uid 为正、硬件条目 uid 为 INVALID、USER 条目以 userId 标识；percent 类查询= power/totalPowerMah，总耗电为 0 时返回默认值 0 |
| 并发约束 | 结果集为静态链表，写读依赖服务级 `mutex_` 串行（ComputePower 与查询互斥） |

### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | `BatteryStatsService` 查询出口、NAPI `getBatteryStats` 响应 |
| 下游影响 | 应用耗电排行 UI、thermal_manager 等系统消费者 |
| 平级关联 | `ConsumptionType` 与 `GetEntity` 的 switch 映射一一对应；NAPI 侧导出 `StatsType` 枚举供应用传参 |

### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `BatteryStatsInfo`、`ParcelableBatteryStatsList`（`interfaces/inner_api/include/battery_stats_info.h`，实现 `frameworks/native/src/battery_stats_info.cpp`） |
| 核心方法 | `Marshalling`/`Unmarshalling`/`ReadFromParcel`、`SetUid/SetUserId/SetConsumptioType/SetPower`、`ConvertConsumptionType` |

## 耗电统计客户端（BatteryStatsClient）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | 耗电统计客户端 SDK / BatteryStatsClient |
| 实体定义 | 面向系统组件与 NAPI 的进程内查询入口；身份边界：纯转发者，唯一本地状态是代理指针与最近错误码，不缓存统计值 |
| 核心特征 | `DelayedRefSingleton` 单例；成员 `proxy_`、`deathRecipient_`、`lastError_`/`tempError_`、`mutex_`（`interfaces/inner_api/include/battery_stats_client.h:33-69`） |
| 类型/分类 | inner_api 公共 C++ SDK（`bundle.json` inner_kits，含 `libbatterystats_client.map` 版本脚本） |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | 各查询 → `Connect()`（首次 `GetSystemAbility(3304)`——懒启动触发点，取代理挂死亡通知）→ `proxy->*Ipc` 出参取值 → `tempError_` 暂存业务错误；服务死亡 → `OnRemoteDied` → `ResetProxy`，下次调用重连（`frameworks/native/src/battery_stats_client.cpp:35-120`） |
| 状态流转 | 代理 null ↔ connected；`lastError_` 在连接失败时置 `ERR_CONNECTION_FAIL`，查询成功后可由 `GetLastError()` 读取服务端回传的 `tempError_` |

### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | 所有数值查询失败返回 `StatsUtils::DEFAULT_VALUE(0)`；`GetBatteryStats` 失败返回空列表；错误语义经 `StatsError`（连接类错误码在客户端产生，业务类来自服务端） |
| 并发约束 | 多线程共享单例；`Connect`/`ResetProxy` 持 `mutex_` |

### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | NAPI `batteryStatistics` 模块、taihe 生成层、thermal_manager 等系统组件 |
| 下游影响 | `IBatteryStats` proxy（生成物）；IDL 变更需同步其调用面 |
| 平级关联 | `BatteryStatsDeathRecipient`（内嵌类，`STATS_SERVICE_DEATH_UT` 宏控制可见性供测试） |

### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `BatteryStatsClient`、内嵌 `BatteryStatsDeathRecipient` |
| 核心方法 | `Connect`/`ResetProxy`/`GetLastError`；查询 `GetBatteryStats`/`GetAppStatsMah`/`GetAppStatsPercent`/`GetPartStatsMah`/`GetPartStatsPercent`/`GetTotalTimeSecond`/`GetTotalDataBytes`；控制 `SetOnBattery`/`Reset`/`Dump` |

## 耗电统计服务（BatteryStatsService）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | 耗电统计系统服务 / BatteryStatsService（SA 3304） |
| 实体定义 | powermgr 进程内的统计服务门面：权限门控、依赖就绪管理、统计核心/解析器/探测器编排、错误码暂存与 IPC 桥接；身份边界：不做具体统计计算（委托 Core）与事件解析（委托 Listener） |
| 核心特征 | 继承 `SystemAbility` + `BatteryStatsStub`；成员 `core_`/`parser_`/`detector_`/`subscriberPtr_`/`listenerPtr_`、`ready_`、`isBootCompleted_`（atomic）、`lastError_`（atomic_int32_t）、`mutex_`；静态单例 `instance_` + `singletonMutex_`（`services/native/include/battery_stats_service.h:33-94`） |
| 类型/分类 | 懒启动系统服务（`run-on-create: false`） |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | 首次 IPC 拉起 → `OnStart`：`Init`（parser/core/detector 创建，core Init 含 OnBattery 初值与 Load 恢复）→ 监听 DFX_SYS_EVENT_SERVICE_ABILITY_ID 与 COMMON_EVENT_SERVICE_ID → `Publish` → 注册 boot 回调 → ready；依赖 SA 上线 → `OnAddSystemAbility` 分别 `AddHiSysEventListener`（ListenerRule 两规则）与 `SubscribeCommonEvent`（两事件）；`OnStop`：移除监听、退订（`battery_stats_service.cpp:68-147`） |
| 状态流转 | 未启动 →（懒启动）ready → boot 完成（`isBootCompleted_`）门控 Dump；错误码：`lastError_` 写入（权限拒绝）→ `*Ipc` 读取并清零 |

### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | 全部查询先 `ComputePower()` 再取数；非系统调用返回默认值 + `ERR_SYSTEM_API_DENIED`；`Dump/ShellDump` 双门槛（boot 完成 + 系统权限）；`Reset` 仅系统调用 |
| 并发约束 | 服务级 `mutex_` 串行化 GetBatteryStats/各 Mah 查询/ShellDump；`OnEvent`/`OnReceiveEvent` 事件回调与查询在各自线程，靠 Core/实体锁与 ComputePower 先清后建时序保证一致 |

### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | samgr（SA 注册与依赖监听）、HiSysEventManager、CommonEventManager |
| 下游影响 | `BatteryStatsCore`/`BatteryStatsParser`/`BatteryStatsDetector`/`BatteryStatsDumper`；thermal_manager（utils 可见性白名单） |
| 平级关联 | 同进程 SA 3301/3302/3303/3308；`BatteryStatsClient`（IPC 对端）；事件源子系统（HiSysEvent 生产者） |

### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `BatteryStatsService`（`services/native/include/battery_stats_service.h`，实现 `services/native/src/battery_stats_service.cpp`） |
| 核心方法 | 生命周期 `OnStart/OnStop/OnAddSystemAbility`；`Init`/`SubscribeCommonEvent`/`AddHiSysEventListener`/`RegisterBootCompletedCallback`；查询 `GetBatteryStats`/`GetAppStatsMah(Percent)`/`GetPartStatsMah(Percent)`/`GetTotalTimeSecond`/`GetTotalDataBytes`；IPC 桥 `GetBatteryStatsIpc` 等 10 个 `*Ipc`；`Dump`/`ShellDump`/`Reset`/`SetOnBattery` |

## 统计核心（BatteryStatsCore）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | 统计核心 / BatteryStatsCore |
| 实体定义 | 15 个耗电实体的持有者与分发中枢：事件 → `UpdateStats` 路由到实体 timer/counter；查询 → `ComputePower` 重算并汇总结果集；生命周期 → Save/Load JSON；身份边界：不理解事件格式（由 Listener/Detector 完成），不做权限（由 Service 完成） |
| 核心特征 | 15 个实体 shared_ptr 成员；`isCameraOn_`/`lastCameraUid_`（相机去重）、`isScreenOn_`/`lastBrightnessLevel_`（屏幕去重）；`mutex_`、`debugInfo_`（`services/native/include/battery_stats_core.h:34-112`） |
| 类型/分类 | 服务端组件（`std::shared_ptr` 持有于 Service） |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | 初始化：`Init` → `CreateAppEntity`（11 个软件实体）+ `CreatePartEntity`（5 个硬件实体）→ 查询电池插拔定 OnBattery → `LoadBatteryStatsData`（`battery_stats_core.cpp:127-144`）。运行：Detector → `UpdateStats`（按 statsType 走屏幕/相机/电话/连接性/通用五类专路或计数直通）→ 实体 timer/counter。查询：Service → `ComputePower` → `GetBatteryStats`/各 Get*。关机：Subscriber → `SaveBatteryStatsData` |
| 状态流转 | 计时器随事件启停；`isCameraOn_`/`isScreenOn_`/`lastBrightnessLevel_` 作同源事件去重；结果集每轮重算重建；`Reset` 清全部实体与结果集 |

### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | uid > INVALID 时先 `uidEntity_->UpdateUidMap(uid)` 注册应用；相机开启去重（已开忽略），关闭时联动关闪光灯计时；亮度变化 stop 旧档 start 新档，屏幕灭时忽略亮度事件；GetEntity 未知类型返回 nullptr |
| 并发约束 | `mutex_` 保护 ComputePower/GetBatteryStats/UpdateDebugInfo；timer/counter 写在事件线程、读在查询线程，靠实体内锁与 `GetRunningTimeMs` 的读时结算设计保证一致 |

### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | `BatteryStatsService`（唯一所有者）、`BatteryStatsDetector`（分发上游） |
| 下游影响 | 15 个 `BatteryStatsEntity`；`StatsHelper` 时钟；`StatsJsonUtils`/cJSON（持久化） |
| 平级关联 | `BatteryStatsParser`（Calculate 取系数）、`OhosAccountKits`（用户聚合）、`BatterySrvClient`（OnBattery 初值） |

### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `BatteryStatsCore`（`services/native/include/battery_stats_core.h`，实现 `services/native/src/battery_stats_core.cpp`，1207 行） |
| 核心方法 | `Init`/`ComputePower`/`UpdateStats`（状态与时长两个重载）/`UpdateTimer`/`UpdateCounter`/`UpdateScreenStats`/`UpdateCameraStats`/`UpdatePhoneStats`/`UpdateConnectivityStats`/`UpdateCommonStats`/`GetBatteryStats`/`GetAppStatsMah(Percent)`/`GetPartStatsMah(Percent)`/`GetTotalTimeMs`（两重载）/`GetTotalDataCount`/`GetTotalConsumptionCount`/`GetEntity`/`SaveBatteryStatsData`/`LoadBatteryStatsData`/`UpdateStatsEntity`/`Reset`/`DumpInfo`/`UpdateDebugInfo` |

## 耗电实体族（BatteryStatsEntity / 15 派生实体）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | 耗电实体 / BatteryStatsEntity 及其 15 个派生 |
| 实体定义 | 单一耗电项的统计自治单元：内部按 (uid, statsType[, level]) 或 (statsType[, level]) 维度建 ActiveTimer 表、按 statsType 建 Counter 表，`Calculate` 把时长×系数转为 mAh；身份边界：不感知事件与时间来源，只暴露 timer/counter 读写与计算接口 |
| 核心特征 | 基类虚接口 `Calculate/GetEntityPowerMah/Reset/GetActiveTimeMs/GetOrCreateTimer/GetOrCreateCounter/GetConsumptionCount/GetCpuTimeMs/UpdateCpuTime/DumpInfo`，静态 `totalPowerMah_`/`statsInfoList_`（`services/native/include/entities/battery_stats_entity.h:29-65`）。派生：Uid（APP 聚合）、Cpu、Bluetooth、Idle、Phone、Screen、Wifi、User、Camera、Flashlight、Audio、Sensor、Gnss、Wakelock、Alarm |
| 类型/分类 | 软件实体（按 uid 计）11 个 + 硬件实体（全局计）5 个（Idle/Phone/Screen/Wifi/Bluetooth）+ User 聚合 |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | Core `UpdateTimer/UpdateCounter` 创建并启停 timer/累加 counter → `ComputePower` 调 `Calculate`（UidEntity 级联调用各软件实体的按 uid 计算并汇总，`uid_entity.cpp:64-110`；CpuEntity 从 `CpuTimeReader::GetUidCpuTimeMs` 取时长后按 active/cluster/speed 三路计算，`cpu_entity.cpp:61-100`）→ 结果写 `statsInfoList_` 与实体查询 map |
| 状态流转 | timer 启停态；CpuEntity 维护 `cpuTimeMap_`/`cpuTotalPowerMap_` 按 uid 更新或新建；`Reset` 清空全部 |

### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | 新 uid 首次出现即入 `UidEntity::uidPowerMap_`（默认 0）；硬件实体 timer 无 uid 维度；Phone 按 level（信号档）建 timer；Screen 亮度档范围 (INVALID, SCREEN_BRIGHTNESS_BIN]；基类未实现接口默认记 "No need" 日志返回默认值（派生按需覆写） |
| 并发约束 | 各实体内部 mutex（如 `uidEntityMutex_`）；timer 对象本身无锁，依赖创建线程（事件线程）与查询线程的 `GetRunningTimeMs` 自结算语义 |

### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | `BatteryStatsCore`（创建/分发/计算编排） |
| 下游影响 | `StatsHelper::ActiveTimer/Counter`（存储）；`BatteryStatsParser`（系数）；`statsInfoList_`（结果出口） |
| 平级关联 | UidEntity 与各软件实体（级联 Calculate）；Camera 与 Flashlight 联动（相机关闭带动闪光灯计时停止）；CpuEntity 独占 `CpuTimeReader` |

### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `BatteryStatsEntity`（基类）与 `UidEntity`/`CpuEntity`/`BluetoothEntity`/`IdleEntity`/`PhoneEntity`/`ScreenEntity`/`WifiEntity`/`UserEntity`/`CameraEntity`/`FlashlightEntity`/`AudioEntity`/`SensorEntity`/`GnssEntity`/`WakelockEntity`/`AlarmEntity`（`services/native/include/entities/` + `services/native/src/entities/`） |
| 核心方法 | 基类 `GetOrCreateTimer`（3 重载：type+level / uid+type / deviceId+uid+type）/`GetOrCreateCounter`/`UpdateStatsInfoList`/`ResetStatsEntity`/`GetTotalPowerMah`；UidEntity `CalculateForCommon`/`CalculateForConnectivity`/`UpdateUidMap`/`GetUids`；CpuEntity `Calculate`/`GetCpuTimeMs`/`CalculateCpuActivePower`/`CalculateCpuClusterPower`/`CalculateCpuSpeedPower` |

## 电池供电时钟与计时计数器（StatsHelper）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | 统计辅助器 / StatsHelper（含 ActiveTimer、Counter） |
| 实体定义 | 统计的时间基础设施：维护 OnBattery 全局态与"电池供电单调时钟"，提供 ActiveTimer（区间计时）与 Counter（供电期计数）两个原语；身份边界：不知道任何耗电项语义，只提供时间与计数运算 |
| 核心特征 | 静态成员 `onBattery_`/`screenOff_`、`onBatteryBootTimeMs_`/`onBatteryUpTimeMs_`、`latestUnplug*`（`utils/native/include/stats_helper.h:26-133`）；ActiveTimer：`isRunning_`/`startTimeMs_`/`totalTimeMs_`；Counter：`totalCount_` |
| 类型/分类 | 跨仓共享工具（`utils/native/`，`bundle.json` inner_kits 对 thermal_manager 可见） |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | OnBattery 维护三来源：`Core::Init` 初值（BatterySrvClient 查询）、`BATTERY_CHANGED` 事件、IPC `SetOnBattery`（`SetOnBattery` 内部换算并推进时钟基线）。timer：实体 Start/StopRunning；查询时 `GetRunningTimeMs` 若仍在运行则结算到当前 |
| 状态流转 | OnBattery true ↔ false（插拔）；timer idle → running（记起点）→ stopped（累加差值）；`AddRunningTimeMs` 外部注入时长（仅正值）；Counter 仅 OnBattery 时累加正数 |

### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | 重复 Start/Stop 幂等（返回 false 不重复计）；`GetRunningTimeMs` 是"读时结算"（推进起点不中断计时）；Counter 的 `IsOnBattery` 门控保证充电期不计 |
| 并发约束 | 静态全局态无锁（事件与 IPC 频率低、写路径单一为既成事实；新增并发写路径需评估）；timer 对象自身靠实体锁串行 |

### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | `BatteryStatsCore::UpdateTimer/UpdateCounter`（唯一生产写入方） |
| 下游影响 | 所有实体的时长/计数存储；`GetTotalTimeSecond` 查询换算（ms→s 四舍五入） |
| 平级关联 | `StatsUtils`（INVALID_VALUE/DEFAULT_VALUE 常量与类型枚举）；OnBattery 状态与 Subscriber/Service 联动 |

### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `StatsHelper`、内嵌 `ActiveTimer`、`Counter`（`utils/native/include/stats_helper.h`，实现 `utils/native/src/stats_helper.cpp`） |
| 核心方法 | `SetOnBattery`/`IsOnBattery`/`GetOnBatteryBootTimeMs`/`GetOnBatteryUpTimeMs`/`SetScreenOff`/`IsOnBatteryScreenOff`；ActiveTimer `StartRunning`/`StopRunning`/`GetRunningTimeMs`/`AddRunningTimeMs`/`Reset`；Counter `AddCount`/`GetCount`/`Reset` |

## 平均耗电配置解析器（BatteryStatsParser / power_average.json）

### 实体概念
| 子维度 | 内容 |
|---|---|
| 实体名称 | 耗电系数解析器 / BatteryStatsParser |
| 实体定义 | 平均电流系数表的加载与查询器：把 `power_average.json` 的标量/数组键解析为 mA 系数与 CPU 簇/频点结构；身份边界：只读配置，不参与计算与统计 |
| 核心特征 | `averageMap_`（标量）、`averageVecMap_`（数组按 level 取）、`clusterNum_`、`speedNum_`（`services/native/include/battery_stats_parser.h:31-51`） |
| 类型/分类 | 服务端组件（shared_ptr 持有于 Service，`GetBatteryStatsParser` 出借给 Dumper） |

### 上下文与场景
| 子维度 | 内容 |
|---|---|
| 交互流程 | `Init`：配置策略 `GetOneCfgFile("etc/power_config/power_average.json")` 定位（vendor 优先）→ 无 config_policy part 时直接尝试 `/vendor/etc/power_config/` 回退 `/system/etc/power_config/` → cJSON 解析入库（`battery_stats_parser.cpp:36-58`）。运行：实体 Calculate 调 `GetAveragePowerMa(type)` 或 `GetAveragePowerMa(type, level)` |
| 状态流转 | 一次加载常驻；数组键按 level 索引取值；CPU 结构经 `GetClusterNum/GetSpeedNum` 供 CpuEntity 遍历 |

### 规格与约束
| 子维度 | 内容 |
|---|---|
| 业务规则 | 键缺失返回默认值 0 并告警；默认系数以 `services/profile/power_average.json` 为准（camera_on 810、wifi_on 83、screen_on 90、radio_on 五档 [50,70,90,120,150] 等）；vendor 可覆盖 |
| 并发约束 | 启动期单线程加载，运行期只读 |

### 知识关联
| 子维度 | 内容 |
|---|---|
| 上层依赖 | `BatteryStatsService::Init`（加载时机） |
| 下游影响 | 所有实体 `Calculate` 的系数来源；HIDump `-poweraverage` 输出 |
| 平级关联 | CpuEntity（cluster/speed 结构消费方）；`StatsJsonUtils`（解析工具） |

### 代码与符号
| 子维度 | 内容 |
|---|---|
| 核心类 | `BatteryStatsParser`（`services/native/include/battery_stats_parser.h`，实现 `services/native/src/battery_stats_parser.cpp`） |
| 核心方法 | `Init`/`GetAveragePowerMa`（2 重载）/`GetClusterNum`/`GetSpeedNum`/`DumpInfo`/`LoadAveragePowerFromFile`/`ParsingArray` |
