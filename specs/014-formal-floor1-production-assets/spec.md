# Feature Specification: Floor 1 Production 46 DataAsset 播种与校验

**Feature Branch**: `codex/formal-floor1-production-assets`

**Created**: 2026-07-18

**Status**: Approved for implementation

**Input**: 按 Spec 011/013 已冻结的 46-package Production manifest，提供只补缺失资产的受控播种入口、结构校验、首次默认值对比和完整 AssetRegistry/LFS 证据；不创建正式地图或 Host。

## Wacom Rule Context

**Primary Domain**: Data/card content authoring / Battle content / Run content / Editor tooling / Tests

**Rule Truth Docs**:

- [x] `AGENTS.md`
- [x] `Docs/AgentIntegrationWorkflow.md`
- [x] `Docs/UnrealMCPWorkflow.md`
- [x] `Docs/WacomData.md`
- [x] `Docs/WacomDataAuthoring.md`
- [x] `Docs/WacomBattle.md`
- [x] `Docs/WacomRun.md`
- [x] `Docs/Architecture.md`
- [x] `Docs/TODO.md`、`Docs/Questions.md`、`Docs/Roadmap.md`
- [x] Spec 011 核心内容 manifest、Spec 012 FormalProduction 校验、Spec 013 八卡/十一 Part 合同

**Expected Owning Module(s)**: `WacomEditor` 保存 seed-only manifest、构建服务、commandlet 与 Editor 内触发入口；`WacomData` schema 与 `WacomBattle/WacomRun` runtime 合同保持不变；`WacomTests` 覆盖 manifest、结构校验和真实资产。

**Non-Goals / Boundaries**:

- 不修改 `WacomData` 公共 schema、GameplayTag、SaveGame、Snapshot、Command、Resolution、ResultPacket、Build.cs 或模块依赖。
- 不创建或修改 Journey/Floor DataAsset、`uasset/umap` 地图、Host Blueprint、Player/GameMode、UI、材质、纹理、音频、敌人美术或卡牌表现。
- 不运行 `WacomRegenerateContent`、Snake/TrainingWarrior builder、Debug map builder、DreamShader、背包、卡牌或材质全量重建。
- 不迁移或重存 TrainingWarrior、Snake、Starter 卡、`PoisonFang`；四张既有 Shop 依赖只读。
- 不提供 `Force`、`Replace`、`Regenerate` 或删除接口；已存在的正确 class 资产永远不由播种入口保存。
- 不把 seed 默认值变成永久数值锁。正式资产创建后，文案与批准的数值字段允许人工调优；默认结构校验只守稳定身份、引用和规则拓扑。
- 不做正式 Floor 1 世界摆放、Production Journey/Floor 绑定、PIE Golden Path 或平衡结论。

## User Scenarios & Testing

### User Story 1 - 以完整 manifest 审计 46 个 Production package (Priority: P1)

作为内容制作与集成人员，我需要一个唯一的 46-package manifest 和只读检查入口，在任何保存发生前就能发现缺失、错误 class、重复身份、非法引用或结构漂移。

**Independent Test**: transient manifest test 在没有目标资产时也能验证 `12 Card / 4 Enemy / 4 Behavior / 11 Part / 6 Encounter / 4 Event / 4 Pickup / 1 Shop = 46`，所有 package、object name、class、stable ID 和组别唯一。

**Acceptance Scenarios**:

1. **Given** 默认命令未提供 `-SeedMissing`，**When** 检查一个缺失 package，**Then** 只报告缺失并返回校验失败，不创建目录、Package 或 dirty 文件。
2. **Given** package 已存在但 class 不匹配，**When** inspect 或 seed，**Then** 返回阻断错误，不重命名、不覆盖、不删除该对象。
3. **Given** package 已存在且 class 正确，**When** 使用 `-SeedMissing`，**Then** 只加载和校验，不调用保存，也不覆盖人工文案或数值。
4. **Given** manifest，**When** 运行唯一性审计，**Then** 46 个 package/object/stable ID 均唯一，且只读依赖不计入新资产数。

---

### User Story 2 - 按依赖顺序只播种缺失资产 (Priority: P1)

作为正式内容作者，我需要先创建 12 张卡，再创建 19 个敌人图资产，最后创建 15 个节点 Definition，使所有硬引用在保存时都指向已存在的正式资产。

**Independent Test**: 在空目标根上依次执行 `Cards`、`EnemyGraph`、`NodeDefinitions`，只生成对应 `12 / 19 / 15` 个 package；第二次相同执行创建与保存数量均为 0。

**Acceptance Scenarios**:

1. **Given** Cards 组缺失，**When** `SeedMissing Group=Cards`，**Then** 创建 12 个 `UCardDefinition`，写入冻结身份、结构和首次 seed 默认值。
2. **Given** Cards 已通过，**When** `SeedMissing Group=EnemyGraph`，**Then** 依次创建 4 Behavior、11 Part、4 Enemy；Part 显式引用所属 Aid/Destroy 卡并清空 legacy。
3. **Given** Cards 与 EnemyGraph 已通过，**When** `SeedMissing Group=NodeDefinitions`，**Then** 创建 6 Encounter、4 Event、4 Pickup、1 Shop，并只读引用四张现有正式 Shop 卡。
4. **Given** 任一依赖缺失或非法，**When** 创建下游组，**Then** 整组在首个保存前失败，不生成带空引用的 Production 资产。
5. **Given** 所有资产已存在，**When** 再次执行所有组，**Then** 0 create、0 save、0 unexpected dirty path。

---

### User Story 3 - 区分稳定结构与可调 seed 默认值 (Priority: P2)

作为策划和内容作者，我需要后续能调文案与数值，而制作校验仍持续阻止身份、引用拓扑和规则形状被破坏。

**Independent Test**: transient/real-asset tests 证明默认结构校验允许一次文案或数值变化，但拒绝 CardId、package class、关键词、TargetMode、效果类型/目标/顺序、Part 奖励映射、Enemy/Encounter/Event/Pickup/Shop 引用拓扑变化；`-CompareSeedDefaults` 会额外报告文案/数值 drift。

**Acceptance Scenarios**:

1. **Given** Card cost、rarity、effect magnitude 或文本被人工调节，**When** 默认 inspect，**Then** 只要 schema 仍合法且结构合同不变就通过。
2. **Given** Part HP/EXP、Intent initiative/resistance/magnitude、Shop price 或 Event 数值被人工调节，**When** 默认 inspect，**Then** 结构合法时通过。
3. **Given** stable ID、引用、数组顺序、类型、target、policy、flag/pressure identity 或 credential 被改变，**When** 默认 inspect，**Then** 明确报告 package 和字段路径并失败。
4. **Given** 初次播种验收，**When** 使用 `-CompareSeedDefaults`，**Then** 全部 editable seed 字段必须与 manifest 默认值一致；任何 drift 返回失败但不保存资产。

---

### User Story 4 - 通过 Unreal MCP 安全交付二进制资产 (Priority: P2)

作为资产所有者，我需要所有 46 个 `.uasset` 在准确 worktree、branch、HEAD 和 run/8140 Editor 会话中生成，并留下分组 writer audit、哈希和 LFS 证据。

**Independent Test**: 每组写入前 `AssertReady` 通过并取得只包含该组 package 的 writer lease；保存后 `ReleaseWriter` 成功且 audit JSON 的 dirty paths 与 `12/19/15` allowlist 子集一致。

**Acceptance Scenarios**:

1. **Given** 新 worktree 没有 Binaries，**When** 首次启动 Editor，**Then** 先 `AssertClosedForBuild`、默认 Unity 编译，再由正式脚本启动 run/8140。
2. **Given** 46-package 完整写集合已经列明，**When** 分组 mutation，**Then** Cards、EnemyGraph、NodeDefinitions 串行取得 writer lease；不存在并行 writer。
3. **Given** 保存完成，**When** 释放 writer，**Then** allowlist 外变化导致 fail-closed；不会自动清理或删除任何资产。
4. **Given** 需要再次编译，**When** Editor 正常关闭且 writer 已释放，**Then** `AssertClosedForBuild` 通过后才执行编译。

---

### User Story 5 - 用真实资产证明 Data/Battle/Run 合同 (Priority: P3)

作为集成人员，我需要真实加载 46 个资产并运行 Data、击倒奖励、Encounter、Event、Pickup、Shop 与 Run 获得卡回归，证明它们不是只满足路径数量的空壳。

**Independent Test**: `Wacom.Data.FormalFloor1Content` 加载全部 package，运行通用 validator 与 FormalProduction Part profile，验证精确结构、引用闭包、路线数量和禁止依赖；相关 Battle/Run 前缀全部通过。

**Acceptance Scenarios**:

1. **Given** 46 个 package，**When** AssetRegistry/failed-load 审计，**Then** 恰好加载 46 个期望 class，0 missing、0 wrong class、0 failed load。
2. **Given** 11 个 Part，**When** FormalProduction validation，**Then** Aid/Destroy 都有效、所属 Archetype 对应、legacy 全空。
3. **Given** 24 个 Intent、6 Encounter、4 Event、4 Pickup、1 Shop，**When** 运行 validator 和 manifest test，**Then** 所有稳定结构与只读依赖均符合合同。
4. **Given** 八张分支卡进入击倒事务，**When** 运行现有 Battle/Run 回归，**Then** Aid/Destroy 不串卡，Victory/Withdraw/Defeat 与既得奖励语义不变。

### Edge Cases

- `Group` 大小写不敏感，只允许 `Cards / EnemyGraph / NodeDefinitions / All`；未知值或互相矛盾参数返回参数错误且零写入。
- `All + SeedMissing` 仍按三组依赖顺序执行；MCP 正式写入以三个独立 writer lease 调用，不用一个长事务跨越全部资产。
- `-Report` 写入 `Saved/` 或明确的非 Content 路径；不得把 JSON 报告作为 Production package。
- 创建阶段出现任一保存失败时不删除已成功保存的早先 package；报告精确现场，停止后续创建，由人工审计后重跑只补缺失项。
- 已存在资产即使 `-CompareSeedDefaults` 失败也不能被 seed 修正；需要人工决定调参还是定向修复。
- 结构校验必须区分合法的 `BrushSnake` 名称与被禁止的 legacy Snake 依赖，禁止使用宽泛 `Snake` 字符串误报。
- Spec 013 `card-manifest.md` 的 `ShallowGuardian.Destroy` 行历史上缺失 TargetMode 列；长期 Docs/data-model 的权威值为 `AllEnemyParts`，两条 Effect 也均为 `Target.AllEnemyParts`。
- Description 中 `{Effect.N}` 目前只是可调 seed 文案，不是规则解析来源；结构真相始终是 `Effects[]`。

## Requirements

### Functional Requirements

- **FR-001**: 必须提供恰好 46 条 manifest 记录，分类为 `12 Card / 4 Enemy / 4 Behavior / 11 Part / 6 Encounter / 4 Event / 4 Pickup / 1 Shop`。
- **FR-002**: 每条记录必须声明唯一 package、object name、UClass、stable content ID、creation group 和依赖；四张现有 Shop 卡必须声明为只读依赖而非 manifest 资产。
- **FR-003**: 必须提供 `UWacomBuildFormalFloor1ContentCommandlet`，支持 `-SeedMissing`、`-Group=Cards|EnemyGraph|NodeDefinitions|All`、`-CompareSeedDefaults` 与 `-Report=<path>`。
- **FR-004**: commandlet 默认必须 inspect-only；没有 `-SeedMissing` 时不得创建 package、目录或 dirty 内容。
- **FR-005**: `-SeedMissing` 只能创建缺失 package；已存在正确 class 只加载/校验，错误 class 阻断，不能覆盖、替换、重命名、删除或保存。
- **FR-006**: 必须按 Cards → EnemyGraph → NodeDefinitions 的依赖顺序创建；每组开始保存前必须预检本组全部外部依赖。
- **FR-007**: 必须提供 Editor 内复用同一服务的受控触发入口，使正式 mutation 能发生在 run/8140 MCP writer lease 内；不得复制第二套制作逻辑。
- **FR-008**: commandlet/Editor 入口结果必须提供机器可读 JSON，包含 schemaVersion、mode、group、strict flag、counts、每 package 状态、错误、warning 与实际 saved package。
- **FR-009**: 退出码必须为 `0=success`、`1=manifest/structure/dependency validation failure`、`2=argument failure`、`3=create/save/reload failure`。
- **FR-010**: 默认结构校验必须锁定 package/class/stable ID、Card keyword/TargetMode/effect type-target-order、Behavior/Enemy/Encounter 拓扑、Event condition/effect/policy identity、Pickup reward/credential、Shop offer card/order、Part Aid/Destroy 映射与 legacy null。
- **FR-011**: 默认结构校验必须允许文案与批准数值字段人工调优，包括 Card cost/rarity/magnitude、Part HP/EXP、Intent initiative/resistance/magnitude、Shop price、Event numeric values。
- **FR-012**: `-CompareSeedDefaults` 必须额外精确比较全部首次 seed editable 字段，但只报告 drift，不保存或回写已有资产。
- **FR-013**: 12 张卡必须包含 Spec 011 的 4 张核心卡和 Spec 013 的 8 张分支卡；`ShallowGuardian.Destroy` 固定 `TargetMode=AllEnemyParts` 且两条 Effect target 均为 `Target.AllEnemyParts`。
- **FR-014**: 11 个正式 Part 必须显式填写所属 Archetype 的 Aid/Destroy 卡对并清空 `KnockdownRewardCard`；使用 FormalProduction profile 通过。
- **FR-015**: 4 Behavior 必须包含一份 Default phase、11 个 per-part Sequence IntentSet 和 Spec 011 的 24 条有序 Intent；4 Enemy 必须引用对应 Behavior/Part/IntentSet。
- **FR-016**: 6 Encounter、4 Event/13 Choice、4 Pickup、1 Shop 必须按 Spec 011 引用和顺序创建，Shop 四张既有卡保持只读。
- **FR-017**: Production 引用闭包不得包含 Debug、Authoring、`Test.*`、BadgeDisplayTests、TrainingWarrior、legacy Snake、地图、Host、UI、材质或卡牌表现资产。
- **FR-018**: 必须为 manifest/参数/inspect/no-overwrite/strict-vs-structural 提供小型 transient 自动化测试，并为真实 46 资产提供独立 `Wacom.Data.FormalFloor1Content` 测试；不得扩大现有巨型 spec。
- **FR-019**: 必须在实际写入前记录 46 个完整 Package allowlist，并按 `12/19/15` 三组串行执行 `AssertReady → AcquireWriter → mutate/save → git status → ReleaseWriter`。
- **FR-020**: 必须连续执行结构 inspect 两次，第二次 `SeedMissing` 证明 0 create/0 save；记录每组 audit JSON、实际 `.uasset`、前后 SHA-256 和 Git LFS 状态。
- **FR-021**: 每个 C++ checkpoint 必须在 Editor 关闭、writer 释放且 `AssertClosedForBuild` 通过后编译默认 Unity `WacomEditor Win64 Development`，并运行对应定向测试；所有 Unreal 命令带 `-NoDreamShaderEditorBridge`。
- **FR-022**: 最终必须运行 Card/EnemyPart/Enemy/Behavior/Encounter/Event/Pickup/Shop validator、Battle RuleContent/KnockdownReward、Run reward/Event/Pickup/Shop 受影响回归及 AssetRegistry/failed-load 审计。
- **FR-023**: 必须同步 `Docs/WacomData.md`、`WacomDataAuthoring.md`、`WacomBattle.md`、`WacomRun.md`、`Architecture.md`、`TODO.md`、`Questions.md` 与 `Roadmap.md`；长期事实不能只留在 Spec 014。
- **FR-024**: 不得修改 GameplayTag、SaveGame、Build.cs、模块依赖、Runtime schema、地图或非授权二进制资产；不得运行全量内容/材质/卡牌/背包 builder。
- **FR-025**: 必须以两个独立提交交付：先提交无二进制的 Editor seeder/manifest/tests/spec，再提交 46 个 DataAsset、真实资产测试与完成后的长期文档；不 merge main、不 push。

### Wacom-Specific Requirements

- **Docs-first evidence**: Spec 011/013 的内容值与 live Docs/schema 是 seed 真相；本 Spec 只纠正历史表格列错位，不重设计数值。
- **Module/API boundary**: 所有新逻辑位于 `WacomEditor/Private`，测试视图只在确有跨模块测试需要时暴露最小 editor-only 非反射面；运行时模块零变更。
- **Data/GameplayTag impact**: 创建现有 class 的实例，不改字段或 tag；46 个 package 是唯一二进制写集合。
- **Battle/Run impact**: 只提供静态内容；运行时规则和 AP/奖励/事务合同不变。
- **UI/App lifecycle impact**: 无 UI 或输入改动；Editor 内触发入口只用于制作，不参与游戏运行。
- **Testing expectation**: 编译、transient manifest、真实资产、Data validation、Battle/Run smoke、AssetRegistry、failed-load、双跑、hash/LFS 和 MCP audit。
- **Temporary debt**: seed-only 入口是正式制作工具，不是临时资产权威；首次播种后默认职责转为只读结构检查。未来若需要批量数值迁移，必须另案设计显式 migration，不能扩大本工具覆盖行为。

## Key Entities

- **Formal Floor 1 manifest entry**: package、object、class、stable ID、组别、依赖、seed configurator 与结构 comparator 的单一记录。
- **Seed default**: 首次创建时写入的完整 editable 字段；不是永久平衡权威。
- **Stable structure**: 后续必须持续保持的身份、引用、规则类型/目标/顺序和制作 profile。
- **Build report**: inspect/seed/strict 运行的机器可读结果，记录每条 manifest 状态且不作为运行时数据。
- **Writer group**: Cards 12、EnemyGraph 19、NodeDefinitions 15 三个可独立验证的保存边界。

## Success Criteria

### Measurable Outcomes

- **SC-001**: manifest 恰好 46 条且分类、group 总数分别为 `46` 与 `12/19/15`，package/object/stable ID 无重复。
- **SC-002**: 空目标根上默认 inspect 产生 0 文件；三组播种后实际新增恰好 46 个 `.uasset`，allowlist 外新增/修改为 0。
- **SC-003**: 第二次每组 `SeedMissing` 均报告 `created=0, saved=0`；已生成资产 SHA-256 在第二次运行前后完全相同。
- **SC-004**: `CompareSeedDefaults` 初次验收 46/46 通过；默认结构校验允许至少一组 transient 文案/数值 drift 用例，并拒绝全部结构 drift 用例。
- **SC-005**: 11/11 Part 通过 FormalProduction，0 legacy 引用；8 张分支卡按 4 Aid/4 Destroy 精确映射。
- **SC-006**: 24 Intent、6 Encounter、4 Event/13 Choice、4 Pickup、1 Shop 与 12 Card 的 stable structure 全部匹配冻结合同。
- **SC-007**: AssetRegistry 加载 46/46，missing/wrong-class/failed-load/forbidden-production-reference 均为 0。
- **SC-008**: 默认 Unity WacomEditor 编译和全部聚焦 Automation 通过；没有新增 Blueprint/PIE 验证要求，因为本轮不创建 Blueprint/map/可进入场景。
- **SC-009**: 三个 writer audit 成功，实际 dirty `.uasset` 集合分别为 12/19/15；Git LFS fsck 通过且所有新增 `.uasset` 为有效 LFS 对象。
- **SC-010**: 两个提交边界清晰，最终 worktree/LFS 干净，未 merge main、未 push。

## Assumptions

- `d7c6b70b1dc1006f08fff8c598d58e65f53a5813` 是开工时最新干净 main 后继；它只增加背包表现修复，与本功能无文件或规则重叠。
- 四张只读 Shop 依赖已存在且 class/CardId 正确；任何不一致都阻断 NodeDefinitions 组，不由本工具修复。
- 初次 seed 文案可按已冻结标题和现有内容语气补齐；DisplayName/Description 后续可人工改，规则不解析自然语言。
- 数值调优不改变 Card/Intent/Event 的类型、target、顺序、policy、flag/pressure identity 或引用拓扑；如需要改变这些稳定结构，必须另开内容修订。
- 本轮没有 Production Journey/Floor/map 或 Host，因此 PIE 不能提供额外结构证据；真实 Golden Path 留给正式场景制作轮。
