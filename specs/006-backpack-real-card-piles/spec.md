# Feature Specification: 背包真实卡牌牌堆与即时携带

**Feature Branch**: `codex/backpack-workspace`

**Created**: 2026-07-16

**Status**: Approved

**Input**: 用户希望折叠牌堆与展开牌堆都直接显示真实卡牌，并消除多卡携带时相对鼠标的明显延迟；已经确认折叠、展开、特殊身份卡、投影卡、负重卡与动态卡面效果的产品口径。

## Wacom Rule Context

**Primary Domain**: UI-App shell

**Rule Truth Docs**:
- [x] `AGENTS.md`
- [x] `Docs/Architecture.md`
- [x] `Docs/WacomUI.md`
- [x] `Docs/UI_Backpack_WBP_Binding.md`
- [x] `Docs/WacomUIFoundation.md`

**Expected Owning Module(s)**: `WacomApp`、`WacomEditor`、`WacomTests`

**Non-Goals / Boundaries**:
- 不改变 `WacomRun` 中的区域容量、卡牌归属、投影、移动、原子拒绝、销毁奖励或 revision 语义。
- 不改变 SaveGame schema、GameplayTag、卡牌 DataAsset 或内容制作字段。
- 不接入 Battle 的发牌、出牌、目标、溶解、卡槽或输入状态机。
- 不修改 DreamShader 材质实现；仅复用现有卡面表现能力。
- Cowart 目录中的用户内容不属于本功能。

**Open Rule Questions**: 无。折叠牌堆内容、展开排布、动态效果上限和携带跟随语义均已确认。

## User Scenarios & Testing

### User Story 1 - 直接看见牌堆中的真实卡牌 (Priority: P1)

玩家打开背包后，不需要进入另一个列表或识别抽象数字预览，就能在工作台内直接看见备战区、特殊区和负重区中的全部真实卡牌。牌堆折叠时紧凑排列，展开时在可用空间内形成可读、可操作的手风琴。

**Why this priority**: 这是本轮最直接的体验目标，也是移除原型缩略牌的最小可交付切片。

**Independent Test**: 使用包含 0、1、3、15、21 张卡的区域打开背包；检查折叠与展开状态下卡面身份、数量、顺序、尺寸和框体范围。

**Acceptance Scenarios**:

1. **Given** 备战区包含实体卡和投影卡，**When** 玩家打开背包，**Then** 折叠牌堆显示全部真实卡面，投影卡仍清楚标识为只读。
2. **Given** 特殊区包含主卡和内容卡，**When** 玩家展开牌堆，**Then** 主卡位于第一张并保持只读，内容卡随后排列并可按现有规则操作。
3. **Given** 牌堆包含最多约 21 张卡，**When** 玩家展开牌堆，**Then** 卡牌保持固定尺寸、无需滚动，并根据剩余空间向左或向右完整展开。
4. **Given** 牌堆处于折叠状态，**When** 玩家点击卡面区域，**Then** 整个牌堆展开而不是直接选中其中一张卡。

---

### User Story 2 - 多卡携带立即跟随鼠标 (Priority: P1)

玩家一次携带一张或多张卡时，整组卡牌作为一个扇形立即跟随鼠标；卡牌数量增加不会产生逐渐累积的滞后，也不会让工作台中无关卡牌反复刷新。

**Why this priority**: 当前延迟直接损害核心整理操作，必须和真实卡牌数量增加同时解决。

**Independent Test**: 分别携带 1、7、15、21 张卡快速移动鼠标；视觉锚点在下一界面帧抵达最新位置，且卡牌数量不改变跟随延迟。

**Acceptance Scenarios**:

1. **Given** 玩家已携带任意数量卡牌，**When** 鼠标移动，**Then** 整个扇形以同一个锚点跟随且没有人为平滑或累计落后。
2. **Given** 玩家快速把鼠标移出原卡牌范围，**When** 鼠标仍被背包捕获，**Then** 携带卡继续稳定跟随。
3. **Given** 玩家滚轮切换当前卡或释放部分卡牌，**When** 携带组发生结构变化，**Then** 扇形只在必要时重排，剩余卡继续停留在携带状态。
4. **Given** 玩家只移动鼠标而不改变携带结构，**When** 多个界面帧过去，**Then** 非携带卡的布局和卡面状态不被重复更新。

---

### User Story 3 - 保留高质量卡面反馈且控制开销 (Priority: P2)

玩家在展开牌堆中悬停卡牌或携带当前卡时，能看到与现有第一人称卡牌一致的清晰卡面和适量空间反馈；大量静置卡牌不会持续运行动态材质。

**Why this priority**: 真实卡面需要延续已有视觉质量，但不能重新引入 20 多张动态 Retainer 带来的性能风险。

**Independent Test**: 在折叠、展开、悬停、携带和取消状态间切换，确认静态卡保持清晰，最多只有一个 Backpack 卡面持续使用动态效果，结束交互后状态完全复位。

**Acceptance Scenarios**:

1. **Given** 所有牌堆都处于静置或折叠状态，**When** 玩家不与卡牌交互，**Then** 卡面保持正常不透明度且不持续运行动态表面效果。
2. **Given** 玩家悬停一张展开卡，**When** 没有进行携带，**Then** 该卡可显示 Fake3D、视差和接触阴影，其他卡保持静态。
3. **Given** 玩家携带多张卡，**When** 当前最前卡发生变化，**Then** 只有新的当前卡启用动态效果，旧卡立即恢复静态。
4. **Given** 投影卡、特殊区主卡或负重卡不可操作，**When** 它们显示在牌堆中，**Then** 只读身份清晰但不会被统一降低为错误的禁用透明度。

### Edge Cases

- 空牌堆保留可识别的标题、数量和可投放区域，但不生成虚假卡面。
- 牌堆接近工作台边缘时选择能完整容纳内容的展开方向，并把最终视觉边界夹紧在工作台内。
- Snapshot revision 在携带期间变化时，卡牌实例按稳定身份重用；失效卡牌安全移除，合法剩余卡不重复生成。
- CommonUI Activate/Deactivate、B 关闭和 Escape 分层取消必须清理悬停、实时卡面、鼠标捕获和携带层。
- 携带悬停自动展开、跨区移动、批量销毁、原子拒绝和部分释放沿用既有行为。
- 同一张卡因实体、投影或特殊主卡身份同时出现时，各视图保持独立身份和正确的只读状态。

## Requirements

### Functional Requirements

- **FR-001**: 备战区、特殊区和负重区 MUST 在折叠和展开状态下显示其全部真实卡牌，而不是缩略卡或最多三张预览。
- **FR-002**: 折叠实体卡 MUST 保持正常卡面和固定尺寸，但不得被单独选择、框选或携带；点击牌堆内容区域 MUST 展开牌堆。
- **FR-003**: 展开牌堆 MUST 使用固定卡面尺寸、自适应水平露出和轻微扇转，在约 21 张卡时无需滚动或动态缩小。
- **FR-004**: 备战投影卡 MUST 保持只读和投影标识；特殊区主卡 MUST 位于第一张并保持只读；负重卡 MUST 由牌堆显示锁定或压力警告。
- **FR-005**: 同时 MUST 最多展开一个普通牌堆或负重牌匣，并保留携带悬停自动展开行为。
- **FR-006**: 携带卡牌 MUST 使用单一视觉锚点跟随最新鼠标位置，不得使用引入累计延迟的人为平滑。
- **FR-007**: 单纯鼠标移动 MUST 只更新携带锚点，不得重新刷新工作台中非携带卡的布局或卡面状态。
- **FR-008**: 携带扇形 MUST 只在卡牌数量、当前卡、样式或有效几何发生变化时重新计算局部布局。
- **FR-009**: 展开 Hover 卡或当前携带最前卡 MAY 使用现有高质量空间卡面反馈；任意时刻最多一个 Backpack 卡面持续动态重绘。
- **FR-010**: 禁止交互 MUST 与只读身份、透明度和动态效果策略分离，避免折叠实体卡被错误显示为禁用卡。
- **FR-011**: Workspace Reconciler MUST 跨静态层和携带层稳定复用卡牌视图，避免 Snapshot 刷新产生重复 Widget。
- **FR-012**: 现有卡牌跨区移动、容量、投影、批量销毁、部分释放、原子拒绝、同 Run 布局保留与新 Run 清空 MUST 保持不变。
- **FR-013**: 正式制作路径 MUST 提供牌堆 WBP 绑定合同，并完全移除缩略牌运行时依赖和制作合同。
- **FR-014**: 功能 MUST 保持 Wacom 模块依赖方向，权威 Run 状态不得进入 Widget 或表现控制器。
- **FR-015**: 功能 MUST 更新长期 UI/绑定文档，并通过编译、Focused Automation、Builder 幂等和 PIE 表现验证。

### Wacom-Specific Requirements

- **Docs-first evidence**: 实现前读取 `Docs/WacomUI.md`、`Docs/UI_Backpack_WBP_Binding.md`、`Docs/WacomUIFoundation.md`、`Docs/Architecture.md`；完成后同步前三者及必要的 TODO/TechDebt。
- **Module/API boundary**: `WacomApp` 拥有 Workspace、牌堆布局、输入和卡面表现；`WacomEditor` 生成正式 WBP；`WacomTests` 验证合同。仅 FirstPersonCardView 的可复用渲染策略可作为稳定 UI 能力暴露，其余结构保持 App Private。
- **Data/GameplayTag impact**: 无新 DataAsset 字段、GameplayTag、卡牌定义或验证器。
- **Battle contract impact**: 无；仅复用被动卡面 View 能力，不复用 Battle Slot、规则事件或命令。
- **Run contract impact**: 无公共 API、FRunState、revision、PersistentId 或 SaveGame 变化。
- **UI/App boundary**: Screen 从 Run Snapshot 构造 Scene/ViewData；Workspace 是唯一输入 owner；牌堆和卡面只消费 ViewData 并发送意图；Activate/Deactivate 时统一清理瞬态状态和实时渲染。
- **Testing expectation**: `WacomEditor`、`Wacom.UI.Backpack`、`Wacom.Run.Backpack`、相关 FirstPersonCardView 测试、Builder 双运行与多分辨率 PIE。
- **Temporary debt**: 不接受重新引入 Tick 轮询、缩略卡或全工作台逐帧刷新；未完成的跨平台性能采样必须写入 `Docs/TechDebt.md`。

### Key Entities

- **Workspace Scene**: 一次背包 Snapshot 对应的瞬态卡牌、牌堆与投放目标表现集合，由 Screen/Reconciler 生成。
- **Card View Identity**: 由卡牌 InstanceId、OwnerInstanceId、物理区域和显示角色共同确定，可区分实体、投影、特殊主卡和内容卡。
- **Pile Layout**: 牌堆标题锚点、框体边界、展开方向及每张卡的固定尺寸布局。
- **Carry Presentation**: 携带卡牌集合、当前卡索引、鼠标锚点和局部扇形布局；不拥有权威卡牌归属。
- **Card Presentation Mode**: 卡牌的交互可用性、只读身份和静态/实时渲染策略。

## Success Criteria

### Measurable Outcomes

- **SC-001**: 玩家能在折叠牌堆中直接识别全部卡牌，并在一次点击后展开为可操作布局，无需进入额外列表或滚动容器。
- **SC-002**: 对 1、7、15、21 张携带卡，视觉锚点均在下一界面帧内到达最新鼠标位置，位置误差不超过 1px，卡牌数量不增加跟随帧数。
- **SC-003**: 21 张卡的折叠和展开布局保持相同卡面尺寸，所有卡牌与标题位于工作台可用边界内。
- **SC-004**: 静置时没有 Backpack 卡面持续动态重绘；Hover 或携带时最多一个卡面持续动态重绘。
- **SC-005**: 自动化证明普通鼠标移动不会重算携带扇形或更新非携带卡布局；既有跨区移动、拒绝、删除和关闭重开测试保持通过。
- **SC-006**: 正式 WBP、Builder 和文档中不再存在缩略牌运行时合同，关键生成资产连续构建两次保持稳定。
- **SC-007**: 目标分辨率 PIE 中折叠、展开、框选、携带、自动展开、跨区移动、销毁和关闭重开均可完成且无明显累计输入延迟。

## Assumptions

- 卡面逻辑尺寸保持 `220×320`，局部卡面缩放保持 `0.75`；本功能不把它们变成随 Workspace 高度变化的动态比例。
- 折叠牌堆默认露出 16px，可根据空间在 10–24px 内调整；展开牌堆露出 32–72px。
- 折叠卡不旋转；展开卡使用轻微扇转。
- 牌堆持久化位置继续以标题锚点为准，展开框体可以在其左右扩张。
- 同 Run 的牌堆位置、ZOrder 和展开项继续保存于现有 Workspace State Store，不写入 SaveGame。
- 现有 generated test cards 足以覆盖 0、1、3、7、15、21 张验证矩阵。
