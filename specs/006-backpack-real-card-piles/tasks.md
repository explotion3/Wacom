# Tasks: 背包真实卡牌牌堆与即时携带

**Input**: Design documents from `specs/006-backpack-real-card-piles/`

**Prerequisites**: `spec.md`, `plan.md`, `research.md`, `data-model.md`, `contracts/`, `quickstart.md`

**Tests**: 本功能要求自动化锁定 Scene 身份、布局、携带性能结构、Retainer 上限和既有事务回归；表现手感由 PIE/Insights 补充。

## Phase 1: Setup and Baseline

- [X] T001 记录当前 branch、HEAD、`git status`，保护 `canvas/` 用户改动并审计 `Source/WacomApp/Private/UI/Backpack/` 与 `Source/WacomEditor/Private/ContentBuilders/BackpackUIBuilder.cpp` 的 live baseline。
- [X] T002 读取 `Docs/WacomUI.md`、`Docs/UI_Backpack_WBP_Binding.md`、`Docs/WacomUIFoundation.md` 和 `Docs/Architecture.md`，在 `specs/006-backpack-real-card-piles/plan.md` 固定模块与生命周期边界。
- [X] T003 运行或记录实现前 `WacomEditor`、`Wacom.UI.Backpack`、`Wacom.Run.Backpack` baseline 到 `specs/006-backpack-real-card-piles/quickstart.md`。

## Phase 2: Foundational Contracts

- [X] T004 [P] 在 `Source/WacomTests/Private/UI/Backpack/BackpackWorkspaceLayoutSpec.cpp` 增加 0/1/3/15/21 张折叠/展开、固定尺寸、左右展开和 frame bounds 合同。
- [X] T005 [P] 在 `Source/WacomTests/Private/UI/Backpack/BackpackRealPileSceneSpec.cpp` 增加 Battle 实体/投影、Special 主卡/内容、Burden 全卡与 ViewKey 身份合同。
- [X] T006 [P] 在 `Source/WacomTests/Private/UI/Backpack/BackpackCarryLayerSpec.cpp` 增加 1/7/15/21 张锚点、扇形重算计数和非携带卡不刷新合同。
- [X] T007 [P] 在 `Source/WacomTests/Private/UI/Backpack/BackpackCardPresentationSpec.cpp` 增加静态/实时渲染、单活动卡和状态清理合同。
- [X] T008 在 `Source/WacomTests/Private/UI/Backpack/BackpackWorkspaceAssetSpec.cpp` 增加正式 ZonePile、Workspace 分层、FPCardView、无 Preview 链路的资产合同。

## Phase 3: User Story 1 — 直接看见牌堆中的真实卡牌 (Priority: P1)

**Independent Test**: 0/1/3/15/21 张牌堆在折叠和展开状态都显示正确全卡，固定尺寸、框体、顺序与只读身份通过自动化。

- [X] T009 [US1] 在 Backpack Workspace Scene 类型中拆分交互模式、只读身份、牌堆归属与完整卡牌 ViewData，并移除 Preview ViewData。
- [X] T010 [US1] 在 `Source/WacomApp/Private/UI/Backpack/WacomBackpackDeckCardListReconciler.{h,cpp}` 收敛可跨层复用的完整 Card ViewKey 与角色身份。
- [X] T011 [US1] 在 `Source/WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceReconciler.{h,cpp}` 令所有牌堆无论折叠/展开都生成完整卡牌 ViewData，并正确生成投影、Special owner/content 与 Burden 项。
- [X] T012 [US1] 在 `Source/WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceLayoutSolver.{h,cpp}` 实现折叠全卡布局、展开方向、动态 frame/header bounds 和固定卡面尺寸。
- [X] T013 [US1] 在 `Source/WacomApp/Public/UI/Backpack/WacomBackpackZonePileWidget.h` 与 `Private/UI/Backpack/WacomBackpackZonePileWidget.cpp` 删除 PreviewHost/preview 生成，应用 resolved frame/header/status 并保持被动意图。
- [X] T014 [US1] 删除 `Source/WacomApp/Public/UI/Backpack/WacomBackpackPilePreviewWidget.h` 与 `Private/UI/Backpack/WacomBackpackPilePreviewWidget.cpp`，移除所有 `PilePreviewWidgetClass` 生产引用。
- [X] T015 [US1] 在 `Source/WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceWidget.cpp` 应用所有牌堆卡的折叠/展开布局、点击展开与折叠卡非交互正常视觉。
- [X] T016 [US1] 在 `Source/WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceWidget.cpp::EnsureFallbackTree` 建立 PileFrame/StaticCard/Marquee/Carry 分层 fallback 树。
- [X] T017 [US1] 编译 `WacomEditor` 并运行真实牌堆 Scene/Layout focused tests，修复所有 US1 回归。

## Phase 4: User Story 2 — 多卡携带立即跟随鼠标 (Priority: P1)

**Independent Test**: 1/7/15/21 张携带锚点在下一帧到达最新鼠标位置，普通指针移动不重算扇形、不更新非携带卡。

- [X] T018 [US2] 在 `Source/WacomApp/Public/UI/Backpack/WacomBackpackWorkspaceWidget.h` 增加静态层/携带层绑定、增量锚点与局部扇形状态，删除插值字段和接口。
- [X] T019 [US2] 在 `Source/WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceWidget.cpp` 实现携带开始/结束的定向重挂载、跨层 Widget 复用和 CarryLayer 单锚点。
- [X] T020 [US2] 在 `Source/WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceWidget.cpp` 把鼠标移动路径收敛为只更新 CarryLayer slot，删除 `PointerFollowSeconds` 插值 ActiveTimer 和逐帧全量 `RefreshInteractionPresentation()`。
- [X] T021 [US2] 在 `Source/WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceWidget.cpp` 仅在开始、滚轮、当前索引、卡牌数量、样式或有效几何变化时重算局部扇形。
- [X] T022 [US2] 在 `Source/WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceReconciler.{h,cpp}` 跨 StaticCardLayer/CarryLayer 查找现有 Widget并保护携带实例。
- [X] T023 [US2] 通过 `UWacomBackpackWorkspaceWidget` automation view 与 `BackpackScreenTestAccess` 暴露非反射锚点、重算计数和静态更新计数。
- [X] T024 [US2] 编译 `WacomEditor` 并运行 CarryLayer、既有 selection/move/delete/lifecycle specs，修复所有 US2 回归。

## Phase 5: User Story 3 — 高质量卡面反馈且控制开销 (Priority: P2)

**Independent Test**: 折叠/静置卡静态重绘，Hover 或当前携带卡最多一个实时卡面，退出交互后全部复位。

- [X] T025 [US3] 在 `Source/WacomApp/Public/UI/Card/WacomFirstPersonCardViewWidget.h` 与 `Private/UI/Card/WacomFirstPersonCardViewWidget.cpp` 增加可复用实时渲染策略和最终静态重绘入口。
- [X] T026 [US3] 在 `Source/WacomApp/Private/UI/Backpack/WacomBackpackCardPresentationController.{h,cpp}` 实现 Backpack 私有 hover/current/read-only/rejected 到卡面表现的映射。
- [X] T027 [US3] 在 `Source/WacomApp/Public/UI/Backpack/WacomDeckCardWidget.h` 与 `Private/UI/Backpack/WacomDeckCardWidget.cpp` 把卡面 Host 迁移到 `UWacomFirstPersonCardViewWidget` 并分离交互/只读/透明度语义。
- [X] T028 [US3] 在 `Source/WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceWidget.cpp` 保证 Hover 与 Carry current 互斥、最多一个实时卡面并在取消/关闭时复位。
- [X] T029 [US3] 编译 `WacomEditor` 并运行 CardPresentation、FirstPersonCardView 与 Backpack lifecycle focused tests。

## Phase 6: Formal Assets, Docs, and Validation

- [X] T030 在 `Source/WacomEditor/Private/ContentBuilders/BackpackUIBuilder.cpp` 生成正式 `WBP_BackpackZonePile`、Workspace 四层树和 DeckCard 内 `WBP_FPCardView`，移除 Preview 与 `WBP_BackpackCardView` 生成路径。
- [X] T031 在 `Source/WacomTests/Private/UI/Backpack/BackpackWorkspaceAssetSpec.cpp` 完成正式 WBP、单 Retainer、无 Preview 和 required binding 合同。
- [X] T032 连续运行两次 `WacomBuildBackpackUI`，编译所有相关 Blueprint 并检查关键资产引用稳定。
- [X] T033 [P] 更新 `Docs/UI_Backpack_WBP_Binding.md`：真实折叠/展开卡、正式 ZonePile、Workspace 分层、FPCardView 与 CarryLayer 合同。
- [X] T034 [P] 更新 `Docs/WacomUI.md` 与 `Docs/WacomUIFoundation.md`：Scene 身份、单输入 owner、实时渲染上限和 CommonUI 清理语义。
- [X] T035 [P] 仅在存在真实未解决项时更新 `Docs/TODO.md` / `Docs/TechDebt.md`，不得把已完成的缩略牌或插值路径保留为现状。
- [X] T036 运行 `WacomEditor -DisableUnity`、`Wacom.UI.Backpack`、`Wacom.Run.Backpack` 和相关 FirstPersonCardView tests，修复所有新增失败。
- [ ] T037 按 `specs/006-backpack-real-card-piles/quickstart.md` 完成四分辨率、0/1/3/7/15/21 cards、跨区、拒绝、销毁、ESC/B 与同 Run 重开 PIE。
- [ ] T038 使用 `stat unit`、`stat slate` 或 Unreal Insights 对比 1/21 张携带，记录实际结果到 `specs/006-backpack-real-card-piles/quickstart.md`。
- [X] T039 运行 `git diff --check`、残留依赖 `rg` 和最终 `git status`；生产代码仅保留 Builder 对 `PreviewHost` 不存在性的迁移断言，未恢复旧 Preview/插值链路，并保留既有 Cowart 及其他非本轮改动。

## Dependencies & Execution Order

```text
Setup -> failing contracts -> Scene identity -> pile layouts -> CarryLayer
      -> FPCardView presentation -> Builder/assets -> automation/PIE/docs
```

- US1 与 US2 都是 P1；US2 依赖 US1 的全卡 Scene 和跨层身份。
- US3 依赖 US1 的全卡 Widget，但其 FirstPersonCardView 渲染策略可在 CarryLayer 后独立接入。
- Builder 集中在单文件，所有生产资产生成必须串行。

## Parallel Opportunities

- T004–T008 位于不同测试文件，可并行准备。
- T033–T035 位于不同长期文档，可并行。
- Workspace、Reconciler 和 Builder 的生产改动存在共享文件依赖，必须由同一 owner 串行集成。

## Implementation Strategy

1. 先让 Scene/Layout 自动化证明所有牌堆使用真实卡。
2. 再以 CarryLayer 完成可独立验收的即时跟随路径。
3. 最后接入 FPCardView 动态反馈，避免视觉效果干扰性能根因定位。
4. 资产与长期文档只在运行时合同稳定后生成/回写。

## Format Validation

- Total tasks: 39.
- US1: 9 tasks；US2: 7 tasks；US3: 5 tasks。
- All tasks use `- [ ] T###` format, story tasks include `[US#]`, and each task names an exact file or validation artifact.
- Suggested MVP: T001–T024（真实牌堆 + 即时携带，不含最终动态卡面制作）。
