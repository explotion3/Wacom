# Tasks: 击倒分支奖励合同基线

**Input**: `spec.md`、`plan.md`、`research.md`、`data-model.md`、`contracts/`、`quickstart.md`

**Tests**: 本功能改变 Data、Battle 与 UI 公共合同，所有实现切片必须采用小型定向 Automation，并在每个 C++ checkpoint 后编译 `WacomEditor`。

## Phase 1: Setup（共享基线）

- [x] T001 确认 `codex/knockdown-branch-reward-baseline` 的 HEAD 与 merge-base 均为 `8b5fe0db30bd0b23f7c6917a40d20082f3d9ad02`，并记录 Git/LFS/Unreal 进程状态到 `specs/012-knockdown-branch-reward-baseline/quickstart.md`
- [x] T002 配置 D 盘独立本地资产补水层并确认 Git 管理内容不受影响，记录到 `specs/012-knockdown-branch-reward-baseline/quickstart.md`
- [x] T003 读取 `AGENTS.md`、`Docs/AgentIntegrationWorkflow.md`、相关长期 Docs、Spec 011 和 live 源码/测试，记录架构结论到 `specs/012-knockdown-branch-reward-baseline/research.md`
- [x] T004 创建 Spec 012 的 `spec.md`、`plan.md`、`research.md`、`data-model.md`、`contracts/`、`quickstart.md` 和 requirements checklist
- [x] T005 更新 `.specify/feature.json` 与 `AGENTS.md` 托管指针到 `specs/012-knockdown-branch-reward-baseline/`
- [x] T006 完成 Spec Kit 跨工件一致性分析与 contract-quality checklist，结果记录到 `specs/012-knockdown-branch-reward-baseline/quickstart.md`

---

## Phase 2: Foundational（数据合同与制作门禁）

**Purpose**: 先建立 Battle 与 UI 共用的唯一分支奖励查询和制作校验。

- [x] T007 [P] 为 General/FormalProduction、legacy fallback、混填错误和显式分支查询扩展 `Source/WacomTests/Private/Run/DataDefinitionValidationSpec.cpp`
- [x] T008 [P] 为现有 TrainingWarrior 二进制资产的 legacy-only 状态与统一查询扩展 `Source/WacomTests/Private/Data/TrainingWarriorContentSpec.cpp`
- [x] T009 在 `Source/WacomData/Public/Enemies/EnemyPartDefinition.h` 添加 `AidRewardCard`、`DestroyRewardCard`、deprecated legacy 字段和 `ResolveKnockdownRewardCard`
- [x] T010 在 `Source/WacomEditor/Public/Validation/EnemyPartDefinitionValidation.h` 与 `Source/WacomEditor/Private/Validation/EnemyPartDefinitionValidation.cpp` 实现 General/FormalProduction 两档验证
- [x] T011 [P] 修改 `Source/WacomEditor/Private/ContentBuilders/TrainingWarriorBuilder.cpp` 与 `Source/WacomEditor/Private/ContentBuilders/SnakeBuilder.cpp`，使未来 builder 写入两个显式字段并清空 legacy，但不执行 builder
- [x] T012 编译 `WacomEditor`，运行 `Wacom.Data.EnemyPart` 与 `Wacom.Data.Enemy.TrainingWarrior`，并将 checkpoint 结果写入 `specs/012-knockdown-branch-reward-baseline/quickstart.md`

**Checkpoint**: Data 查询、两档制作门禁与 legacy 二进制兼容独立可验证。

---

## Phase 3: User Story 2 — Battle 原子分支奖励（Priority: P1）

**Goal**: Aid/Destroy 仅授予所选分支的卡，空奖励、Withdraw、终局持久化和广播语义保持现状。

**Independent Test**: `Wacom.Battle.KnockdownReward` 加既有 Knockdown/Run 回归。

- [x] T013 [US2] 在 `Source/WacomTests/Private/Battle/KnockdownBranchRewardSpec.cpp` 添加显式 Aid/Destroy 不串卡、legacy fallback、空奖励和 Withdraw 用例
- [x] T014 [US2] 扩展 `Source/WacomBattle/Public/Session/BattleSession.h` 的 `FKnockdownChoiceOptionView` 只读奖励摘要字段
- [x] T015 [US2] 在 `Source/WacomBattle/Private/Commands/KnockdownChoiceAvailability.cpp` 使用统一数据查询构造每个选项的奖励摘要
- [x] T016 [US2] 在 `Source/WacomBattle/Private/Commands/KnockdownChoiceResolver.cpp` 使用统一数据查询完成原子分支奖励结算
- [x] T017 [US2] 编译 `WacomEditor`，运行 `Wacom.Battle.Knockdown`、`Wacom.Battle.KnockdownReward`、`Wacom.Run.BattleRewardCardsAddedToBackpack` 与 `Wacom.Run.NotificationCoalescing`，并更新 `specs/012-knockdown-branch-reward-baseline/quickstart.md`

**Checkpoint**: 规则层可独立证明分支选择、ViewData、事件和 Run 交接一致。

---

## Phase 4: User Story 3 — 被动 UI 奖励预览（Priority: P2）

**Goal**: 原生击倒面板显示 Aid/Destroy 奖励文本，连续刷新正确，且不自行推导规则。

**Independent Test**: 独立 `Wacom.UI.Battle.KnockdownChoice` Dialog spec。

- [x] T018 [P] [US3] 扩展 `Source/WacomTests/Private/UI/WacomUITestAccess.h`，仅在测试侧读取原生 Dialog 奖励文本和按钮状态
- [x] T019 [P] [US3] 新建 `Source/WacomTests/Private/UI/KnockdownChoiceDialogSpec.cpp`，覆盖初次绑定、连续 `SetContext`、空奖励和禁用状态
- [x] T020 [US3] 在 `Source/WacomApp/Public/UI/Battle/WacomKnockdownChoiceDialog.h` 添加可选奖励文本锚点和统一刷新声明
- [x] T021 [US3] 在 `Source/WacomApp/Private/UI/Battle/WacomKnockdownChoiceDialog.cpp` 实现原生 fallback 与 ViewData 驱动的奖励摘要刷新
- [x] T022 [US3] 编译 `WacomEditor`，运行 `Wacom.UI.Battle.KnockdownChoice`、`Wacom.UI.Battle.PresentationQueue.KnockdownDialogDelayedAndGuarded` 与 `Wacom.UI.Battle.PresentationPlan.Gained`，并更新 `specs/012-knockdown-branch-reward-baseline/quickstart.md`

**Checkpoint**: 原生 Dialog 的被动文本预览和既有焦点/队列流程独立通过。

---

## Phase 5: User Story 4 — Floor 1 迁移与长期事实（Priority: P3）

**Goal**: 冻结 8 个未来 CardId、路径、总资产数量和 legacy 删除门槛。

- [x] T023 [P] [US4] 在 `Docs/WacomData.md` 记录字段/统一查询及 Floor 1 八张分支奖励卡身份
- [x] T024 [P] [US4] 在 `Docs/WacomBattle.md` 记录分支结算、空奖励、Withdraw 与 ViewData 合同
- [x] T025 [P] [US4] 在 `Docs/WacomDataAuthoring.md` 记录两档验证、builder 迁移、未来路径和“38 核心 + 8 分支奖励”制作数量
- [x] T026 [P] [US4] 在 `Docs/WacomApp.md` 记录被动 Dialog 奖励摘要和刷新/焦点边界
- [x] T027 [P] [US4] 在 `Docs/WacomRun.md` 记录 `FBattleGainedCard.SourceChoice` 已足够表达来源且 Run/Save 合同不变
- [x] T028 [P] [US4] 在 `Docs/Architecture.md` 记录 Data 查询、Battle ViewData、App 被动展示和 Editor 校验的模块边界
- [x] T029 [P] [US4] 更新 `Docs/TODO.md` 与 `Docs/Roadmap.md`，关闭分支奖励合同基线并保留八张卡设计/资产制作任务
- [x] T030 [P] [US4] 更新 `Docs/Questions.md` 与 `Docs/TechDebt.md`，关闭奖励粒度选择并记录其它分支效果与 legacy 删除门槛

---

## Phase 6: Polish & Cross-Cutting Validation

- [x] T031 对 TrainingWarrior Part 与 BrokenCleave 执行前后 SHA-256、只读 AssetRegistry/failed-load 和 Blueprint compile 审计，结果写入 `specs/012-knockdown-branch-reward-baseline/quickstart.md`
- [x] T032 运行最终默认 Unity `WacomEditor Win64 Development` 编译和全部计划聚焦 Automation，所有 Unreal 命令带 `-NoDreamShaderEditorBridge`，结果写入 `specs/012-knockdown-branch-reward-baseline/quickstart.md`
- [x] T033 运行 Spec Kit 最终一致性、`git diff --check`、禁止范围、Git/LFS fsck 和二进制零改动审计，结果写入 `specs/012-knockdown-branch-reward-baseline/quickstart.md`
- [x] T034 更新 `specs/012-knockdown-branch-reward-baseline/tasks.md`、checklists 和 `quickstart.md` 的最终完成状态、跳过 PIE 理由与剩余风险
- [x] T035 提交 `codex/knockdown-branch-reward-baseline`，确认 worktree/LFS 干净且不 merge main、不 push

## Dependencies & Execution Order

- Phase 1 完成后进入 Phase 2；T009/T010 必须在任何 Battle/UI 实现前完成。
- T007、T008 是 T009/T010 的测试先行任务；T011 只修改未来写入源码，不授权 builder 执行。
- T012 通过后才能执行 T013–T017；T017 通过后才能执行 T018–T022。
- Phase 5 可在 T022 后按文件并行，但主会话负责统一口径审阅。
- T031–T034 必须在提交 T035 前完成。

## Parallel Opportunities

- 标为 `[P]` 的测试草稿或不同长期文档在依赖满足后可独立处理；本轮由主会话顺序执行以保持单一工作区一致性。
- T015 与 T016 修改不同实现文件，但共享查询和 ViewData，按 T014 → T015 → T016 顺序更易验证。

## Implementation Strategy

1. 先冻结唯一 Data 查询和制作门禁，保持旧二进制资产可读。
2. 在既有击倒事务内替换奖励来源并补小型回归，不建立第二条结算路径。
3. 最后让 App 被动显示 ViewData，保持 Modal/focus/Back 流程不变。
4. 长期 Docs、只读资产审计与最终全套验证完成后才提交。
