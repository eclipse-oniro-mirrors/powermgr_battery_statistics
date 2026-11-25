# 编码铁律

以下规则提炼自本仓现有实现、AGENTS.md 约束边界与 OpenHarmony 系统部件通用要求，适用于本仓全部生产代码（`services/`、`frameworks/`、`utils/`、`interfaces/`）。架构背景见 [架构设计及约束](architecture.md)，实体语义见 [业务实体知识](batterystats-Entity.md)。

## IPC 与 ABI 稳定性

- **禁止**复用或调整 `services/IBatteryStats.idl` 中已有 `[ipccode N]` 码值，**采用**新接口追加新码值（当前 10 个方法）。注意：本仓当前使用 IDL 生成 proxy/stub（`services/BUILD.gn` 的 `idl_gen_interface`），AGENTS.md 中"手写 stub/proxy 需两侧同步"的描述已过时——修改接口应改 `.idl` 后由构建重生成，**不要手改生成产物**。
- **禁止**变更 `BatteryStatsInfo` 的 `Marshalling/ReadFromParcel` 字段顺序或 `ConsumptionType` 枚举值，**采用**末尾追加（`interfaces/inner_api/include/battery_stats_info.h:30-69`；跨进程 Parcelable 布局是 ABI）。
- **禁止**绕过 `*.map` 版本脚本（`interfaces/inner_api/libbatterystats_client.map` 等）新增导出符号或降级可见性；公共 API 变更须同步 NAPI/taihe 两套绑定与 `StatsType` 枚举（`frameworks/napi/src/battery_stats_module.cpp` 的 `CreateEnumStatsType`）。
- **采用**双错误码协议：IPC 方法返回值恒 `ERR_OK`（送达语义），业务错误经 `tempError` 出参；服务端写 `lastError_` 后在 `*Ipc` 入口一次性读清（`battery_stats_service.cpp:344-359`）。新增接口必须遵守此模式，不得把业务错误放进 IPC 返回值。

## 并发模型

- **禁止**绕过 `BatteryStatsService::mutex_` 并发触发 `ComputePower`/`Reset`/`ShellDump`，**采用**服务级锁串行化查询路径（`battery_stats_service.cpp:191-342` 各入口均 `std::lock_guard lock(mutex_)`）。
- **禁止**在 `BatteryStatsListener::OnEvent` / `BatteryStatsDetector::HandleStatsChangedEvent` 回调中执行阻塞 IO 或长耗时计算，**采用**纯内存 timer/counter 更新；重算由查询触发的 `ComputePower` 承担（AGENTS.md 禁止事项；事件回调在 HiSysEvent 分发线程执行）。
- **禁止**无锁读写 `UidEntity::uidPowerMap_` 与实体内部 timer/counter 表，**采用**各实体自带 mutex（如 `uidEntityMutex_`，`services/native/src/entities/uid_entity.cpp:40-58`）。
- **采用**静态结果集的写读时序：`ComputePower` 内必须先 `ResetStatsEntity()` 再逐实体 `Calculate`，查询线程依赖服务锁保证不会读到半成品 `statsInfoList_`（`battery_stats_core.cpp:146-170`）。

## 错误处理

- **禁止**吞掉权限拒绝，**采用**`Permission::IsSystem()` 失败时置 `lastError_ = ERR_SYSTEM_API_DENIED` 并返回默认值（`StatsUtils::DEFAULT_VALUE` 即 0），让客户端经 `GetLastError()` 观测（`battery_stats_service.cpp:195-198`）。
- **禁止**在 cJSON 解析路径上崩溃，**采用**`StatsJsonUtils::IsValidJson*` 系列判空 + `continue`/早退（`battery_stats_core.cpp:1092-1133`、`utils/native/include/stats_cjson_utils.h`）；解析失败必须记 `STATS_HILOGE/W`。
- **禁止**为通过测试删除日志、HiSysEvent、XCollie 看护（AGENTS.md 禁止事项）；每个 IPC 入口保留 `StatsXCollie` 构造，`ComputePower` 保留 60s XCollie 定时器。
- **采用**哨兵值语义：`StatsUtils::INVALID_VALUE(-1)`（uid/level）、`DEFAULT_VALUE(0)`（power/计数）；客户端连接失败返回默认值 + `ERR_CONNECTION_FAIL`。

## 统计口径

- **禁止**在事件链路外直接调用实体 `GetOrCreateTimer/GetOrCreateCounter` 写数据，**采用**经 `BatteryStatsDetector::HandleStatsChangedEvent` → `BatteryStatsCore::UpdateStats` 的分发路径（AGENTS.md 架构不变量"事件订阅是数据源"）。
- **禁止**新增实体后不接入 Core：新实体必须 (1) 在 `battery_stats_core.h` 增加成员；(2) 在 `CreateAppEntity/CreatePartEntity` 创建；(3) 在 `GetEntity` switch 注册 `ConsumptionType`；(4) 需要参与耗电排行时在 `ComputePower` 与 `SaveForSoftware/Hardware` 补充（`battery_stats_core.cpp:59-125, 146-164, 172-210, 784-1090`）。
- **禁止**硬编码耗电系数，**采用**`BatteryStatsParser::GetAveragePowerMa(type[, level])`（`services/profile/power_average.json` 为默认值，vendor 可覆盖；数组型键按 level 取值）。
- **采用**OnBattery 时钟计时：时长一律用 `StatsHelper::GetOnBatteryBootTimeMs()`（`ActiveTimer` 内建），计数累加须过 `StatsHelper::IsOnBattery()` 门控（`stats_helper.h:93-104`）；禁止改用系统 uptime。
- **采用**信号/亮度分档键：radio 类按 `RADIO_SIGNAL_BIN` 档、屏幕亮度按 `SCREEN_BRIGHTNESS_BIN` 档建 timer（`battery_stats_core.cpp:514-543, 844-869`）；档位数常量在 `stats_utils.h`，改档位数需评估持久化数组兼容性。

## 配置与持久化

- **禁止**修改 `power_average.json` 键名（`bluetooth_br_on`、`screen_brightness`、`cpu_speed_cluster0` 等）而不评估旧版本兼容，**采用**只增不改；schema 变更需产品团队评审（AGENTS.md 需确认后再修改）。
- **禁止**变更 `/data/service/el0/stats/battery_stats.json` 的三节结构（Power/Hardware/Software）语义，**采用**`SaveFor*`/`UpdateStatsEntity` 成对修改；Power 节约定：正数键为 uid、负数键为 `ConsumptionType` 值（`battery_stats_core.cpp:1011-1141`）。
- part 检测宏（`HAS_BATTERYSTATS_*_PART`）与 `bundle.json` 的 `deps.components` 必须同步增删（`batterystats.gni:16-47`）。

## 安全

- **禁止**依赖客户端做权限校验，**采用**服务端入口统一 `Permission::IsSystem()`；`Dump`/`ShellDump` 还要求 `isBootCompleted_`（`battery_stats_service.cpp:204-227, 331-342`）。
- **禁止**在 dump/日志输出中泄露隐私字段：`STATS_HILOGD` 打印 uid/time 时事件 deviceId 用 `%{private}s`（`battery_stats_detector.cpp:29-40` 的既有约定）。
