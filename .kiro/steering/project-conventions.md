---
inclusion: always
---

# Wacom 项目总约定

本文是项目级的持久开发规则。每次对话默认加载。

## 项目身份

- 单人第一人称 HD-2D 卡牌冒险游戏，UE 5.7，主导程序单人。
- 文档位于 `Docs/`：`Game_Design.md` / `Architecture.md` / `WacomBattle.md` / `WacomRun.md` / `WacomApp.md` / `WacomData.md` / `TODO.md` / `DevLog/` / `Characters/BugGirl.md` / `Karpathy_Guidelines_CN.md`

## 任务启动流程

每次接到新任务时：

1. 判断任务属于哪个领域
2. 读取对应文档和代码
3. **如果需求模糊**（多种理解方式、边界不清、成功标准不明确）：激活 `karpathy-guidelines` skill，显式列出假设和疑问，与主导达成一致后再动手
4. 完成后同步更新对应文档

| 领域 | 读取文档 | 读取代码 |
|---|---|---|
| `Battle` | `Docs/WacomBattle.md` | `Source/WacomBattle/` |
| `Run` | `Docs/WacomRun.md` | `Source/WacomRun/` |
| `Data` | `Docs/WacomData.md` | `Source/WacomData/` |
| `App/UI` | `Docs/WacomApp.md` | `Source/WacomApp/` |
| `Editor` | `Docs/Architecture.md` | `Source/WacomEditor/` |
| `Docs` | 目标文档本身 | 不需要读代码 |

跨领域任务读取多个文档。不确定归属时先问。

## 虚幻相关任务的 skill 调用

在**计划或落实任何与 Unreal Engine 相关的代码改动之前**（包括但不限于：模块/Build.cs 调整、UPROPERTY/UFUNCTION 反射边界、Actor/Component/Subsystem 设计、UMG/Slate UI、Blueprint 工作流、保存/复制、PCG、性能/打包、世界交互、调试验证），**必须先查可用 skill 列表**，命中就激活，避免凭直觉绕过 UE 既有最佳实践（典型反例：UI 用 Tick 拉数据 vs 用 MVVM/委托推送）。

**触发条件**：任务涉及 UE API、UE 子系统、UE 资产工作流、UE 性能/打包、UE 测试，任一条命中即查。

**当前可用 UE skill**：

| skill 名 | 适用范围 |
|---|---|
| `ue5-auto-assistant` | 不确定该选哪个 skill 时的入口路由 |
| `ue5-module-router` | 涉及模块名 / Build.cs 依赖 / 跨模块边界判定 |
| `ue5-architecture` | 模块布局 / Public-Private 边界 / 反射暴露策略 / 防循环依赖 |
| `ue5-cpp-gameplay` | Actor / Component / DataAsset / GameplayTags / 蓝图暴露 |
| `ue5-blueprint-workflow` | Blueprint 图、节点连线、输入事件、图验证 |
| `ue5-ui-umg-slate` | Widget / Slate / WBP / 输入焦点 / 视口适配 / **MVVM** |
| `ue5-save-load-replication` | SaveGame / 序列化 / RepNotify / RPC |
| `ue5-world-interaction` | 拾取 / 触发器 / Overlap / 交互反馈 / Spawner |
| `ue5-pcg-building` | PCG / 程序化建筑 / Shape Grammar |
| `ue5-debug-validation` | 调试日志 / 资产校验 / 最小复现 / 修复步骤 |
| `ue5-performance-packaging` | PIE 性能检查 / stat / 打包前验证 / 构建配置 |

**调用顺序**：
1. 任务进来 → 先判断是否涉及 UE → 命中就在表里挑最贴切的 skill 激活
2. 不确定挑哪个 → 激活 `ue5-auto-assistant` 路由
3. 同时仍按"任务启动流程"读领域文档和代码
4. 需求模糊时叠加 `karpathy-guidelines` 列假设

**反面用例**：
- 写探索 HUD 直接 `NativeTick + SetText` 拉数据 → 应先查 `ue5-ui-umg-slate`，命中 MVVM 章节
- 加新 UCLASS 凭感觉决定 Public 还是 Private → 应先查 `ue5-architecture`
- 调试 Editor 崩溃直接看代码猜 → 应先查 `ue5-debug-validation`

## UI 约定

- CommonUI 管理层：`UWacomGameUIManagerSubsystem` + `UWacomPrimaryGameLayout`
- 四层 Layer：`Game` / `GameMenu` / `Modal` / `Overlay`
- Widget 血统：`UWacomActivatableWidget`（根）→ `UWacomBattleWidgetBase`（战斗）/ `UWacomMenuWidgetBase`（菜单）
- C++ 定义结构和接口，WBP 做布局和样式
- UI 只读 Snapshot + 发 Command，不修改战斗状态
- Widget 不直接 SubmitCommand / OpenLevel，委托给 HUD / GameMode

### Widget 生命周期契约（必须）

每个新建的 Activatable Widget 在头文件类注释中**必须显式声明**以下五项，缺一不可：

```
数据源：XXX（如 URunSession / UBattleSession / UWacomRunViewModelProvider）
更新触发：Push（订阅 multicast）/ Pull（Snapshot Version 比较）/ 一次性
订阅时机：NativeConstruct + NativeOnActivated（补订阅）
反订阅时机：NativeDestruct
焦点/输入：Game / Menu / All + CapturePermanently / NoCapture
```

**为什么 NativeOnActivated 也要处理**：CommonUI Stack Push 新 widget 时底层 Deactivate（不 Destruct），Pop 后底层 Reactivate（不重新 Construct）。如果只在 NativeConstruct 订阅/刷新，Reactivate 后会错过战斗期间的状态变更。

### Run 域 widget 数据流（M1+M2 起强制）

新增 Run 域 widget（探索 HUD / 背包 / 菜单 / 节点事件 UI）数据获取顺序：

1. **不直接订阅 `URunSession`**——业务事件订阅由 `UWacomRunViewModelProvider` 集中处理
2. **订阅 `Provider.OnRunViewModelRefreshedNative`**（粗粒度多播），不订阅 ViewModel 的 FieldNotify
3. **数据来源是 `UWacomRunViewModel` 字段**（`Provider->GetRunViewModel()->GetXxx()`）
4. **NativeConstruct + NativeOnActivated 都调 `TrySubscribeAndRefresh`**，NativeDestruct 反订阅

例外：动态列表（卡牌实例数组 / 部位实例数组）允许直接读 `RunSession.GetBackpack()`——MVVM 不擅长数组绑定，列表数据保留命令式读取。

### 战斗域 widget 数据流（保持现有 Snapshot 推送）

战斗 widget 继承 `UWacomBattleWidgetBase`，由 `BattleHUD` 作 Controller 递归 `RefreshFromSnapshot`。子 widget 不订阅 Session，命令通过委托发回 BattleHUD。**不要硬迁 ViewModel**——战斗 widget 短生命周期 + 同步 push 没有 Reactivate 风险，迁了反而复杂。

**反面用例**：ExplorationHUD 第一版只在 NativeConstruct 订阅 + 用 Tick 拉数据 → 战斗结束后不刷新 → 反复修补。修法是抽 ViewModel + Provider 层，让订阅生命周期与 widget 解耦。

## 反射使用门槛

只有以下理由才用 UCLASS / USTRUCT / UENUM / UPROPERTY：蓝图访问、DataAsset 承载、序列化、GC 引用管理、反射遍历、UMG Widget。否则用纯 C++。

## Public / Private 边界

- `Public/`：对外协议、轻量类型、入口。`Private/`：规则实现、内部服务。
- 头文件优先前向声明，具体 include 放 `.cpp`。
- 战斗真相必须留在 `WacomBattle/Private/`，外部只通过 `UBattleSession` + Snapshot + Event 访问。

## 术语表

- ✅ `EnemyPart`（敌方部位）、`HandZone`（手牌区域）、`HandQueue`（手牌队列）、`RuntimeCost` vs `BaseCost`、`HandAnchor`（左手牌/右手牌）

## GameplayTag 约束

- 所有 tag 在 `WacomCore/Public/Tags/WacomGameplayTags.h` 用 `UE_DECLARE_GAMEPLAY_TAG_EXTERN` 声明
- 严禁字符串拼 tag
- 新增 tag 同步更新 `WacomData.md §5`

## 完成即验证

每个切片写完必须编译通过 + 自动化测试全绿。

```
编译: "e:\UE_5.7\Engine\Build\BatchFiles\Build.bat" WacomEditor Win64 Development -Project="d:\UE_Project\5.7\Wacom\Wacom.uproject" -WaitMutex -FromMsBuild
测试: "e:\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "d:\UE_Project\5.7\Wacom\Wacom.uproject" -ExecCmds="Automation RunTests Wacom; Quit" -Unattended -NoPause -NoSplash -NullRHI
```

## 修 bug 不取捷径

修复问题时**禁止**选最省事的修法（加判空、try/catch 吞错、加 flag 兜底、改 oldStr 改最少行），而要追根因，选**正规、长期有效、与项目架构方向一致**的方案。捷径修法会让症状消失但根因还在，下一次返工会成倍花时间。

判断方法：

1. **找根因**：用调用栈、日志、代码追踪，说出"为什么会发生"。仅说"加判空就不会崩"不算找到根因。
2. **找规范修法**：查 UE 官方文档 / 相关 skill / 项目已有同类问题的解决先例。如果存在更正规的 API（例子：`Collection.InitializeDependency` 决定 Subsystem 销毁顺序，比每个使用点判空更正规），优先用规范方案。
3. **判空 / 兜底 / try-catch 只能作为防御层**：在已经走过 1+2 之后再加，作为"即使根因方案失效也不崩"的兜底；不可作为唯一修法。
4. **改动方向**：每次修复要让代码**离正确架构更近**，不能更远。修一个 bug 引入一个临时 hack 是负向重构，必须避免。
5. **如果时间紧迫只能取捷径**：必须显式标注 `// TODO(技术债)` 并写进 `Docs/TODO.md §2 临时写法` 表，注明"正式方案"是什么。

**反面用例**：
- ViewModelProvider 在 Deinitialize 崩 → 第一反应"加 nullptr 判空"。判空只是兜底，根因是 Subsystem 销毁顺序不保证。规范修法是 `InitializeDependency` 声明依赖顺序；判空作为防御层留在 Deinitialize，但**不是唯一修法**。
- 探索 HUD 战斗结束后不刷新 → 第一反应"在 NativeOnActivated 里补刷一次"。补刷只是兜底，根因是 widget 直接订阅业务层数据，CommonUI 生命周期不匹配。规范修法是 MVVM 解耦，让 ViewModel 维持订阅、widget 只读 ViewModel。

## 随机与确定性

所有随机使用 `BattleState.Rng`（FRandomStream），禁止 `FMath::Rand*`。测试可注入 seed。

## 临时决定

先查 `TODO.md`。有就按它走；没有就新建写进去；正式化后标记。

## 规则真相

规则真相在 `Docs/*.md`，不在脑内。遇到分歧：先查文档 → 没覆盖就标注"规则分歧"向主导提问 → 共识后先改文档再改代码。绝不悄悄引入未写入文档的规则分支。

## 开发日志

大规模任务完成后在 `Docs/DevLog/` 新建一个 md 文件：
- 文件名一眼看懂（如 `存档系统S1-S4.md`、`主菜单M1-M4.md`）
- 内容简单明了：做了什么、关键决策、踩坑点
- 不追加到旧文件，每次新开一个
