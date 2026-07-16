# Tasks: 正式 Floor 1 内容设计与稳定身份冻结

**Input**: [spec.md](./spec.md), [plan.md](./plan.md), [research.md](./research.md), [data-model.md](./data-model.md), [contracts/](./contracts/), [quickstart.md](./quickstart.md)

**Tests**: 本功能是纯文档设计冻结。验证使用静态图/身份/AP 审计、跨工件一致性、`git diff --check` 和 Git/LFS 边界检查；不运行 Unreal 编译、Automation、AssetRegistry 或 PIE。

## Phase 1: Setup and workspace guard

**Purpose**: 锁定目标 worktree、分支和非目标边界。

- [x] T001 检查 branch、HEAD、git status、Git LFS 与 Unreal 进程，并把结果写入 `specs/007-formal-floor1-content-freeze/quickstart.md`
- [x] T002 读取 `AGENTS.md`、`Docs/Architecture.md`、`Docs/WacomMap.md`、`Docs/WacomRun.md`、`Docs/WacomDataAuthoring.md`、`Docs/WacomApp.md` 与上一轮 `specs/006-run-level-authoring-baseline/` 工件
- [x] T003 创建并验证 `specs/007-formal-floor1-content-freeze/spec.md` 与 `checklists/requirements.md`，确保无 clarification 或模板占位

**Checkpoint**: 规格边界明确，工作区无未归属改动，允许进入设计工件阶段。

---

## Phase 2: Foundational design decisions

**Purpose**: 先冻结共享决策，阻止各故事维护不同身份或拓扑。

- [x] T004 把三层骨架、20/21 图、结构化身份、内容槽、Camp、分支语义、蛇印阻塞和纯文档边界写入 `specs/007-formal-floor1-content-freeze/research.md`
- [x] T005 在 `specs/007-formal-floor1-content-freeze/plan.md` 声明模块所有权、零 public API/schema/asset 影响、Constitution gate 与静态验证策略
- [x] T006 在 `AGENTS.md` 和 `.specify/feature.json` 只更新托管的当前 Spec Kit feature/plan 指针

**Checkpoint**: 所有故事共享同一冻结口径，不存在需要实现者决定的规则分支。

---

## Phase 3: User Story 1 - 使用稳定的正式地图身份 (Priority: P1)

**Goal**: 提供与 Authoring/Debug 分离、未来可持久化且可审计的 Journey/Floor/Node/Edge/Card 身份注册表。

**Independent Test**: 注册表包含 1 Journey、3 Floor、20 Node、21 Edge 和 1 蛇印 CardId，全部唯一；Host PersistentId 可由 FloorId/NodeId 唯一派生。

- [x] T007 [US1] 在 `specs/007-formal-floor1-content-freeze/data-model.md` 写入 Journey/Floor 身份、20 Node 与 21 Edge 的 canonical tables
- [x] T008 [US1] 在 `specs/007-formal-floor1-content-freeze/contracts/stable-identity-contract.md` 冻结身份 scope、Host PersistentId 派生公式和可调表现字段
- [x] T009 [US1] 在 `specs/007-formal-floor1-content-freeze/quickstart.md` 增加身份唯一性、Authoring/Debug 隔离和非目标文件审计

**Checkpoint**: 下游无需使用数组下标、坐标、资产路径、Actor GUID 或独立人工 Host ID。

---

## Phase 4: User Story 2 - 按正式 Floor 1 节奏制作内容 (Priority: P2)

**Goal**: 冻结可达的双分岔图、类型配比、内容 ID、Camp 落点和 AP 节奏。

**Independent Test**: 20 节点全部可达，A/B 与 C/D 分别汇合，Key 支配 Guardian/Exit；最短路线 8–9 AP，完整探索 14–15 AP。

- [x] T010 [US2] 在 `specs/007-formal-floor1-content-freeze/data-model.md` 写入 15 个 Production 内容槽、默认 MapPosition、节点类型总数与 AP 演算
- [x] T011 [US2] 在 `specs/007-formal-floor1-content-freeze/contracts/floor1-graph-content-contract.md` 定义图、Camp、landmark、入口与内容验收合同
- [x] T012 [US2] 把三层身份、Floor 1 拓扑、内容密度、Camp 和 AP 长期事实同步到 `Docs/WacomMap.md`

**Checkpoint**: 当前 8 节点 Authoring/Debug 图不再承担正式 Floor 1 内容设计职责。

---

## Phase 5: User Story 3 - 在安全边界内规划生产实现 (Priority: P3)

**Goal**: 阻止 Debug 晋升、蛇印软锁、空壳 Floor 2/3 和未经审计的二进制迁移。

**Independent Test**: Production readiness gate 明确四个硬前置；长期 Docs 不把设计冻结描述为已交付资产。

- [x] T013 [US3] 在 `specs/007-formal-floor1-content-freeze/contracts/production-readiness-gate.md` 定义三类资产权威、蛇印软锁、Floor 2/3 和资产审计阻塞
- [x] T014 [US3] 把 Production 路径、内容 ID、Host PersistentId 与禁止 Debug 占位同步到 `Docs/WacomDataAuthoring.md`
- [x] T015 [US3] 在 `Docs/TODO.md` 完成设计冻结项，并新增任务凭证、Floor 2/3、Production 内容和正式场景后续任务
- [x] T016 [US3] 在 `Docs/Questions.md` 移除已解决的规模/拓扑/身份/入口选择问题，只保留任务凭证保留/恢复决策

**Checkpoint**: 设计冻结完成，但任何 Production DataAsset/关卡实施都会被明确的 readiness gate 阻止。

---

## Phase 6: Final static validation and handoff

**Purpose**: 证明工件一致、无运行时/资产改动并完成可审计提交。

- [x] T017 按 `specs/007-formal-floor1-content-freeze/quickstart.md` 验证节点/边计数、类型分布、可达性、Key 支配、MapPosition 与 AP
- [x] T018 运行 Spec Kit 跨工件一致性分析并修复 `spec.md`、`plan.md`、`tasks.md`、contracts 和长期 Docs 的冲突
- [x] T019 使用 `rg`、`git diff --check`、`git diff --name-only` 和 `git lfs status` 证明无模板残留、Source/Config/Content 变化或 LFS 对象
- [x] T020 将最终静态结果、跳过的 Unreal 验证和剩余生产阻塞写入 `specs/007-formal-floor1-content-freeze/quickstart.md`
- [x] T021 审阅变更、提交当前分支并确认 worktree/LFS 干净，不 merge main、不 push

---

## Dependencies & execution order

```text
Phase 1 workspace guard
  -> Phase 2 shared decisions
    -> US1 stable identities
      -> US2 graph/content/AP
        -> US3 production readiness
          -> final validation and commit
```

- US1 blocks US2 because content nodes and edges must use the frozen registry.
- US2 blocks US3 because readiness checks require the final graph and content slots.
- Long-term Docs sync occurs inside the owning story, not only in final polish.

## Parallel opportunities

- After T004–T006, T008 and T009 touch different files but both depend on T007's canonical registry.
- T011 can be drafted after T010 while T012 prepares the long-term map summary.
- T014–T016 affect different Docs and can be reviewed independently after T013 fixes the blocker terminology.
- Final validations remain sequential so quickstart records the final tree rather than an intermediate state.

## Implementation strategy

1. Complete US1 first as the minimum useful design freeze.
2. Add the exact graph/content/AP contract in US2.
3. Finish US3 before calling the design production-ready.
4. Do not start a binary asset slice from this task list; use the readiness gate to create separate follow-up specs.
