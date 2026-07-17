# Tasks: 正式 Floor 1 击倒分支奖励卡内容冻结

**Input**: Design documents from `specs/013-formal-floor1-knockdown-reward-content-freeze/`

**Prerequisites**: plan.md、spec.md、research.md、data-model.md、contracts/

**Tests**: 本轮为纯文档内容冻结；不编译或运行 Unreal。使用 Spec Kit、表格计数、schema/reference、Markdown link 和 Git/LFS 静态验证。

**Organization**: Tasks 按卡牌合同、Part/路线合同和 Production 交接三个 user story 分组。

## Phase 1: Setup and rule-truth alignment

- [x] T001 确认 branch/HEAD/merge-base 均基于 `10815349220a2315e6221d8d3d9bda4965b71d4e`，并把 Git/LFS/Unreal 进程结果写入 `specs/013-formal-floor1-knockdown-reward-content-freeze/quickstart.md`
- [x] T002 确认 `codex/formal-floor1-knockdown-reward-content-freeze` 使用指定 D 盘 worktree，且不复用 Spec 012 worktree
- [x] T003 读取 `AGENTS.md`、`Docs/AgentIntegrationWorkflow.md`、相关长期 Docs、constitution、Spec 011/012，并把研究结论写入 `specs/013-formal-floor1-knockdown-reward-content-freeze/research.md`
- [x] T004 创建 `specs/013-formal-floor1-knockdown-reward-content-freeze/spec.md` 和 `checklists/requirements.md`
- [x] T005 创建 plan、research、data-model、contracts 和 quickstart，并通过 constitution gate
- [x] T006 更新 `.specify/feature.json` 与 `AGENTS.md` 托管指针到 Spec 013

---

## Phase 2: User Story 1 — Exact eight-card contract (Priority: P1)

**Goal**: 冻结八张卡的全部可制作字段与 package。

**Independent Test**: card manifest 恰好 8 行，ID/package 唯一，字段全部属于现有 schema。

- [x] T007 [US1] 在 `specs/013-formal-floor1-knockdown-reward-content-freeze/contracts/card-manifest.md` 写入八个 CardId 与完整 package path
- [x] T008 [US1] 在 `specs/013-formal-floor1-knockdown-reward-content-freeze/contracts/card-manifest.md` 冻结名称、描述模板、费用、稀有度、关键词、TargetMode 与 Effects 顺序
- [x] T009 [US1] 在 `specs/013-formal-floor1-knockdown-reward-content-freeze/data-model.md` 建立卡牌字段模型和八条精确记录
- [x] T010 [US1] 在 `specs/013-formal-floor1-knockdown-reward-content-freeze/research.md` 记录敌人拟态、强度梯度与被拒绝替代方案
- [x] T011 [US1] 静态核对 `2 White / 4 Blue / 2 Yellow`、`4 Tool / 4 Weapon`、`7 Cost1 / 1 Cost2` 和零额外关键词/高级字段

**Checkpoint**: 下一轮内容作者无需再决定八张卡的任何规则字段。

---

## Phase 3: User Story 2 — Part assignment and route yield (Priority: P2)

**Goal**: 冻结十一 Part 的显式卡对引用、重复语义和 `14–17 / 20` 奖励量。

**Independent Test**: Part mapping 恰好 11 行且为 `2/3/2/4`；路线算式精确通过。

- [x] T012 [US2] 在 `specs/013-formal-floor1-knockdown-reward-content-freeze/contracts/part-assignment-contract.md` 枚举十一 Part 的 Aid/Destroy/legacy 字段
- [x] T013 [US2] 在同一合同中冻结每部位一次选择、同卡独立实例、允许重复和无去重/上限语义
- [x] T014 [US2] 在同一合同与 `data-model.md` 中记录 A/C=14、B/C=15、A/D=16、B/D=17、Full=20 的演算
- [x] T015 [US2] 在 `specs/013-formal-floor1-knockdown-reward-content-freeze/spec.md` 明确最后部位、手牌上限、重复 Archetype 和 Effect 顺序边界
- [x] T016 [US2] 静态核对奖励选择不新增 AP，Floor 1 仍为 `8–9 / 14–15 AP`

**Checkpoint**: 引用粒度和卡组膨胀规模可以独立审阅。

---

## Phase 4: User Story 3 — Production handoff and durable docs (Priority: P3)

**Goal**: 把 8 张卡增量并入未来 46 资产写集合，并将长期事实同步到 Docs。

**Independent Test**: Production 合同为 `38+8=46`，禁止引用和未来验证门禁完整，Git diff 仅包含批准文本路径。

- [x] T017 [US3] 在 `specs/013-formal-floor1-knockdown-reward-content-freeze/contracts/production-readiness-contract.md` 记录关闭项、剩余 blocker、未来写集合与 exit criteria
- [x] T018 [P] [US3] 在 `Docs/WacomData.md` 记录八张卡完整字段、拟态来源与十一 Part 引用原则
- [x] T019 [P] [US3] 在 `Docs/WacomDataAuthoring.md` 记录八个 package leaf、46 资产 manifest 扩展和 builder/validator 门禁
- [x] T020 [P] [US3] 在 `Docs/WacomBattle.md` 记录 Floor 1 每部位一次、二选一、允许重复及奖励量事实
- [x] T021 [P] [US3] 在 `Docs/WacomMap.md` 记录四条关键路线与完整探索奖励量，保持 AP 合同
- [x] T022 [P] [US3] 在 `Docs/TODO.md` 关闭八张卡设计 blocker，并把 46 DataAsset 实现升级为 ready
- [x] T023 [P] [US3] 在 `Docs/Questions.md` 关闭八张卡具体效果问题，保留背包容量和其它击倒后果
- [x] T024 [P] [US3] 在 `Docs/Roadmap.md` 把 Floor 1 击倒奖励从待设计改为待制作/验证

---

## Phase 5: Cross-artifact validation and delivery

- [x] T025 创建并完成 `specs/013-formal-floor1-knockdown-reward-content-freeze/checklists/content-quality.md`
- [x] T026 运行 Spec Kit prerequisites 与只读跨工件分析，修正所有 Critical/High/Medium 问题并更新 `quickstart.md`
- [x] T027 运行八卡/十一 Part/路线算式/schema/禁止引用/Markdown link 静态审计并更新 `quickstart.md`
- [x] T028 运行 `git diff --check`、allowed-path、Git/LFS/fsck 与二进制零变化审计并更新 `quickstart.md`
- [x] T029 在 `quickstart.md` 记录编译、Automation、AssetRegistry、Builder、Blueprint 与 PIE 跳过原因及未来卡组膨胀风险
- [x] T030 提交 `docs(content): freeze floor1 knockdown reward cards`，确认 worktree/LFS 干净，不 merge main、不 push

## Dependencies & Execution Order

- Phase 1 必须先完成；Spec 012 是全部内容合同的硬前置。
- Phase 2 冻结卡牌 Definition 后，Phase 3 才能安全引用这些身份。
- Phase 4 的长期文档必须以 Phase 2/3 的最终合同为准。
- T026–T029 必须在 T030 前完成；分析是只读步骤，若发现问题由主会话定向修正文档后重新分析。

## Parallel Opportunities

- T018–T024 修改不同长期文档，可在卡牌/Part 合同稳定后并行；本轮由主会话统一审阅以避免口径漂移。
- 各静态审计可并行读取，但最终 ledger 和提交必须顺序收口。

## Implementation Strategy

1. 先锁定八张卡及 package manifest。
2. 再锁定十一 Part 引用与路线奖励量。
3. 将 46 资产 Production gate 写入长期 Docs。
4. 最后完成 Spec Kit、范围、链接和 Git/LFS 静态验证并提交。
