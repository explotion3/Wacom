# Feature Specification: Debug Shop 卡牌强化可玩竖切

**Feature Branch**: `codex/shop-card-upgrade-debug-vertical-slice`

**Created**: 2026-07-20

**Status**: Approved for implementation

**Input**: 在现有 `Shop.Snake` 上以隔离 Debug 内容跑通补金币、战斗、购买测试白卡、强化为蓝卡和返回 Run 的完整 Golden Path，并把正式 Shop Screen 升级为可复用购买/强化双页签。

## Wacom Rule Context

**Primary Domain**: Shop UI / Run presentation / Debug content authoring / Editor validation

**Rule Truth Docs**: `AGENTS.md`、`Docs/WacomData.md`、`Docs/WacomDataAuthoring.md`、`Docs/WacomRun.md`、`Docs/WacomApp.md`、`Docs/WacomUI.md`、`Docs/UnrealMCPWorkflow.md`。

**Expected Owning Modules**:

- `WacomRun` 保持 Spec 019 的强化规则真相，不新增规则字段。
- `WacomApp` 将 Shop Snapshot 编译为被动购买/强化 ViewData，并把玩家意图提交给 `URunSession`。
- `WacomEditor` 持有四个 Debug/UI Package 的定向 inspect/seed 服务。
- `WacomTests` 以独立小型 spec 覆盖表现、Screen flow、PIE 命令与资产合同。

**Non-goals / Boundaries**:

- 不实现强化事件、正式 Production 强化链、完整四阶内容或动态价格。
- 不修改 GameplayTag、SaveGame、Build.cs、模块依赖、地图、Floor、Host、Journey 或角色 StarterDeck。
- 不运行 `WacomRegenerateContent`、完整 `ShopBuilder` 或其它全量 builder。
- 不让 WBP 直接引用 Debug Card/Shop，不把测试资产带入 Production dependency closure。
- 用户 PIE 通过前不 stage、不 commit。

## User Scenarios & Testing

### User Story 1 - 在正式 Shop Screen 浏览强化候选 (Priority: P1)

作为玩家，我希望在同一个商店界面切换“购买/强化”，看到每张可强化实体卡的当前版本、下一版本、价格和规则差异，并清楚知道金币不足等不可用原因。

**Independent Test**: `Wacom.UI.Shop.UpgradePresentation` 从 transient Quote 编译稳定的候选 ViewData、差异摘要和中文不可用文案。

**Acceptance Scenarios**:

1. 同一 Definition 的两个实体实例分别显示，稳定键为 InstanceId。
2. 只有存在 NextDefinition 的 Quote 进入列表；金币不足仍显示但操作禁用。
3. 选择候选后同时显示当前/下一卡面，并列出稀有度、费用和 Effect Magnitude/Duration 变化。
4. 服务关闭时隐藏强化页并回落购买页。

### User Story 2 - 原子提交一次强化 (Priority: P1)

作为玩家，我希望点击内联强化按钮后，Screen 只提交缓存的 InstanceId 和前后 Definition guard，由 RunSession 权威重算并返回明确结果。

**Independent Test**: `Wacom.UI.Shop.UpgradeScreen` 覆盖成功、失败、过期、访问关闭、刷新保留/清除选择、Toast 和 Activate/Deactivate 对称性。

**Acceptance Scenarios**:

1. 成功显示“已强化：卡名（旧稀有度 → 新稀有度）”；同一强化族允许前后 DisplayName 相同，刷新后同一 Instance 若仍可升级则继续选中，否则清除为空状态。
2. 失败按稳定原因映射中文 warning Toast，UI 不自行改金币或卡牌。
3. 强化引发 Shop visit 关闭时，先应用 Exploration Resolution，再退出 Screen。
4. ESC/Back 与关闭按钮共用既有结束访问链，不重复结算。

### User Story 3 - 用隔离 Debug 内容完成 Golden Path (Priority: P2)

作为开发者，我希望通过一条 Editor-only PIE 命令获得准确启动金币，在现有 Debug Battle/Shop 路径中买入白卡并强化为蓝卡，验证真实资产、输入和返回 Run 生命周期。

**Independent Test**: `Wacom.UI.Shop.UpgradePIEValidation` 覆盖命令注册和资格策略；真实路线由用户 PIE 验收。

**Acceptance Scenarios**:

1. 命令只在 PIE、`Journey.Debug / Floor.Debug.01 / Node.Entry`、活动 Run 且无活动节点交互时把金币补到 3。
2. Debug Shop 保留原 24 个 Offer 顺序，末尾追加测试白卡 1 Gold，并启用 White/Blue/Yellow 为 2/3/4 Gold。
3. White/Blue 卡同族且结构一致，只有 Damage `3→5`、Poison `1→2` 和稀有度改变。
4. 第二次定向 seed 为 `0 created / 0 modified / 0 saved`。

## Functional Requirements

- **FR-001**: Shop UI 必须保持 passive；规则只来自 `FRunShopSnapshot` 与 `FRunShopCardUpgradeResult`。
- **FR-002**: 强化列表必须按 InstanceId 区分实体卡，刷新签名覆盖服务、金币、Quote、InstanceId 和前后 Definition。
- **FR-003**: Screen 必须保持当前页签和仍有效的选中 Instance；服务关闭或候选消失时安全回退。
- **FR-004**: 内联操作必须提交 Spec 019 的完整 stale guard，不提交 UI 计算价格。
- **FR-005**: 成功/失败必须通过 AppToast 展示稳定中文反馈。
- **FR-006**: WBP 必须继承 `UWacomShopScreen`，全局注册 `UI.Widget.ShopScreen`，且无 Debug 内容引用。
- **FR-007**: C++ fallback 必须提供同等可操作的购买/强化双页签，不依赖 WBP 才能工作。
- **FR-008**: 定向 seeder 只允许两张测试卡、Debug Shop 和 Shop WBP 四个 Package；错误 class/父类/结构时 fail closed。
- **FR-009**: Debug Shop 修改只能追加目标 Offer 和写入强化价格，不得重排或覆盖原 24 条。
- **FR-010**: 旧 `ShopBuilder` 的 Debug defaults 必须同步，但本轮不得执行它。
- **FR-011**: PIE 金币命令不得发卡、不得降金币、不得在 Entry 之外或活动交互中修改状态。
- **FR-012**: 用户完整 PIE 验收前不得提交 Spec 020。

## Success Criteria

- **SC-001**: 默认 Unity WacomEditor 编译通过。
- **SC-002**: 新三组 UI/PIE tests 与列出的 Run/Shop 回归全部通过。
- **SC-003**: 首次定向 seed 为 `3 created / 1 modified / 4 saved`，第二次为全零。
- **SC-004**: 两卡链、Debug Shop 25 Offer、价格表、WBP parent/binding/config、Blueprint compile 与 failed-load 全部通过。
- **SC-005**: 四个目标均为 Git LFS；地图、角色、生产内容和现有 CardView 哈希不变。
- **SC-006**: 用户 PIE 路线确认金币 `3→2→0`、首次交易 1 AP、强化 0 AP、返回 Run 输入/UI 正常。
