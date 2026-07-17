# Feature Specification: Journey 成功结算与终局交接基线

**Feature Branch**: `codex/run-level-authoring-baseline`

**Created**: 2026-07-17

**Status**: Approved

**Input**: 在基线 `70023f91` 上实现可复用的 Journey 成功终局：静态终局定义、原子 Run Outcome/摘要/事件、SaveGame v5 持久语义、被动总结页与主菜单交接。

## Wacom Rule Context

**Primary Domain**: Run-exploration / Data authoring / Save-load / UI-App shell / Testing

**Rule Truth Docs**:

- [x] `AGENTS.md`
- [x] `Docs/Architecture.md`
- [x] `Docs/WacomRun.md`
- [x] `Docs/WacomMap.md`
- [x] `Docs/WacomDataAuthoring.md`
- [x] `Docs/WacomApp.md`
- [x] `Docs/AgentIntegrationWorkflow.md`

**Expected Owning Module(s)**: `WacomData` 定义终局，`WacomRun` 持有规则、状态和存档，`WacomApp` 负责总结表现与 travel，`WacomEditor` 校验制作合同，`WacomTests` 覆盖合同。

**Non-Goals / Boundaries**:

- 不实现失败、压力满或手指耗尽的统一总结页。
- 不实现 Journey History、Journey 选择页、多存档记录，也不开启实际自动存档或 Continue。
- 不创建 Production Journey/Floor/内容 Definition 或关卡，不修改任何 `uasset/umap`。
- 不修改 GameplayTag、Build.cs、模块依赖、Camp、跨层门槛、AP 规则或现有卡牌资产。
- 不运行 builder；不存在 Production Floor 3 时不伪造 Golden Path PIE 结果。

**Open Rule Questions**: 无。本轮 Outcome、终局优先级、摘要字段、存档迁移、总结确认行为和返回目标均已批准。

## User Scenarios & Testing

### User Story 1 - 制作人员声明唯一成功终局 (Priority: P1)

作为 Journey 制作人员，我需要在 Journey 定义中声明唯一终局，系统能区分合法 Production Guardian 与未配置的旧 Debug Journey，使正式内容不会用 Actor label 或硬编码 NodeId 判定成功。

**Why this priority**: 静态终局是 Run 结算与 App 交接共同依赖的权威身份。

**Independent Test**: `Wacom.Data.Map.Validation` 覆盖合法终局、最后一层/Encounter/Boss/可达/无出边/无 FloorEntrance 约束，并证明未配置终局只 warning。

**Acceptance Scenarios**:

1. **Given** Journey 配置最后一层可达、无出边的 Boss Encounter，**When** 执行 Data Validation，**Then** 终局合同通过。
2. **Given** 已配置终局落在前层、非 Encounter、非 Boss、不可达或仍有出边，**When** 初始化或校验，**Then** 系统拒绝该 Journey。
3. **Given** 旧 Debug/Authoring Journey 未配置终局，**When** 校验和启动，**Then** 产生 warning 但仍可运行，且不会自动成功。

---

### User Story 2 - Guardian 胜利原子完成 Journey (Priority: P1)

作为玩家，我需要终局 Guardian 的有效非撤离胜利在一次事务中结算奖励、节点、AP、Outcome 与摘要，并只触发一次成功事件。

**Why this priority**: 这是成功状态的规则真相，必须先于存档和 UI。

**Independent Test**: `Wacom.Run.JourneyCompletion` 与既有 Encounter/Result/notification 测试证明一次版本递增、一次广播、事件顺序、重复回调幂等和所有反例。

**Acceptance Scenarios**:

1. **Given** 有效终局 Encounter ticket，**When** 返回非撤离 Victory，**Then** 奖励、压力、经验、AP、Node Resolve、完成摘要与 `Succeeded` 原子提交，`JourneySucceeded` 是最后事件。
2. **Given** 同一胜利令战后压力达到失败线或属于 mutual destruction，**When** 结算，**Then** Guardian 胜利优先，Outcome 仍为 `Succeeded`，最终压力写入摘要。
3. **Given** 撤离、普通 Boss 胜利、Defeat、Undetermined、错误或 stale ticket，**When** 结算，**Then** 不产生 Journey 成功。
4. **Given** Run 已成功，**When** 再请求玩法写操作或重复战果，**Then** 零修改、零事件、零广播。

---

### User Story 3 - 成功结果具备稳定持久语义 (Priority: P2)

作为存档维护者，我需要 v5 明确保存 Outcome 与最近成功摘要，同时保持旧档迁移并阻止终态档被恢复为活动 Run。

**Why this priority**: 成功状态若只在内存或复用旧布尔值，会让未来 Continue/History 无法可靠演进。

**Independent Test**: `Wacom.Run.Save` 覆盖 v0→v5、v4 active/inactive、成功摘要 roundtrip、非法摘要和终态原子拒绝。

**Acceptance Scenarios**:

1. **Given** v4 active/inactive 存档，**When** 迁移到 v5，**Then** 分别得到 `InProgress`/`Failed` 且摘要为空。
2. **Given** v5 `Succeeded`，**When** 序列化，**Then** 必须有合法独立磁盘摘要；`InProgress` 不得携带摘要。
3. **Given** v5 `Succeeded` 或 `Failed` 档，**When** 尝试恢复为当前 Run，**Then** 原子拒绝且运行态不改变。

---

### User Story 4 - 总结后安全返回主菜单 (Priority: P2)

作为完成 Journey 的玩家，我需要在战斗镜头平滑返回后看到可操作的成功总结，并通过确认或 Back 返回主菜单；即使 Screen push 失败也不能被困在终局世界。

**Why this priority**: 规则成功必须有确定的 App 交接，但表现不能反向拥有 Run 状态。

**Independent Test**: `Wacom.UI.JourneySummary`、GameMode handoff、Battle return staging 与输入/焦点测试覆盖被动 ViewData、单次 intent、成功路径不恢复手牌/Toast、次帧 travel 及 push fallback。

**Acceptance Scenarios**:

1. **Given** GameMode 消费 `JourneySucceeded`，**When** 战斗 HUD 清理与 Return-to-Run staging 完成，**Then** 进入 `JourneySummary` 而不恢复探索手牌和交互 Toast。
2. **Given** 总结页可见，**When** 玩家点击继续或按 ESC/Back，**Then** Screen 只上报一次 intent，GameMode 拆除 PrimaryLayout 并在下一帧进入 `/Game/Wacom/Maps/L_MainMenu`。
3. **Given** 总结页 push 失败，**When** App 处理终局，**Then** 执行同样的主菜单 travel fallback。

### Edge Cases

- `DisplayName` 为空时总结标题回退 `JourneyId`。
- 已配置但非法的终局必须拒绝运行时初始化；未配置仅表示无自动成功能力。
- 成功只接受同一 active Encounter ticket 的非撤离 Victory；Node handle 必须精确匹配。
- `JourneySucceeded` 必须为本次 Resolution 最后事件，不能被后续压力失败事件覆盖。
- 完成摘要必须来自 working state 的最终事务结果，而不是提交前 Snapshot 或 UI 推断。
- 终态后的直接数值、经济、背包、活动和探索写入口必须保持零修改、零广播。
- UI push、焦点或 Blueprint 表现缺失不能阻塞返回主菜单；原生 C++ fallback 始终可用。
- 现阶段 Save 系统总开关保持关闭；v5 只冻结底层持久语义。

## Requirements

### Functional Requirements

- **FR-001**: `UWacomJourneyDefinition` MUST 提供可选 `DisplayName` 和唯一 `SuccessTerminalNode`；标题为空回退 JourneyId。
- **FR-002**: 已配置终局 MUST 位于最后一层、引用存在的 Encounter Boss、从 Entry 可达、无出边，且最后一层不得包含 FloorEntrance。
- **FR-003**: 未配置终局的 Debug/Authoring Journey MUST 可运行并产生 warning；已配置但非法终局 MUST 被 validator 和 runtime 初始化拒绝。
- **FR-004**: Run MUST 用 `ERunOutcome { InProgress, Succeeded, Failed }` 表达终态，并保留 `IsRunActive()` / `IsRunFailed()` 兼容查询。
- **FR-005**: `FRunCompletionSummary` MUST 固定包含 JourneyId、终局 Node handle、完成天数、进入/总 Floor 数、解决/总 Node 数和最终压力。
- **FR-006**: 仅同一终局 Encounter ticket 的非撤离 Victory MUST 触发成功；所有其它结果不得触发。
- **FR-007**: 战斗奖励、压力、经验、AP、节点解决、摘要与 Outcome MUST 在同一 working-state 事务提交，版本递增和状态广播各一次。
- **FR-008**: `JourneySucceeded` MUST 是成功 Resolution 最后事件，Node 为终局 handle，Detail 为 JourneyId。
- **FR-009**: 终局 Victory MUST 优先于本次战后压力失败线和 mutual destruction，最终压力仍进入摘要。
- **FR-010**: 成功后所有玩法写入口和重复战果 MUST 零修改、零事件、零广播；保存成功摘要仍允许。
- **FR-011**: Snapshot MUST 暴露 Outcome 与完成摘要，UI 只能读取该投影和事件。
- **FR-012**: SaveGame MUST 升级 v5，持久化 Outcome 和独立摘要结构，旧 `bRunActive` 仅作为 v4 迁移来源。
- **FR-013**: v4 active/inactive MUST 迁移为 InProgress/Failed；v5 Succeeded 必须有合法摘要，InProgress 不得携带摘要。
- **FR-014**: 终态存档 MUST 原子拒绝恢复为活动 Run；Save 系统总开关不得开启。
- **FR-015**: `WacomApp` MUST 提供 `JourneySummary` flow state、被动 ViewData、原生 fallback Screen 和 Blueprint 表现钩子，不新增 GameplayTag/WBP。
- **FR-016**: GameMode MUST 只消费 `JourneySucceeded`，在战斗返回 staging 完成后展示总结，成功路径不得恢复探索手牌或交互 Toast。
- **FR-017**: 确认与 Back MUST 上报同一单次 intent，由 GameMode 拆除 PrimaryLayout 并下一帧 travel 到 `L_MainMenu`；push 失败执行同样 fallback。
- **FR-018**: 必须同步长期 Docs，按 checkpoint 编译和定向测试，并记录哈希、跳过项与风险。

### Wacom-Specific Requirements

- **Docs-first evidence**: 更新 `Docs/WacomRun.md`、`WacomMap.md`、`WacomDataAuthoring.md`、`WacomApp.md`、`Architecture.md`、`TODO.md`、`Questions.md`。
- **Module/API boundary**: 静态定义在 Data，原子规则和存档在 Run，被动表现/流程在 App，Editor 只校验，Tests 只做 harness；不改依赖。
- **Data/GameplayTag impact**: 只增加 Journey DataAsset 字段和 validator；零 GameplayTag、零二进制资产。
- **Run contract impact**: 新 Outcome、Summary、Snapshot event；不改变 Command/Resolution 的结构语义或 AP/Camp/门槛规则。
- **UI/App boundary**: Screen 消费 ViewData 并上报 intent；GameMode 持有输入、焦点、staging、PrimaryLayout teardown 和 travel。
- **Testing expectation**: 每个 checkpoint 默认 Unity 编译并运行指定定向测试；所有 Unreal 命令带 `-NoDreamShaderEditorBridge`。
- **Temporary debt**: Defeat/压力/手指失败统一总结交接保留为明确后续，不用成功 UI 临时承载。

### Key Entities

- **Journey terminal definition**: Journey 标题与唯一成功终局 handle。
- **Run outcome**: InProgress/Succeeded/Failed 的单局状态机。
- **Completion summary**: 成功事务冻结的只读完成统计。
- **Success event**: 一次 Resolution 的最后一个 `JourneySucceeded` 通知。
- **Save v5 summary entry**: 与运行时结构解耦的磁盘摘要。
- **Journey summary ViewData**: App 层只读展示数据与单次 continue intent。

## Success Criteria

### Measurable Outcomes

- **SC-001**: 终局 validator 对全部 7 项静态约束给出确定结果，旧 Journey 缺失终局只 warning。
- **SC-002**: 有效终局胜利恰好递增一次版本、广播一次状态、发出一个末尾成功事件，并生成字段完整的摘要。
- **SC-003**: 撤离、非终局、Defeat、Undetermined、错误/stale/重复 ticket 共 7 类反例均不成功且无重复奖励。
- **SC-004**: 成功后受保护的探索、活动、商店、事件、拾取、背包、经济和直接数值写入均零修改、零广播。
- **SC-005**: Save v0→v5 连续迁移、v4 active/inactive、成功摘要 roundtrip、非法摘要和终态恢复拒绝均通过自动化。
- **SC-006**: 总结页确认、Back 和 push-failure 三条路径均只安排一次 `L_MainMenu` travel，成功 staging 不恢复手牌/Toast。
- **SC-007**: 默认 Unity `WacomEditor` 编译和全部受影响定向测试通过；`git diff --check`、AssetRegistry failed-load、Git/LFS 审计无异常。
- **SC-008**: 最终提交不包含 Content、Config、Build.cs、GameplayTag 或模块依赖改动，worktree/LFS 干净。

## Assumptions

- 项目是本地单玩家 Run；本轮 Runtime/Save 字段不复制为网络状态。
- 正式 Floor 3 终局资产尚未制作，因此规则与 App handoff 由自动化覆盖，Golden Path PIE 留给 Production 资产轮。
- 当前失败语义仍由现有压力/手指/Defeat 合同决定；本轮只把原始失败布尔状态迁移为 Outcome。
- 原生 C++ Screen 是长期 fallback，未来 WBP 只能替换表现，不能拥有成功规则或 travel。
