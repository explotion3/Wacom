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

## Implementation Audit — T001–T004 (2026-07-13)

### Protected baseline

- `54ce8781 feat: add card pile transfer presentation and backpack refactor plan` 已提交全部先前工作区内容。
- 本背包切片不得回退或重写该提交中的 Battle、first-person hand、pile-transfer、DreamShader 和对应测试/资产；仅在真实编译依赖冲突时做最小兼容修改并记录原因。
- 审计开始时 `git status --short` 为空。

### Legacy drag owner inventory

| Symbol | Current producers/hosts | Current consumers/tests | Migration decision |
|---|---|---|---|
| `UWacomCardDragOperation` | `WacomDeckCardWidget.cpp` 构造单卡 payload；`WacomSpecialZoneWidget.cpp` 为 Owner/内容卡提供拖拽；`WacomBackpackScreen.cpp` / runtime builder 间接创建卡牌源 | `WacomZoneDropTarget.cpp`、`WacomDeleteZoneDropTarget.cpp`、`WacomBackpackCommandFlow.*`、`WacomBackpackScreen.*`；大量断言集中在 `BackpackScreenSpec.cpp` / `BackpackScreenTestAccess.*` | 在 US2 新 Workspace 输入 owner 可覆盖单卡后停止生产；US3/US4 迁移 move/delete 后按 T061 删除，不保留并行状态机 |
| `UWacomZoneDropTarget` | `BackpackRuntimeZoneBuilder.*`、`WacomBackpackScreen.cpp`、`WacomSpecialZoneWidget.cpp` 动态创建 | `WacomBackpackScreen` 预览/提交、`BackpackScreenSpec.cpp` | 基础切片保留；ZoneRack preview/intent 接通后移除旧 drag/drop owner |
| `UWacomDeleteZoneDropTarget` | `BackpackRuntimeZoneBuilder.*`、`WacomBackpackScreen.cpp` | `WacomBackpackCommandFlow.*`、`BackpackScreenSpec.cpp` | 基础切片保留；US4 batch confirm 完成后移除 |

Legacy symbol live-file set:

```text
Source/WacomApp/{Public,Private}/UI/Backpack/
  WacomCardDragOperation, WacomZoneDropTarget, WacomDeleteZoneDropTarget,
  WacomDeckCardWidget, WacomSpecialZoneWidget, WacomBackpackScreen,
  WacomBackpackCommandFlow, BackpackRuntimeZoneBuilder
Source/WacomTests/Private/UI/
  BackpackScreenSpec.cpp, BackpackScreenTestAccess.{h,cpp}
Docs/
  UI_Backpack_WBP_Binding.md, TechDebt.md
```

### Single-card Run API call-site decision

- `URunSession::MoveInstance()` / `ValidateMoveInstance()` 的规则实现位于 `RunSession.cpp` → `FRunDeckRules`；生产 Backpack UI 提交集中在 `WacomBackpackCommandFlow.cpp`。
- `URunSession::DeleteCardForGoldByInstance()` / validation 同样由 `WacomBackpackCommandFlow.cpp` 作为生产 Backpack UI 入口。
- 其余直接调用主要位于 `BackpackSpec.cpp`、`RunEventSpec.cpp`、`SaveGameRoundtripSpec.cpp`、`SnapshotRevisionSpec.cpp`、`SpecialZoneBattleEnabledValidationSpec.cpp`、`RunFirstPersonCardLayerSpec.cpp` 和 `BackpackScreenSpec.cpp`，用于规则构造或现有单卡契约验证。
- 决策：保留现有 public 单卡 API 兼容其它正式调用与测试；新增 C++-only atomic batch API。新 Workspace 不允许循环调用 public 单卡 API 实现 batch。待一项 batch parity 测试通过后，可在内部让单卡入口委托一项 batch helper，但不在 T001–T011 提前改变行为。

### Module/API boundary confirmed

| Module | Foundational ownership | Exposure |
|---|---|---|
| `WacomRun` | batch request/result contract 与 `URunSession` C++ API 声明；`FRunDeckRules` working-state helper 声明 | Public contract 不反射、不 Blueprint；算法保持 Private |
| `WacomApp` | ZoneKey/layout/selection/carry 基础值类型；Workspace presentation style；只读 automation view | 只有 WBP/style/production test view 所需类型进入 Public；交互状态保持 Private |
| `WacomTests` | 只消费 production automation view 的 private wrapper | 不向 production module 反向暴露任何测试类型 |

### Toolchain verification

- `E:\UE_5.8\Engine\Build\BatchFiles\Build.bat`：存在。
- `E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe`：存在。
- `quickstart.md` 的 UE 5.8 compile 和 focused automation 命令无需修正。
- CodeGraph MCP 在当前工具会话不可调用，因此此次 live call-site 审计使用 `rg`；真正编辑仍以当前文件内容为准。

## Migration Audit — T067 (2026-07-13)

已按 `quickstart.md` 运行完整旧路径 `rg`，并在 T059–T062 完成后再次复核。新 Workspace/ZoneRack/批量 command flow 是唯一输入路径；旧 UMG DragDrop 生产 owner 已删除。剩余旧 Host 名仅为资产字段兼容和只读 fallback：

| Remaining match | Why it remains | Current boundary |
|---|---|---|
| `DeleteZoneHost`、`BattleDeckZoneHost`、`FluxContentDropTargetHost`、`SpecialZonesHost`、`BurdenZoneHost` | 旧资产字段和 fallback 测试仍需要稳定绑定名 | `BackpackRuntimeZoneBuilder` 只创建提示、WrapBox 和只读卡牌；不创建 DragDrop target，不提交 move/delete |
| `ContentDropTargetHost` | `WBP_WacomSpecialZoneWidget` 的历史 UPROPERTY 名称，改名会无谓破坏旧资产绑定 | 运行时仅填充普通内容容器；右键入战仍转发给 Screen flow，跨区移动只走 Workspace |
| 历史文档中的旧类名 | 研究审计和迁移搜索命令需要说明被移除对象 | 不属于生产引用；当前制作合同明确禁止第二输入 owner |

最终生产源码搜索不再命中 `UWacomCardDragOperation`、`NativeOnDragDetected`、`UWacomZoneDropTarget`、`UWacomDeleteZoneDropTarget`、`BuildDragOperation` 或旧 preview wrapper。Graph-first 反馈材质由 Wacom 自有 `.dsm` / `.dsh` 生成并通过 Style 注入，未复制 Godot Demo 代码、shader 或资产。
