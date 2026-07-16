# Feature Specification: Run 正式关卡制作基线收口

**Feature Branch**: `[006-run-level-authoring-baseline]`

**Created**: 2026-07-16

**Status**: Draft

**Input**: User description: "收口 Run 正式关卡制作基线，使正式探索关卡可安全手工制作，调试生成与正式内容隔离，并建立可重复执行的场景绑定验证。"

## Wacom Rule Context *(mandatory)*

**Primary Domain**: Run-exploration / Data-card authoring / Architecture / Testing

**Rule Truth Docs**:
- [x] `AGENTS.md`
- [x] `Docs/Architecture.md`
- [x] `Docs/WacomMap.md`
- [x] `Docs/WacomApp.md`
- [x] `Docs/WacomDataAuthoring.md`

**Expected Owning Module(s)**: WacomData / WacomApp / WacomEditor / WacomTests

**Non-Goals / Boundaries**:
- 不定义正式第一层的最终节点数量、节点类型配比、稳定 NodeId 或美术布局。
- 不实现跨层入口、Camp 内容、压力曲线、正式地图内容或旅程存档。
- 不修改 `WacomRun` Snapshot、Command、Resolution、Traversal Ticket 或探索规则。
- 不修改战斗、商店、RunEvent、背包、first-person 手牌的既有玩家语义。
- 不新增模块，不改变 `Build.cs` 依赖方向。

**Open Rule Questions**:
- 无。本切片将正式第一层内容设计留在后续独立功能中。

## User Scenarios & Testing *(mandatory)*

### User Story 1 - 安全制作正式探索关卡 (Priority: P1)

作为关卡制作人员，我可以直接编辑正式 Run 关卡中的节点、道路、交互宿主和场景美术，并确信任何调试资产生成流程都不会删除、重建或覆盖这些人工调整。

**Why this priority**: 这是后续正式关卡制作的前提；如果正式关卡仍由调试生成器拥有，任何内容投入都可能在一次工具运行后丢失。

**Independent Test**: 记录正式关卡及其正式制作依赖的内容哈希，连续运行调试构建两次后确认哈希、Actor 身份和人工布局均未变化。

**Acceptance Scenarios**:

1. **Given** 正式探索关卡已经包含人工调整的节点、道路和美术 Actor，**When** 开发人员运行 Run 调试内容构建，**Then** 正式关卡及其制作依赖保持逐文件不变。
2. **Given** 正式探索关卡被独立加载，**When** 运行时准备绑定当前 Floor，**Then** 关卡通过唯一、显式的 Floor 声明确定数据来源，不从 Actor 集合猜测所属 Floor。
3. **Given** 关卡缺少 Floor 声明、存在重复声明、声明为空或与当前 Run Snapshot 不匹配，**When** 运行时准备场景绑定，**Then** 整次绑定被原子拒绝，既有运行状态和表现绑定不被部分替换。
4. **Given** 当前原型图被迁移为制作基线，**When** 策划开始正式第一层设计，**Then** 该基线不会被文档或资产语义误认为最终第一层或稳定存档身份。

---

### User Story 2 - 在进入 PIE 前验证当前 Run Floor (Priority: P2)

作为关卡制作人员，我可以对当前打开的 Run Floor 执行一次只读验证，并得到可定位到具体场景声明、节点、道路、分支或交互宿主的诊断，从而在进入 PIE 前发现绑定错误。

**Why this priority**: Run 场景绑定错误通常在移动、分支选择或活动返回时才暴露；制作期验证可以显著缩短反馈路径，并避免把坏场景交给后续系统调试。

**Independent Test**: 对有效场景、缺失或重复声明、节点/道路数量不符、错误身份、无效样条和端点偏差样例运行同一验证入口，确认结果、严重度和定位信息稳定。

**Acceptance Scenarios**:

1. **Given** 当前关卡与其 Floor 数据完整一致，**When** 制作人员执行当前 Run Floor 验证，**Then** 验证通过且不会把任何 Package 标记为脏。
2. **Given** 当前关卡存在阻止安全绑定的问题，**When** 执行验证，**Then** 返回失败并报告可操作的具体原因，而不是修改场景尝试修复。
3. **Given** 道路端点存在轻微制作偏差，**When** 执行验证，**Then** 返回警告；**Given** 偏差足以破坏导航语义，**Then** 返回错误。
4. **Given** 自动化或持续集成环境只有地图路径，**When** 使用同一验证合同执行无界面检查，**Then** 能通过稳定退出码区分通过、合同失败和参数/加载失败。

---

### User Story 3 - 独立重建 Run 调试夹具 (Priority: P3)

作为开发人员，我可以重复生成一个与正式关卡完全隔离的 Run 调试场景，用于自动化、路径与返回流程回归，而不要求关卡制作人员维护调试 Actor。

**Why this priority**: 自动化仍需要确定性的场景夹具，但该便利不能继续以覆盖正式关卡和共享资产为代价。

**Independent Test**: 在相同输入下连续执行调试构建两次，确认调试地图及其专属数据稳定，随后分别验证正式和调试场景均满足绑定合同。

**Acceptance Scenarios**:

1. **Given** 调试内容不存在或处于旧版本，**When** 运行调试构建，**Then** 只创建或更新调试命名空间中的地图、数据和配置资产。
2. **Given** 调试内容已经是最新版本，**When** 再次运行构建，**Then** 结果幂等，不新增重复 Actor、映射或引用。
3. **Given** 正式关卡、玩家角色或共享 Run Actor Blueprint 已被人工调整，**When** 运行调试构建，**Then** 这些共享或正式资产不被保存、重新编译或修改。
4. **Given** 调试构建需要的共享依赖缺失或父类错误，**When** 运行构建，**Then** 构建明确失败并报告依赖问题，不擅自重建共享资产。

### Edge Cases

- 关卡中不存在 Floor 声明、存在多个声明、声明指向空资产或错误 Floor。
- 当前 Run Snapshot 在场景扫描期间发生版本变化，或当前 activity 不允许建立探索绑定。
- Floor 数据包含节点或 Edge，但场景中对应 Anchor、Path、BranchTarget、activity host 缺失、重复或身份错误。
- Path 样条点不足、长度接近零、Transform 含非有限值、源终点颠倒或端点偏离 Anchor。
- 单出口节点错误包含分支选择入口，多出口节点缺失合法 Edge 的入口。
- 调试生成中途失败、依赖缺失或目标 Package 无法保存；失败不得波及正式资产。
- 正式关卡被单独加载、通过 Run 流程加载或在 Battle/Shop/RunEvent 返回后重新绑定时，均使用同一 Floor 声明合同。

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: 系统 MUST 将正式探索关卡定义为人工制作资产，任何 Run 调试生成入口均不得修改、保存或重新生成该关卡。
- **FR-002**: 每个可独立加载的 Run Floor 关卡 MUST 且只能声明一个关联的 Floor 数据资产，且声明不得复制节点图、规则门槛或目标关卡信息。
- **FR-003**: 运行时 MUST 在枚举和应用场景绑定前验证声明唯一、引用有效且 Floor 身份与当前显式 Snapshot 一致。
- **FR-004**: 声明或场景绑定预检失败 MUST 原子拒绝；不得留下半绑定的节点、道路、交互宿主、镜头或 HUD 状态。
- **FR-005**: 系统 MUST 提供与正式第一层身份分离的过渡制作基线；其节点图可供继续制作，但不得被视为最终内容或稳定存档身份。
- **FR-006**: 系统 MUST 提供独立的 Run 调试地图及专属数据/配置，使调试构建不依赖修改正式关卡或默认正式 GameMode 配置。
- **FR-007**: 调试构建 MUST 只拥有调试命名空间中的生成资产；共享 Actor Blueprint、玩家角色 Blueprint 和正式配置只能作为只读依赖。
- **FR-008**: 调试构建 MUST 可重复执行并产生确定结果；依赖缺失或不合法时 MUST 明确失败而不是创建或覆盖共享替代品。
- **FR-009**: 系统 MUST 提供一套可被编辑器交互入口、无界面命令和自动化共同使用的只读 Run Floor 场景验证合同。
- **FR-010**: 场景验证 MUST 覆盖 Floor 声明、节点/道路/分支/活动宿主身份与数量、样条有效性、方向及端点偏差。
- **FR-011**: 场景验证 MUST 为每条诊断提供稳定严重度、稳定原因类别和可定位的对象信息。
- **FR-012**: 无界面验证 MUST 使用稳定退出结果区分验证通过、场景合同失败和参数/地图/声明解析失败。
- **FR-013**: 场景验证 MUST 严格只读，不修改 Actor、不调用保存、不把 Package 标记为脏。
- **FR-014**: 迁移当前正式关卡时 MUST 保留已有 Actor 身份、Transform、Spline、交互宿主和场景美术，仅移除调试生成器对它们的所有权标记。
- **FR-015**: 系统 MUST 保持现有 Run 规则、Traversal Ticket、Battle/Shop/RunEvent 往返和 first-person 表现合同不变。
- **FR-016**: 实现 MUST 保持既有 Wacom 模块依赖方向，不新增模块或 `Build.cs` 依赖。
- **FR-017**: 实现 MUST 将已落地的关卡制作、绑定和验证事实同步到对应长期 `Docs/` 文档，并把正式第一层内容设计保留为明确后续工作。
- **FR-018**: 实现 MUST 通过编译、定向自动化、调试构建幂等检查、Blueprint 编译和正式关卡 PIE 回归，或明确记录无法执行的人工验收风险。

### Wacom-Specific Requirements *(include as applicable)*

- **Docs-first evidence**: 实现前读取 `Docs/WacomMap.md`、`Docs/WacomApp.md`、`Docs/WacomDataAuthoring.md`、`Docs/Architecture.md`；完成后同步这些文档以及 `Docs/TODO.md`、必要时的 `Docs/Questions.md`。
- **Module/API boundary**: Floor 静态数据继续属于 WacomData；场景声明和运行时绑定属于 WacomApp；制作工具与验证属于 WacomEditor；测试只进入 WacomTests。WacomRun 公共规则合同不变。
- **Data/GameplayTag impact**: 增加过渡制作基线 DataAsset 和调试专属资产；不新增 GameplayTag，不改变现有地图 DataAsset schema。
- **Battle contract impact**: N/A；战斗初始化、命令结果和 HUD 表现合同不变。
- **Run contract impact**: 不修改 `URunSession`、`FRunState`、Snapshot、Command、Resolution、PersistentId 或 SaveGame schema。
- **UI/App boundary**: PlayerController 在任何场景 Actor 枚举前消费只读 Floor 声明并执行原子预检；失败只产生诊断，不让场景 Actor 修改规则。
- **Testing expectation**: 新增小型场景声明与验证 spec；运行 Run scene-binding/path/map 与 Battle/Shop/RunEvent return 定向回归、`WacomEditor` 编译、Blueprint 全量编译和 PIE 黄金路径。
- **Temporary debt**: 过渡制作基线不是正式内容；正式第一层图、稳定 NodeId 和存档身份保留在 `Docs/TODO.md` / `Docs/Questions.md`，不伪装成已完成规则。

### Key Entities *(include if feature involves data/state)*

- **Run Floor Scene Declaration**: 单个可加载 Run Floor 对其唯一 Floor 数据来源的只读声明；不持有图副本或运行时规则状态。
- **Authoring Baseline Floor**: 与 Debug 和未来正式 Floor 身份分离的过渡制作数据，用于承接当前可运行图并允许人工继续制作。
- **Debug Run Fixture**: 由工具拥有的专属地图、数据和配置集合，只服务自动化与开发回归。
- **Scene Binding Diagnostic**: 包含稳定严重度、原因类别、对象定位和说明的只读验证结果。
- **Identity**: FloorId、NodeId、EdgeId、ActivityId 继续来自 Floor 数据；场景 Actor 只声明与这些身份的绑定。

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: 连续运行调试构建两次后，正式 `L_Exploration`、过渡制作数据、正式 GameMode、玩家角色和共享 Run Blueprint 的内容哈希全部保持不变。
- **SC-002**: 有效正式关卡与有效调试关卡均通过同一场景验证；缺失、重复、空引用和 Floor 不匹配声明均被稳定拒绝。
- **SC-003**: 场景验证覆盖节点、道路、分支、活动宿主和样条几何，并能用稳定退出结果供无界面流程判定成功或失败。
- **SC-004**: 运行任意验证入口前后，目标地图及其依赖 Package 均未新增脏状态。
- **SC-005**: `WacomEditor` 编译、定向 Run/Battle/Shop/RunEvent 回归和 Blueprint 全量编译无新增失败。
- **SC-006**: PIE 中首次进入 Run、单/多出口移动、地图传送、进入并返回 Battle/Shop/RunEvent 后仍能重新绑定正确关卡、HUD、镜头和 first-person 手牌。
- **SC-007**: 长期文档明确区分正式人工场景、过渡制作基线和调试夹具，且未把当前原型图承诺为最终第一层。

## Assumptions

- 当前 `L_Exploration` 的可运行 Actor、Spline、交互宿主和美术可作为迁移起点，但其图结构仍是可替换的制作基线。
- 一个独立加载的 Run Floor 关卡只对应一个 Floor 数据资产；正式跨层和子关卡组合在后续内容设计中另行扩展。
- 现有共享 Run Actor Blueprint 的父类和运行时合同可复用，调试构建不再拥有这些资产。
- 当前阶段不启用正式旅程存档，因此过渡制作基线身份不会写入长期玩家档案。
- Spec Kit 工件用于组织实施；最终规则真相仍必须回写 `Docs/`。
