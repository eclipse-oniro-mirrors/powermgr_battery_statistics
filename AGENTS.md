# AGENTS.md - OpenHarmony 耗电统计服务（Battery Statistics）

## 1. 代码地图

本仓库实现 OpenHarmony 耗电统计服务（Battery Statistics），SA ID 3304，进程名 `powermgr`，库 `libbatterystats_service.z.so`，**`run-on-create: false`（懒启动，由首次调用触发）**。核心职责是软件耗电统计（按 Uid 统计每个应用的 CPU/移动无线/WiFi/GNSS/传感器/相机/闪光灯等耗电）和硬件耗电统计（用户/通话/屏幕/WiFi/蓝牙等硬件耗电）。最重要的架构边界是**`BatteryStatsService` 订阅多个子系统事件（电池/电源/显示/蓝牙/WiFi/通话）聚合成耗电数据，统计是累加型快照，不持有业务状态机**。

### 非本项目维护的目录

以下目录属于其他团队或生成产物，不属于本项目维护范围，修改时请跳过：

- `frameworks/ets/taihe/`：taihe FFI 生成产物（由 `taihe_ffi_gen` 工具生成，不要手改）
- `frameworks/napi/`：ArkTS NAPI 绑定（仅在新增 ArkTS API 时进入）

### 嵌套指引

本仓目前无嵌套 AGENTS.md / CLAUDE.md / rules / skills 文件。以下子模块可按需新建嵌套 AGENTS.md 以提供更具体的指引：

- `utils/native/`：跨仓共享工具层，可新建 `utils/AGENTS.md` 聚焦跨仓工具变更影响评估
- `services/native/include/entities/` + `services/native/src/entities/`：按耗电实体组织的统计模块（CPU/WiFi/蓝牙等），可新建 `services/native/include/entities/AGENTS.md` 聚焦实体注册顺序、Uid/Pid 转换、统计口径一致性
- `services/profile/`：平均耗电配置文件目录，schema 变更影响所有产品耗电计算结果，可单独新建 AGENTS.md 聚焦配置 schema 与产品差异
- `frameworks/napi/`：ArkTS NAPI 绑定（含 `include/` + `src/`），可新建嵌套 AGENTS.md 聚焦 NAPI 绑定规则与 `*.map` 版本脚本维护
- `frameworks/native/`：客户端 SDK 实现层，可新建嵌套 AGENTS.md 聚焦 `BatteryStatsClient` IPC 转发约束与懒启动时序处理
- `test/`：测试组织目录，可新建 `test/AGENTS.md` 聚焦 fuzztest/systemtest/unittest 三类测试的运行约定，特别是 30+ 个 fuzzer 的维护成本

### 关键区域

- `interfaces/inner_api/include/`：公共 C++ API 头文件（注意：本仓 inner_api 在 `interfaces/inner_api/include/` 而非 `interfaces/inner_api/native/include/`）。
- `frameworks/native/`：客户端 SDK 实现（`BatteryStatsClient` 入口）。
- `frameworks/napi/`：ArkTS NAPI 绑定（`include/` + `src/`）。
- `frameworks/ets/taihe/`：taihe FFI **生成产物**。
- `services/native/include/`：服务端头文件，关键类 `BatteryStatsService` / `BatteryStatsCore` / `BatteryStatsDetector` / `BatteryStatsListener` / `BatteryStatsParser` / `BatteryStatsSubscriber` / `BatteryStatsDumper` / `CpuTimeReader` + `entities/`（按耗电实体组织的统计模块）。
- `services/native/src/`：服务端实现。
- `services/profile/`：平均耗电配置文件（`battery_stats_config.xml` 等）。
- `services/zidl/`：**本仓无独立 zidl 目录，IPC stub/proxy 直接在 `services/native/` 中实现**。
- `sa_profile/3304.json`：SA 3304 注册配置，`run-on-create: false`，`min_hdi_proxy_version: []`（无 HDI 依赖）。
- `batterystats.gni`：**无 `declare_args()` 特性开关**，只有 part 检测和路径常量定义。
- `batterystats.yaml`：HiSysEvent 配置。
- `utils/native/`：跨仓共享工具。
- `test/`：`fuzztest/`（30+ 个 fuzzer）+ `systemtest/` + `unittest/`。

### Where to look

| 任务类型 | 先看哪里 |
|---|---|
| 公共 API 变更 | `interfaces/inner_api/include/` 头文件 -> `frameworks/native/` 实现 -> `frameworks/napi/` ArkTS 绑定 -> `*.map` 版本脚本 |
| IPC 接口变更 | `services/native/include/battery_stats_service.h` + `services/native/src/battery_stats_service.cpp`（**直接手写 stub/proxy，无 `.idl`**）+ `interfaces/inner_api/include/` 中 IPC 码枚举 |
| 耗电统计核心逻辑 | `services/native/include/battery_stats_core.h` + `services/native/src/battery_stats_core.cpp` |
| 耗电实体（CPU/WiFi/蓝牙等） | `services/native/include/entities/` + `services/native/src/entities/` |
| 事件订阅 | `services/native/include/battery_stats_subscriber.h` + `services/native/include/battery_stats_listener.h` |
| 耗电数据采集 | `services/native/include/battery_stats_detector.h` |
| CPU 时间读取 | `services/native/include/cpu_time_reader.h` |
| 耗电配置文件 | `services/profile/`（平均耗电配置）+ `services/native/include/battery_stats_parser.h` |
| 耗电数据导出 | `services/native/include/battery_stats_dumper.h` + `BatteryStatsService::ShellDump` |
| 应用耗电查询 | `interfaces/inner_api/include/` 中 `GetAppStatsMah` / `GetAppStatsPercent` 等接口 + `services/native/src/` |
| 硬件耗电查询 | `interfaces/inner_api/include/` 中 `GetPartStatsMah` / `GetPartStatsPercent` 等接口 |
| 总耗电查询 | `interfaces/inner_api/include/` 中 `GetTotalTimeSecond` / `GetTotalDataBytes` |
| 跨仓依赖调用 | `bundle.json` 中 `deps.components`（依赖 `battery_manager` / `power_manager` / `display_manager` / `bluetooth` / `wifi` / `call_manager` / `os_account`） |
| 特性开关 | `batterystats.gni` part 检测段（bluetooth/wifi/display_manager/call_manager/config_policy） |
| 新增/修改测试 | `test/unittest/` + `test/systemtest/` + `test/fuzztest/`（30+ 个） |
| 构建配置 | `bundle.json` + `batterystats.gni` + 子目录 `BUILD.gn` |
| DFX（日志/事件） | `batterystats.yaml`（HiSysEvent 配置）+ grep `HiSysEvent` 调用点 |

### 架构分层

```
应用层
  ├─ ArkTS 应用 -> frameworks/napi (NAPI 绑定)
  │                └─ frameworks/ets/taihe (taihe FFI 生成)
  └─ C++ 应用/系统组件 -> interfaces/inner_api (完整 C++ API)
          ↓
客户端 SDK
  BatteryStatsClient (frameworks/native)
    -> IBatteryStatsService proxy (services/native，手写)
        ↓ IPC (SystemAbility 3304, 懒启动)
服务端
  BatteryStatsService (services/native/include/battery_stats_service.h, 继承 SystemAbility)
    ├─ BatteryStatsSubscriber (订阅子系统事件)
    │    ├─ 订阅 battery_manager (电池状态)
    │    ├─ 订阅 power_manager (电源状态/运行锁)
    │    ├─ 订阅 display_manager (屏幕亮灭)
    │    ├─ 订阅 bluetooth (蓝牙活动)
    │    ├─ 订阅 wifi (WiFi 活动)
    │    ├─ 订阅 call_manager (通话活动)
    │    └─ 订阅 os_account (账户切换)
    ├─ BatteryStatsDetector (周期采集，如 CPU 时间)
    │    └─ CpuTimeReader (读取 /proc/stat 等)
    ├─ BatteryStatsCore (统计核心，累加型快照)
    │    └─ entities/ (按耗电实体组织：CpuEntity / WifiEntity / BluetoothEntity / ...)
    ├─ BatteryStatsParser (解析 services/profile/ 配置)
    ├─ BatteryStatsListener (事件监听)
    └─ BatteryStatsDumper (hidumper 入口)
```

## 2. 知识路由

在规划或编辑前，先对任务分类，读取对应的代码路径和文档。

### Task-based routing

| 任务类型 | 读取 |
|---|---|
| 公共 API 新增/修改 | `interfaces/inner_api/include/*.h` + `frameworks/native/` + `frameworks/napi/` + `*.map` 版本脚本 |
| IPC 接口变更 | `services/native/include/battery_stats_service.h` + `services/native/src/battery_stats_service.cpp` + `interfaces/inner_api/include/` 中 IPC 码（**本项目不使用 `.idl`，直接手写 stub/proxy**） |
| 耗电统计核心逻辑 | `services/native/include/battery_stats_core.h` + `services/native/src/battery_stats_core.cpp` |
| 耗电实体（CPU/WiFi/蓝牙等）新增/修改 | `services/native/include/entities/` + `services/native/src/entities/` + 在 `BatteryStatsCore` 中注册实体 |
| 事件订阅 | `services/native/include/battery_stats_subscriber.h` + `services/native/include/battery_stats_listener.h` + 调用下游子系统订阅接口 |
| 耗电数据采集 | `services/native/include/battery_stats_detector.h` + `services/native/include/cpu_time_reader.h` |
| 耗电配置文件变更 | `services/profile/`（平均耗电配置）+ `services/native/include/battery_stats_parser.h` |
| 耗电数据导出 | `services/native/include/battery_stats_dumper.h` + `BatteryStatsService::ShellDump` |
| 应用耗电查询 | `interfaces/inner_api/include/` 中 `GetAppStatsMah` / `GetAppStatsPercent` + `services/native/src/` |
| 硬件耗电查询 | `interfaces/inner_api/include/` 中 `GetPartStatsMah` / `GetPartStatsPercent` |
| 总耗电查询 | `interfaces/inner_api/include/` 中 `GetTotalTimeSecond` / `GetTotalDataBytes` |
| 跨仓依赖调用 | `bundle.json` 中 `deps.components` + grep 调用 `battery_manager` / `power_manager` / `display_manager` / `bluetooth` / `wifi` / `call_manager` / `os_account` 的位置 |
| 权限校验 | grep `AccessTokenKit` / `VerifyAccessToken` 调用点 |
| 特性开关 | `batterystats.gni` part 检测段（bluetooth/wifi/display_manager/call_manager/config_policy） |
| 新增/修改测试 | `test/unittest/` + `test/systemtest/` + `test/fuzztest/`（30+ 个） |
| 构建配置 | `bundle.json`（依赖、syscap、rom/ram）+ `batterystats.gni`（part 检测、路径常量）+ 各 `BUILD.gn` |
| DFX（日志/事件） | `batterystats.yaml`（HiSysEvent 配置）+ grep `HiSysEvent` 调用点 |

### Path-based routing

| 修改路径 | 需了解的上下文 |
|---|---|
| `interfaces/inner_api/include/` | 所有 C++ 消费者的公共 API，变更需同步 `frameworks/napi/`、`frameworks/ets/taihe/` 两套绑定并检查 `*.map` 版本脚本 |
| `services/native/include/battery_stats_service.h` | 服务端核心类，所有统计流程的入口，**直接手写 stub/proxy**（无 `.idl`） |
| `services/native/include/battery_stats_core.h` | 统计核心，影响所有耗电实体的计算 |
| `services/native/include/entities/` | 耗电实体组织，新增实体需在此目录新建文件并在 `BatteryStatsCore` 中注册 |
| `services/native/include/battery_stats_subscriber.h` | 事件订阅入口，订阅哪些下游子系统事件在此决定 |
| `services/native/include/battery_stats_detector.h` | 周期采集入口，采集频率影响系统负载 |
| `services/native/include/cpu_time_reader.h` | CPU 时间读取，依赖 `/proc/stat` 等内核接口 |
| `services/native/include/battery_stats_parser.h` | 配置文件解析器，schema 变更需评估配置文件兼容性 |
| `services/profile/` | 平均耗电配置文件，修改直接影响耗电计算结果 |
| `frameworks/ets/taihe/` | taihe FFI **生成产物**，不要手改 |
| `batterystats.gni` | 无特性开关，只有 part 检测和路径常量定义，part 检测变更需同步 `bundle.json` |
| `utils/native/` | 跨仓共享工具，修改影响其他仓 |
| `batterystats.yaml` | HiSysEvent 事件定义，新增事件需在此声明 |
| `sa_profile/3304.json` | `run-on-create: false` 懒启动，首次调用时拉起服务 |

### Vocabulary-based routing

当任务、issue、日志、API 名称中出现以下术语时，先理解其含义和风险再动手：

| 术语 | 含义与风险 | 读取 |
|---|---|---|
| BatteryStats | Battery Statistics，本服务简称，SA 3304 | 本文件 + `sa_profile/3304.json` |
| BatteryStatsClient | 耗电统计客户端入口 | `frameworks/native/` 中 `battery_stats_client.h` |
| BatteryStatsService | 耗电统计服务端入口 | `services/native/include/battery_stats_service.h` |
| BatteryStatsCore | 统计核心，管理所有耗电实体 | `services/native/include/battery_stats_core.h` |
| BatteryStatsSubscriber | 事件订阅器，订阅下游子系统事件 | `services/native/include/battery_stats_subscriber.h` |
| BatteryStatsDetector | 周期采集器 | `services/native/include/battery_stats_detector.h` |
| CpuTimeReader | CPU 时间读取器，从 `/proc/stat` 读取 | `services/native/include/cpu_time_reader.h` |
| entities/ | 耗电实体组织目录（CPU/WiFi/蓝牙等） | `services/native/include/entities/` |
| AppStatsMah / AppStatsPercent | 应用耗电毫安数/百分比 | `interfaces/inner_api/include/` 对应接口 |
| PartStatsMah / PartStatsPercent | 硬件耗电毫安数/百分比 | `interfaces/inner_api/include/` 对应接口 |
| TotalTimeSecond / TotalDataBytes | 总时间/总数据量 | `interfaces/inner_api/include/` 对应接口 |
| 懒启动 | `run-on-create: false`，首次调用时拉起 SA | `sa_profile/3304.json` |
| 跨仓依赖 | 本仓订阅 `battery_manager` / `power_manager` / `display_manager` / `bluetooth` / `wifi` / `call_manager` / `os_account` 事件 | `bundle.json` 中 `deps.components` |
| Uid | Linux 用户 ID，耗电按 Uid 统计 | `services/native/include/entities/` |
| SystemAbility / SA | OpenHarmony 系统服务框架，本服务 SA ID 3304 | `sa_profile/3304.json` |
| sptr | OpenHarmony 共享指针（`refbase.h`），非 `std::shared_ptr` | 全项目 |
| HWTEST_F | OpenHarmony 测试用例宏 | 全项目测试代码 |
| part 检测 | `batterystats.gni` 中检测 `global_parts_info` 决定是否编译某 part 支持 | `batterystats.gni` |
| powermgr 进程 | 本仓与 power_manager/battery_manager/thermal_manager/display_manager 共享同一进程，多 SA 共进程 | `sa_profile/3304.json` |
| ShellDump | hidumper 入口，导出耗电统计快照 | `services/native/include/battery_stats_dumper.h` |

在计划阶段，必须声明：
- **任务分类**（如：公共 API 变更 / IPC 接口变更 / 统计核心变更 / 耗电实体变更 / 事件订阅变更 / 配置文件变更 / 新增 part 支持 / 测试修改）
- **已读取的代码路径和文档**（具体到文件路径）
- **发现的约束**（架构不变量、禁止事项、part 检测依赖、跨仓依赖）
- **是否需要同步修改其他层**：
  - 公共 API 变更 -> 同步 NAPI / taihe 两套绑定 + `*.map` 版本脚本 + IPC 码枚举
  - `services/native/` IPC 接口变更 -> 同步 stub + proxy（手写）
  - part 检测变更 -> 同步 `bundle.json` 的 `deps.components`
  - 跨仓依赖调用变更 -> 检查 `battery_manager` / `power_manager` / `display_manager` 等是否受影响
  - `batterystats.yaml` 变更 -> 同步 HiSysEvent 事件定义
  - `services/profile/` 变更 -> 评估对耗电计算结果的影响

## 3. 约束边界

### 架构不变量

- **客户端不持有业务状态**：`BatteryStatsClient` 仅做 IPC 转发，所有统计由服务端 `BatteryStatsService` 持有
- **耗电数据是累加型快照**：所有耗电数据按时间累加，不支持回滚，禁止业务直接修改统计字段
- **事件订阅是数据源**：所有耗电数据必须经 `BatteryStatsSubscriber` 从下游子系统订阅事件触发更新，禁止业务直接构造统计数据
- **`BatteryStatsCore` 是统计唯一入口**：所有耗电计算必须经 `BatteryStatsCore`，禁止耗电实体直接修改统计字段
- **公共 API 表达稳定能力意图，不暴露服务端内部字段**
- **权限校验在服务端入口完成**：`GetAppStatsMah` / `GetPartStatsMah` 等查询接口需按调用方权限返回不同粒度数据
- **IPC 调用必须设置死亡通知**，回调订阅必须支持客户端异常断开后自动清理
- **跨进程回调必须用 Parcelable 序列化**
- **DFX（日志、HiSysEvent、HiCollie、错误码）必须观测所有统计累加和事件订阅**
- **`BatteryStatsService` 是懒启动 SA**：首次调用时拉起，启动时需重置统计快照
- **耗电配置文件变更必须向后兼容**：`services/profile/` 的新增字段必须可被旧版本忽略

### 禁止事项

- **不要修改公共 API 签名、错误码、权限行为或耗电数据语义**，除非任务明确要求；修改 `*.map` 中已有符号的可见性属于破坏性变更
- **不要直接编辑 `frameworks/ets/taihe/` 下的生成文件**，应修改 `.taihe` 源文件后由构建系统重生成
- **不要为通过测试而删除日志、HiSysEvent 事件、错误码或诊断信息**
- **不要绕过现有的 DFX、安全、兼容性检查**
- **不要直接修改下游子系统接口**：本仓是 `battery_manager` / `power_manager` / `display_manager` / `bluetooth` / `wifi` / `call_manager` / `os_account` 的消费方
- **不要绕过 `BatteryStatsSubscriber` 直接订阅下游子系统事件**：所有订阅必须经 subscriber 统一管理
- **不要绕过 `BatteryStatsCore` 直接修改统计字段**：会导致统计数据不一致
- **不要在 `services/native/` 中只改 stub 不改 proxy**（或反之）：两套必须同步
- **不要在 `BatteryStatsDetector` 的周期采集回调中执行阻塞 IO 或长耗时操作**：采集频率高，阻塞会导致数据丢失
- **不要硬编码耗电系数或阈值**：必须用 `services/profile/` 配置文件
- **不要在 `powermgr` 进程中执行长耗时同步操作**：本进程与 power/battery/thermal/display 共享
- **不要引入新的生产依赖**而不经过 `bundle.json` 评审
- **不要修改 `sa_profile/3304.json` 的 `run-on-create` 字段**：改为 true 会增加首屏开销

### 需确认后再修改

- **公共 API 签名变更**（需确认兼容性影响和版本策略，更新 `*.map` 版本脚本）
- **IPC 码值变更**（新增接口追加新码值，不能复用或调整已有码值）
- **`batterystats.gni` part 检测段变更**（需确认 `bundle.json` 的 `deps.components` 同步，特别是新增 `bluetooth` / `wifi` / `call_manager` 依赖）
- **耗电配置文件 schema 变更**（需确认产品团队评审，影响所有产品耗电计算结果）
- **耗电系数调整**（需评估对耗电统计准确性的影响，特别是 `services/profile/` 中的平均耗电系数）
- **跨仓依赖调用变更**（需确认 `battery_manager` / `power_manager` / `display_manager` / `bluetooth` / `wifi` / `call_manager` / `os_account` 接口稳定性）
- **新增耗电实体**（需确认实体命名和统计口径与其他统计系统一致）
- **`sa_profile/3304.json` 的 `run-on-create` 字段调整**（需评估对首屏开销的影响）
- **新增外部依赖**（需确认许可证、包大小、`bundle.json` 同步）
- **`batterystats.yaml` 中 HiSysEvent 事件变更**（需确认 DFX 团队评审）
- **`utils/` 跨仓工具的接口变更**（需确认下游仓是否受影响）

### 项目特定陷阱

- **手写 proxy/stub 同步**：本项目不使用 `.idl`，修改接口签名必须同时改 stub（服务端入参解码）和 proxy（客户端出参编码），漏改一侧会导致 IPC 数据错乱
- **IPC 码值稳定性**：IPC 码值是 ABI 契约，新增接口必须追加新码值，复用或调整已有码值会让旧客户端调用错误接口
- **懒启动时序**：SA 3304 `run-on-create: false`，首次调用时才拉起服务，启动期间的事件可能丢失，订阅下游子系统事件需有重放机制
- **跨子系统事件时序**：`BatteryStatsSubscriber` 订阅多个子系统事件，事件时序不一致会导致统计误差，需有同步点
- **`/proc/stat` 读取的原子性**：`CpuTimeReader` 读取 `/proc/stat` 非原子，高频读取可能得到不一致快照，需有读取间隔保证
- **耗电实体的注册顺序**：`BatteryStatsCore` 中实体注册顺序影响计算顺序，新增实体需评估注册位置
- **配置文件的产品差异**：不同产品有不同的 `services/profile/` 配置，修改时需评估对其他产品的影响
- **`Uid` 与 `Pid` 的转换**：耗电按 `Uid` 统计，但子系统事件可能按 `Pid` 上报，需有转换逻辑
- **账户切换的统计隔离**：多用户场景下，`os_account` 切换需重置统计快照
- **`powermgr` 多 SA 共进程**：SA 3301/3302/3303/3308 在同一进程，一个 SA 崩溃会拖死全部
- **30+ 个 fuzzer 的维护成本**：新增 IPC 接口必须同步新增 fuzzer，否则会引入模糊测试覆盖空白
- **跨仓依赖的循环风险**：`battery_statistics` 依赖 `battery_manager` / `power_manager` / `display_manager`，但这些仓也可能依赖 `battery_statistics`（如 `thermal_manager` 依赖 `battery_statistics`），需避免循环

## 4. 验证闭环

### 最小验证

```bash
# 构建 battery_statistics 子系统（从 OpenHarmony 根目录执行）
./build.sh --product-name rk3568 --build-target battery_statistics

# 构建全部测试
./build.sh --product-name rk3568 --build-target battery_statistics_test
```

### 任务特定验证

| 任务类型 | 验证命令 |
|---|---|
| 公共 API 变更 | `./build.sh --product-name rk3568 --build-target battery_statistics` + 同步构建依赖本仓的下游仓（`thermal_manager`） + 跑 `test/unittest/` |
| IPC 接口变更 | 跑 `test/unittest/` + `test/fuzztest/` 所有 30+ 个 fuzzer（特别是 `batterystatsclient_fuzzer` / `batterystatsinfo_fuzzer` / `batteryparcel_fuzzer`） |
| 统计核心变更 | 跑 `test/unittest/` 统计用例 + `test/fuzztest/statscore_fuzzer` / `statshelper_fuzzer` |
| 耗电实体变更 | 跑 `test/unittest/` 实体用例 + 对应实体 fuzzer（如 `activetimer_fuzzer` / `counter_fuzzer`） |
| 事件订阅变更 | 跑 `test/systemtest/` 多子系统场景 + `test/fuzztest/subscriber_fuzzer` / `listener_fuzzer` / `onremotedied_fuzzer` |
| 配置文件变更 | 真机验证耗电计算结果 + `test/fuzztest/configparser_fuzzer` |
| 跨仓依赖调用 | 同步构建 `battery_manager` / `power_manager` / `display_manager` + 验证调用接口未变 |
| part 检测变更 | 重新构建 `battery_statistics` 全量 + 验证 `bundle.json` 中 `deps.components` 同步 |
| DFX 变更 | 真机验证 HiSysEvent 事件 + `test/fuzztest/dump_fuzzer` / `resetdump_fuzzer` |

### Done 定义

- 构建通过（子系统 + 单元测试 + 模糊测试）
- 无新增编译警告
- 变更范围与任务要求一致，未夹带未关联的重构
- IPC 接口变更已同步 stub/proxy/IPC 码三处
- part 检测变更已同步 `batterystats.gni` + `bundle.json`
- 公共 API 变更已同步 NAPI / taihe / `*.map` 版本脚本
- 耗电配置变更已评估对耗电计算结果的影响
- 跨仓依赖调用变更已评估对下游仓的影响
- 涉及懒启动时序的变更已评估对首屏统计的影响

### 最终响应期望

完成报告必须包含：
1. 修改的文件清单（按 `file:line` 引用）
2. 任务分类与对应验证命令的执行结果
3. 是否触发跨层同步修改（NAPI / taihe / `*.map` / IPC 码 / `bundle.json` / `batterystats.yaml` / `services/profile/`）
4. 是否影响 part 检测、跨仓依赖或懒启动时序
5. 是否触及架构不变量或需确认事项
6. 涉及统计核心/耗电实体的变更需额外说明统计数据一致性保证

### 无法验证时

如果构建环境不可用，列出应执行的命令并说明预期结果，明确标注「未验证」字样，不能假称已通过。涉及耗电统计/事件订阅/懒启动时序的变更，必须人工复核代码逻辑并说明无法在沙箱验证的限制。
