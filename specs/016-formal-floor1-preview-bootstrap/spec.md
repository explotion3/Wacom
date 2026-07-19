# Feature Specification: Floor 1 Preview 启动层

**Feature Branch**: `codex/formal-floor1-production-scene-baseline`

**Created**: 2026-07-19

**Status**: Approved for implementation

**Input**: 为 Spec 015 的正式 Floor 1 灰盒地图提供仅限 Editor PIE 的本地启动层，使直接打开 `L_Run_Floor_Main_01` 时能构造与场景 Descriptor 一致的临时 Journey，并保持主菜单和发行启动路径不变。

## Wacom Rule Context

**Primary Domain**: Run/探索、UI/App shell、内容制作/校验、世界交互

**Rule Truth Docs**:

- [x] `AGENTS.md`
- [x] `Docs/AgentIntegrationWorkflow.md`
- [x] `Docs/UnrealMCPWorkflow.md`
- [x] `Docs/WacomApp.md`
- [x] `Docs/WacomMap.md`
- [x] `Docs/WacomDataAuthoring.md`
- [x] `Docs/Architecture.md`
- [x] Spec 015 Floor/scene/Host contract and verification ledger

**Expected Owning Modules**: `WacomApp` owns PIE-only Preview bootstrap; `WacomEditor` owns allowlisted asset seeding and validation; `WacomTests` owns focused coverage. `WacomRun` and `WacomData` contracts remain unchanged.

**Continuation Constraint**: 用户明确授权在 `codex/formal-floor1-production-scene-baseline` 原 worktree 上连续实施，以复用补水、Binaries 和 DDC。Spec 016 因而依赖 Spec 015 完整提交链，不能脱离该链单独集成；不得把最新 main merge/rebase 到本分支。

## Frozen Scope

This slice changes exactly two binary packages:

1. new `/Game/Wacom/Run/Preview/GM_WacomRunFloorPreview`
2. existing `/Game/Wacom/Maps/Run/L_Run_Floor_Main_01`

Runtime capability:

- `AWacomGameMode` exposes a non-reflection C++ journey-resolution seam.
- `AWacomPlayerController` initializes `URunSession` from that seam.
- `AWacomRunFloorPreviewGameMode` resolves one loaded Floor Descriptor and owns one transient one-Floor Preview Journey during Editor PIE.
- The Production map selects the Preview GameMode and contains one Preview-only `APlayerStart` at `Node.Entry`.

## Non-Goals / Boundaries

- Main menu continues to open `/Game/Wacom/Maps/L_Exploration`; Preview is direct-map PIE only.
- Preview is unavailable outside Editor PIE and must not become a packaged/release startup path.
- Do not create `Journey.Main.01`, Floor 2/3 assets, cross-floor transition, FloorEntrance interaction, Journey success, save/continue, or Production Journey selection.
- Do not modify `WacomRun` rules, `FRunState`, Snapshot, Command, Resolution, SaveGame schema, GameplayTag, Build.cs, module dependencies, AP/Decay rules, Floor graph, content Definitions, Descriptor, Anchor, Path, BranchTarget, Host, viewpoint, or manual world transforms.
- Do not run any existing content builder. Only the named Preview seed command may mutate the two allowlisted packages.
- Do not create a Level Blueprint startup path or copy Floor graph data into map actors.
- Do not use subagents for Unreal MCP or asset mutation.
- Do not create a commit before user PIE acceptance. Any later source or asset change invalidates acceptance and requires another relevant PIE pass.

## User Scenarios & Testing

### User Story 1 - Directly preview the Production Floor (Priority: P1)

As a level author, I can open `L_Run_Floor_Main_01` and start PIE at Entry with a valid Run whose Floor is exactly the map Descriptor Floor, without creating an incomplete Production Journey.

**Why this priority**: It removes the current `MissingJourney` / `DescriptorFloorMismatch` startup failure and makes the graybox map usable for local authoring validation.

**Independent Test**: `Wacom.App.RunFloorPreviewBootstrap` proves transient Journey identity, exact Floor reference, character/config behavior, same-instance caching, and fail-closed cases.

**Acceptance Scenarios**:

1. **Given** a PIE world with one valid Descriptor, **When** the Preview GameMode resolves a new Run Journey, **Then** it returns one transient Journey with ID `Journey.Preview.<FloorId>`, exactly that Floor, the configured character, default AP/Decay values, and no success terminal.
2. **Given** the same Descriptor and Floor remain loaded, **When** resolution repeats, **Then** it returns the same transient object.
3. **Given** a non-PIE world, missing/duplicate Descriptor, invalid Floor, missing character, or Descriptor drift after first resolution, **When** resolution is requested, **Then** it returns null and logs a stable rejection without creating a Run.

---

### User Story 2 - Seed only the Preview bootstrap assets (Priority: P1)

As a content author, I can run one named Editor command that creates the generic Preview GameMode if missing and makes the existing Floor 1 map directly playable while preserving every Spec 015 scene binding and manual transform.

**Why this priority**: Runtime code alone cannot make World Settings or spawn placement authoritative in the saved map.

**Independent Test**: `Wacom.Editor.FormalFloor1PreviewBootstrap` inspects Blueprint parent/CDO, map override/PlayerStart, Spec 015 scene contract, exact package scope, and second-pass idempotence.

**Acceptance Scenarios**:

1. **Given** the Preview GameMode package is absent, **When** `WacomSeedFormalFloor1PreviewBootstrap` runs under the exact writer allowlist, **Then** it creates a Blueprint derived from `AWacomRunFloorPreviewGameMode`, copies six presentation/gameplay class settings from `GM_Wacom`, keeps `DefaultJourneyDefinition` empty, and saves only that package.
2. **Given** the Production map has no valid Preview PlayerStart and may currently override `GM_Wacom`, **When** the command runs, **Then** it changes only the GameMode override and adds one `PlayerStart_FloorMain01Preview` at the Entry Anchor transform.
3. **Given** a pre-existing Preview GameMode with wrong parent/configuration, an unexpected PlayerStart, an invalid Entry Anchor, or a broken Spec 015 scene contract, **When** preflight runs, **Then** it fails before saving and does not repair or overwrite the conflicting authority.
4. **Given** both assets already satisfy the contract, **When** the command performs its internal second pass, **Then** it reports zero created and zero saved.

---

### User Story 3 - Manually validate the local Floor loop (Priority: P1)

As a designer, I can complete one representative local interaction loop in PIE and return to normal Run controls, while the Floor Exit remains an explicit dead end.

**Why this priority**: Automation proves the contract, but camera/input/CommonUI and authored staging require PIE observation.

**Independent Test**: The one-pass PIE checklist in `quickstart.md`, repeated after any subsequent runtime or binary change.

**Acceptance Scenarios**:

1. **Given** direct PIE from the Production map, **When** the player starts, **Then** the Run logs Preview Journey plus `Floor.Main.01`, anchors at Entry, and provides mouse look, W/S, HUD, hand, M map, and clickable ESC menu.
2. **Given** the player completes a path, a battle-return loop, and at least one Event/Reward/Shop interaction, **When** control returns to Run, **Then** camera, M, ESC, hand, and movement remain functional.
3. **Given** the player reaches Exit, **When** interaction is attempted, **Then** no transition or Journey Summary occurs.
4. **Given** PIE stops, **When** binary hashes are compared, **Then** the Preview GameMode remains unchanged; any intentional map adjustment is committed only after explicit user acceptance and a focused validation rerun proves the accepted map hash stable.

## Edge Cases

- More than one Descriptor exists, or the only Descriptor has an empty/invalid Floor.
- The Descriptor Floor pointer or FloorId changes after the Preview Journey was cached.
- World type is Editor, Game, GamePreview, or packaged Game rather than PIE.
- `DefaultCharacter` is missing when PlayerController requests initialization.
- `GM_Wacom` is missing or its CDO class references cannot be copied.
- Preview Blueprint exists with a different parent or any copied CDO field drift.
- The map contains any PlayerStart other than the exact Preview one.
- Entry Anchor is missing, duplicated, or has a non-finite transform.
- The command's first pass partially fails; no second pass may claim success.
- PIE or viewport inspection dirties a package after validation.

## Requirements

### Functional Requirements

- **FR-001**: `AWacomGameMode` MUST provide a non-reflection virtual `ResolveJourneyDefinitionForNewRun()` whose base implementation returns `DefaultJourneyDefinition`.
- **FR-002**: `AWacomPlayerController` MUST initialize `URunSession` with the resolved Journey and MUST log the actual resolved object on failure.
- **FR-003**: Preview resolution MUST be available only for `EWorldType::PIE` in Editor builds and MUST fail outside that boundary.
- **FR-004**: Preview resolution MUST require exactly one valid `AWacomRunFloorSceneDescriptorActor` and MUST use its exact Floor reference without copying or rebuilding the graph.
- **FR-005**: The transient Journey MUST be GameMode-owned, `RF_Transient`, cached, and populated with the frozen Preview identity, display name, one Floor, configured character, current Journey defaults, and empty success terminal.
- **FR-006**: Repeated resolution MUST return the same object only while Descriptor and Floor identity remain unchanged; drift MUST fail closed.
- **FR-007**: The generic Preview GameMode Blueprint MUST derive from `AWacomRunFloorPreviewGameMode`, copy PlayerControllerClass, DefaultPawnClass, DefaultCharacter, BattleHUDClass, ExplorationHUDClass, and JourneySummaryScreenClass from `GM_Wacom`, and keep `DefaultJourneyDefinition` null.
- **FR-008**: The Production map MUST override the Preview GameMode and contain exactly one plain Preview PlayerStart at the exact Entry Anchor transform without Run binding identity.
- **FR-009**: Asset mutation MUST be limited to the two frozen packages under one verified `run` writer lease; only the named Preview seed command may save them.
- **FR-010**: Existing Preview Blueprint configuration conflicts, unexpected PlayerStarts, invalid Entry Anchor, or broken Spec 015 scene contracts MUST fail before save rather than being overwritten.
- **FR-011**: One command invocation MUST save, reload/inspect, and run a second idempotence pass that produces zero created and zero saved.
- **FR-012**: Descriptor, 20 Anchors, 21 Paths, 4 BranchTargets, 16 content Hosts, 8 enemy Hosts, 11 viewpoints, Floor Definition, and all manual transforms MUST remain unchanged.
- **FR-013**: Main-menu travel, Floor Exit behavior, Journey success, storage, and all `WacomRun` public/runtime schemas MUST remain unchanged.
- **FR-014**: Default Unity compilation and the focused App/Editor/regression automation set MUST pass before asset mutation and again after any relevant source correction.
- **FR-015**: User PIE acceptance MUST pass before either planned commit; a post-acceptance runtime or asset change MUST invalidate the acceptance.
- **FR-016**: Long-term Preview semantics, authoring workflow, release blocker, validation evidence, and remaining Production Journey debt MUST be synchronized to the named `Docs/` files.

### Key Entities

- **Preview Journey**: GameMode-owned transient `UWacomJourneyDefinition` used solely to initialize a direct-map PIE Run.
- **Preview GameMode**: Reusable App GameMode subclass with a strict PIE/Descriptor resolver.
- **Preview GameMode Blueprint**: Generic content configuration copied from `GM_Wacom`, with no default Journey asset.
- **Preview PlayerStart**: The single plain spawn marker colocated with the Entry Anchor.
- **Bootstrap Seed Report**: First/second-pass create/save counts, diagnostics, and exact package paths.
- **Preview Writer Audit**: MCP provenance and the exact two-package save allowlist.

## Success Criteria

- **SC-001**: Direct-map PIE initializes one valid Run on `Floor.Main.01` with no `MissingJourney`, `DescriptorFloorMismatch`, or `SceneBindingSnapshotInvalid` diagnostic.
- **SC-002**: Automated failure-matrix coverage demonstrates zero Preview Journey creation for every non-PIE or invalid Descriptor case and stable same-instance behavior for the valid case.
- **SC-003**: Asset inspection reports one correct Preview GameMode Blueprint and one correct Preview PlayerStart while preserving the exact Spec 015 `1/20/21/4/16/8/11` scene contract.
- **SC-004**: The internal second seed pass reports `0 created / 0 saved`, and only the two allowlisted binary paths differ from the Spec 015 baseline.
- **SC-005**: The user completes the full PIE checklist with normal input/UI after battle and interaction return; Exit stays non-interactive and no Journey Summary appears.
- **SC-006**: Default Unity build and all listed focused automation prefixes pass with Unreal commands using `-NoDreamShaderEditorBridge`.
- **SC-007**: User PIE acceptance precedes both commits; the Preview GameMode hash remains stable, while any intentional user-authored map adjustment is explicitly accepted, recorded as the new authority, and remains stable through the focused post-save validation rerun.

## Assumptions

- Spec 015's seven committed packages and source chain are the authority beneath this slice.
- The current uncommitted map hash `3F121021CAB235B50AF092EBE753242043916E16ABF2A7B4F6E560DA61EC145A` is preserved as user-owned input; its temporary `GM_Wacom` override is intentionally replaced by this feature.
- Existing `GM_Wacom` is the configuration source, not the runtime parent or Journey authority of Preview.
- The default `UWacomJourneyDefinition` constructor continues to provide the accepted AP and Decay defaults.
- Preview is a release blocker removed when the complete Production Journey and Production startup GameMode exist.
