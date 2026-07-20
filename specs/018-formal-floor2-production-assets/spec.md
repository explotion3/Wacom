# Feature Specification: Floor 2 Production 47 DataAsset 播种与校验

**Feature Branch**: `codex/formal-floor2-production-assets`

**Created**: 2026-07-19

**Status**: Approved for implementation

**Input**: 以 Spec 017 冻结的 exact 47-package MoltCavern 合同为权威，抽取可复用的正式内容 seed-only 内核，并按 Cards、EnemyGraph、NodeDefinitions 三组创建、验证 Floor 2 Production DataAsset；不修改运行时规则、地图或表现资产。

## Wacom Rule Context

**Primary Domain**: Data/card authoring / Battle content / Run content / Editor tooling / Tests

**Rule Truth Docs**:

- [x] `AGENTS.md`
- [x] `Docs/Architecture.md`
- [x] `Docs/AgentIntegrationWorkflow.md`
- [x] `Docs/UnrealMCPWorkflow.md`
- [x] `Docs/WacomData.md`
- [x] `Docs/WacomDataAuthoring.md`
- [x] `Docs/WacomBattle.md`
- [x] `Docs/WacomRun.md`
- [x] `Docs/WacomMap.md`
- [x] `Docs/TODO.md`、`Docs/Questions.md`、`Docs/Roadmap.md`
- [x] `specs/017-formal-floor2-production-content-freeze/`

**Expected Owning Module(s)**: `WacomEditor` 持有私有共享 seed-only 执行服务、Floor 1/2 content profile、commandlet 与 Editor command；`WacomTests` 提供小型 manifest、共享服务与真实资产测试。`WacomData/WacomBattle/WacomRun/WacomApp` 运行时合同保持不变。

**Non-Goals / Boundaries**:

- 不修改 GameplayTag、DataAsset schema、SaveGame、Snapshot、Command、Resolution、ResultPacket、Build.cs 或模块依赖。
- 不创建或修改 Journey/Floor、地图、Host Blueprint、GameMode、Player、UI、材质、纹理、音频、敌人美术或卡牌表现。
- 不运行 `WacomRegenerateContent`、Snake/TrainingWarrior/Debug map/DreamShader/背包/卡牌/材质全量 builder。
- 不覆盖或重存任何已存在正确 class 的内容；不提供 Force/replace/regenerate/delete。
- 不修改 `DA_Character_BugGirl`，不削弱 validator，不把其既有 StarterDeck 污染误报为已解决。
- 本轮不执行 PIE；Floor 2 scene、Journey 与跨层 handoff 留给后续独立轮次。

**Open Rule Questions**: None。Spec 017 与用户批准计划已冻结全部产品和制作决策。

## User Scenarios & Testing

### User Story 1 - 复用安全的正式内容播种内核 (Priority: P1)

作为内容工具维护者，我需要 Floor 1 和 Floor 2 共用一套 inspect-first、seed-missing-only、no-overwrite、报告与 comparator 执行内核，使后续 Floor 不复制整套保存流程。

**Why this priority**: 这是 Floor 2 安全落盘和未来 Floor 3 扩展的基础；若复制 Floor 1 的约 2000 行专用实现，任何修复都会发生漂移。

**Independent Test**: 默认 Unity 编译通过；`Wacom.Editor.FormalProductionContentSeedService`、现有 `Wacom.Editor.FormalFloor1Content.Manifest` 与 `Wacom.Data.FormalFloor1Content` 通过，且 Floor 1 的 46 个 `.uasset` 哈希与 Git 状态不变。

**Acceptance Scenarios**:

1. **Given** 已存在并人工可调的 Floor 1 资产，**When** Floor 1 profile 改由共享服务执行 structural/strict inspect，**Then** 命令、报告、验证结果保持兼容且零资产保存。
2. **Given** 缺失、错误 class 或结构漂移的目标，**When** inspect 或 seed preflight，**Then** 服务按合同返回失败，不覆盖、不删除且在首个保存前阻断可预检错误。
3. **Given** 未知或危险参数，**When** 解析命令，**Then** 返回参数错误并拒绝 mutation。

---

### User Story 2 - 审计 Floor 2 exact 47-package profile (Priority: P2)

作为内容制作人员，我需要一个可在资产不存在时完整构造并验证的 MoltCavern profile，证明 47 个 package、稳定身份、引用、有序结构与冻结初始值全部能由当前 schema 表达。

**Why this priority**: 只有 transient profile 与命令预检通过，才允许取得真实资产写锁。

**Independent Test**: `Wacom.Editor.FormalFloor2Content.Manifest` 验证精确类型/组别计数、26 Intent、10 Choice、12 explicit branch-reward Part、Guardian Destroy、MoltSeal Credential、命令语法和 comparator 边界；空目录 inspect 报告 `47 missing / 0 created / 0 saved`。

**Acceptance Scenarios**:

1. **Given** 七个 MoltCavern 目标根均不存在，**When** 运行默认 inspect，**Then** 只生成 Saved JSON，Content 下不产生目录或资产。
2. **Given** 完整 transient graph，**When** 执行通用与 FormalProduction validation，**Then** 47 个 Definition 全部通过并且 read-only dependency 不进入 writable manifest。

---

### User Story 3 - 串行播种并验收 47 个 Production DataAsset (Priority: P3)

作为集成人员，我需要在已验证的 run Editor session 和精确 writer allowlist 下，按 Cards 12、EnemyGraph 20、NodeDefinitions 15 串行创建真实资产，并获得加载、引用、哈希、幂等与 LFS 证据。

**Why this priority**: 真实资产是 Floor 2 scene 制作的前置，但必须建立在 P1/P2 的无覆盖门禁之后。

**Independent Test**: 三组首次报告合计 `47 created / 47 saved / 0 failed`；严格 inspect 和第二次 seed 为 `47 existing / 0 created / 0 saved / 0 failed`；`Wacom.Data.FormalFloor2Content`、AssetRegistry、failed-load、forbidden closure、SHA-256 与 `git lfs fsck` 通过。

**Acceptance Scenarios**:

1. **Given** Cards 组精确 12-package writer，**When** seed 成功，**Then** 只创建 12 张卡且下一组才能开始。
2. **Given** Cards 已存在，**When** EnemyGraph 组 seed，**Then** 创建 4 Behavior、12 Part、4 Enemy，所有 Part 显式引用同敌卡对且 legacy 为空。
3. **Given** Cards 与 EnemyGraph 已存在，**When** NodeDefinitions 组 seed，**Then** 创建 7 Encounter、3 Event、4 Pickup、1 Shop，并保持三张外部 Shop 卡哈希不变。
4. **Given** 47 个资产已经存在且合法，**When** 再次执行三组 seed，**Then** 不保存任何 Package，聚合哈希不变。

### Edge Cases

- 目标 package 存在但 object load 失败或 class 不符：整组选中范围 preflight 失败，零新保存。
- 已存在正确 class 但 stable structure 不符：报告 validation failure，不覆盖或恢复 seed defaults。
- 只读依赖缺失：对应组在首个保存前失败；不得创建替代卡。
- 串行保存中途发生磁盘/重载失败：停止后续项、保留已生成资产和 writer 证据，不自动清理；修复后 seed-missing-only 续跑。
- writer allowlist 外出现变化：ReleaseWriter fail closed，停止交付并审计，不 clean/reset/stash。
- Broad `Wacom.Data` 仍可能暴露既有 BugGirl StarterDeck 污染；本轮只将它记录为外部失败，不更改权威规则。

## Requirements

### Functional Requirements

- **FR-001**: 必须从 `main@8e54505e` 创建 `codex/formal-floor2-production-assets` 并复用已审计 D 盘 worktree。
- **FR-002**: 必须提供 WacomEditor 私有共享正式内容 seed-only 服务，使 Floor 1/2 复用参数、manifest preflight、no-overwrite、比较、保存/重载和 JSON 报告逻辑。
- **FR-003**: Floor 1 命令、46-entry manifest、报告 schema、structural/strict comparator 与真实资产结果必须保持兼容，且重构不得保存 Floor 1 二进制。
- **FR-004**: 必须提供 `WacomBuildFormalFloor2Content` 和 `Wacom.BuildFormalFloor2Content`，支持 `Group/SeedMissing/CompareSeedDefaults/Report` 精确语法。
- **FR-005**: 默认必须 inspect-only；只有 `SeedMissing` 可创建缺失资产，任何已有正确 class 资产均不得被保存。
- **FR-006**: 必须拒绝 `Force/Replace/Regenerate`、未知参数、重复参数和非法 group，并使用固定 `0/1/2/3` 退出码。
- **FR-007**: writable manifest 必须恰好 47 个唯一 package/class/stable ID，类型计数为 `4/4/12/7/3/4/1/12`。
- **FR-008**: 分组必须恰好为 `Cards=12 / EnemyGraph=20 / NodeDefinitions=15`，依赖顺序不可颠倒。
- **FR-009**: Floor 2 profile 必须实现 Spec 017 的 12 Card、4 Enemy、4 Behavior、12 Part、26 Intent、7 Encounter、3 Event/10 Choice、4 Pickup 与 1 Shop 精确合同。
- **FR-010**: 所有 Part 必须显式引用同 Archetype Aid/Destroy 且 `KnockdownRewardCard=null`；Guardian Destroy 必须为 `2/Yellow/AllEnemyParts/Damage5+Poison2`。
- **FR-011**: MoltSeal Pickup 必须在同一 Definition 中引用 `Card.Run.MoltSeal` 并授予 `Credential.Run.MoltSeal`。
- **FR-012**: DeepWayfarer 必须按冻结顺序引用五张卡；HerbalPoultice、ChitinWard、MoltCut 是只读依赖且前后哈希不变。
- **FR-013**: 必须通过 run role、准确 session identity 和逐组 exact package allowlist 获取 writer；不得使用 `AllowExistingDirtyPackages`。
- **FR-014**: 每组必须记录 seed、strict inspect、第二次 idempotence、Git status、SHA-256、LFS 与 writer audit JSON。
- **FR-015**: 必须执行真实 47/47 load、Data Validation、AssetRegistry class/count、failed-load 与 forbidden dependency closure。
- **FR-016**: forbidden closure 不得包含 Debug、Authoring、Test、BadgeDisplayTests、TrainingWarrior、Character、地图、Host、UI、材质或 legacy reward 依赖。
- **FR-017**: 不得修改 Runtime schema/规则、GameplayTag、SaveGame、Build.cs、模块依赖、Blueprint、map 或其它 Agent 资产。
- **FR-018**: 必须同步长期 Docs 和 Spec 018 quickstart 证据；Floor 2 scene/Journey/PIE blocker 继续保持开放。
- **FR-019**: 用户明确审阅前不得 stage 或 commit；确认后才整理两笔依赖顺序提交。
- **FR-020**: 不得 merge main、push、删除 branch/worktree，或使用 destructive Git 命令。

### Wacom-Specific Requirements

- **Docs-first evidence**: 以 `Docs/WacomData*`、Battle/Run/Map/Architecture 与 Spec 017 为权威，并同步 Data、DataAuthoring、Battle、Run、Map、Architecture、TODO、Questions、Roadmap。
- **Module/API boundary**: 共享执行服务与 profiles 留在 `WacomEditor/Private`；仅现有 commandlet/console 与非反射 automation test view 对测试可见。
- **Data/GameplayTag impact**: 创建 47 个现有 schema 的 DataAsset；零新字段、零 tag。
- **Battle contract impact**: 零运行时变化；真实 Parts/Encounters/Cards 使用已实现合同。
- **Run contract impact**: 零运行时变化；真实 Pickup/Shop/Event 使用已实现事务。
- **UI/App boundary**: N/A。
- **Testing expectation**: 三个可编译 checkpoint、focused Automation、真实 AssetRegistry/closure/hash/LFS；无 PIE。
- **Temporary debt**: 无新增临时实现；BugGirl 外部污染和 Floor 2 scene/Journey blocker继续保留。

### Key Entities

- **Formal production content profile**: exact manifest、预期 class/group counts、只读依赖、配置/验证 callbacks 与报告身份。
- **Manifest entry**: Group、PackagePath、StableId、AssetClass。
- **Build report**: options、entries、created/existing/missing/failed/saved counts、exit code 与 failure category。
- **MoltCavern Production DataAsset**: 47 个静态 Definition，路径与稳定 ID 见 exact manifest。
- **Identity**: package/object leaf 与内容 stable ID；展示文本和批准的平衡字段不是持久身份。

## Success Criteria

### Measurable Outcomes

- **SC-001**: Floor 1 共享内核回归全部通过，46 个既有资产的 hash 与 Git 状态变化数为 0。
- **SC-002**: Floor 2 manifest 恰好 47 条，group 计数 `12/20/15`，package/class/stable ID 重复数为 0。
- **SC-003**: 首次真实播种精确创建并保存 47 个 LFS `.uasset`，白名单外资产和 `.umap` 变化数为 0。
- **SC-004**: 26 Intent、10 Choice、12 explicit Part reward、7 Encounter 与 12 Card 的 frozen initial contract 全部通过 strict validation。
- **SC-005**: 三组第二次执行均为 `0 created / 0 saved / 0 failed`，47 文件聚合 SHA-256 不变。
- **SC-006**: 47/47 load、AssetRegistry、failed-load 和 forbidden closure 全部通过；三张只读依赖 hash 变化数为 0。
- **SC-007**: 默认 Unity 编译和全部列出的 focused tests 通过；任何 broad suite 外部失败被准确隔离和记录。
- **SC-008**: 交付只含共享 Editor 工具、测试、Spec/Docs 和 47 DataAsset；零运行时 schema、tag、map、Blueprint 或模块依赖变化。

## Assumptions

- Spec 017 的字段、顺序、初始值与 exact manifest 是资产权威。
- 首次验收使用 strict comparator；后续 structural validation 允许 presentation 与经批准的 balance tuning，但 seeder 永不覆盖。
- `run` endpoint 为 `ue_wacom_run:8140`，ThreadId 使用当前 Run 正式关卡任务身份。
- 没有可验证的 Floor 2 world，因此本轮 Automation/AssetRegistry 替代 PIE；真实 Golden Path 由场景轮完成。
