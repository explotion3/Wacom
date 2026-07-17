# Tasks: Journey 成功结算与终局交接基线

**Input**: `specs/010-journey-success-settlement-baseline/`

**Prerequisites**: `spec.md`, `plan.md`, `research.md`, `data-model.md`, `contracts/`, `quickstart.md`

**Tests**: 规则、SaveGame、Data validation、CommonUI/App flow 均要求小型定向 Automation；每个可编译 checkpoint 运行默认 Unity WacomEditor 编译。

## Phase 1: Setup and Spec alignment

- [x] T001 读取 `AGENTS.md`、长期 Docs、Spec 009 与 live Journey/Run/Save/App 源码并记录基线到 `specs/010-journey-success-settlement-baseline/quickstart.md`
- [x] T002 生成 `specs/010-journey-success-settlement-baseline/spec.md`、`plan.md`、`research.md`、`data-model.md`、`contracts/`、`quickstart.md` 与 `checklists/`
- [x] T003 更新 `.specify/feature.json` 与 `AGENTS.md` 托管指针到 Spec 010
- [x] T004 运行 Spec Kit 跨工件一致性分析并修正 `specs/010-journey-success-settlement-baseline/` 内 Critical/High/Medium 问题

---

## Phase 2: User Story 1 - Static terminal authoring contract (Priority: P1)

**Goal**: Journey 可声明唯一成功终局；Editor/Runtime 对已配置终局执行相同静态约束，legacy 缺失终局仍可运行。

**Independent Test**: `Wacom.Data.Map.Validation`

- [x] T005 [P] [US1] 在 `Source/WacomTests/Private/Data/JourneySuccessTerminalValidationSpec.cpp` 添加合法、缺失 warning 与 7 项非法终局测试
- [x] T006 [US1] 在 `Source/WacomData/Public/Map/WacomJourneyDefinition.h` 增加 `DisplayName` 和 `SuccessTerminalNode` 公共静态合同
- [x] T007 [US1] 在 `Source/WacomEditor/Private/Validation/WacomMapDefinitionValidation.cpp` 实现终局静态校验与 legacy warning
- [x] T008 [US1] 在 `Source/WacomRun/Private/RunSession.cpp` 为已配置非法终局增加初始化原子拒绝，未配置保持兼容
- [x] T009 [US1] 编译 WacomEditor 并运行 `Wacom.Data.Map.Validation`，将命令、结果和当前 diff hash 写入 `specs/010-journey-success-settlement-baseline/quickstart.md`

**Checkpoint**: 静态终局身份可被后续 Run transaction 安全消费。

---

## Phase 3: User Story 2 - Atomic Journey success settlement (Priority: P1)

**Goal**: 终局 Guardian 有效胜利原子生成 Outcome、摘要和最后事件；成功后玩法写入全部拒绝。

**Independent Test**: `Wacom.Run.JourneyCompletion` + affected Encounter/Result/notification tests

- [x] T010 [P] [US2] 在 `Source/WacomTests/Private/Run/JourneyCompletionSpec.cpp` 添加成功、优先级、反例、幂等、事件顺序和终态写入拒绝测试
- [x] T011 [US2] 在 `Source/WacomRun/Public/RunOutcomeTypes.h` 与 `Source/WacomRun/Private/RunOutcomeTypes.cpp` 定义 `ERunOutcome`、`FRunCompletionSummary` 及合法性
- [x] T012 [US2] 在 `Source/WacomRun/Public/RunState.h`、`Source/WacomRun/Public/Exploration/RunExplorationTypes.h` 和 snapshot builder 中以 Outcome/摘要替代运行时活动布尔并投影成功事件
- [x] T013 [US2] 在 `Source/WacomRun/Private/RunSession.cpp` 将 terminal Encounter victory、节点完成、摘要、Outcome 与末尾事件纳入同一 working-state 事务
- [x] T014 [US2] 在 `Source/WacomRun/Private/` 各公共写路径入口统一拒绝 `Succeeded` 且保证零修改、零广播
- [x] T015 [US2] 编译 WacomEditor 并运行 JourneyCompletion、Encounter、Result、notification 定向测试，将结果和 diff hash 写入 quickstart

**Checkpoint**: Run 规则层可在无 UI/资产条件下完整表达并冻结成功。

---

## Phase 4: User Story 3 - SaveGame v5 (Priority: P2)

**Goal**: Outcome/成功摘要有稳定磁盘语义，旧档连续迁移，终态档不能伪恢复为活动 Run。

**Independent Test**: `Wacom.Run.Save`

- [x] T016 [P] [US3] 在 `Source/WacomTests/Private/Run/SaveGameOutcomeSpec.cpp` 添加 v0→v5、v4 active/inactive、成功 roundtrip、非法摘要和终态拒绝测试
- [x] T017 [US3] 在 `Source/WacomRun/Public/WacomSaveGame.h` 增加 v5 Outcome 与独立摘要 schema，并保留 v4 legacy source
- [x] T018 [US3] 在 `Source/WacomRun/Private/WacomSaveGame.cpp` 实现 v4→v5 migration 和 schema validation
- [x] T019 [US3] 在 `Source/WacomRun/Private/Save/WacomRunSaveGameSerializer.cpp` 实现确定性转换、成功摘要保存与终态 apply 原子拒绝
- [x] T020 [US3] 编译 WacomEditor 并运行 `Wacom.Run.Save`，将结果和 diff hash 写入 quickstart

**Checkpoint**: v5 持久语义完整，但实际 Save/Continue 开关保持关闭。

---

## Phase 5: User Story 4 - Journey summary and main-menu handoff (Priority: P2)

**Goal**: 成功事件在镜头返回后进入被动总结页，确认/Back/push failure 安全且仅一次返回主菜单。

**Independent Test**: `Wacom.UI.JourneySummary` + GameMode handoff/return staging/input-focus

- [x] T021 [P] [US4] 在 `Source/WacomTests/Private/UI/JourneySummaryScreenSpec.cpp` 添加 ViewData、fallback tree、focus/back/单次 intent 测试
- [x] T022 [P] [US4] 在 `Source/WacomTests/Private/UI/JourneySummaryHandoffSpec.cpp` 添加事件消费、staging、不恢复 hand/toast、push fallback 和单次 travel 测试
- [x] T023 [US4] 在 `Source/WacomApp/Public/WacomGameFlowTypes.h` 增加 `JourneySummary` flow state
- [x] T024 [US4] 在 `Source/WacomApp/Public/UI/Menus/WacomJourneySummaryScreen.h` 与 `Source/WacomApp/Private/UI/Menus/WacomJourneySummaryScreen.cpp` 实现被动 ViewData、原生 fallback、Blueprint hook 和生命周期
- [x] T025 [US4] 在 `Source/WacomApp/Public/GameFramework/WacomGameMode.h` 与 `Source/WacomApp/Private/GameFramework/WacomGameMode.cpp` 接入 success event、Return-to-Run barrier、CommonUI push 与次帧主菜单 handoff
- [x] T026 [US4] 编译 WacomEditor 并运行 JourneySummary、GameMode handoff、Battle return staging、输入/焦点定向测试，将结果和 diff hash 写入 quickstart

**Checkpoint**: 自动化环境中可完整验证规则事件到 App travel 的终局交接。

---

## Phase 6: Docs, final validation and delivery

- [x] T027 [P] 更新 `Docs/WacomRun.md`、`Docs/WacomMap.md`、`Docs/WacomDataAuthoring.md`、`Docs/WacomApp.md` 与 `Docs/Architecture.md` 的长期合同
- [x] T028 [P] 更新 `Docs/TODO.md` 和 `Docs/Questions.md`，关闭 success blocker 并保留失败总结后续
- [x] T029 运行默认 Unity WacomEditor 编译及全部受影响定向测试，把完整日志摘要与最终 diff hash 写入 quickstart
- [x] T030 运行现有 Journey 资产只读 AssetRegistry/failed-load 审计，确认零二进制写入并记录 Floor 3 PIE 跳过原因
- [x] T031 运行 Spec Kit 最终一致性、身份/边界 `rg`、`git diff --check`、Git/LFS 和 changed-file audit
- [x] T032 按 `Docs/AgentIntegrationWorkflow.md` 创建独立提交并确认 target worktree/LFS 干净，不 merge main、不 push

## Dependencies & execution order

```text
Phase 1 -> US1 static terminal -> US2 atomic runtime -> US3 Save v5 -> US4 App UI -> final docs/validation
```

- US2 依赖 US1 的 terminal handle。
- US3 依赖 US2 的 Outcome/summary。
- US4 依赖 US2 的 event/summary；为保持 checkpoint 清晰，在 US3 后实现。
- 每个 story 的测试先于或与最小 production contract 同步落地，编译/定向验证通过后才进入下一 story。

## Parallel opportunities

- 仅不同文件的 test skeleton 与 production contract 可标 `[P]`；主会话仍按 checkpoint 串行集成。
- Docs 可在最终行为稳定后并行阅读，但实际 patch 统一由主会话完成，避免术语漂移。

## Implementation strategy

1. 先冻结可验证的 static handle。
2. 再让 Run 生成唯一权威 outcome/summary/event。
3. 然后冻结磁盘语义。
4. 最后让 App 被动消费事件并 travel。
5. 每个 checkpoint 留下可编译、可回归的提交前状态；不机械执行未验证的后续任务。
