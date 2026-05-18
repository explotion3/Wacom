# UI 架构 MVVM 迁移 M1+M2

> 时间：2026-05-18
> 范围：Run 域 widget 从"直接订阅 RunSession + 手动 Tick 拉数据"迁到"ViewModel + Provider 订阅模型"

## 背景

ExplorationHUD（Stage 5）和 BackpackScreen（Stage 4.2）原先都直接订阅 `URunSession::OnRunStateChangedNative`，widget 自己读 RunState 字段调 SetText。出现了几个反复修补的 bug：

- 探索 HUD 用 `NativeTick` 每帧拉数据 → 性能浪费
- 探索 HUD 战斗结束后不刷新 → CommonUI Stack Reactivate 时 `NativeConstruct` 不重新调用，订阅丢失
- 背包 UI 操作后手动 `RebuildFromRunState()` → 命令式刷新散落各处

根因：widget 直接耦合业务层数据源，CommonUI Stack 的 widget 生命周期（Construct → Activate ↔ Deactivate ↔ Activate → Destruct）跟订阅生命周期不匹配。

## M1：Run 域 ViewModel 层

### 启用 MVVM 插件

- `Wacom.uproject` 加 `ModelViewViewModel` 插件
- `WacomApp.Build.cs` 加 `ModelViewViewModel` + `FieldNotification` 依赖

### 新建数据层

| 类 | 角色 |
|---|---|
| `UWacomRunViewModel` | `UMVVMViewModelBase` 子类。21 个 FieldNotify 字段（Phase / NodeCount / Pressure × 8 / Capacity / Gold / ...）。**纯数据，不持有 Session，不订阅事件**。便于单测 |
| `UWacomRunViewModelProvider` | GameInstance Subsystem。**唯一**订阅 `RunSession.OnRunStateChangedNative` 的地方。读 RunState → 调 ViewModel.SetXxx → 末尾广播粗粒度 `OnRunViewModelRefreshedNative` |

### 关键设计

- ViewModel 注册到 **MVVM Global Viewmodel Collection**（ContextName=`WacomRunViewModel`），让多个 widget 共享同一实例
- Provider 在 `Initialize` 中调 `Collection.InitializeDependency(UMVVMGameSubsystem)` —— 保证 MVVMSubsystem 早于本 Subsystem Initialize、晚于本 Subsystem Deinitialize；Deinitialize 末尾仍判空 RemoveViewModel 作防御层
- PC::BeginPlay 创建 RunSession 后立刻调 `Provider->BindToPlayerController(this)`，订阅 RunSession 事件 + 立即同步当前状态到 ViewModel

### Setter 实现

每个 FieldNotify 字段有对应 `SetXxx` 函数，内部 `UE_MVVM_SET_PROPERTY_VALUE(Field, NewValue)` 宏：
- 自动对比新旧值，相同则不广播（dedupe）
- 不同则赋值 + 触发 FieldNotify

派生 FieldNotify 函数（如 `GetExperienceRatio`）需手动级联：在依赖字段的 Setter 末尾调 `UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetExperienceRatio)`。

## M2：Widget 切到 ViewModel

### ExplorationHUD

- 删 `RefreshFromRunSession`，改 `RefreshFromViewModel`
- 删 `OnRunStateChangedNative` 订阅，改订阅 `Provider.OnRunViewModelRefreshedNative`
- 完全不再认识 RunSession，只认 ViewModel + Provider
- `NativeOnActivated` 调 `TrySubscribeAndRefresh` —— Reactivate 时补刷一次防漏

### BackpackScreen

- 顶部统计（金币 / 备战 N/M / 背包 N/M）：读 ViewModel 字段
- WrapBox 列表（卡片实例）：仍读 RunSession（数组数据 ViewBinding 不擅长，保留命令式）
- HandleMoveCard / HandleDeleteCard 删除手动 `RebuildFromRunState()` 调用 → RunSession 写入后通过 Provider 自动回流刷新
- 同款 `NativeOnActivated` + `TrySubscribeAndRefresh`

### GameMode 配置入口（混合模式）

- `AWacomGameMode::ExplorationHUDClass` UPROPERTY：BP 子类未配则回退 `UWacomExplorationHUD::StaticClass()`
- `AWacomPlayerController::BackpackScreenClass` UPROPERTY：同上
- 跟现有 `BattleHUDClass` 同款。美术阶段建 BP 子类 + 在 Details 面板拖 WBP 即可激活完整 MVVM

## 单测

新文件 `Source/WacomTests/Private/UI/RunViewModelSpec.cpp`，3 个 case：

1. `SetterBroadcasts` — Setter 改值 + 广播次数正确
2. `SameValueDoesNotRebroadcast` — UE_MVVM_SET_PROPERTY_VALUE dedupe 验证
3. `ExperienceRatioCascadesOnDependencyChange` — 派生 FieldNotify 函数级联

`Wacom.* 测试 94 → 97，全过`。

## 关键决策的诚实复盘

### 为什么不彻底迁 WBP + ViewBinding

美术参考图未到，Designer 里画临时布局 + 配 ViewBinding 等美术阶段还得重做一遍。当前 widget 端用 C++ 父类硬编码布局 + 订阅 Provider 粗粒度多播 + 手动 SetText，**FieldNotify 字段已就位但暂未被 WBP ViewBinding 消费**。

这意味着当前的"MVVM"严格说是**预备状态**——付了插件成本，将来切 WBP 时直接套 ViewBinding 兑现，不需要重写数据层。

### 为什么战斗 UI 不迁

战斗 UI 用 Snapshot 推送模型（BattleHUD 作 Controller 递归 RefreshFromSnapshot）天然没有 Reactivate 风险（战斗开始 Push、结束 Pop，整棵树重建）。9 个子 widget 同一棵树，Controller 递归刷新简洁，迁 ViewModel 收益不明显。

### 为什么用 MVVM 插件而非纯 multicast

考虑 1-2 个月后美术阶段切 WBP 时可以直接复用插件提供的 ViewBinding 编辑器、FieldNotify 自动级联、Global Collection。当前没用上插件全部能力，但提前付的成本将来兑现，不浪费。

如果美术阶段在更远的将来（半年以上），更优解是不用插件、纯 multicast + UObject，等切 WBP 时一次性升级——但这是事后判断，当前路径已稳定不回退。

## 踩坑

- `UMVVMGameSubsystem::Deinitialize` 把 `ViewModelCollection = nullptr`。如果 Provider Deinitialize 在 MVVMSub 之后跑，`MVVMSub->GetViewModelCollection()` 返回 nullptr，调 `RemoveInstance` 崩。修法：`InitializeDependency(UMVVMGameSubsystem)` 声明依赖 + 判空兜底
- 测试 fixture 用 `INotifyFieldValueChanged::AddFieldValueChangedDelegate` 订阅指定字段，需要 include `FieldNotification/IFieldValueChanged.h` + `FieldNotification/FieldId.h`
- WacomTests Build.cs 加 `WacomApp / ModelViewViewModel / FieldNotification` 依赖

## 验证

- 编译：`Build.bat WacomEditor` 通过
- 自动化测试：Wacom.* **97/97** 全绿（94 → 97）
- 手动 PIE：探索期 HUD / 背包 / 战斗前后切换 / 背包操作刷新 / PIE 退出无崩

## 待办

参考 `Docs/TODO.md`：

- 美术阶段切 WBP + 配 View Bindings（M1+M2 已铺好路径）
- 战斗 UI 是否加 BattleViewModel：等"非战斗 widget 需要看战斗状态"的需求出现再决定（如 Stage 7 击倒事件 UI / 探索期战斗状态小窗）
