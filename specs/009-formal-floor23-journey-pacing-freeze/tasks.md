# Tasks: 正式 Floor 2/3 图与 Journey 总节奏冻结

**Input**: [spec.md](./spec.md), [plan.md](./plan.md), [research.md](./research.md), [data-model.md](./data-model.md), [contracts/](./contracts/), [quickstart.md](./quickstart.md)

**Tests**: 本功能是纯文档设计冻结。验证使用静态图/身份/坐标/AP 审计、Spec Kit 跨工件一致性、`rg`、`git diff --check` 和 Git/LFS 边界检查；不运行 Unreal 编译、Automation、AssetRegistry、Blueprint 或 PIE。

## Phase 1: Setup and workspace guard

**Purpose**: 锁定目标 worktree、基线、现有规则与零资产边界。

- [x] T001 检查 branch、HEAD、git status、Git LFS 与 Unreal 进程，并把结果写入 `specs/009-formal-floor23-journey-pacing-freeze/quickstart.md`
- [x] T002 读取 `AGENTS.md`、`Docs/AgentIntegrationWorkflow.md`、`Docs/Architecture.md`、`Docs/WacomMap.md`、`Docs/WacomRun.md`、`Docs/WacomDataAuthoring.md`、`Docs/TODO.md`、`Docs/Questions.md`、Spec 007/008 与当前 map/credential/settlement live 源码
- [x] T003 创建 `specs/009-formal-floor23-journey-pacing-freeze/spec.md` 与 `checklists/requirements.md`，确认用户批准的图、门槛、Camp、终局和节奏没有 clarification

**Checkpoint**: 工作区与 LFS 干净，当前 runtime 缺少 Journey success 的事实已确认，允许进入设计工件。

---

## Phase 2: Foundational design decisions

**Purpose**: 先冻结两层共享的身份作用域、双分岔结构、内容命名和 Production 边界。

- [x] T004 在 `research.md` 记录 20/21 双分岔、结构 ID 作用域、蜕印 Card/Credential、Floor 3 无 Shop/Entrance、CoreBoon、Camp、AP/天数和终局 blocker 的决策与替代方案
- [x] T005 在 `plan.md` 声明零 public API/schema/asset 影响、未来模块所有权、Constitution gate、静态验证和 Unreal 跳过理由
- [x] T006 更新 `.specify/feature.json` 与 `AGENTS.md` 托管指针，并通过 `setup-plan.ps1 -Json` 验证 Spec 009 路径

**Checkpoint**: 两层共享同一身份、图、Camp、节奏和 readiness 口径，不存在实现者自行选择的规则分支。

---

## Phase 3: User Story 1 - 使用完整的正式三层图身份 (Priority: P1)

**Goal**: 为 Floor 2/3 提供完整、Floor-scoped、可静态审计的 Node/Edge/内容注册表。

**Independent Test**: 两层各 20 Node/21 Edge、全部可达、MapPosition 合法、31 个内容 ID 非 Debug，跨层 key 携带 FloorId。

- [x] T007 [US1] 在 `data-model.md` 写入 Floor 2/3 canonical Node/Edge 表、默认 MapPosition、类型计数、内容 registry 与稳定/可调字段矩阵
- [x] T008 [US1] 在 `contracts/stable-identity-extension.md` 冻结两层身份作用域、蜕印双身份和 `<FloorId>.<NodeId>` Host PersistentId
- [x] T009 [US1] 在 `quickstart.md` 增加身份唯一性、坐标、Authoring/Debug 隔离和非目标文件审计

**Checkpoint**: Production 制作不再需要空壳 Floor、数组下标、Actor GUID 或裸跨层 NodeId。

---

## Phase 4: User Story 2 - 通过蜕印安全进入终层 (Priority: P2)

**Goal**: 冻结 Floor 2 的 20/21 图、15 个内容槽、蜕印保证来源与 8–9/14–15 AP。

**Independent Test**: Key 支配 Guardian/Exit；Exit 只指向 Floor 3 并只要求 `Credential.Run.MoltSeal`；删除表现卡不改变设计资格。

- [x] T010 [US2] 在 `data-model.md` 与 `contracts/floor2-graph-content-contract.md` 写入 Floor 2 精确拓扑、类型、Camp、landmark、内容槽、门槛和 AP
- [x] T011 [US2] 在 `Docs/WacomMap.md` 同步 Floor 2 节点/图/蜕印/节奏长期事实
- [x] T012 [US2] 在 `Docs/WacomDataAuthoring.md` 同步 `MoltCavern` IDs、`Card.Run.MoltSeal`、`Credential.Run.MoltSeal` 与 future Pickup/Entrance 制作合同

**Checkpoint**: Floor 2 设计完整且不会重引入普通卡软锁，但仍未创建任何 Production asset。

---

## Phase 5: User Story 3 - 按明确总节奏抵达 Journey 终点 (Priority: P3)

**Goal**: 冻结 Floor 3 terminal graph、Journey AP/天数与剩余 Production readiness。

**Independent Test**: Core 支配终局链；Guardian 无出边/无 Entrance；Floor 3 为 10/16 AP，Journey 为 26–28/44–46 AP；runtime success 仍明确阻塞。

- [x] T013 [US3] 在 `data-model.md` 与 `contracts/floor3-terminal-graph-contract.md` 写入 Floor 3 精确拓扑、类型、Camp、CoreBoon、终局链和 capability boundary
- [x] T014 [US3] 在 `contracts/journey-pacing-production-readiness.md` 冻结三层 AP、3 天关键推进、5–6 天完整目标和剩余三项 Production blocker
- [x] T015 [US3] 在 `Docs/WacomMap.md` 与 `Docs/WacomDataAuthoring.md` 同步 Floor 3、`VenomCore` 内容槽、总节奏和禁止终局绕过合同
- [x] T016 [US3] 在 `Docs/TODO.md` 完成 Floor 2/3 图冻结并登记 Journey success/31 个新 definitions；在 `Docs/Questions.md` 关闭缺图/时长问题并新增 success handoff 问题

**Checkpoint**: 正式三层图与 Journey pacing 已成为长期事实；Production 仍不能运行终局或保存二进制资产。

---

## Phase 6: Final static validation and handoff

**Purpose**: 证明 canonical tables、长期 Docs 和文件边界一致，并形成独立可审计提交。

- [x] T017 按 `quickstart.md` 验证两层 Node/Edge/type/reachability/merge/dominance/layout/content/AP 与 Guardian terminal 合同
- [x] T018 运行 Spec Kit `check-prerequisites` 与 `speckit-analyze` 只读跨工件分析，确认 FR/SC/task coverage 完整且无冲突/模板残留
- [x] T019 使用 `rg`、`git diff --check`、`git diff --name-only`、binary diff 与 `git lfs status` 证明 Source/Config/Content/LFS 零变化
- [x] T020 将最终验证、跳过的 Unreal 项、运行时 blocker、变更范围和最终 commit 写入 `quickstart.md`
- [x] T021 审阅并提交当前分支，确认 worktree/LFS 干净，不 merge main、不 push

---

## Dependencies & execution order

```text
Phase 1 workspace guard
  -> Phase 2 shared decisions
    -> US1 stable graph identities
      -> US2 Floor 2 gate/content/pacing
        -> US3 Floor 3 terminal/Journey pacing/readiness
          -> final static validation and commit
```

- US1 blocks US2/US3 because both Floors must use one canonical identity registry.
- US2 precedes US3 because Floor 2 Exit and its guaranteed Credential establish the only transition into the terminal Floor.
- Journey totals are calculated only after all three Floor AP contracts are fixed.
- Long-term Docs sync occurs inside the owning story rather than being deferred to final polish.

## Parallel opportunities

- After T004–T006, stable identity contract and quickstart boundary audit affect different files.
- Floor 2 and Floor 3 contract drafting use separate files after data-model field names are fixed.
- `Docs/TODO.md` and `Docs/Questions.md` can be reviewed independently after readiness terminology is final.
- Final graph, cross-artifact and Git/LFS validations remain sequential so quickstart records one final tree.

## Implementation strategy

1. Freeze all identities and graph facts before updating long-term Docs.
2. Treat Floor 2 gate and Floor 3 terminal as separate independently auditable contracts.
3. Do not convert design readiness into asset or runtime implementation authority.
4. End with a docs-only commit and a clean worktree; any Journey success or Production asset work requires a new feature slice.
