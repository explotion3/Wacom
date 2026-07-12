# Research: Backpack Workspace Refactor

## Decision 1 — Use an explicit persistent carry state machine

**Decision**: 新工作台不以 `UDragDropOperation` 作为输入生命周期所有者。使用 Screen/Workspace 协调的显式状态机：Idle → Marquee/Pressed → Carrying → ConfirmingDelete/Commit → Carrying 或 Idle。

**Rationale**: 需求明确要求第一次左键 release 之后仍继续携带，后续左键逐张放下、右键全放。标准 UMG drag operation 把 release/drop 当成一次拖拽终点，继续扩展会产生 dirty-release、鼠标捕获和双状态机冲突。显式状态机可测试、可统一取消，并能在 CommonUI Deactivate 时确定释放输入。

**Alternatives considered**:
- 扩展现有 `UWacomCardDragOperation` 并在 drop 后立即启动下一次 drag：会出现首帧鼠标捕获、视觉重建和 release 重入问题，拒绝。
- 让每张 CardWidget 自己持有 carry：多选扇形和批量命令需要跨 Widget 协调，容易形成分布式状态，拒绝。

## Decision 2 — Run batch operations use working-copy commit

**Decision**: `FRunDeckRules` 在 `FRunState` working copy 上对唯一 InstanceId 集合执行来源核对、现有单卡规则验证与变更；任一步失败即丢弃 working copy。全部成功后由 `URunSession` 一次替换权威状态并一次通知。

**Rationale**: 现有单卡 `MoveInstance`/`DeleteCardForGoldByInstance` 每次都会改变状态、重算并广播。UI 循环调用无法保证回滚，也会暴露中间状态。Working-copy 复用现有规则并天然提供失败零修改；批量请求来自一个活动工作区，拥有共同预期来源和单一目标，事务语义清楚。

Workspace 请求必须携带起手/确认前 Snapshot 的 storage revision，并在提交时与当前 Run 严格相等；任何 revision 漂移都先整体拒绝，再由 UI 刷新后重新发起，不提供 Workspace opt-out。

**Alternatives considered**:
- UI 先逐张 `Validate` 再逐张提交：validation 与提交之间状态可变化，仍可能部分成功，拒绝。
- 在真实 state 上执行并手写逆操作：特殊区、负重、Owner entry 和金币回滚容易遗漏，拒绝。
- 只增加“批量循环”而不返回统一 result：无法稳定显示失败原因、影响数量和总奖励，拒绝。

## Decision 3 — Layout is App-owned transient Run-scoped state

**Decision**: 用 `WacomApp` 瞬态 workspace state subsystem/store 保存 `{Run identity, ZoneKey, InstanceId -> LayoutEntry}`。同一 Run 的 Screen 实例可恢复；绑定到新 Run 时清空；不写 `FRunState` 或 SaveGame。

**Rationale**: 用户确认布局纯粹用于整理且只在当前 Run 保留。Screen 内字段不足以覆盖 Screen 重建；Run/SaveGame 又会错误地把表现数据提升为规则/存档事实。App subsystem/store 是合适生命周期边界。

**Alternatives considered**:
- 放在 `UWacomBackpackScreen`：新建 Screen 时丢失，拒绝。
- 放在 `URunSession`/`FRunState`：污染规则和 SaveGame 语义，拒绝。
- 使用全局静态 map：生命周期和 GC 不清晰，测试互相污染，拒绝。

## Decision 4 — One zone key and stable InstanceId drive reconciliation

**Decision**: 活动区域用 `{EZoneKind, optional OwnerInstanceId}` 标识。Card layout、selection 和 carry 一律以 `FCardInstance.InstanceId` 为身份；Widget 地址与 Definition 只用于展示。

**Rationale**: SpecialZone 需要 OwnerInstanceId 区分，普通区域应规范化为 invalid owner。InstanceId 已是现有 Run 查找与 Snapshot 的稳定身份，能在 refresh 后复用 Widget 和布局。

**Alternatives considered**:
- 以数组 index 保存布局：跨区、负重重算和 Snapshot reorder 会错位，拒绝。
- 以 Definition 保存：同卡多实例冲突，拒绝。

## Decision 5 — Selection uses card-center marquee hit testing

**Decision**: 框选命中采用卡牌渲染中心落入框内；普通 click 单选、Ctrl click/marquee toggle、blank click clear、Ctrl+A 选择活动区全部 movable physical cards。

**Rationale**: 卡牌可有旋转和重叠，中心点规则确定、低成本且不因边缘轻微相交误选。movable flag 必须由 ViewData/Screen 筛选，投影和只读卡不进入 interaction model。

**Alternatives considered**:
- 任意矩形相交即选：在扇形和重叠布局中误选率高，拒绝。
- 精确旋转多边形相交：首版收益不足，且增加 Slate geometry 复杂度，拒绝。

## Decision 6 — Layout and fan motion are separate from rules

**Decision**: Layout solver 负责默认排列、30% 可见 clamp、扇形 transform 和 ZOrder；presentation style 负责尺寸、角度、lift、跟随和回弹参数。规则层只看到 InstanceId batch intent。

**Rationale**: 允许后续更换扇形、牌匣视觉或 DreamShader 效果，而不改 Run contract。动画只在活动过渡时更新，不 Tick 轮询 Snapshot。

**Alternatives considered**:
- 把扇形 transform 存入 Snapshot：把表现参数扩散到 Run，拒绝。
- 在 Screen 中硬编码 Demo 数值：无法复用和制作调参，拒绝。

## Decision 7 — Same-zone rack is collect, cross-zone rack is transaction

**Decision**: Drop target 与活动 ZoneKey 相同则删除所选 layout entry 并回到默认整理位置，不调用 Run API。不同 ZoneKey 才构建 atomic batch move request。

**Rationale**: 同区 `MoveInstance` 会无意义地重排规则数组和 revision；用户已确认牌匣承担快速收拢语义。

**Alternatives considered**:
- 同区视为无效：失去用户确认的快速收纳入口，拒绝。
- 同区也调用 move：产生规则噪音与顺序变化，拒绝。

## Decision 8 — Delete confirmation revalidates and resumes carry on cancel/failure

**Decision**: Drop 到 Delete target 时保存不可变的 carry InstanceId 顺序和当前 index，显示一次数量/总金币 preview；取消直接恢复 carry；确认时重新调用 batch validation，失败也恢复 carry且零修改。

**Rationale**: 弹窗期间 Run 可能变化。重新预检消除 stale preview 风险，保存 carry snapshot 能满足用户确认的取消语义。

**Alternatives considered**:
- 打开弹窗即结束 carry：取消后需要重新选择，违背确认结果，拒绝。
- 只使用弹窗打开前 validation：存在 TOCTOU，拒绝。

## Decision 9 — Migrate, then remove the old single-card drag owner

**Decision**: 先让新 Workspace 覆盖单卡和多卡，再把 `UWacomCardDragOperation`、旧 `UWacomZoneDropTarget` 中的输入所有权删除或降为不持状态的过渡 facade；最终只保留一条 Screen command flow。

**Rationale**: 两套 drag/drop 生命周期并存会重复提交或争抢捕获。现有详情、CardView、Toast、Snapshot reconcile 和 SpecialZone toggle 可复用，不需要整页推倒重写。

**Alternatives considered**:
- 永久保留旧路径作为 fallback：形成行为差异和长期测试成本，拒绝。
- 一开始删除全部旧类：会让分片不可编译，拒绝；应按可验证切片迁移。

## Decision 10 — DreamShader remains a Wacom-native optional presentation layer

**Decision**: 交互稳定后，复用现有 card surface/fake-3D 管线；新效果只用 Wacom 自有 `.dsm` 与 `DShader/Shared` helper 编写，并通过 style/material slot 接入 Workspace card view。

**Rationale**: Godot Demo 与外部案例只提供行为/节奏参考。分离材质可在没有效果资产时保持完整功能，也避免 GPL/第三方工程结构复制。

**Alternatives considered**:
- 直接移植 Demo shader 或脚本：许可证与项目边界不合，拒绝。
- 把 hover/carry shader 逻辑写死到 Screen：难以复用和调参，拒绝。
