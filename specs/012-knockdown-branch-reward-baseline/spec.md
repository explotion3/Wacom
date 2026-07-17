# Feature Specification: 击倒分支奖励合同基线

**Feature Branch**: `codex/knockdown-branch-reward-baseline`

**Created**: 2026-07-17

**Status**: Ready for Planning

**Input**: 为击倒事件的 Aid / Destroy 建立差异化奖励卡合同、原子战斗结算、被动文本预览和正式内容制作门禁，同时保留现有资产的 legacy 兼容读取。

## Wacom Rule Context *(mandatory)*

**Primary Domain**: Battle rules / Data-card authoring / Battle UI

**Rule Truth Docs**:
- [x] `AGENTS.md`
- [x] `Docs/Architecture.md`
- [x] `Docs/WacomBattle.md`
- [x] `Docs/WacomData.md`
- [x] `Docs/WacomDataAuthoring.md`
- [x] `Docs/WacomRun.md`
- [x] `Docs/WacomApp.md`

**Expected Owning Module(s)**: `WacomData`、`WacomBattle`、`WacomApp`、`WacomEditor`、`WacomTests`

**Non-Goals / Boundaries**:
- 不创建、保存或迁移任何 `uasset/umap`，不运行 builder。
- 不修改 GameplayTag、SaveGame、`FBattleResultPacket`、`FBattleGainedCard`、`FRunState`、Build.cs 或模块依赖。
- 不加入压力、RunFlag、战后 RunEvent、Destroy 伤口、完整 CardView、缩略图或新的输入/焦点流程。
- 不设计 Floor 1 八张未来分支奖励卡的数值、美术或 Host/世界摆放。
- Withdraw、最后存活部位、经验、AP、撤离重入和 Defeat 持久化规则保持现状。

**Open Rule Questions**:
- 无。本轮采用“每个敌人一对 Aid/Destroy 奖励卡”的已确认方案；其它击倒分支效果继续留待后续规则设计。

## User Scenarios & Testing *(mandatory)*

### User Story 1 - 内容作者明确配置分支奖励 (Priority: P1)

内容作者可以为敌方部位分别配置 Aid 与 Destroy 奖励，并通过通用或正式生产两档校验得到明确结果；旧资产在授权迁移前继续可用。

**Why this priority**: 分支数据合同和迁移边界是战斗规则、UI 预览及未来 Floor 1 正式资产的共同基础。

**Independent Test**: `Wacom.Data.EnemyPart` 与 `Wacom.Data.Enemy.TrainingWarrior` 覆盖显式字段、legacy fallback、混填错误、正式生产缺失奖励和现有二进制资产兼容读取。

**Acceptance Scenarios**:

1. **Given** 一个只配置 Aid/Destroy 新字段的部位，**When** 查询对应选择奖励，**Then** 各自返回明确且不串用的卡牌。
2. **Given** 一个只配置旧奖励字段的现有部位，**When** 查询 Aid 或 Destroy，**Then** 两者都兼容回退到旧卡；Withdraw/None 返回空。
3. **Given** 一个同时填写旧字段和任一新字段的部位，**When** 执行任一制作校验，**Then** 报告制作错误。
4. **Given** 一个没有奖励的 Debug/测试部位，**When** 执行 General 校验，**Then** 允许通过；同一部位执行 FormalProduction 校验时拒绝。

---

### User Story 2 - 玩家按所选分支原子获得卡牌 (Priority: P1)

玩家在击倒对话框选择 Aid 或 Destroy 后，战斗只授予该分支配置的奖励，现有即时入手、事件、战后包和 Run 结算语义保持一致。

**Why this priority**: 差异化奖励必须落在现有击倒选择事务中，不能产生第二条结算路径或破坏撤离/失败语义。

**Independent Test**: `Wacom.Battle.KnockdownReward` 与既有 `Wacom.Battle.Knockdown`、`Wacom.Run.BattleRewardCardsAddedToBackpack`、`Wacom.Run.NotificationCoalescing`。

**Acceptance Scenarios**:

1. **Given** Aid 与 Destroy 配置不同卡，**When** 玩家选择其中一个分支，**Then** 只创建、发布并记录所选分支的卡。
2. **Given** 所选分支没有奖励，**When** 玩家选择该分支，**Then** 选择仍合法且事务成功，但不创建卡、不发布 `CardGained`、不写 `GainedCards`。
3. **Given** 玩家选择 Withdraw，**When** 部位配置任意奖励，**Then** Withdraw 不获得卡并保持既有 0 AP/可重入语义。
4. **Given** 战斗中已获得分支奖励，**When** 战斗 Victory 或 Withdraw，**Then** Run 持久获得；Defeat 不持久化。

---

### User Story 3 - 玩家在选择前看到简单奖励摘要 (Priority: P2)

玩家在击倒选择面板中可以看到 Aid 与 Destroy 各自的卡牌奖励名称；没有奖励时看到明确的“无卡牌奖励”。

**Why this priority**: 分支奖励会影响选择，但 UI 必须保持被动，只展示 Battle 提供的 ViewData。

**Independent Test**: 独立 `Wacom.UI.Battle.KnockdownChoice` Dialog 测试覆盖初次绑定、连续刷新、空奖励及禁用状态。

**Acceptance Scenarios**:

1. **Given** 两个分支配置不同奖励，**When** 面板绑定 ViewData，**Then** 两个分支分别显示对应卡名。
2. **Given** 面板已显示一组奖励，**When** 连续 `SetContext` 传入另一组 ViewData，**Then** 文本和按钮可用性都刷新，不保留旧内容。
3. **Given** 某个分支没有奖励，**When** 面板显示该分支，**Then** 展示“无卡牌奖励”，但不因此禁用选项。

---

### User Story 4 - Floor 1 获得可实施的迁移边界 (Priority: P3)

后续正式内容制作可以按每个 SerpentWood 敌人一对奖励卡创建八张资产，并知道 legacy 字段何时可以删除；本轮不创建这些资产。

**Why this priority**: Spec 011 的 38 个核心资产需要在正式制作前补齐击倒奖励数量、路径和校验门禁。

**Independent Test**: 文档/Spec 静态审计确认 4 个敌人、8 个唯一 CardId、4 个主题目录，且 Production 总量明确为“38 个核心资产 + 8 张分支奖励卡”。

**Acceptance Scenarios**:

1. **Given** Floor 1 四个敌人原型，**When** 查看制作合同，**Then** 每个敌人都有 `<Archetype>.Aid` 与 `<Archetype>.Destroy` 两个稳定 CardId 和主题路径。
2. **Given** legacy 字段仍被 TrainingWarrior、Snake 或正式部位依赖，**When** 评估删除旧字段，**Then** 删除保持阻塞，直到授权迁移和 AssetRegistry 零依赖审计完成。

### Edge Cases

- 没有待处理击倒事件时，ViewData 保持空且不泄漏上一事件奖励。
- 待处理部位或其 Definition 无效时，奖励摘要为空，但现有选择可用性和原子失败语义不被 UI 重算。
- Aid 与 Destroy 指向同一张卡在 schema 上允许；FormalProduction 只要求显式配置，不替策划判断奖励是否应该不同。
- 显示名为空时，奖励预览回退到稳定 `CardId`；两者都为空时仍显示“无卡牌奖励”。
- 手牌已满时，奖励继续沿用现有即时入手后手牌上限处理；战后包仍记录已获得卡。
- 连续多部位击倒时，每个新队头 ViewData 都从对应部位重新构造，不复用上一部位奖励。

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: 每个敌方部位 MUST 可以分别声明 Aid 与 Destroy 奖励卡。
- **FR-002**: 系统 MUST 为 Aid/Destroy 提供单一兼容查询：新字段优先，旧字段回退，Withdraw/None 始终为空。
- **FR-003**: 旧奖励字段 MUST 保留为 deprecated 兼容来源；旧字段与任一新字段混填 MUST 是制作错误。
- **FR-004**: General 校验 MUST 允许纯 legacy、纯新字段或完全无奖励；FormalProduction MUST 要求 Aid/Destroy 显式配置且旧字段为空。
- **FR-005**: TrainingWarrior 与 Snake builder 的未来写入 MUST 使用两个显式新字段并清空 legacy，但本轮 MUST NOT 执行 builder 或保存资产。
- **FR-006**: Aid/Destroy 选择事务 MUST 只消费对应分支奖励，并保持即时入手、`CardGained`、`FBattleGainedCard.SourceChoice` 和手牌上限顺序。
- **FR-007**: 缺少奖励 MUST NOT 改变选项可用性；只产生零卡牌奖励。
- **FR-008**: Withdraw MUST 不查询或授予奖励；Victory/Withdraw/Defeat 的战后持久化语义 MUST 保持现状。
- **FR-009**: `FKnockdownChoiceOptionView` MUST 提供只读奖励存在性、稳定 CardId 和显示名，不向 App 暴露可写规则对象。
- **FR-010**: 奖励 ViewData MUST 来自与结算相同的数据查询，并对每次待处理击倒事件重新构造。
- **FR-011**: 原生击倒面板 MUST 在 Aid/Destroy 分支下展示奖励卡名；空奖励 MUST 显示“无卡牌奖励”。
- **FR-012**: UI MUST 保持被动，选项可用性只来自既有 ViewData，奖励存在性不得成为 UI 自行推导的禁用条件。
- **FR-013**: Floor 1 MUST 预留 8 个稳定奖励 CardId，命名为 `Reward.SerpentWood.<Archetype>.Aid/Destroy`，并使用每敌人主题子目录。
- **FR-014**: 正式内容总量 MUST 记为 38 个核心 DataAsset 加 8 张击倒分支奖励卡；本轮不得设计数值或创建资产。
- **FR-015**: legacy 字段删除 MUST 等待 TrainingWarrior、Snake 和全部正式 Part 授权迁移，并由 AssetRegistry 证明零旧字段依赖。
- **FR-016**: `FBattleResultPacket`、`FBattleGainedCard`、`FRunState`、SaveGame、GameplayTag、Build.cs 和模块依赖 MUST 保持不变。
- **FR-017**: 系统 MUST 更新 Battle/Data/DataAuthoring/App/Run 长期文档及 TODO、Questions、Roadmap、TechDebt。
- **FR-018**: 每个 C++ checkpoint MUST 编译 `WacomEditor` 并运行对应定向 Automation；Unreal 命令 MUST 带 `-NoDreamShaderEditorBridge`。
- **FR-019**: 最终验证 MUST 包含只读 AssetRegistry/failed-load、Blueprint compile、指定资产 SHA-256、Spec Kit 一致性、Git/LFS 和范围审计，且不得保存二进制资产。

### Wacom-Specific Requirements

- **Docs-first evidence**: 读取并同步 `Docs/WacomBattle.md`、`WacomData.md`、`WacomDataAuthoring.md`、`WacomApp.md`、`WacomRun.md`、`Architecture.md`、`TODO.md`、`Questions.md`、`Roadmap.md`、`TechDebt.md`。
- **Module/API boundary**: 静态字段/查询归 `WacomData`；战斗 ViewData 和结算归 `WacomBattle`；被动文本归 `WacomApp`；制作校验归 `WacomEditor`；测试归 `WacomTests`。
- **Data/GameplayTag impact**: 新增两个反射 DataAsset 字段和一个非反射 C++ 查询；不新增 GameplayTag，不保存资产。
- **Battle contract impact**: 扩展 `FKnockdownChoiceOptionView`；不改 Command、Event 类型、ResultPacket 或 runtime state schema。
- **Run contract impact**: 无公共或状态变化；只回归验证现有战后获得卡语义。
- **UI/App boundary**: 面板消费每次 push/`SetContext` 的 ViewData；不订阅、不持有规则对象、不改变 Modal 焦点/Back 拦截。
- **Testing expectation**: 新建小型 Battle 与 Dialog spec，扩展 EnemyPart validation/TrainingWarrior 覆盖，并运行计划列出的聚焦前缀。
- **Temporary debt**: 旧字段作为明确迁移债记录在 `Docs/TechDebt.md`，删除触发条件见 FR-015。

### Key Entities

- **UEnemyPartDefinition**: 敌方部位静态奖励配置，包含 Aid/Destroy 显式字段和 legacy 兼容字段。
- **FKnockdownChoiceOptionView**: Battle 输出给 App 的单个选项只读可用性与奖励摘要。
- **FBattleGainedCard**: 现有战后奖励记录；继续用 `SourceChoice` 表达奖励来源。
- **Enemy-part validation profile**: General 与 FormalProduction 两档 C++ 制作校验语义。
- **Floor 1 branch reward identity**: 四个 Archetype × Aid/Destroy，共 8 个稳定 CardId。

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Aid 与 Destroy 配置不同卡时，100% 的聚焦用例只授予所选分支卡，零串卡。
- **SC-002**: legacy-only TrainingWarrior/Snake 资产继续通过 General 校验并由统一查询返回现有奖励。
- **SC-003**: 混填、正式生产缺 Aid、缺 Destroy 或残留 legacy 的四类错误均被 FormalProduction 校验拒绝。
- **SC-004**: 空奖励不改变 Aid/Destroy 可用性，且产生零 `CardGained` 和零 `GainedCards`。
- **SC-005**: Dialog 初次绑定和连续刷新均显示正确奖励文本；空奖励明确显示“无卡牌奖励”。
- **SC-006**: 既有 Withdraw、最后部位、连续击倒、手牌上限、Defeat、Victory/Withdraw Run 持久化和一次广播测试全部通过。
- **SC-007**: Floor 1 文档枚举恰好 8 个唯一分支奖励 CardId，路径全部位于四个 Archetype 子目录。
- **SC-008**: 默认 Unity `WacomEditor Win64 Development` 编译及全部指定聚焦 Automation 通过。
- **SC-009**: TrainingWarrior Part 与 BrokenCleave 前后 SHA-256 不变，Git diff 不含 `.uasset/.umap`、GameplayTag、SaveGame、Build.cs 或模块依赖修改。
- **SC-010**: worktree 和 Git LFS 提交状态干净，最终提交不 merge main、不 push。

## Assumptions

- 每个敌人一对 Aid/Destroy 卡只冻结身份与数量，不冻结效果。
- 战内 Destroy 不新增伤口；战外右手破坏规则不在本功能中改变。
- 旧二进制资产在迁移前继续通过 legacy fallback 加载。
- C++ 原生 fallback 仍是本轮可验证的 UI 表现；正式 WBP 美术另案。
- 自动化足以覆盖本轮规则和原生 Dialog；没有正式差异化二进制内容，因此不要求 PIE。
