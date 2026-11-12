# 术语表

## 耗电统计服务（BatteryStatsService / SA 3304）
本部件系统服务端，进程 `powermgr`，库 `libbatterystats_service.z.so`，`run-on-create: false` 懒启动（首次 IPC 拉起）。持有统计核心并对外提供查询。区别于客户端 `BatteryStatsClient`：服务端持有状态，客户端纯转发。代码：`services/native/include/battery_stats_service.h`。

## 耗电统计信息（BatteryStatsInfo / ConsumptionType）
跨进程耗电数据单元（uid/userId/consumptionType/power mAh），实现 Parcelable；17 值 `ConsumptionType` 枚举（INVALID=-17 至 ALARM=-1）区分 APP/BLUETOOTH/IDLE/PHONE/RADIO/SCREEN/USER/WIFI/CAMERA/FLASHLIGHT/AUDIO/SENSOR/GNSS/CPU/WAKELOCK/ALARM 等耗电项；`ParcelableBatteryStatsList` 承载列表经 IPC 传输。代码：`interfaces/inner_api/include/battery_stats_info.h`。

## IPC 接口（IBatteryStats / IDL）
10 方法接口契约，源文件 `services/IBatteryStats.idl`，proxy/stub 由 `idl_gen_interface` 构建生成（注意 AGENTS.md 中"手写 stub/proxy"的描述已过时）。`[ipccode N]` 码值是 ABI，只能追加。

## 统计核心（BatteryStatsCore）
服务端统计中枢：创建并持有 15 个耗电实体，`UpdateStats` 把事件分发给实体 timer/counter，`ComputePower` 全量重算，`Save/LoadBatteryStatsData` 跨开关机持久化。代码：`services/native/include/battery_stats_core.h`。

## 耗电实体（BatteryStatsEntity / entities/）
按耗电项组织的统计模块基类（虚接口 `Calculate/GetEntityPowerMah/Reset/GetOrCreateTimer/GetOrCreateCounter`），派生 15 个实体：Uid（应用聚合）、Cpu、Bluetooth、Idle、Phone、Screen、Wifi、User、Camera、Flashlight、Audio、Sensor、Gnss、Wakelock、Alarm。静态共享 `statsInfoList_`/`totalPowerMah_` 作结果集。代码：`services/native/include/entities/battery_stats_entity.h`。

## 应用耗电（UidEntity / CONSUMPTION_TYPE_APP）
按 Uid 聚合的软件耗电：`Calculate(uid)` 依次调用相机/闪光灯/音频/传感器/GNSS/CPU/唤醒锁/Alarm/蓝牙等实体的按 uid 计算（`CalculateForCommon/CalculateForConnectivity`），汇总进 uidPowerMap_。用户维度再由 `CONSUMPTION_TYPE_USER` 经 `OhosAccountKits::GetDeviceAccountIdByUID` 聚合。代码：`services/native/src/entities/uid_entity.cpp`。

## 统计类型与状态（StatsType / StatsState）
`StatsUtils::StatsType`（`utils/native/include/stats_utils.h:61-92`）枚举 30+ 个统计项（蓝牙 BR/BLE on/scan、WiFi on/scan、电话 active/data、相机/闪光灯/GNSS/音频/传感器、屏幕 on/brightness、唤醒锁、CPU cluster/speed/active/suspend、ALARM 等）；`StatsState` 区分 ACTIVATED/DEACTIVATED 等状态。`StatsData` 结构是事件解析的标准载荷。

## 电池供电时钟（OnBattery / StatsHelper 时间轴）
`StatsHelper` 维护的"设备正在使用电池"全局态与单调时钟：`GetOnBatteryBootTimeMs()` 只在电池供电区间前进，ActiveTimer 计时基于它，充电期间的使用不计入耗电归因。状态来源：服务启动初值（`BatterySrvClient::GetPluggedType`）、`BATTERY_CHANGED` 事件、IPC `SetOnBattery`。代码：`utils/native/include/stats_helper.h`。

## 活动计时器（ActiveTimer）
`StatsHelper::ActiveTimer`：Start/StopRunning 记录起止（OnBattery 时钟），`GetRunningTimeMs` 返回含进行中时段的总时长，幂等防重（已启动再启动返回 false）。按 (uid, statsType) 或 (statsType, level) 维度在实体内建表。区别于 Counter：timer 计时长，counter 计次数/数据量。

## 计数器（Counter）
`StatsHelper::Counter`：`AddCount` 仅在 `IsOnBattery()` 时累加（WiFi 扫描次数、Alarm 次数等）。这保证充电期间的扫描不计入耗电统计。

## 事件监听器（BatteryStatsListener / PowerStats 域）
继承 `HiviewDFX::HiSysEventListener`，按 `ListenerRule("PowerStats")` 全域 + `("DISTSCHEDULE", START_REMOTE_ABILITY)` 订阅 HiSysEvent；`OnEvent` 把事件 JSON 解析为 `StatsData` 并按事件名分派 `Process*Event`。是统计数据的唯一在线输入口。代码：`services/native/src/battery_stats_listener.cpp`。

## 事件订阅器（BatteryStatsSubscriber）
继承 CommonEventSubscriber，订阅 `COMMON_EVENT_SHUTDOWN`（触发持久化保存）与 `COMMON_EVENT_BATTERY_CHANGED`（容量满 100 触发 `Reset`；pluggedType 变化维护 OnBattery）。代码：`services/native/src/battery_stats_subscriber.cpp`。

## 事件探测器（BatteryStatsDetector）
`StatsData` 的二级分派器：duration 类（WIFI_SCAN/ALARM）→ `UpdateStats(type, time, data)`；state 类（屏幕/相机/蓝牙等）→ `UpdateStats(type, state, level, uid)`；同时聚合调试信息进 `Core::UpdateDebugInfo`。注意：AGENTS.md"周期采集器"的描述与当前代码不符——现实现为事件驱动，无周期采集逻辑。代码：`services/native/src/battery_stats_detector.cpp`。

## CPU 时间读取器（CpuTimeReader）
从内核接口 `/proc/uid_concurrent_active_time`、`/proc/uid_concurrent_policy_time`、`/proc/uid_time_in_state`、`/proc/uid_cputime/show_uid_stat` 增量读取每 Uid 的 active/cluster/freq/CPU 时间，维护增量 map（`last*Map_` 与累计 map 的差值算法）。`Init` 时首次刷新。代码：`services/native/src/cpu_time_reader.cpp:29-32`。

## 平均耗电配置（power_average.json / BatteryStatsParser）
各耗电项的平均电流系数表（单位 mA）：标量键如 `camera_on: 810`、`wifi_on: 83`，数组键如 `radio_on`（5 信号档）、`cpu_speed_cluster0`（按频点）。路径 `etc/power_config/power_average.json`，配置策略定位 vendor 优先，回退 `/vendor` → `/system`；默认值在 `services/profile/power_average.json`。代码：`services/native/src/battery_stats_parser.cpp:36-58`。

## 耗电计算（ComputePower）
查询触发的前置重算：`ResetStatsEntity` 清结果集 → 7 个实体 Calculate（Uid/Bluetooth/Idle/Phone/Screen/Wifi/User，内部再级联软件实体）→ XCollie 60s 看护。百分比 = 单项 power / `GetTotalPowerMah()`。mAH = 时长(h) × 系数(mA) 逐档累加。代码：`battery_stats_core.cpp:146-164`。

## 统计持久化（battery_stats.json）
`/data/service/el0/stats/battery_stats.json` 三节结构：Power（uid 或 ConsumptionType → mAh）、Hardware（蓝牙/WiFi/屏幕/亮度数组/radio 数组/cpu_idle 等）、Software（每 uid 的 camera/flashlight/gnss/audio/cpu_awake/sensor/alarm/cpu_time/bluetooth scan）。关机事件保存、启动 `LoadBatteryStatsData` 恢复。代码：`battery_stats_core.cpp:57, 784-1183`。

## 双错误码协议（tempError / lastError）
IPC 返回值恒 `ERR_OK` 表示送达；业务错误（`StatsError`，`interfaces/inner_api/include/battery_stats_errors.h`）经 `tempError` 出参返回，服务端 `lastError_`（atomic）写后读清，客户端另有 `lastError_`/`GetLastError()` 供异步查询。连接类错误码 `E_STATS_GET_SYSTEM_ABILITY_MANAGER_FAILED`/`E_STATS_GET_SERVICE_FAILED` 在客户端侧产生。

## 懒启动（run-on-create: false）
SA 3304 不随进程启动加载，首次 `GetSystemAbility` 调用时才拉起（`sa_profile/3304.json`）。含义：早于服务启动的事件不可在线获得，靠启动初值查询与持久化恢复补偿；禁止把该字段改为 true。

## 耗电导出（BatteryStatsDumper / ShellDump）
`hidumper -s 3304` 通道：`-batterystats` 输出实体统计、`-poweraverage` 输出系数配置、`-h` 帮助；另经 IPC `ShellDumpIpc` 供 CLI（如 battery-shell）远程调用。均要求系统权限且 boot 完成。代码：`services/native/src/battery_stats_dumper.cpp:29-68`。

## 充满复位（BATTERY_LEVEL_FULL）
`BATTERY_CHANGED` 事件 capacity==100 时 `Core::Reset()` 全量清零计时/计数/结果集——开启一个新放电周期的统计（`battery_stats_subscriber.cpp:53-55`）。这是业务上唯一的自动复位点。

## 多账户统计（UserEntity / GetDeviceAccountIdByUID）
多用户场景按账户聚合应用耗电：uid → 设备账户 ID 经 `AccountSA::OhosAccountKits` 转换，`CONSUMPTION_TYPE_USER` 输出每 userId 的汇总 mAh（`battery_stats_core.cpp:1119-1125`）。

## 共享指针（sptr / wptr）
OpenHarmony `refbase.h` 引用计数指针（非 `std::shared_ptr`），代理、单例、实体 shared_ptr 混用体系：实体用 `std::shared_ptr`，IPC/HDI 对象用 `sptr`。

## part 检测（HAS_BATTERYSTATS_*_PART）
`batterystats.gni` 检测 `global_parts_info` 中 bluetooth/wifi/display_manager/call_manager/config_policy 五个部件，决定事件解析代码是否编译；本仓无 `declare_args()` 特性开关（区别于 battery_manager 的 7 开关）。
