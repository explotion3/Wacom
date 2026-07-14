# Implementation Plan: Backpack Workspace Refactor

**Branch**: `main` | **Date**: 2026-07-13 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `specs/003-backpack-workspace-refactor/spec.md`

## Summary

把现有“所有区域同时展开 + 单卡 UMG DragDrop”的背包正式重构为右侧常驻区域牌匣与中央单区域 Canvas 工作台。`WacomRun` 增加原子批量移动、批量销毁和预检 contract；`WacomApp` 用独立的瞬态布局存储、纯 C++ 交互状态模型和被动 Workspace/ZoneRack Widget 承担框选、扇形携带、滚轮当前牌、逐张/整组释放及生命周期清理。现有 Snapshot、详情、Toast、SpecialZone 入战切换继续复用，但旧单卡 `UWacomCardDragOperation` 状态机在新路径稳定后删除，避免两套输入所有权并存。

## Wacom Domain Context

**Primary Domain**: Run-exploration / UI-App / Tests

**Required Docs Read**:
- [x] `AGENTS.md`
- [x] `Docs/Architecture.md`
- [x] `Docs/WacomRun.md`
- [x] `Docs/WacomApp.md`
- [x] `Docs/WacomUI.md`
- [x] `Docs/WacomUIFoundation.md`
- [x] `Docs/UI_Backpack_WBP_Binding.md`

**Docs To Update**:
- [ ] `Docs/WacomRun.md`：原子 batch move/delete、一次 revision/广播、失败零修改。
- [ ] `Docs/WacomUI.md`：工作台数据流、选择/携带输入所有权、布局生命周期。
- [ ] `Docs/UI_Backpack_WBP_Binding.md`：新 Workspace/ZoneRack/Confirm WBP 合同与旧 Host 迁移。
- [ ] `Docs/WacomApp.md`：背包瞬态 UI state owner 与 CommonUI 生命周期。
- [ ] `Docs/TODO.md`：仅在正式 WBP、材质或完整手柄支持延期时记录剩余内容。
- [ ] `Docs/TechDebt.md`：仅在无法一次删除旧拖放双轨时记录明确清理条件。

**Owning Module(s)**: `WacomRun`, `WacomApp`, `WacomTests`

**Dependency Direction**:

```text
WacomCore <- WacomData <- WacomBattle <- WacomRun <- WacomApp

WacomTests -> WacomRun / WacomApp as test harness
```

`WacomBattle`、`WacomData` 和 `WacomCore` 不需要新增依赖或规则。DreamShader 只提供 App 表现材质，不进入 Run contract。

## Technical Context

**Language/Engine**: C++17, Unreal Engine 5.8

**Primary UE Systems**: CommonUI, UMG/Slate input and mouse capture, CanvasPanel free layout, GameInstanceSubsystem transient presentation state, Automation Tests, DataAsset-style presentation tuning, DreamShader Graph-first materials

**Storage/State**:
- 权威卡牌归属继续保存在 `FRunState`。
- 区域卡牌读取继续来自 `FRunBackpackStorageSnapshot`，必要时补充明确的可移动/只读 ViewData 字段。
- 自由布局保存在 `WacomApp` 当前 Run 关联的瞬态 workspace state，不进入 `FRunState` 或 `UWacomSaveGame`。
- 选择、框选、携带、当前牌和起手释放保护只存在于激活的 Backpack Screen/coordinator。

**Runtime Contracts**:
- `FRunDeckBatchMoveRequest` / `FRunDeckBatchDeleteRequest`（名称可在实现时按现有命名风格微调）；两类请求都必须携带 Snapshot 捕获的 storage revision，提交时严格相等才允许继续。
- `FRunDeckBatchValidation` / `FRunDeckBatchResult`，包含统一失败 reason、影响数量和删牌总奖励。
- `URunSession::ValidateMoveInstancesAtomic` / `MoveInstancesAtomic` 与 `ValidateDeleteCardsForGoldAtomic` / `DeleteCardsForGoldAtomic`。
- Workspace 只发出选择、布局、move/delete intent；Screen 私有 command flow 把 intent 转为 Run request。

**Testing**:
- 新建小型 `Source/WacomTests/Private/Run/BackpackBatchTransactionSpec.cpp`。
- 在 `Source/WacomApp/Public/UI/Backpack/WacomBackpackWorkspaceWidget.h` 定义非反射、只读的 production automation view；新建 `Source/WacomTests/Private/UI/Backpack/BackpackWorkspaceInteractionSpec.cpp`、`BackpackWorkspaceLifecycleSpec.cpp` 和只负责访问/断言适配的 private wrapper。不得把 production 状态结构定义在 `WacomTests`，也不得新增散落 Blueprint/`ForTest` API。
- 保留并迁移现有 `Wacom.UI.Backpack` 覆盖，不继续扩大 `BackpackScreenSpec.cpp`。
- 编译 `WacomEditor Win64 Development`，运行 `Wacom.Run.Backpack` 与 `Wacom.UI.Backpack` 聚焦测试，并执行 PIE 手感清单。

**Target Platform**: Windows editor / packaged Windows build；首版工作台精细操作以鼠标键盘为主。

**Performance Goals**:
- 20–100 张活动区卡牌时，指针跟随、框选与扇形重排保持流畅，不按帧查询 Run 状态。
- Snapshot revision 未变化时不重建卡牌树；增量 reconcile 保持 InstanceId 稳定。
- 动画更新仅在携带、回弹或布局过渡活跃时运行，空闲工作台不持续 Tick 轮询。

**Constraints**:
- UI passive；规则校验、容量、负重、金币、revision 和广播只由 Run 提交入口负责。
- 批量提交必须全成全败，重复/无效/来源变化的 InstanceId 必须拒绝。
- 同区牌匣 drop 是 App layout reset，不调用 Run move，也不改变规则数组顺序。
- CommonUI Deactivate、区域切换、确认弹窗和 Snapshot 变化必须释放鼠标捕获并清除过期 carry。
- 外部 Demo 与 shader 只作为参考，不复制 GPL 工程代码或资产。

**Scale/Scope**: 2 个生产模块、3–5 个新 Run contract/helper、约 6–10 个 Backpack UI/state/coordinator 文件、3 个小型测试 spec、3–4 份长期文档、正式 WBP/样式/可选 DreamShader 表现资产。

**Blueprint Exposure Strategy**:
- Run batch request/result 默认使用 C++ public contract，不暴露 BlueprintCallable；WBP 不允许直接调用 Run。
- `UWacomBackpackWorkspaceWidget`、`UWacomBackpackZoneRackWidget`、entry/card view 和 presentation style 仅暴露布局绑定、样式参数与被动事件。
- 编辑器可调数值使用中文 `ToolTip`，包含单位、影响范围和推荐区间，不用无真实约束的 Clamp 限死调参。

**Data/GameplayTag Impact**: 不新增 WacomData 字段或 GameplayTag。新增内容仅为 WacomApp UI presentation style、WBP 和可选材质资产。

**Save/Load Impact**: 无 schema 变化。Workspace layout 明确不序列化；绑定到新的 `URunSession` 时清空旧布局。

**UI/App Lifecycle Impact**:
- Screen Activate：获取当前 Run 对应 workspace state、订阅 provider、设置焦点与输入路由、恢复活动区布局。
- Snapshot refresh：按 InstanceId reconcile，移除失效布局/选择/carry 条目，新卡进入默认布局。
- Region switch：取消 carry/selection 与鼠标捕获，保存旧区布局，切换单一活动区。
- Delete confirm：暂停 carry 提交但保留快照；取消恢复，确认前再次预检。
- Screen Deactivate/Destruct：关闭确认、取消 carry/selection、释放捕获、隐藏详情、反订阅；布局仍保留在当前 Run 的瞬态 store。

## Constitution Check

*Pre-design gate: PASS. Post-design re-check: PASS.*

- **Docs and AGENTS Are the Rule Truth**: 已列出 Run/UI/App/Binding 文档及实现后回写位置；spec/plan 不作为长期事实终点。
- **Wacom Module Boundaries Are Mandatory**: Run 原子事务在 `WacomRun`，App 布局与输入在 `WacomApp`，测试在 `WacomTests`；无反向依赖。
- **Domain Rules Before Presentation**: Widget 不循环调用单卡规则，不自行判容量或奖励；Screen command flow 只提交 Run batch request。
- **Data, GameplayTags, and Authoring Stay Explicit**: 无 GameplayTag/WacomData/SaveGame 变化；UI style 与 WBP 被明确标为 App presentation asset。
- **Reusable Systems Over One-Off Work**: 使用通用 batch transaction、workspace state store、interaction model、layout solver 和被动 Widget，不把全部状态塞进 `UWacomBackpackScreen`。
- **Validation Is Part of the Slice**: 原子事务、输入状态机和生命周期有小型自动化 spec；视觉与手感由 PIE 验收。

## Phase 0: Research

研究结论见 [research.md](./research.md)：

1. 现有 `FRunDeckRules` 适合作为批量事务内核，采用 working-copy validate/apply/commit，而不是 UI 循环调用 `MoveInstance`。
2. UMG `UDragDropOperation` 的 release 生命周期不适合“第一次释放后继续携带”，新背包采用显式 carry state machine 与鼠标捕获。
3. 自由布局由 App 瞬态 subsystem/store 按 Run identity 和 zone key 保存，避免污染 Run/SaveGame。
4. Workspace 通过 Canvas + 稳定 InstanceId reconcile 实现；同区收拢只重置 layout entry。
5. DreamShader 只用于独立 presentation layer，先稳定交互 contract，再接 fake-3D/边缘/选中反馈。

所有计划未知项已解决，无 `NEEDS CLARIFICATION`。

## Phase 1: Design Artifacts

- [data-model.md](./data-model.md)：Run batch request/result、zone key、layout entry、selection/carry state 和生命周期。
- [contracts/run-batch-storage-contract.md](./contracts/run-batch-storage-contract.md)：全成全败、一次 revision/广播、预检与提交一致性。
- [contracts/backpack-workspace-input-contract.md](./contracts/backpack-workspace-input-contract.md)：选择、携带、滚轮、逐张/整组释放及取消语义。
- [contracts/backpack-wbp-binding-contract.md](./contracts/backpack-wbp-binding-contract.md)：新 WBP host、被动边界与旧路径迁移。
- [quickstart.md](./quickstart.md)：编译、聚焦自动化与 PIE 验收。

## Implementation Slices

### Slice A — Run atomic batch foundation

- 在 `RunStateTypes.h` 或新的 `Public/Deck/` contract 文件定义非 Blueprint 的 batch request/validation/result。
- 在 `Private/Deck/RunDeckRules` 上实现 working-state 事务：去重、来源核对、逐项规则验证、统一负重重算、成功 commit。
- `URunSession` 只在成功后更新权威 state 并调用一次 `NotifyRunStateChanged()`；失败不改 state/revision/gold。
- 先补 Run automation，保留现有单卡 API 作为其它功能兼容入口，但新 Workspace 不通过循环调用它们实现 batch。

### Slice B — Workspace state and passive shell

- 新增 App 瞬态 workspace state store，按 `URunSession` identity 和 `FWacomBackpackZoneKey` 保存 layout。
- 新增纯 C++ layout/selection/carry model，避免把算法和可测试状态散在 Widget 字段中。
- 新增被动 Workspace/ZoneRack/ZoneRackEntry WBP parent class，`UWacomBackpackScreen` 只协调 Snapshot、intent 与 command flow。
- 将现有 Snapshot reconcile、详情、Toast 和 SpecialZone toggle 接入单活动区，不改变 Run 规则。

### Slice C — Selection and persistent fan carry

- Card item 发送 pointer intent；Workspace 集中处理单选、Ctrl toggle、中心点框选、Ctrl+A。
- 自定义 carry model 处理起手 release guard、右侧默认当前牌、无默认 lift、滚轮 clamp、逐张 release 与右键全 release。
- Canvas 负责自由位置、角度、ZOrder、30% 可见约束和“整理全部”。
- 区域切换/Deactivate/refresh 统一走 cancel path，释放鼠标捕获和详情。

### Slice D — Batch targets, confirmation, and migration cleanup

- ZoneRack hover/drop preview 通过 Screen command flow 调 Run batch validation；跨区成功后 reconcile 两区。
- Same-zone target 仅执行 layout collect；Delete target 打开一次整组确认，确认时重新 validation。
- 删除或改造成无状态兼容 facade 的 `UWacomCardDragOperation` / `UWacomZoneDropTarget` 旧输入路径；最终不保留两套 drag owner。
- 更新现有 Backpack tests，拆出新小型 spec；更新长期 Docs。

### Slice E — WBP, motion, and DreamShader polish

- 制作/重构 `WBP_BackpackScreen`、Workspace、ZoneRack entry、Delete confirm 与 selection rectangle。
- 用 presentation style 资产统一卡片尺寸、扇形角度/间距、非默认 lift、高度、跟随/回弹时间、选中状态和区域牌匣样式。
- 背包卡面使用独立 `WBP_BackpackCardView`：固定缩放外包一层 invalidation-only Retainer，内部复用 authored `WBP_FirstPersonCardView`；背包实例关闭并释放内层动态 `SurfaceFoilOverlay` 的材质 Brush，不改变共享战斗卡面默认值，也不复用 first-person `WBP_FPCardView` 的 fake-3D、战斗反馈或手势层。
- 复用现有 `UWacomCardView` 与 Wacom DreamShader card material 管线；如需新效果，使用 `.dsm` Graph-first 与 `DShader/Shared` helper，保持 hover/carry/material 层独立。
- PIE 调整手感；不把 Demo 数值写入 Run 或 Screen command flow。

## Project Structure

### Documentation (this feature)

```text
specs/003-backpack-workspace-refactor/
├── spec.md
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── contracts/
│   ├── run-batch-storage-contract.md
│   ├── backpack-workspace-input-contract.md
│   └── backpack-wbp-binding-contract.md
├── checklists/requirements.md
└── tasks.md
```

### Expected Production/Test Areas

```text
Source/WacomRun/
├── Public/
│   ├── RunSession.h
│   └── RunStateTypes.h or Deck/RunDeckBatchTypes.h
└── Private/
    ├── RunSession.cpp
    └── Deck/RunDeckRules.{h,cpp}

Source/WacomApp/
├── Public/UI/Backpack/
│   ├── WacomBackpackScreen.h
│   ├── WacomBackpackWorkspaceWidget.h
│   ├── WacomBackpackZoneRackWidget.h
│   ├── WacomBackpackZoneRackEntryWidget.h
│   └── WacomBackpackWorkspaceStyle.h
└── Private/UI/Backpack/
    ├── WacomBackpackScreen.cpp
    ├── WacomBackpackCommandFlow.{h,cpp}
    ├── WacomBackpackWorkspaceStateSubsystem.{h,cpp}
    ├── WacomBackpackWorkspaceInteractionModel.{h,cpp}
    ├── WacomBackpackWorkspaceLayoutSolver.{h,cpp}
    └── widget implementations and reconcilers

Source/WacomTests/Private/
├── Run/BackpackBatchTransactionSpec.cpp
└── UI/Backpack/
    ├── BackpackWorkspaceInteractionSpec.cpp
    ├── BackpackWorkspaceLifecycleSpec.cpp
    └── BackpackWorkspaceTestAccess.{h,cpp}

Content/Wacom/UI/Backpack/
├── WBP_BackpackScreen.uasset
├── WBP_BackpackWorkspace.uasset
├── WBP_BackpackZoneRack.uasset
├── WBP_BackpackZoneRackEntry.uasset
├── WBP_BackpackDeleteConfirm.uasset
└── DA_BackpackWorkspaceStyle.uasset

DShader/Material/Card/ and DShader/Shared/
└── optional Wacom-native workspace card feedback source
```

**Structure Decision**: `UWacomBackpackScreen` 保持 CommonUI screen/coordinator；自由布局 store、interaction model、layout solver 和 list reconcile 各自独立。只有 WBP 制作需要的 Widget/Style class 放 Public；输入算法、状态和 command flow 保持 Private。Run 只增加稳定 batch contract 与 private rules，不知道 Workspace、鼠标或材质。

`FWacomBackpackWorkspaceAutomationTestView` 是 production public C++ 只读结构，由 Workspace/Screen 返回稳定观察值；`BackpackWorkspaceTestAccess` 只存在于 `WacomTests/Private`，负责调用该 view 和组织断言，不通过反射或 Blueprint 暴露内部字段。

## Validation Plan

**Compile**:

```powershell
& 'E:\UE_5.8\Engine\Build\BatchFiles\Build.bat' WacomEditor Win64 Development -Project='D:\UE_Project\5.7\Wacom\Wacom.uproject' -WaitMutex -NoHotReloadFromIDE
```

**Focused Automation**:

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -ExecCmds='Automation RunTests Wacom.Run.Backpack; Quit' -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache
```

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -ExecCmds='Automation RunTests Wacom.UI.Backpack; Quit' -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache
```

**Manual/Editor Validation**: 按 [quickstart.md](./quickstart.md) 验证 4+ 区域、20+ 卡、框选、扇形、起手 release guard、默认牌无 lift、无索引 UI、滚轮逐张释放、右键全放、跨区/同区收拢、删牌确认取消恢复、画布边界、CommonUI 关闭重开及 DreamShader/fake-3D 手感。

## Complexity Tracking

无 Constitution 违规。旧单卡拖放只在分片迁移期间短暂存在，完成 Slice D 时必须收口；若因资产制作阻塞无法删除，必须登记 `Docs/TechDebt.md` 并写明删除条件。
