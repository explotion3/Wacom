# Research: Floor 1 Production Floor / Map / Host 灰盒基线

**Date**: 2026-07-18

## Decision 1 — World authority is a new isolated Production map

**Decision**: Create `/Game/Wacom/Maps/Run/L_Run_Floor_Main_01` from a new empty world. Do not copy, migrate, resave, or modify `/Game/Wacom/Maps/L_Exploration`.

**Rationale**:

- `L_Exploration` contains legacy/manual authority and unrelated Agent-owned presentation work.
- Production identity must not inherit Debug/Authoring actor labels, generated ownership, test payloads, or accidental references.
- A new world makes the seven-package mutation allowlist exact and auditable.

**Rejected alternatives**:

- **Promote `L_Exploration` in place**: rejected because it would mix legacy and Production authority and create binary conflict risk.
- **Duplicate `L_Exploration` then clean it**: rejected because hidden references and actors would make absence claims difficult to prove.

## Decision 2 — This slice creates Floor 1 but no incomplete Journey

**Decision**: Create only `DA_Floor_Main_01`; do not create `Journey.Main.01` until Floor 2, Floor 3, success terminal, and cross-floor contracts can all be valid together.

**Rationale**:

- The frozen Journey contains three floors in order.
- Production Journey validation requires the terminal node to be on the valid final floor and requires the three-floor structure.
- A temporary one-floor Journey would either fail validation or encode false Production truth.

**Consequence**: Full Journey Golden Path PIE, Floor 1 exit travel, Journey success, and multi-floor pacing validation remain blocked. Local Floor data, map scene binding, Blueprint compilation, and Editor/world inspection are still independently verifiable.

## Decision 3 — FloorEntrance is a visible non-interactive graybox Host

**Decision**: Create `BP_WacomRunFloorEntranceMarker_Graybox` as a temporary Blueprint Host with `RunMapNodeBinding` and a Blueprint-authored `PersistentId` field. It visually marks the exit and satisfies the local non-Navigation Host contract but has no interaction, UI, or travel logic.

**Rationale**:

- The live scene validator requires one Host for every non-Navigation node, including FloorEntrance.
- WacomApp currently has no reusable FloorId-to-world mapping or FloorEntrance interaction/travel actor.
- Adding a fake `OpenLevel` or a one-off C++ travel Actor would create false runtime authority.

**Debt/removal trigger**: Replace this marker when a reusable cross-floor transition contract is implemented. Until then, local validation must report that the Host is non-interactive.

## Decision 4 — Four reusable enemy Host Blueprints, not one prototype Snake Host

**Decision**: Create one `AWacomBattleEnemyActor` Blueprint per formal enemy archetype. Each Blueprint references its Spec 014 EnemyDefinition and synchronizes part child actors from the Definition via the existing Editor authoring service.

**Rationale**:

- The formal enemies have different part sets and hit layouts; a single prototype Host cannot represent all of them without per-instance structural mutation.
- Encounter composition should reuse stable archetype prefabs while preserving slot identity per scene instance.
- The existing `SyncPartsFromDefinition` path already defines the supported authoring relationship.

**Rejected alternative**: Modify `BP_EnemyHost_Snake`. It is existing prototype/shared authority and is not structurally equivalent to the four formal enemies.

## Decision 5 — Placeholder art is controlled and explicitly non-shippable

**Decision**: The four enemy Host Blueprints may reference only existing assets beneath `/Game/Wacom/Art/Placeholders`. No placeholder is copied or modified. The seed report lists every placeholder reference.

**Rationale**:

- A graybox Battle scene needs visible targetable parts and archetype silhouettes.
- Controlled placeholder references avoid touching enemy/material Agent assets and keep all binary writes within the seven-package allowlist.

**Production gate**: Placeholder reference count must reach zero, or each reference must be replaced by approved art, before Floor 1 is release-ready. This slice does not declare art readiness.

## Decision 6 — Content Hosts remain separate from enemy scene Hosts

**Decision**: Each Encounter node has one `BP_BattleTriggerActor` content Host that owns the node binding and EncounterDefinition mirror. Its staged enemy actors are separate `AWacomBattleEnemyActor` instances with Encounter slot IDs.

**Rationale**:

- The Run scene registry binds logical nodes to interaction Hosts, not to individual battle enemy actors.
- Battle scene composition and Run node identity have different lifecycle and cardinality.
- The separation preserves the existing Return-to-Run and scene-host retirement contracts.

## Decision 7 — Seed-only creation with whole-group preflight

**Decision**: Extend WacomEditor with a Formal Floor 1 Production Scene seed service. It supports inspect by default and explicit `SeedMissing` groups. Existing targets are loaded and validated but never configured or saved. Any wrong-class/name collision fails the whole group before the first save.

**Groups**:

1. `Floor` — one Floor DataAsset.
2. `EnemyHosts` — four enemy Host Blueprints.
3. `Scene` — FloorEntrance marker Blueprint plus map.

**Rationale**:

- Binary assets cannot be merged safely and must have deterministic authority.
- Grouping preserves dependency order and gives MCP writer leases small, exact allowlists.
- Second-run zero-save behavior protects future manual transforms and Blueprint presentation tuning.

## Decision 8 — Stable identity and tunable presentation remain separate

**Stable/structural**:

- package/object/class
- `FloorId`, `NodeId`, `EdgeId`
- node type, graph endpoints, payload stable ID
- Camp/boss/exit credential and target Floor semantics
- descriptor FloorDefinition
- Anchor/Path/BranchTarget/content Host bindings
- Host PersistentId derivation
- Encounter enemy slot and archetype composition
- enemy Host parent class, EnemyDefinition, and part identity set

**Tunable after seed**:

- DisplayName/description
- MapPosition (subject to legal/unique validation)
- world transform and graybox dimensions
- spline tangent/shape while endpoints remain correct
- viewpoints and encounter staging transforms
- placeholder visual offsets, scale, and presentation settings
- lighting and non-contract graybox geometry

The default structural validator must allow tunable drift. An optional exact seed-default comparison may report drift but must never overwrite it.

## Decision 9 — World layout follows the logical map without making transforms identity

**Decision**: Use a readable two-stage branch layout with world-forward `+X`, left branch `-Y`, right branch `+Y`, and four final nodes on the center line. The initial manifest is recorded in the scene-binding contract.

**Rationale**:

- It makes the world branch direction match the map semantic used in Floor 1 design.
- It leaves wide traversal lanes around content Hosts and gives viewpoints predictable arrival direction.
- Coordinates are seed defaults only and can be tuned after viewport/PIE review.

## Decision 10 — Validation is local and layered

**Decision**: Validate the slice through:

1. default Unity WacomEditor compilation at each C++ checkpoint;
2. transient manifest and structural Automation;
3. real asset load/class/reference and Blueprint compile checks;
4. exact Floor graph validation;
5. `WacomValidateRunFloorScene` on the Production map;
6. enemy Host part/slot composition tests;
7. second-run zero-save and SHA-256 stability;
8. Unreal MCP session/writer audit and Git LFS checks;
9. read-only viewport inspection of graybox direction, spacing, visibility, and traversal clearance.

**Not claimed**: Full PIE Golden Path. Without Production Journey/Floor 2/3 and cross-floor runtime, that test cannot reach a truthful success condition.

## Decision 11 — Local storage uses a D-drive hydration layer

**Decision**: Keep Git-managed files in the new worktree and hydrate only the missing non-Git `Content/Art` and `Content/Asset` trees as worktree-local junctions to independent D-drive copies. Keep `Content/DreamMaterials` in the worktree because it is now Git/LFS managed. Let Binaries, Intermediate, Saved, and local DDC stay beneath the new worktree, which is already on D:.

**Rationale**:

- The map and Blueprint compile/load paths require referenced local assets.
- Worktrees do not share Binaries safely for Unreal MCP identity; the new worktree's own D-drive directories satisfy that isolation.
- D-drive hydration avoids C-drive pressure and prevents mutation of another worktree's local asset layer.
- Replacing the tracked DreamMaterials tree with a junction would hide Git authority and is therefore forbidden.

**Safety**: Every junction target must be resolved and verified under the dedicated D-drive worktree-data root before creation or deletion. No existing junction or directory is replaced without an audit.

## Open implementation observations

- The exact placeholder flipbook/material selected for each archetype may be chosen from the controlled placeholder root after AssetRegistry inspection. This is presentation-only and cannot change the four EnemyDefinition identities or part topology.
- If the existing generic scene validator cannot inspect the Blueprint `PersistentId` on the FloorEntrance marker, add a narrow Editor-only validation adapter for that marker. Do not weaken the general content Host requirement.
- If an existing content Host Blueprint cannot accept the formal payload through instance properties without modifying the class, stop and report the live mismatch before expanding scope.
