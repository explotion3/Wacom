# Feature Specification: Backpack Workspace Refactor

**Feature Branch**: `main` *(feature directory: `003-backpack-workspace-refactor`)*

**Created**: 2026-07-13

**Status**: Draft

**Input**: 将现有背包重构为右侧常驻区域牌匣与中央单区域自由工作台；支持框选、扇形携带、逐张或整组释放、当前 Run 内自由布局、原子批量跨区移动与原子批量销毁。

## Wacom Rule Context *(mandatory)*

**Primary Domain**: Run-exploration / UI-App shell

**Rule Truth Docs**:
- [x] `AGENTS.md`
- [x] `Docs/Architecture.md`
- [x] `Docs/WacomRun.md`
- [x] `Docs/WacomUI.md`
- [x] `Docs/WacomUIFoundation.md`
- [x] `Docs/UI_Backpack_WBP_Binding.md`

**Expected Owning Module(s)**: `WacomRun`, `WacomApp`, `WacomTests`

**Non-Goals / Boundaries**:
- 不改变卡牌的战斗规则顺序、容量公式、负重公式、特殊区归属、入战规则或删牌金币单价。
- 自由布局只服务整理和表现，不成为任何规则排序或战斗出牌顺序的来源。
- 自由布局只在当前 Run 内保留，不写入磁盘 SaveGame；新 Run 从默认整理状态开始。
- 首版中央工作台使用有限画布，不提供无限画布平移或缩放。
- 首版不提供手动旋转和完整手柄式自由摆放；现有 CommonUI 返回、关闭和区域导航能力仍需保持可用。
- 不复制 Godot Demo、第三方 shader、资产或工程结构；它们只作为交互节奏和视觉目标参考。
- 不重新引入世界空间 WidgetComponent 手牌或旧版 Battle 2D hand 路径。

**Open Rule Questions**:
- 无。当前功能范围、输入语义、布局生命周期和批量事务语义已由用户确认。

## User Scenarios & Testing *(mandatory)*

### User Story 1 - 在区域牌匣与自由工作台之间整理卡牌 (Priority: P1)

玩家打开背包后，始终能看到右侧区域牌匣，并在中央只展开一个当前区域。玩家可切换区域、自由放置当前区域的可移动实体卡，并在离开该区域或暂时关闭背包后继续看到当前 Run 内保存的整理结果。

**Why this priority**: 这是新背包的信息架构和所有后续选择、携带、移动操作的基础。

**Independent Test**: 在一个包含备战区、通量内容区、特殊区和负重区的 Run 中打开背包，逐区切换并摆放卡牌；确认中央始终只有一个活动区域、右侧牌匣常驻、重新打开背包后布局仍在，并在新 Run 中恢复默认布局。

**Acceptance Scenarios**:

1. **Given** 当前 Run 存在多个可见存放区域，**When** 玩家打开背包，**Then** 右侧显示全部可用区域入口，中央仅显示一个活动区域。
2. **Given** 玩家已在活动区域摆放卡牌，**When** 玩家切换到另一地区后再切回，**Then** 原区域的卡牌位置、角度与前后层级保持不变。
3. **Given** 玩家已在当前 Run 中摆放卡牌，**When** 背包停用后在同一 Run 中重新启用，**Then** 布局恢复；当开始一个新 Run 时，旧布局不再存在。
4. **Given** 玩家将卡牌拖向画布边缘，**When** 放下卡牌，**Then** 每张卡仍至少约有 30% 保持在可见工作台内。
5. **Given** 当前区域卡牌已被打乱，**When** 玩家执行“整理全部”，**Then** 当前区域恢复清晰、直立且不重叠到无法辨认的默认排列。

---

### User Story 2 - 框选并以扇形携带卡牌 (Priority: P1)

玩家可以像操作桌面对象一样单选、多选和框选当前工作台中的卡牌。对已选卡牌再次按下左键时，卡牌无需等待鼠标移动就立即组成跟随鼠标的扇形；这次起手对应的左键释放只完成进入持续携带，不会误放卡牌。此后玩家可用滚轮改变当前牌，并逐张或整组放下。

**Why this priority**: 这是用户明确要求复刻的核心手感，也是批量移动和批量销毁的交互入口。

**Independent Test**: 在活动区域放置至少 8 张可移动卡，分别验证单击、Ctrl 增减选择、框选、Ctrl+A、携带起手保护、滚轮切换、逐张释放和整组释放，且全过程没有卡牌序号提示。

**Acceptance Scenarios**:

1. **Given** 工作台上有多张可移动实体卡，**When** 玩家在空白处拖出选择框，**Then** 以卡牌中心进入选择框作为命中标准选中对应卡牌。
2. **Given** 工作台已有选择，**When** 玩家普通单击未选卡牌、Ctrl 单击或 Ctrl 框选、单击空白处，**Then** 分别执行单选、增减选择和清空选择；Ctrl+A 选择活动区域全部可移动实体卡。
3. **Given** 玩家已选中至少一张牌，**When** 玩家不按 Ctrl 再次按下任意已选卡，**Then** 选中集合在 PointerDown 立即进入扇形携带，无需先移动鼠标；这次起手对应的第一次释放只消耗守卫，不放下任何牌。
4. **Given** 多张卡处于携带状态，**When** 扇形首次形成，**Then** 最右侧且层级最高的牌是默认当前可释放牌，但它不额外上抬。
5. **Given** 多张卡处于携带状态，**When** 玩家向上或向下滚动鼠标滚轮，**Then** 当前牌在扇形内向左或向右移动一位且在两端停止；只有非默认当前牌额外上抬。
6. **Given** 多张卡处于携带状态，**When** 玩家完成后续一次左键单击并释放到工作台、区域牌匣或销毁确认，**Then** 只处理当前牌，剩余牌保持稳定顺序并继续扇形携带；Run 事务刷新不得把剩余牌提前送回工作台。
7. **Given** 仍有卡牌处于携带状态，**When** 玩家完成一次右键单击并释放，**Then** 全部剩余卡牌一起放下并退出携带状态。
8. **Given** 选择或携带状态存在，**When** 玩家切换活动区域或关闭背包，**Then** 携带与选择被安全取消，已完成的布局不被破坏。
9. **Given** 玩家正在携带卡牌，**When** 玩家观察顶部、鼠标附近或卡牌扇形，**Then** 不显示 `3 / 8`、索引数字或同类当前序号提示。

---

### User Story 3 - 原子批量跨区移动与同区收拢 (Priority: P2)

玩家可以把携带中的当前卡或全部剩余卡放到右侧另一区域牌匣中，实现单张或批量跨区移动；也可以拖回当前区域牌匣，把这些卡恢复到该区域的默认整理状态。

**Why this priority**: 它把自由工作台与现有 Run 持有区规则连接起来，并避免逐张提交产生部分成功的错误状态。

**Independent Test**: 构造容量刚好足够、容量不足、包含失效卡、目标特殊区失效和同区收拢场景；确认成功时只产生一次完整结果，失败时所有卡仍留在原区且继续携带。

**Acceptance Scenarios**:

1. **Given** 携带集合的全部卡都能合法进入目标区域，**When** 玩家将全部卡释放到目标牌匣，**Then** 所有卡一起完成移动，相关容量、负重、修订号和刷新只呈现一次完整事务结果。
2. **Given** 携带集合中任意一张卡无法进入目标区域，**When** 玩家尝试批量释放，**Then** 没有卡发生移动，玩家收到明确失败反馈并保持携带状态。
3. **Given** 玩家只释放当前牌到另一区域，**When** 单卡移动合法，**Then** 该牌离开携带集合并进入目标区域，其余牌继续携带。
4. **Given** 玩家把当前牌或整组卡释放到当前活动区域牌匣，**When** 收拢完成，**Then** 不改变 Run 规则归属，只清除这些牌的自由布局并按默认布局重新整理。
5. **Given** 批量移动成功，**When** 两个区域刷新，**Then** 已移动卡只出现在目标物理区域；投影卡或只读展示不会被误当作可移动实体。

---

### User Story 4 - 原子批量销毁卡牌 (Priority: P2)

玩家可以把一张或多张携带卡放到销毁目标，看到一次针对整组卡的确认，并在确认后一次性销毁全部合法卡牌并获得合计金币。

**Why this priority**: 用户明确要求批量销毁；确认和原子性可避免误删或只删除部分选择。

**Independent Test**: 分别验证确认、取消、集合中包含不可销毁卡、卡牌在确认期间失效以及成功奖励合计；确认取消或失败后携带状态完整恢复。

**Acceptance Scenarios**:

1. **Given** 携带集合全部可销毁，**When** 玩家释放到销毁目标，**Then** 显示一次包含卡牌数量和预计总金币的整组确认。
2. **Given** 批量销毁确认已显示，**When** 玩家取消，**Then** 不销毁任何卡、不增加金币，并恢复原携带集合与当前牌。
3. **Given** 批量销毁确认已显示，**When** 玩家确认且提交时全部卡仍合法，**Then** 所有卡一次性销毁、金币按每张卡现有规则合计发放，并退出携带状态。
4. **Given** 集合中任意卡不可销毁或在确认期间变得失效，**When** 玩家确认提交，**Then** 不销毁任何卡、不发放金币，并恢复携带状态及明确失败反馈。

### Edge Cases

- 空选择不能进入携带；重复 InstanceId、无效 InstanceId、无 Definition 或已不在预期来源区的卡必须拒绝且不能产生部分结果。
- 投影卡、Snapshot 标记为只读的卡和不可移动 Owner 展示不参与单选、框选、Ctrl+A、携带、跨区移动或销毁。
- 工作台刷新时，仍存在且仍属于该区域的卡保留布局；移走、销毁或不再可见的卡清除布局记录；新出现的卡进入默认整理位置。
- 携带中逐张放下后，剩余卡保持稳定相对顺序并重新收紧扇形；当前索引若越界则夹紧到最近有效牌。
- 滚轮到达扇形两端时不循环；最右侧默认牌重新成为当前牌时取消额外上抬。
- 无效释放目标、容量变化、特殊区 Owner 消失或 Run 修订变化均不得静默放下或部分提交；失败后维持携带并提供可理解反馈。
- CommonUI Activate/Deactivate、屏幕重建和 Run Snapshot 刷新不得留下鼠标捕获、输入租约、悬浮详情或过期选择。
- 批量事务成功时只产生一次外部可见状态变更与刷新；失败时卡牌、金币、负重和区域修订均保持提交前状态。

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: 背包必须使用“右侧常驻区域牌匣 + 中央单一活动区域工作台”的主要结构。
- **FR-002**: 系统必须为当前 Run 中每个可展示区域维护独立的自由布局，并在区域切换和背包重新启用后恢复。
- **FR-003**: 自由布局必须仅包含归一化位置、表现角度和前后层级，不得改变规则顺序；它不得写入磁盘 SaveGame。
- **FR-004**: 工作台必须限制卡牌放置范围，使卡牌至少约 30% 保持可见，并提供恢复活动区域默认排列的“整理全部”操作。
- **FR-005**: 系统必须支持普通单选、Ctrl 增减选择、空白拖动框选、空白单击清空和 Ctrl+A 全选活动区可移动实体卡。
- **FR-006**: 系统必须排除投影卡和只读展示，避免其进入选择、携带、移动和销毁命令。
- **FR-007**: 携带必须使用持续扇形状态；不按 Ctrl 按下已选卡必须立即进入携带且不依赖移动阈值，起手对应的第一次左键释放只能完成进入持续携带，不能放下卡牌。未选卡普通单击仍只选择，拖动未选卡超过阈值可作为直接起手快捷方式。
- **FR-008**: 扇形默认当前牌必须是最右侧、最高层级牌，默认不额外上抬；滚轮切换后的非默认当前牌才上抬。
- **FR-009**: 携带期间不得显示当前牌索引、总数比值或类似 `3 / 8` 的提示。
- **FR-010**: 起手之后的左键单击释放必须只处理当前牌，右键单击释放必须处理全部剩余牌；左键成功提交后，未提交牌必须继续携带并刷新后续事务所需的 storage revision。
- **FR-011**: 跨区批量移动必须全成全败，提交前按整个集合校验来源身份、目标合法性、容量、特殊区 Owner，并要求请求携带的 storage revision 与当前 Run 严格一致。
- **FR-012**: 同区牌匣释放必须是表现层收拢，不得提交无意义的 Run 区域迁移或改变规则数组顺序。
- **FR-013**: 批量销毁必须使用一次整组确认并全成全败；确认提交时必须重新校验全部卡牌并严格匹配当前 storage revision，成功时合计现有单卡金币奖励，取消或失败时恢复携带。
- **FR-014**: 每次成功的批量规则事务必须只产生一次外部可见 Run 状态变更、修订推进和刷新；失败事务不得改变任何相关状态。
- **FR-015**: UI 必须只读取背包 Snapshot/ViewData、维护瞬态选择与布局表现并发送意图；区域移动、容量、负重、销毁和金币仍由 Run 规则入口决定。
- **FR-016**: 输入所有权必须覆盖 Activate/Deactivate、鼠标捕获、焦点、弹窗确认、区域切换和 Snapshot 刷新，且所有取消路径都能释放输入状态。
- **FR-017**: 现有单卡移动、单卡删牌、特殊区入战切换、卡牌详情、Toast 和 C++ fallback 能力必须被正式迁移或明确替代，不能形成并行冲突路径。
- **FR-018**: 系统必须保留 Wacom 模块依赖方向，只暴露稳定的批量 Run contract，并把布局、选择、扇形计算和输入状态留在 App 表现层。
- **FR-019**: 实现必须同步 `Docs/WacomRun.md`、`Docs/WacomUI.md` 与 `Docs/UI_Backpack_WBP_Binding.md`；未完成或延期内容写入 `Docs/TODO.md` 或 `Docs/TechDebt.md`。
- **FR-020**: 实现切片必须通过编辑器编译、聚焦 Run/UI 自动化契约测试与 PIE 手感验收，或明确记录无法执行的验证和风险。

### Wacom-Specific Requirements

- **Docs-first evidence**: 已读取 `AGENTS.md`、`Docs/Architecture.md`、`Docs/WacomRun.md`、`Docs/WacomUI.md`、`Docs/WacomUIFoundation.md`、`Docs/UI_Backpack_WBP_Binding.md`；实现后更新后三份领域/绑定文档及必要的 TODO/TechDebt。
- **Module/API boundary**: `WacomRun` 增加原子批量移动与批量删牌公开契约；具体预检、提交和回滚算法留在其私有规则实现。`WacomApp` 负责工作台布局、选择、携带、输入和视觉反馈的私有协调，不成为 Run 规则所有者。
- **Data/GameplayTag impact**: 不新增卡牌 DataAsset 字段或 GameplayTag；现有卡牌 Definition、InstanceId、区域类型和奖励规则继续使用。
- **Battle contract impact**: 无。自由布局和背包整理不改变 Battle Snapshot、战斗规则顺序或 first-person battle hand。
- **Run contract impact**: 增加按 InstanceId 集合提交的原子移动/销毁 validation 与 result contract；成功仅一次修订/广播，失败零修改。自由布局为当前 Run 关联的非 SaveGame 表现状态。
- **UI/App boundary**: 背包读取 `FRunBackpackStorageSnapshot` 或其演进 ViewData；Screen/coordinator 接收 Widget 意图并调用 Run batch API。Activate 时订阅与获取输入，Deactivate 时取消携带/选择、关闭确认、释放捕获并反订阅。
- **Testing expectation**: 在 production App Widget public contract 中提供非反射、只读 automation view，再由 `Source/WacomTests/Private/UI/Backpack/` access wrapper 访问；新的小型 Run/UI spec 覆盖原子事务、选择/携带状态机、布局生命周期与 CommonUI 取消路径。运行 `Wacom.Run.Backpack`、`Wacom.UI.Backpack` 或新增更聚焦前缀，并进行 PIE 手感验收。
- **Temporary debt**: 目标正式实现不引入临时双轨。若 WBP/材质美术制作分阶段交付，必须在 `Docs/TODO.md` 写清剩余资产、验收入口与移除 fallback 的条件。

### Key Entities

- **Active Workspace Region**: 当前中央展开的存放区域身份，由区域种类和可选 Owner InstanceId 唯一确定。
- **Workspace Layout Entry**: 以 Card InstanceId 为键的当前 Run 表现记录，包含归一化位置、表现角度和层级；不属于 Run 规则或 SaveGame。
- **Workspace Selection**: 活动区域内可移动实体卡 InstanceId 的有序集合；区域切换或 Screen 停用时清空。
- **Carry Session**: 一次持续携带状态，记录稳定的剩余卡顺序、默认最右牌、当前牌和起手释放保护状态。
- **Batch Move Request/Result**: 一组唯一 InstanceId、预期来源、目标区域及一次性 validation/result；成功全体移动，失败零修改。
- **Batch Delete Request/Preview/Result**: 一组唯一 InstanceId、预计数量/总奖励及确认后的原子结果。
- **Card Identity**: `FCardInstance.InstanceId` 是布局、选择和事务的稳定身份；Definition 或屏幕 Widget 地址不得替代该身份。

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: 玩家在包含至少 20 张卡和 4 个可用区域的背包中，能够在 10 秒内切换区域并定位中央唯一活动工作台，没有重复展开区块。
- **SC-002**: 玩家可以对至少 15 张卡完成框选、扇形携带、滚轮选牌、逐张放下和整组放下；50 次起手测试中第一次左键释放误放次数为 0。
- **SC-003**: 100 次包含容量不足、失效卡和目标变化的批量移动/销毁自动化运行中，部分成功次数为 0，失败后的卡牌、金币、负重和修订状态全部保持提交前值。
- **SC-004**: 区域切换和同一 Run 内背包重开后，100% 的仍有效布局条目恢复；新 Run 中旧布局恢复率为 0%。
- **SC-005**: 所有放下后的卡牌至少约 30% 可见，且玩家可用一次“整理全部”恢复当前区域默认排列。
- **SC-006**: 携带 UI 中不存在 `3 / 8` 或其它索引比值；默认最右牌无抬升，滚轮选中的非默认牌具有清晰抬升反馈。
- **SC-007**: 聚焦 Run/UI 自动化覆盖原子事务、输入取消、布局清理和状态机边界；WacomEditor 编译通过，PIE 清单完成且无阻塞级输入或刷新问题。
- **SC-008**: 没有新增模块反向依赖、Widget 规则写入或第三方代码/资产复制；长期事实已回写到相关 Docs。

## Assumptions

- 首版主要交互设备是鼠标和键盘；游戏手柄仍可执行背包返回、关闭及常规区域导航，完整手柄自由工作台操作延期。
- 框选采用“卡牌中心进入选择框”作为确定性命中标准，避免旋转卡牌边缘轻微相交造成误选。
- 滚轮向上从默认最右牌向左选择，向下向右选择；到两端停止，不循环。
- 整组放到工作台时保留当前扇形位置、角度和相对层级；首版不提供手动旋转。
- 最近被放下或直接操作的卡牌进入可见前层；层级只服务表现。
- 批量销毁确认显示卡牌数量与预计总金币，不逐张弹窗。
- 当前 C++ fallback 作为资产缺失的安全路径保留到正式 WBP 完成验收；不得继续承担第二套规则或输入状态机。
