# Feature Specification: Floor 1 Production Floor / Map / Host 灰盒基线

**Feature Branch**: `codex/formal-floor1-production-scene-baseline`

**Created**: 2026-07-18

**Status**: Approved for implementation

**Input**: 在 Spec 014 已落地的 46 个 Production DataAsset 基础上，创建 Floor 1 的正式 Floor DataAsset、全量场景绑定、可复用 SerpentWood 敌人灰盒 Host 和独立 Production 地图；不创建不完整 Journey，不实现 Floor 2/3 或跨层切换。

## Wacom Rule Context

**Primary Domain**: Run/探索、内容制作/校验、世界交互、Battle scene authoring

**Rule Truth Docs**:

- [x] `AGENTS.md`
- [x] `Docs/AgentIntegrationWorkflow.md`
- [x] `Docs/UnrealMCPWorkflow.md`
- [x] `Docs/WacomMap.md`
- [x] `Docs/WacomRun.md`
- [x] `Docs/WacomDataAuthoring.md`
- [x] `Docs/WacomApp.md`
- [x] `Docs/WacomWorldInteraction.md`
- [x] `Docs/Architecture.md`
- [x] Spec 006 scene-binding contracts
- [x] Spec 007 Floor 1 graph/content contracts
- [x] Spec 009 Journey/Floor 2/3 pacing contracts
- [x] Spec 010 Journey terminal contract
- [x] Spec 011/013 content freezes and Spec 014 implemented asset manifest

**Expected Owning Module(s)**: `WacomEditor` owns seed-only creation and validation; `WacomData`/`WacomRun`/`WacomApp` runtime contracts remain unchanged; `WacomTests` provides focused Editor and scene-binding coverage.

## Frozen Scope

This slice creates exactly seven new binary packages:

1. `/Game/Wacom/Data/Map/Production/DA_Floor_Main_01`
2. `/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/BrushSnake/BP_EnemyHost_BrushSnake_Graybox`
3. `/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/MoltGuard/BP_EnemyHost_MoltGuard_Graybox`
4. `/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/RootStalker/BP_EnemyHost_RootStalker_Graybox`
5. `/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/ShallowGuardian/BP_EnemyHost_ShallowGuardian_Graybox`
6. `/Game/Wacom/Run/SceneActors/Graybox/BP_WacomRunFloorEntranceMarker_Graybox`
7. `/Game/Wacom/Maps/Run/L_Run_Floor_Main_01`

The map contains the complete Floor 1 scene contract:

- 1 `AWacomRunFloorSceneDescriptorActor`
- 20 `BP_WacomRunMapNodeAnchorActor` instances
- 21 `BP_WacomRunPathSegmentActor` instances
- 4 `BP_WacomRunPathBranchTargetActor` instances
- 16 bound content Host instances, one for each non-Navigation node
- 8 formal enemy scene Host instances attached to the six Encounter compositions
- viewpoint staging for all Battle, Event, and Shop interactions
- map-only graybox pads/corridors and basic lighting sufficient for viewport inspection

## Non-Goals / Boundaries

- Do not create `Journey.Main.01`; a one-floor Production Journey would violate the frozen three-floor contract and Journey terminal validation.
- Do not create Floor 2/3 DataAssets, maps, content Hosts, cross-floor world mapping, FloorEntrance UI, travel, success flow, or Golden Path PIE.
- Do not modify `/Game/Wacom/Maps/L_Exploration`, Debug/Authoring maps, Debug builders, the 46 Spec 014 assets, shared Host Blueprint classes, Player/GameMode, formal rules, AP, Camp, credentials, GameplayTag, SaveGame, Snapshot, Command, Resolution, Build.cs, or module dependencies.
- Do not promote placeholder visuals to approved art. References under `/Game/Wacom/Art/Placeholders` are permitted only for the four new graybox enemy Host Blueprints and remain a Production release blocker.
- Do not run DreamShader, card, backpack, material, TrainingWarrior, Snake, Debug-map, or other full-content builders.
- Do not overwrite any existing target package. Any target collision is a blocking error requiring an authority decision.
- Do not encode runtime behavior in the Level Blueprint.

## User Scenarios & Testing

### User Story 1 - Author and validate the canonical Floor 1 graph (Priority: P1)

As a Run content author, I need `DA_Floor_Main_01` to express the frozen 20-node/21-edge graph using the 15 Production node Definitions already created by Spec 014, so map UI, Run rules, and world binding share one stable authority.

**Independent Test**: Load the Floor DataAsset without loading the map and validate exact IDs, types, payloads, Camp flags, credential gate, topology, reachability, dominance, coordinate bounds, and Definition references.

**Acceptance Scenarios**:

1. **Given** the Floor asset is absent, **When** the seed-only tool runs with the Floor group, **Then** it creates one `UWacomFloorMapDefinition` with `Floor.Main.01`, exactly 20 nodes and 21 edges.
2. **Given** the graph, **When** it is validated, **Then** every node is reachable from `Node.Entry`, both pairs of branches rejoin, `Node.Key.01` dominates Guardian and Exit, and every ID is unique.
3. **Given** the 15 non-Navigation content nodes except Exit, **When** payloads are inspected, **Then** each points to the exact Spec 014 Encounter/Event/Pickup/Shop Definition.
4. **Given** `Node.Exit.01`, **When** its payload is inspected, **Then** it targets `Floor.Main.02` and requires non-consuming `Credential.Run.SerpentSigil`; it does not claim that world travel is implemented.
5. **Given** Entry/Junction nodes, **When** Camp flags are inspected, **Then** only the four Navigation nodes allow Camp.

---

### User Story 2 - Bind every Floor node and edge into an isolated Production map (Priority: P1)

As a level author, I need an empty Production-owned map with complete descriptor, Anchor, Path, BranchTarget, and Host bindings, so the scene can be validated independently of the legacy exploration map.

**Independent Test**: Open the map read-only and run `WacomValidateRunFloorScene`; it reports the exact expected counts, zero missing/duplicate/mismatched bindings, and no Debug/Authoring identities.

**Acceptance Scenarios**:

1. **Given** the new map, **When** the scene validator resolves its descriptor, **Then** it selects exactly `DA_Floor_Main_01` and no other Floor.
2. **Given** all graph nodes and edges, **When** bindings are enumerated, **Then** each NodeId has exactly one Anchor and each EdgeId has exactly one Path.
3. **Given** the two graph junctions with two outgoing choices, **When** BranchTargets are enumerated, **Then** exactly four targets map to the correct outgoing EdgeId and destination Anchor.
4. **Given** each non-Navigation node, **When** content Hosts are enumerated, **Then** exactly one Host has the correct node type, payload mirror, and `Floor.Main.01.<NodeId>` PersistentId.
5. **Given** Navigation nodes, **When** content Hosts are enumerated, **Then** no Navigation content Host exists.
6. **Given** the map, **When** forbidden-reference and label audits run, **Then** it contains no Debug/Authoring content identity and does not reference `/Game/Wacom/Maps/L_Exploration`.

---

### User Story 3 - Reuse formal enemy composition in graybox encounter staging (Priority: P1)

As a Battle scene author, I need four reusable SerpentWood enemy Host Blueprints whose parts are synchronized from the formal EnemyDefinitions, so all six Encounter Hosts can stage the correct eight enemy slots without relying on the prototype Snake Host.

**Independent Test**: Compile and inspect the four Blueprints, then load the map and verify each Encounter's slot IDs, EnemyDefinition references, Host class, viewpoint, and encounter composition.

**Acceptance Scenarios**:

1. **Given** an enemy Host Blueprint, **When** inspected, **Then** its parent is `AWacomBattleEnemyActor`, its EnemyDefinition is the matching Production asset, and its child part authoring matches every formal EnemyPart.
2. **Given** a Host's placeholder presentation, **When** references are audited, **Then** all visual references are confined to controlled `/Game/Wacom/Art/Placeholders` assets and are reported as release blockers.
3. **Given** all Encounter nodes, **When** scene enemy Hosts are inspected, **Then** the six encounter compositions produce exactly eight instances with the frozen slot-to-enemy mapping.
4. **Given** the four new Blueprint packages already exist, **When** the seed tool runs, **Then** it inspects only and saves nothing; it never resynchronizes or overwrites manually adjusted Blueprint assets.

---

### User Story 4 - Make incomplete cross-floor capability explicit (Priority: P2)

As an integrator, I need the Exit node to satisfy local scene-binding validation without pretending that Floor 2 travel exists, so a green local validator does not hide a false Golden Path claim.

**Independent Test**: The Exit graybox marker binds `Node.Exit.01` and its PersistentId, has no interaction or travel implementation, and all validation reports explicitly mark full Journey/PIE as blocked.

**Acceptance Scenarios**:

1. **Given** the FloorEntrance marker, **When** inspected, **Then** it is visibly labeled as a non-interactive graybox marker and contains no Level Blueprint or `OpenLevel` logic.
2. **Given** local scene validation, **When** it succeeds, **Then** the report distinguishes local Floor/scene readiness from missing Journey/Floor2/3/cross-floor runtime readiness.
3. **Given** the end of this slice, **When** completion evidence is reviewed, **Then** no full Golden Path PIE is claimed; the exact future blockers are listed in `quickstart.md`, `Docs/TODO.md`, and `Docs/Questions.md` where appropriate.

---

### User Story 5 - Preserve binary authority and repeatability (Priority: P1)

As an asset integrator, I need a seed-only, allowlisted workflow whose second run produces zero saves, so binary asset ownership remains reviewable and manual future tuning cannot be silently erased.

**Independent Test**: Record pre/post hashes, writer audit, package list, and second-run results; only the seven allowlisted packages change and the second identical invocation reports zero create/save.

**Acceptance Scenarios**:

1. **Given** any target package collision, **When** preflight runs, **Then** the whole requested group fails before the first save.
2. **Given** a clean target set and exact writer lease, **When** a group seeds, **Then** only its allowlisted packages are saved.
3. **Given** all seven assets exist, **When** the full seed command runs again, **Then** it reports zero created and zero saved, with unchanged SHA-256.
4. **Given** Unreal Editor is open, **When** code compilation is requested, **Then** writer ownership is released, Editor closes normally, and `AssertClosedForBuild` passes first.

## Edge Cases

- A target package exists with the wrong class or unexpected object name.
- A Spec 014 dependency is missing, wrong class, or has moved.
- A Floor node references the correct class but the wrong Production Definition.
- An Anchor/Path/BranchTarget/Host is duplicated, mislabeled, missing, or bound to a wrong ID.
- A Host node binding matches while its Definition/InteractionDefinition mirror does not.
- A battle Encounter has correct EnemyDefinitions but wrong slot IDs or the wrong number of enemy scene Hosts.
- A Blueprint compiles but its part actor authoring no longer matches the EnemyDefinition.
- A placeholder reference leaks outside the four approved enemy Host packages.
- The map is loaded by a different Editor/worktree than the writer lease identity.
- The second seed run dirties packages despite no missing assets.
- Local scene validation passes while cross-floor Journey readiness remains unavailable.

## Requirements

### Functional Requirements

- **FR-001**: The implementation MUST create only the seven packages listed in Frozen Scope and MUST fail before saving on any collision.
- **FR-002**: `DA_Floor_Main_01` MUST contain exactly 20 nodes and 21 edges from the frozen Floor 1 contract.
- **FR-003**: Node/edge IDs, node types, MapPositions, payload identities, Camp flags, boss flag, Floor 2 target, and SerpentSigil credential gate MUST match the canonical manifest.
- **FR-004**: The Production map MUST contain exactly one descriptor bound to `DA_Floor_Main_01`.
- **FR-005**: The Production map MUST contain exactly 20 node Anchors, 21 Path actors, 4 BranchTargets, and 16 non-Navigation content Hosts.
- **FR-006**: Each non-Navigation Host MUST have the correct binding, payload mirror, and Floor-scoped PersistentId.
- **FR-007**: All six Encounter Hosts MUST stage the exact eight Enemy Host instances and frozen slot mappings.
- **FR-008**: The four enemy Host Blueprints MUST derive from `AWacomBattleEnemyActor`, reference the matching formal EnemyDefinition, and synchronize parts from that Definition.
- **FR-009**: Placeholder visuals MUST be confined to controlled placeholder paths and recorded as unresolved Production readiness blockers.
- **FR-010**: The FloorEntrance graybox marker MUST remain non-interactive and MUST NOT implement map travel or success behavior.
- **FR-011**: The creation path MUST be seed-only: existing packages are inspect-only and never saved, rebuilt, or resynchronized.
- **FR-012**: All asset mutations MUST use the `run` Unreal MCP endpoint, exact branch/worktree identity, a single writer lease, full package allowlists, and save-only-allowlisted behavior.
- **FR-013**: The second identical seed execution MUST produce zero create/save and stable hashes.
- **FR-014**: Validation MUST cover DataAsset structure, scene binding, Blueprint compile, enemy part authoring, AssetRegistry load/class/reference closure, LFS, and writer audit.
- **FR-015**: Full Journey Golden Path PIE MUST be skipped and explicitly reported because no valid Production Journey, Floor 2/3, or floor-transition runtime exists.
- **FR-016**: Long-term facts and remaining blockers MUST be synchronized to the appropriate `Docs/` files.

### Key Entities

- **FormalFloor1SceneManifest**: The stable graph, scene actors, content payloads, enemy compositions, package allowlists, and validation expectations.
- **FloorDefinition**: The `UWacomFloorMapDefinition` asset carrying graph and local gameplay payloads.
- **SceneDescriptor**: The map actor declaring which FloorDefinition the world represents.
- **NodeAnchor / Path / BranchTarget**: World binding actors for node destinations, traversal edges, and branch presentation.
- **ContentHost**: The one bound interaction Host required for every non-Navigation node.
- **EnemySceneHost**: A reusable `AWacomBattleEnemyActor` Blueprint instance staged under an Encounter Host.
- **GrayboxFloorEntranceMarker**: A temporary, non-interactive Host used only to make the Exit node spatially visible and locally bindable.
- **SeedReport**: Inspect/create/save/error counts and exact changed package paths.
- **WriterAudit**: Unreal MCP session provenance, lease allowlist, requested/saved packages, and out-of-scope detection.

## Success Criteria

- **SC-001**: The Floor asset validates as exactly 20 nodes / 21 edges with type distribution `4 Navigation / 6 Encounter / 4 RunEvent / 4 Treasure / 1 Shop / 1 FloorEntrance`.
- **SC-002**: All 20 nodes are reachable; both branch pairs merge; `Node.Key.01` dominates Guardian and Exit; all IDs and MapPositions are unique and legal.
- **SC-003**: The map validator reports one descriptor, 20 Anchors, 21 Paths, 4 BranchTargets, 16 content Hosts, and zero binding errors.
- **SC-004**: Six Encounter nodes stage exactly eight enemy Host instances with correct slot/enemy/part authoring.
- **SC-005**: All five new Blueprints compile with zero errors; all seven packages load through AssetRegistry with expected classes and no failed loads.
- **SC-006**: The seven target packages are the only binary changes; all 46 Spec 014 Production assets and shared dependencies remain byte-identical.
- **SC-007**: A second full seed execution reports zero created/zero saved and all seven SHA-256 values remain unchanged.
- **SC-008**: Focused default-Unity WacomEditor compilation and affected Automation suites pass.
- **SC-009**: No Debug/Authoring/Test identity, legacy exploration map reference, uncontrolled placeholder path, or Level Blueprint runtime logic appears in the Production packages.
- **SC-010**: Handoff evidence clearly distinguishes completed local Floor/scene graybox readiness from blocked Production Journey, cross-floor transition, approved art, and full PIE readiness.

## Assumptions

- The Spec 014 assets at main base `9fbdb74858b9b4b705a216c8dd7ff539fe44b19e` are authoritative and read-only for this slice.
- World transforms are initial graybox defaults, not stable identity, and may be tuned manually after creation.
- `MapPosition`, DisplayName, descriptions, graybox dimensions, lighting, and viewpoints are tunable presentation fields.
- The four enemy Host Blueprint packages are reusable Production scene-authoring prefabs, but their placeholder visuals are not shippable art.
- The Exit marker is deliberate temporary debt with a removal trigger: replace it when a reusable FloorEntrance interaction/travel contract is implemented.
