# Feature Specification: Enemy Visual Authoring Modes

**Feature Branch**: `[001-enemy-visual-authoring-modes]`

**Created**: 2026-06-07

**Status**: Draft

**Input**: User description: "为 Wacom 的战斗场景敌人增加两种正式视觉制作模式。普通小怪通常只有一张整体精灵图，多个规则部位直接在这张图上划分命中区域；精英和 Boss 才会使用多个部位各自的精灵图层来表现景深、遮挡和局部动画。支持普通小怪使用 Host 整体视觉，同时由多个 PartActor 提供独立 HitBounds、PartId、PartSlotId、hover、点击、拖卡目标、预测和状态 Badge。继续保留精英 / Boss 的现有路径：每个 PartActor 可以拥有自己的 VisualLayers。两种模式都必须服务同一套 BattleSession 部位规则和 SceneEnemyHost 绑定流程。"

## Wacom Rule Context *(mandatory)*

**Primary Domain**: Battle UI / World interaction / Architecture

**Rule Truth Docs**:
- [x] `AGENTS.md`
- [x] `Docs/Architecture.md`
- [x] `Docs/WacomBattleUI.md`
- [x] `Docs/WacomWorldInteraction.md`

**Expected Owning Module(s)**: `WacomApp`, `WacomEditor`, `WacomTests`

**Non-Goals / Boundaries**:
- 不改变 `UBattleSession` 战斗规则、卡牌目标校验、敌人部位 Snapshot 语义或 Battle command 入口。
- 不改变 `UEnemyDefinition` / `UEncounterDefinition` 的身份模型，不改 `BattleTrigger` 的 Encounter / Host slot 绑定合同。
- 不做部位破坏换图、复杂动画状态机、PaperZD 接线、动画蓝图或资产迁移。
- 不把 sprite / flipbook 的尺寸、透明区域或视觉层排序作为命中规则来源；命中仍只由 `HitBounds` 决定。
- 不要求普通小怪每个 PartActor 都拥有独立视觉资源。

**Open Rule Questions**:
- 无。当前需求默认以编辑器制作合同和表现层为范围，规则层保持不变。

## User Scenarios & Testing *(mandatory)*

### User Story 1 - 普通小怪整体视觉制作 (Priority: P1)

美术或关卡制作可以用一张整体 sprite / flipbook 制作普通小怪，并在同一个 Host 下放置多个 PartActor 作为规则部位命中区。PartActor 不需要各自配置独立视觉资源，也不应该因为缺少独立视觉而显示错误状态。

**Why this priority**: 这是新反馈的核心制作诉求，可以显著降低普通敌人的美术拆图成本，并保持现有多部位战斗规则。

**Independent Test**: `Wacom.UI.Battle.BattleSceneEnemyActor` 覆盖 Host 整体视觉 + 多 PartActor 命中区；PIE 手动验证普通怪不同部位 hover / 点击 / 拖卡目标。

**Acceptance Scenarios**:

1. **Given** 一个配置了整体视觉的敌人 Host，且子 PartActor 只配置 `PartId / PartSlotId / HitBounds`，**When** 制作者刷新 Authoring 状态或进入 PIE，**Then** Host 显示整体敌人图，子 PartActor 作为可命中的规则部位正常绑定。
2. **Given** 普通小怪的某个 PartActor 没有独立 VisualLayers，**When** Validate Map 或查看 Details `Authoring Status`，**Then** 该 PartActor 不因为缺少独立视觉资源而报错。

---

### User Story 2 - 精英 / Boss 独立部位视觉保留 (Priority: P2)

美术可以继续为精英或 Boss 的每个 PartActor 配置独立 VisualLayers，用多个 sprite / flipbook 表现景深、遮挡、局部 idle 或简单部位动画。

**Why this priority**: 保留并明确现有多部位视觉路径，避免普通怪新路径削弱 Boss 级敌人的表现能力。

**Independent Test**: `Wacom.UI.Battle.BattleSceneEnemyActor` 覆盖 PartActor 独立 VisualLayers 生成、排序、颜色、显隐、缺资源 warning 和 feedback target。

**Acceptance Scenarios**:

1. **Given** 一个 Boss Host 的 PartActor 配置了自己的 VisualLayers，**When** 刷新 Authoring 状态或进入 PIE，**Then** 该部位继续显示自己的 sprite / flipbook 视觉层，并作为同一个规则部位目标参与 hover / click / drag。
2. **Given** Boss 的多个 PartActor 各自有视觉层，**When** target cue、hover 或 drag preview 触发，**Then** 反馈作用于对应部位视觉组，而不是 Host 整体图。

---

### User Story 3 - 编辑器诊断区分视觉模式 (Priority: P3)

策划、关卡制作和程序可以在 Details / debug summary / validation 中看出敌人当前使用的是 Host 整体视觉、PartActor 独立视觉，还是缺少视觉资源，并能快速定位缺身份、重复 slot、无命中盒等制作问题。

**Why this priority**: 新增两种合法视觉模式后，如果诊断不清晰，普通怪会被误判为缺资源，Boss 也可能误走整体视觉路径。

**Independent Test**: `Wacom.UI.Battle.BattleSceneEnemyActor` 覆盖 Authoring Status / debug view；Validate Map 手动或自动化覆盖合法普通怪、合法 Boss 和错误配置。

**Acceptance Scenarios**:

1. **Given** 一个 Host 有整体视觉且 PartActor 是命中区模式，**When** 查看 Details `Authoring Status`，**Then** 能看到 Host 走整体视觉路径，PartActor 不要求独立视觉。
2. **Given** Host 和 PartActor 都没有任何视觉资源，**When** Validate Map，**Then** 系统给出清晰 warning，提示该敌人只有命中体和调试信息可见。

### Edge Cases

- `EnemyDefinition` 为空时，Host 不能推断子 PartActor 的 `PartId / PartSlotId`，应继续显示 `MissingEnemyDefinition` 或对应制作状态。
- 子 PartActor 缺 `PartId / PartSlotId` 或 `HitBounds` 非法时，仍是错误配置；Host 整体视觉不能掩盖身份错误。
- 子 PartActor 之间 `PartSlotId` 重复时，仍必须被 validation 判为 invalid。
- 普通怪整体视觉存在，但某个 PartActor 没有视觉资源时，该 PartActor 仍应可作为命中区使用。
- Boss PartActor 配置 VisualLayers 但缺少对应 sprite / flipbook 时，应 warning 该 layer 不会生成组件。
- PIE 中从 BattleTrigger 进入战斗后，Host 整体视觉和 PartActor 命中区必须使用当前 SceneEnemyHost registry，不读取同关卡其它 Host。
- CommonUI / BattleHUD 生命周期不应改变；敌人视觉模式只是场景 Actor authoring，不新增 UI state owner。

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST support a Host-level enemy visual authoring mode for normal enemies where one Host visual represents the full enemy.
- **FR-002**: System MUST allow child PartActors under a Host-level visual enemy to act as hit-only battle part targets without requiring independent visual assets.
- **FR-003**: System MUST preserve the existing PartActor VisualLayers authoring path for elite and Boss enemies.
- **FR-004**: System MUST keep `HitBounds` as the only source for hover, click, and first-person card drag target detection.
- **FR-005**: System MUST keep BattleSession, card target validation, EncounterDefinition, and BattleTrigger enemy identity rules unchanged.
- **FR-006**: System MUST expose editor diagnostics that distinguish Host-level visual authoring, PartActor visual authoring, hit-only PartActors, legacy prototype visuals, and missing visual resources.
- **FR-007**: System MUST validate Host-level visual + hit-only PartActors as a legal normal-enemy configuration.
- **FR-008**: System MUST warn when neither Host nor PartActor has any visible resource, while still reporting identity and hit-bound errors as invalid where applicable.
- **FR-009**: System MUST preserve Wacom module dependency direction and expose only documented public contracts.
- **FR-010**: System MUST keep authoritative Battle/Run/Data rules outside widgets, scene-only Actors, and presentation view data.
- **FR-011**: System MUST update relevant Wacom docs when rule truth, authoring semantics, binding contracts, or validation expectations change.
- **FR-012**: System MUST include compile and focused validation for the implemented slice, or document skipped validation risk.

### Wacom-Specific Requirements *(include as applicable)*

- **Docs-first evidence**: Read and update `Docs/WacomBattleUI.md` and `Docs/WacomWorldInteraction.md`; update `Docs/TODO.md` only if implementation leaves explicit future work.
- **Module/API boundary**: Runtime authoring Actors and components stay in `WacomApp`; editor validation stays in `WacomEditor` or existing actor validation hooks; automation stays in `WacomTests`.
- **Data/GameplayTag impact**: No new GameplayTag expected. No `WacomData` schema change expected in this feature; enemy identity continues through existing `EnemyDefinition.Parts`, `PartId`, and `PartSlotId`.
- **Battle contract impact**: No Battle command, Snapshot, Event, ResultPacket, target validation, resolver, or effect executor change expected.
- **Run contract impact**: N/A. No RunSession, SaveGame, PersistentId, or transaction change expected.
- **UI/App boundary**: BattleHUD remains passive over battle rules and continues consuming scene target bridges. Visual authoring mode lives on scene enemy Host / PartActor and affects presentation only.
- **Testing expectation**: Compile `WacomEditor Win64 Development`; run `Automation RunTests Wacom.UI.Battle.BattleSceneEnemyActor` and `Automation RunTests Wacom.UI.Battle.BattleSceneEnemyTargetRegistry`.
- **Temporary debt**: None planned. If PaperZD / advanced animation state machine is deferred, document it as future work only, not as required debt for this feature.

### Key Entities *(include if feature involves data/state)*

- **Scene Enemy Host**: `AWacomBattleEnemyActor`; owns enemy prefab grouping, authoring diagnostics, Host-level visual path, and child PartActor discovery.
- **Scene Enemy PartActor**: `AWacomBattleEnemyPartActor`; owns battle part identity facade, `HitBounds`, target bridge, badges, and optional independent VisualLayers.
- **Host-level Visual**: A presentation-only visual resource set on the Host for normal enemies; does not define hit detection or battle identity.
- **Hit-only PartActor**: A PartActor with identity and `HitBounds` but no independent visible sprite / flipbook, used under a Host-level visual.
- **PartActor VisualLayers**: Existing per-part sprite / flipbook layers for elite and Boss enemies.
- **Identity**: `EncounterId + EnemySlotId + PartSlotId` remains the runtime part identity direction; `PartId` remains the static part definition ID.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A normal enemy Host can be authored with one overall visual and at least three child PartActors that remain independently targetable in PIE.
- **SC-002**: A Boss-style Host can still be authored with multiple PartActors that each own independent VisualLayers.
- **SC-003**: Details `Authoring Status` and validation clearly classify legal normal-enemy and Boss-enemy configurations without false errors for hit-only PartActors.
- **SC-004**: Focused automation covers Host-level visual + hit-only PartActors, existing PartActor VisualLayers, and missing visual resource warnings.
- **SC-005**: No Wacom module boundary violation or undocumented public Battle rule API is introduced.
- **SC-006**: `Docs/WacomBattleUI.md` and `Docs/WacomWorldInteraction.md` reflect the final authoring semantics.

## Assumptions

- Normal enemies usually use one Host-level full-body visual and several invisible/hit-only PartActors.
- Elite and Boss enemies continue to use per-PartActor VisualLayers when they need depth, occlusion, or local animation.
- Simple sprite / flipbook support is sufficient for this feature; PaperZD or advanced animation state machines remain future work.
- Focused UI battle automation plus PIE manual validation is sufficient before a full `Wacom` automation run.
