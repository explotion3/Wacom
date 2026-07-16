# Implementation Plan: 背包真实卡牌牌堆与即时携带

**Branch**: `codex/backpack-workspace` | **Date**: 2026-07-16 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `specs/006-backpack-real-card-piles/spec.md`

## Summary

以 `WacomApp` 的统一 Backpack Workspace 为唯一输入和卡牌表现 owner，删除最多三张的缩略牌预览，令备战、特殊和负重牌堆在折叠与展开状态都使用完整卡牌视图。工作台增加独立 CarryLayer：携带开始/结束时重挂载卡牌一次，鼠标移动只更新一个视觉锚点，从根本上删除指数插值和全工作台逐帧刷新。完整卡面改用现有 `WBP_FPCardView`，通过 Backpack 私有表现控制器和明确的实时渲染开关限制动态 Retainer 数量。

## Wacom Domain Context

**Primary Domain**: UI-App

**Required Docs Read**:
- [x] `AGENTS.md`
- [x] `Docs/Architecture.md`
- [x] `Docs/WacomUI.md`
- [x] `Docs/UI_Backpack_WBP_Binding.md`
- [x] `Docs/WacomUIFoundation.md`

**Docs To Update**:
- [ ] `Docs/WacomUI.md`
- [ ] `Docs/UI_Backpack_WBP_Binding.md`
- [ ] `Docs/WacomUIFoundation.md`
- [ ] `Docs/TODO.md` / `Docs/TechDebt.md` 仅在真实剩余项存在时

**Owning Module(s)**: `WacomApp`、`WacomEditor`、`WacomTests`

**Dependency Direction**:

```text
WacomCore <- WacomData <- WacomBattle <- WacomRun <- WacomApp

WacomEditor -> WacomApp/runtime modules for editor-only asset generation
WacomTests  -> WacomApp/runtime/editor validation as test harness
```

## Technical Context

**Language/Engine**: C++, Unreal Engine 5.8

**Primary UE Systems**: UMG/Slate、CanvasPanel、CommonUI lifecycle、RetainerBox、Automation Tests、Widget Blueprint builder

**Storage/State**: 权威状态仍为 `URunSession` Snapshot；牌堆位置、ZOrder、展开项和通量自由布局仍为现有 Run-scoped transient Workspace State Store。

**Runtime Contracts**: Backpack Scene/ViewData、Card ViewKey、Pile Layout、Carry Presentation、Card Presentation Mode、现有 Screen command flow。

**Testing**: `Wacom.UI.Backpack`、`Wacom.Run.Backpack`、相关 FirstPersonCardView 测试、Builder 双运行、Blueprint 编译、多分辨率 PIE 与 Slate/Unit 性能采样。

**Target Platform**: Windows editor / packaged Windows build

**Performance Goals**: 鼠标锚点在下一 Slate 帧内抵达最新局部位置且误差不超过 1px；普通鼠标移动不重算扇形、不刷新非携带卡；任意时刻最多一个 Backpack 卡面持续动态重绘。

**Constraints**: Passive Widget；不改变 Run/Battle/Data/SaveGame/DreamShader；固定 `220×320` 卡面逻辑尺寸与 `0.75` 卡面缩放；最多约 21 张无滚动、无动态缩小。

**Scale/Scope**: Backpack Workspace、Scene/Reconciler、Layout Solver、ZonePile、DeckCard、FirstPersonCardView 渲染策略、Backpack Builder、Focused UI tests 和长期 UI 文档。

**Blueprint Exposure Strategy**: 仅正式 WBP 所需类/BindWidget/样式字段保持反射；布局算法、ViewKey、Carry 计数和表现控制器保持 `WacomApp/Private`。FirstPersonCardView 只增加一个非规则性的 C++ 实时渲染策略入口。

**Data/GameplayTag Impact**: 无。

**Save/Load Impact**: 无 schema 或迁移；继续使用现有 Run-scoped transient store。

**UI/App Lifecycle Impact**: Workspace Activate/Deactivate、B 关闭和 Escape 取消统一关闭实时卡面、释放鼠标捕获、清空 CarryLayer 并恢复静态父层；Screen 的 Snapshot/command 生命周期不变。

## Constitution Check

*GATE: Passed before research and re-checked after design.*

- **Docs and AGENTS Are the Rule Truth**: 已读取 UI、绑定、Foundation 和架构文档；本 Spec 只替代其中旧缩略牌/静态卡面事实，完成后回写长期文档。
- **Wacom Module Boundaries Are Mandatory**: 权威规则不出 `WacomRun`；本轮生产改动归 `WacomApp`，Editor 只生成资产，Tests 只做 harness。
- **Domain Rules Before Presentation**: Scene/ViewData 不拥有卡牌归属；所有移动/销毁继续经 Screen command flow 原子提交。
- **Data, GameplayTags, and Authoring Stay Explicit**: 无 DataAsset schema 或 GameplayTag；只改变 WBP 制作合同。
- **Reusable Systems Over One-Off Work**: CarryLayer、实时渲染策略和卡牌表现模式均设计为可复用 UI 能力，不新增页面专用 Tick 或临时单例。
- **Validation Is Part of the Slice**: 自动化、Builder/Blueprint、编译、PIE 和性能采样均列入任务。

## Design Decisions

### Scene identity and reconciliation

- 私有卡牌键保持 `InstanceId + OwnerInstanceId + PhysicalZone + ReuseRole`，同一实体可安全拥有物理、投影或特殊主卡视图。
- Scene 为通量卡和每个牌堆都生成完整卡牌 ViewData，不再只为当前展开牌堆生成卡面。
- Reconciler 同时扫描 StaticCardLayer 与 CarryLayer；正在携带的 Widget 不被错误删除或复制，Snapshot 只定向更新其 ViewData。
- 交互可用性、只读身份和渲染模式分开表达，折叠实体卡不因禁止输入而改变透明度。

### Pile layout and formal WBP

- `WBP_BackpackZonePile` 只包含 frame/header/count/status/drag/drop feedback，不包含 PreviewHost。
- Solver 为每堆返回稳定标题锚点、标题矩形、框体矩形、展开方向和完整卡牌布局。
- 折叠牌堆无旋转，前卡位于最右/最高 Z；露出默认 16px，可在 10–24px 适配。
- 展开牌堆保持固定卡面，露出 32–72px 并轻微扇转；根据标题锚点左右空间选择方向并夹紧最终视觉边界。
- 持久化、拖动和标题防覆盖仍以标题锚点/标题矩形为准；通量整理避让当前完整视觉矩形。

### Card presentation

- `WBP_WacomDeckCardWidget` 承载 `WBP_FPCardView`，删除 `WBP_BackpackCardView` 正式生成路径。
- Backpack 私有表现控制器把 selected/hover/current/read-only/rejected 状态映射为 FirstPersonCardView 的反馈和实时策略；不接入 Battle Slot 或 transition hint。
- 折叠卡与非当前携带卡使用静态渲染；展开 hover 或当前最前携带卡中仅一个启用实时 Fake3D/视差/接触阴影/表面效果。
- FirstPersonCardView 提供显式实时渲染策略：静态时只因内容/状态变化请求重绘，实时状态允许动态材质更新；状态退出时立即复位。

### Carry performance path

- Workspace 正式树包含 PileFrameLayer、StaticCardLayer、MarqueeLayer、CarryLayer；CarryLayer 高 Z 且不裁剪子项。
- 开始携带时重挂载现有卡牌并构建一次局部扇形；鼠标移动只写 CarryLayer slot position。
- 滚轮、当前卡变化、部分释放、样式/几何变化才重算局部扇形。
- 删除 `DisplayedCarryPointer`、`bHasDisplayedCarryPointer`、携带插值 ActiveTimer 和 `PointerFollowSeconds` 的生产路径。
- 保留鼠标捕获与全局光标采样；采样回调只更新锚点，不广播全量 InteractionChanged 或 Scene refresh。

## Generated Design Artifacts

- [research.md](./research.md): 原型根因、牌堆布局、卡面复用和 CarryLayer 决策。
- [data-model.md](./data-model.md): Scene、ViewKey、Pile Layout、Carry 与 Presentation 状态。
- [contracts/backpack-real-card-pile-contract.md](./contracts/backpack-real-card-pile-contract.md): 正式 WBP、输入、渲染、性能与生命周期合同。
- [quickstart.md](./quickstart.md): 编译、Builder、自动化、PIE 和性能验证流程。

## Project Structure

```text
specs/006-backpack-real-card-piles/
├── spec.md
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── checklists/requirements.md
├── contracts/backpack-real-card-pile-contract.md
└── tasks.md

Source/WacomApp/{Public,Private}/UI/{Backpack,Card}/
Source/WacomEditor/Private/ContentBuilders/BackpackUIBuilder.cpp
Source/WacomTests/Private/UI/Backpack/
Content/Wacom/UI/{Backpack,Card}/
Docs/{WacomUI.md,WacomUIFoundation.md,UI_Backpack_WBP_Binding.md}
```

**Structure Decision**: 不新建模块。真实牌堆、布局、Scene 和表现控制器都留在 `WacomApp`；公开头只保留 UMG 绑定和可复用 CardView 渲染入口。测试继续拆成小型 Backpack specs。

## Implementation Phases

1. **Contracts and foundations**: 先锁定无缩略牌、完整 Scene 身份、固定尺寸布局、Carry 锚点和 Retainer 上限。
2. **Real pile scene/layout**: 移除 Preview 链路，令所有牌堆生成真实卡并完成折叠/展开动态框体。
3. **Card presentation**: 迁移 DeckCard 到 FPCardView，加入 Backpack 私有表现控制器与实时渲染策略。
4. **CarryLayer performance**: 拆层、重挂载和增量锚点更新，删除插值与全量刷新。
5. **Builder/assets/docs/validation**: 生成正式 WBP、清理旧资产合同、回写 Docs，完成自动化与 PIE。

## Validation Plan

**Compile**:

```powershell
& 'E:\UE_5.8\Engine\Build\BatchFiles\Build.bat' WacomEditor Win64 Development -Project='C:\Users\ahhh\.codex\worktrees\0b47\Wacom\Wacom.uproject' -WaitMutex -NoHotReloadFromIDE -DisableUnity
```

**Focused Automation**: `Wacom.UI.Backpack`、`Wacom.Run.Backpack` 与相关 FirstPersonCardView 前缀。

**Asset Build**: 连续执行两次 `WacomBuildBackpackUI` 并编译必要 Blueprint。

**Manual/Editor Validation**: 见 [quickstart.md](./quickstart.md) 的四分辨率、0/1/3/7/15/21 cards、跨区事务、CommonUI 生命周期和 1/21 卡携带性能矩阵。

## Complexity Tracking

无 Constitution violation。FirstPersonCardView 的实时渲染开关属于通用表现能力；它不接触 Battle/Run 规则。若实现发现 Retainer 无法在静态缓存与动态材质间安全切换，必须记录验证证据并更新 `research.md`，不得退回所有卡逐帧重绘。
