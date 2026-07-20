# Feature Specification: 商店卡牌强化规则与制作合同基线

**Feature Branch**: `codex/shop-card-upgrade-baseline`

**Created**: 2026-07-20

**Status**: Approved for implementation

**Input**: 在不创建正式 UI 或生产强化资产的前提下，为商店建立不可变卡牌强化链、原子强化交易、制作校验、身份兼容、存档兼容和可供后续 UI 消费的只读结果合同。

## Wacom Rule Context

**Primary Domain**: Data/card authoring / Run-exploration / Shop / UI-App integration / Save-load / Testing

**Rule Truth Docs**:

- [x] `AGENTS.md`
- [x] `Docs/Architecture.md`
- [x] `Docs/AgentIntegrationWorkflow.md`
- [x] `Docs/WacomData.md`
- [x] `Docs/WacomDataAuthoring.md`
- [x] `Docs/WacomRun.md`
- [x] `Docs/WacomBattle.md`
- [x] `Docs/WacomApp.md`
- [x] `Docs/TODO.md`、`Docs/Questions.md`、`Docs/Roadmap.md`、`Docs/TechDebt.md`

**Expected Owning Module(s)**: `WacomData` 持有不可变强化链与 Shop 静态服务配置；`WacomRun` 持有运行时 Quote、Command、Result、访问状态和原子事务；`WacomApp` 只传递静态配置并展示现有卡面经济值；`WacomEditor` 持有制作校验；`WacomTests` 覆盖合同。

**Non-Goals / Boundaries**:

- 不创建或修改 Card/Shop DataAsset、WBP、地图、Host、材质、卡面美术或其它二进制资产。
- 不实现正式 Shop 双页签、强化对比、确认对话框、AppToast 或玩家可操作的强化 UI。
- 不实现随机词条、动态价格、Camp 强化、商店状态存档、多人同步或 Production 强化数值。
- 不新增 GameplayTag，不修改 SaveGame schema、Build.cs、模块依赖或 WacomBattle 公共合同。
- 不运行 Builder，不保存二进制资产，不做 PIE。

**Open Rule Questions**: None。强化模型、资格、价格/AP、回收价值和 UI 分轮边界均已由用户批准。

## User Scenarios & Testing

### User Story 1 - 制作可验证的强化链 (Priority: P1)

作为内容设计人员，我需要用互相链接的独立卡牌版本表达强化，使每个版本保持不可变、身份可追踪，并在错误链进入正式内容前获得明确制作错误。

**Why this priority**: 静态链是运行时交易、存档和后续 UI 的共同数据真相。

**Independent Test**: `Wacom.Data.CardUpgrade` 使用 transient CardDefinition 覆盖合法四级链、跨级、循环、合流、结构漂移、非法卡种和缺少数值变化。

**Acceptance Scenarios**:

1. **Given** 同一强化族的 White/Blue/Yellow/Purple 四个独立版本，**When** 设计人员连接相邻版本，**Then** 每步可被识别为合法的单级强化且每个版本保留唯一 CardId。
2. **Given** 循环、跨级、合流、不同强化族或规则结构漂移，**When** 执行制作校验，**Then** 校验失败并指出具体链节点和原因。
3. **Given** 未配置强化字段的旧卡，**When** 加载或校验，**Then** 旧卡继续合法但不可强化。

---

### User Story 2 - 在商店原子强化精确卡牌实例 (Priority: P2)

作为玩家，我希望商店在服务已启用、链和价格合法且金币足够时，强化我选择的精确卡牌实例，并与买卡共享每次访问的首次交易 AP 规则。

**Why this priority**: 这是强化功能的权威规则切片，必须在正式 UI 之前可独立测试。

**Independent Test**: `Wacom.Run.Shop.CardUpgrade` 覆盖四个物理区域、重复 Definition 实例、连续强化、购买/强化 AP 顺序、阶段推进关闭和全部失败回滚。

**Acceptance Scenarios**:

1. **Given** 玩家拥有两个相同 Definition 的实例，**When** 提交其中一个 InstanceId 的合法强化，**Then** 只替换该实例的 Definition，并保留 InstanceId、区域、顺序和特殊区参战标记。
2. **Given** 本次访问尚无成功交易，**When** 首次买卡或强化成功，**Then** 原子消耗 1 AP；之后任意成功买卡或强化消耗 0 AP。
3. **Given** 服务关闭、价格缺失、金币不足、卡牌不属于玩家、选择已过期或 Run 已终结，**When** 提交强化，**Then** 返回稳定失败原因且状态、revision 和广播次数均不变化。
4. **Given** 同一实例仍有后续链且金币足够，**When** 在一次访问内连续强化，**Then** 每次按最新版本和对应价格重新校验并成功推进一层。

---

### User Story 3 - 跨系统保持强化身份与持久语义 (Priority: P3)

作为系统维护者，我需要强化后的卡牌继续满足按卡族表达的 Run 条件、进入战斗时采用新版本规则、按现有存档格式恢复，并使用统一的稀有度回收价值。

**Why this priority**: 防止强化造成任务支付软锁、存档丢版本或 UI 与规则经济值漂移。

**Independent Test**: Run workspace、Map requirement、world interaction、RunEvent payment、Battle initialization、Save v5 roundtrip 和 Card presentation 定向测试通过。

**Acceptance Scenarios**:

1. **Given** 条件按稳定强化族 CardId 配置，**When** 玩家持有任意强化版本，**Then** 该版本满足条件；按精确 Definition 配置的条件仍只接受该版本。
2. **Given** 已强化实例进入战斗，**When** 创建战斗卡，**Then** 使用强化版本的费用、稀有度和效果且不改变战斗公共协议。
3. **Given** 已强化 Run 被保存并恢复，**When** 使用现有 Save v5 roundtrip，**Then** InstanceId 和当前 Definition 版本保持不变。
4. **Given** White/Blue/Yellow/Purple/Intrinsic 卡，**When** 查询回收价值或卡面展示，**Then** 两处统一得到 `1/2/3/4/0`。

### Edge Cases

- `NextUpgradeDefinition` 为空、自引用、跨级、循环、超过四层、不同强化族或被多个前驱引用。
- 强化步骤只改变稀有度或表现字段，没有改变 BaseCost、Magnitude 或 Duration。
- Intrinsic、`Card.Run.*`、`Physique.Capacity > 0` 容器卡尝试进入强化链。
- 商店启用服务但价格表为空、重复稀有度、包含 Purple/Intrinsic 来源或负价格。
- UI Quote 生成后卡牌已被移动、删除、强化或替换；提交必须用预期前后 Definition 防止陈旧请求。
- 正式探索的首次交易使阶段推进；访问、activity ticket 与 token 必须同步关闭。
- legacy Run 没有正式探索结果时保持现有商店行为，不伪造成功的探索 Resolution。
- SaveGame 加载不到强化 Definition 时沿用现有原子拒绝，不能部分恢复。

## Requirements

### Functional Requirements

- **FR-001**: 卡牌强化必须由独立不可变 CardDefinition 版本和单向下一版本引用表达，运行时不得修改 DataAsset 字段。
- **FR-002**: 每个版本必须拥有唯一 CardId；同链版本必须共享非空稳定 UpgradeFamilyId；未加入链的旧卡可回退使用自身 CardId。
- **FR-003**: 合法链只允许 `White → Blue → Yellow → Purple` 单步推进，最多四层，且不得循环、分叉或合流。
- **FR-004**: Intrinsic、`Card.Run.*` 和 `Physique.Capacity > 0` 的容器卡不得加入强化链。
- **FR-005**: 相邻版本必须保持关键词、目标、Physique、效果结构、ZoneHook 和 Passive 一致；只允许调整 BaseCost、Effect/PerfectRelease Effect 的 Magnitude/Duration 与表现字段。
- **FR-006**: 每个相邻版本除稀有度和表现外，至少必须有一个 BaseCost、Magnitude 或 Duration 的实际变化。
- **FR-007**: ShopDefinition 必须可选择性启用强化服务，并按当前 White/Blue/Yellow 稀有度配置唯一非负价格；零价格合法。
- **FR-008**: 现有 ShopDefinition 默认关闭强化服务，旧 Actor 手工 Offers 路径同样默认关闭。
- **FR-009**: 商店快照必须为每个已拥有精确实例提供只读强化 Quote，包括前后 Definition、强化族、稀有度、价格和稳定不可用原因。
- **FR-010**: 强化命令必须包含 InstanceId 和预期前后 Definition；Run 层必须重新权威计算资格和价格。
- **FR-011**: 成功强化必须在一个 working-state 事务内扣金币并只替换目标实例的 Definition，保留身份、物理归属、顺序、SpecialZone 关系和参战标记。
- **FR-012**: 成功强化必须只推进一次状态、Backpack/Shop/Economy revision 和广播；任何失败必须零修改、零 revision、零广播。
- **FR-013**: 买卡和强化必须共享首次成功交易 AP 语义：首次 1 AP、后续 0 AP；首次交易造成阶段推进时访问立即关闭。
- **FR-014**: 现有 `bShopVisitHasPurchase`、`bHasPurchaseThisVisit`、`bFirstPurchaseThisVisit` 名称保持兼容，但长期文档必须声明其语义涵盖强化服务交易。
- **FR-015**: `AllowedCardDefinitions` 必须继续精确匹配；`AllowedCardIds` 必须匹配当前 CardId 或 UpgradeFamilyId。
- **FR-016**: 升级族匹配必须统一用于 Run workspace、Map owned-card requirement、world interaction 和 RunEvent card payment。
- **FR-017**: 强化后进入 Battle 必须自然使用新 Definition；不得修改 FBattleDeckEntry、Battle Snapshot、Command 或 ResultPacket。
- **FR-018**: SaveGame schema 保持 v5；现有 DefinitionAssetPath 必须足以 roundtrip 强化版本和 InstanceId。
- **FR-019**: 卡牌回收金币必须统一为 White/Blue/Yellow/Purple/Intrinsic=`1/2/3/4/0`，App 展示不得维护第二套映射。
- **FR-020**: 必须提供 Card/Shop 制作校验、聚焦自动化、默认 Unity 编译和现有 Card/Shop 资产只读加载/哈希审计。
- **FR-021**: 必须同步对应长期 Docs；验证结果、跳过项和风险持续写入 quickstart。
- **FR-022**: 用户审阅前不得 stage 或 commit；不得 merge main、push、运行 Builder 或保存二进制资产。

### Wacom-Specific Requirements

- **Docs-first evidence**: `WacomData*`、Run、Battle、App、Architecture、TODO/Questions/Roadmap/TechDebt 是长期真相。
- **Module/API boundary**: 静态 schema 在 WacomData；事务和运行时状态在 WacomRun；App 只传配置/读结果；深层结构校验在 WacomEditor。
- **Data/GameplayTag impact**: 增加 CardDefinition 与 ShopDefinition 可制作字段；零新 tag、零二进制资产修改。
- **Battle contract impact**: 零公共合同变化，Battle 初始化继续读取实例当前 Definition。
- **Run contract impact**: 新增 visit request、service state、Quote/Command/Result 和原子升级入口；FRunState 商店状态仍仅内存态。
- **UI/App boundary**: 本轮不新增操作 UI；未来 UI 只读 Quote 并提交 Command，现有 Shop Screen 保持 purchase-only fallback。
- **Testing expectation**: 每个 C++ checkpoint 默认 Unity 编译并运行对应聚焦 Automation；最终只读 AssetRegistry/failed-load/hash/LFS。
- **Temporary debt**: 正式 Shop UI、AppToast、Production 强化资产/数值和商店价格留给 Spec 020，不建立临时按钮。

### Key Entities

- **Card upgrade family**: 多个不可变 CardDefinition 版本组成的稳定族，身份为 UpgradeFamilyId。
- **Shop upgrade service**: ShopDefinition 上的可选静态价格表及 Run 内第一次访问冻结的运行时副本。
- **Upgrade quote**: 给后续被动 UI 的精确实例只读资格、前后版本、价格和原因。
- **Upgrade command/result**: 陈旧保护输入及原子事务结果。
- **Card instance**: InstanceId 和物理区域保持稳定，Definition 是可经权威强化事务切换的当前版本引用。

## Success Criteria

### Measurable Outcomes

- **SC-001**: 合法四级链全部通过；循环、合流、跨级、非法卡种和结构漂移样例全部被拒绝，漏检数为 0。
- **SC-002**: 对四个物理区域和同 Definition 多实例的强化测试均只改变指定实例，身份/区域变化数为 0。
- **SC-003**: 买卡/强化的全部顺序组合都满足首次 `1 AP`、后续 `0 AP`，阶段关闭行为无差异。
- **SC-004**: 所有失败场景的 RunState、三个 UI revision 和通知计数变化均为 0。
- **SC-005**: 升级族在四类 Run 条件中匹配，精确 Definition 仍不放宽；Battle 与 Save v5 使用强化版本。
- **SC-006**: 五档回收价值在 Run 规则与 App 卡面展示中完全一致。
- **SC-007**: 默认 Unity WacomEditor 编译和全部列出的聚焦测试通过，现有 Card/Shop 二进制哈希变化数为 0。
- **SC-008**: 交付只包含 C++、测试、Spec 和 Docs；GameplayTag、SaveGame 版本、Build.cs、uasset/umap 变化数均为 0。

## Assumptions

- 当前功能为单机运行时，不增加复制或 RPC。
- UpgradeFamilyId 是内容稳定身份而非 GameplayTag；DisplayName、描述和插画不参与稳定身份。
- 强化服务状态和商店库存一样暂不写入 SaveGame。
- 本轮没有玩家可操作的强化 UI 或真实强化资产，因此 Automation 与只读资产审计替代 PIE。
- Spec 020 冻结首批 Production 强化卡、各 Shop 价格和正式双页签 UI。
