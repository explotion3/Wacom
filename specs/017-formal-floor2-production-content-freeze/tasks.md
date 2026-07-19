---
description: "Dependency-ordered documentation tasks for Spec 017"
---

# Tasks: 正式 Floor 2 Production 内容合同冻结

**Input**: `spec.md`、`plan.md`、`research.md`、`data-model.md`、`contracts/`、`quickstart.md`

**Scope**: 纯文档。禁止修改 Source、Config、Content、GameplayTag、Build.cs、SaveGame、DataAsset、地图或其它二进制资产。

## Phase 1: Baseline and Spec Kit setup

- [x] T001 在目标 worktree 确认 branch/HEAD/merge-base、Git status、Git LFS 与 Unreal Editor 进程归属。
- [x] T002 读取 `AGENTS.md`、constitution、集成流程、Data/Battle/Map/Authoring 长期 Docs 与 Spec 009/011/013/014 历史合同。
- [x] T003 将 `.specify/feature.json` 指向 `specs/017-formal-floor2-production-content-freeze`。
- [x] T004 创建 `spec.md` 与 `checklists/requirements.md`，写明纯文档范围、47 资产总数、非目标与用户审阅前不提交门禁。
- [x] T005 通过 `.specify/scripts/powershell/setup-plan.ps1 -Json` 建立 plan 入口并完成 constitution gate。

**Checkpoint**: 基线、范围和需求质量已固定，未触碰运行时或资产。

## Phase 2: Research and foundational contracts

- [x] T006 在 `research.md` 记录 schema 复用、Intent target、Encounter 梯度、奖励粒度、事件经济、AP 和外部污染隔离决策。
- [x] T007 在 `data-model.md` 冻结 4 Enemy、4 Behavior、12 Part、26 Intent 与 7 Encounter 的完整结构和数值。
- [x] T008 在 `data-model.md` 冻结 12 Card、4 Pickup、3 Event/10 Choice、1 Shop、节点映射与 stable/tunable 边界。
- [x] T009 在 `contracts/production-asset-manifest.md` 列出 47 条精确 package/class/stable ID 和三张只读 Shop 依赖。
- [x] T010 在其余 contracts 中冻结 Enemy/Encounter、Card/Pickup/Shop、RunEvent 和 Production readiness gate。
- [x] T011 使用 agent-context update 只更新 `AGENTS.md` 托管 Spec Kit 指针到 Spec 017。
- [x] T012 通过 `.specify/scripts/powershell/setup-tasks.ps1 -Json` 确认设计工件可用于任务生成。

**Checkpoint**: 所有跨文档共享的 stable identity、package、结构和后续门禁已建立。

## Phase 3: User Story 1 — MoltCavern 战斗内容

**Goal**: 让未来资产作者可无额外规则判断地制作四敌人和七 Encounter。

- [x] T013 [US1] 核对每个 Part 的 HP/EXP、Sequence IntentSet、D/I/R、effect target、Slow 投递和空 selector/cooldown/fallback。
- [x] T014 [US1] 核对 Encounter authored slots、总 HP `21/36/42/36/34/57/70`、最多两敌人和 `bBoss` Floor-only 边界。
- [x] T015 [US1] 将敌人、Intent 与 Encounter 长期事实同步到 `Docs/WacomData.md` 和 `Docs/WacomBattle.md`。

## Phase 4: User Story 2 — 奖励、事件与经济

**Goal**: 固定十二卡、四 Pickup、三 Event、一个 Shop 及路线经济。

- [x] T016 [US2] 核对四张固定卡、八张 Aid/Destroy 卡、十二 Part 显式映射与 legacy-null。
- [x] T017 [US2] 核对 MoltSeal Card/Credential、Event 十 Choice、A/B 购买路径、D 路 flag 与 Shop 顺序/价格。
- [x] T018 [US2] 核对关键路线奖励 `17/18/17/18`、完整探索 24 与 AP `8–9 / 14–15`。
- [x] T019 [US2] 将卡牌、Pickup、Event、Shop、奖励与 AP 事实同步到 `Docs/WacomData.md`、`Docs/WacomBattle.md`、`Docs/WacomMap.md`。

## Phase 5: User Story 3 — 47-package Production 交接

**Goal**: 形成可供下一轮安全资产制作使用的长期 manifest/readiness 入口。

- [x] T020 [US3] 在 `Docs/WacomDataAuthoring.md` 记录 MoltCavern 路径、47 数量、stable structure、只读依赖和未来 seed-only 门禁。
- [x] T021 [US3] 在 `Docs/TODO.md` 关闭 Floor 2 内容设计 blocker并新增 47 资产、Floor/map/Host 与 Floor 3 独立任务。
- [x] T022 [US3] 在 `Docs/Questions.md` 关闭 Floor 2 卡牌/事件/经济选择并保留背包膨胀和其它击倒后果问题。
- [x] T023 [US3] 在 `Docs/Roadmap.md` 将 Floor 2 从“待设计”改为“待制作/验证”，Floor 3 保持待设计。

**Checkpoint**: 长期 Docs 与 Spec 017 对数量、内容、路径和未完成门禁一致。

## Phase 6: Static validation and review gate

- [x] T024 完成 `checklists/content-quality.md` 并逐项核对内容质量。
- [x] T025 运行 `.specify/scripts/powershell/check-prerequisites.ps1 -Json -RequireTasks -IncludeTasks`。
- [x] T026 运行 47-package/type/stable ID、Enemy/Intent、Encounter、Card/Event/Shop、Part reward、路线/AP 静态计数与唯一性审计。
- [x] T027 运行禁止引用与 Markdown link 审计，确认 manifest 零 Debug/Authoring/Test/BadgeDisplayTests/TrainingWarrior/Character 引用。
- [x] T028 以只读方式运行 Spec Kit 跨工件一致性分析；不在 analyze 阶段修改文件。
- [x] T029 运行 `git diff --check`、范围审计、Git/LFS status 与 `git lfs fsck`，确认零 Source/Config/Content/binary change。
- [x] T030 在 `quickstart.md` 记录最终验证、跳过项、已知 BugGirl 外部问题和零运行时回归面。
- [x] T031 向用户报告未提交 diff、文件清单、验证结果与剩余风险，等待明确确认。

## Phase 7: Commit after explicit user approval

- [x] T032 仅在用户明确确认后提交 `docs(content): freeze floor2 production content`。
- [x] T033 提交后确认 worktree/LFS 干净，回报 final commit；不 merge main、不 push。

## Dependencies and execution order

1. Phase 1 → Phase 2：先固定范围和 Spec 指针，再生成共享合同。
2. Phase 2 → Phase 3/4：Enemy/奖励/事件必须引用同一 data-model 和 manifest。
3. Phase 3/4 → Phase 5：长期 Docs 只同步已经冻结的事实。
4. Phase 5 → Phase 6：跨工件分析只能针对完整工件和长期 Docs。
5. Phase 6 → Phase 7：用户审阅是 commit 的硬门禁。

## Validation notes

- 本轮无可编译 checkpoint；纯文档不运行 WacomEditor、Automation、AssetRegistry、Builder、Blueprint 或 PIE。
- 后续 47 资产轮必须补受控 writer workflow、真实加载、Data Validation、Automation、AssetRegistry、引用/哈希、幂等和 LFS。
- `DA_Character_BugGirl` 的已知污染保持外部问题：不修改、不隐藏、不把 closure 记为通过。
