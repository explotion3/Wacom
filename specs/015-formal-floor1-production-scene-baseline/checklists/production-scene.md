# Production Scene Readiness Checklist

**Purpose**: Gate Floor 1 local graybox scene acceptance without overstating full Journey readiness.

## Package authority

- [x] Exactly seven new target packages exist.
- [x] No target path existed before authorized seeding.
- [x] No existing target was overwritten or resaved.
- [x] No binary package outside the seven-path allowlist changed.
- [x] All 46 Spec 014 Production assets remain byte-identical.
- [x] Writer audit matches actual saved paths.
- [x] Second seed creates/saves zero and seven hashes remain stable.

## UWacomFloorMapDefinition

- [x] FloorId is exactly `Floor.Main.01`.
- [x] Exactly 20 nodes and 21 edges exist.
- [x] Type distribution is `4/6/4/4/1/1`.
- [x] All node/edge IDs and MapPositions are unique.
- [x] Every node is reachable from Entry.
- [x] A/B and C/D branches merge at the canonical nodes.
- [x] Key dominates Guardian and Exit.
- [x] Only four Navigation nodes allow Camp.
- [x] Fifteen Production Definition references are exact.
- [x] Guardian has `bBoss=true`.
- [x] Exit targets Floor 2 and checks non-consuming SerpentSigil Credential.

## Scene binding

- [x] Exactly one descriptor points to the Production Floor asset.
- [x] Exactly 20 Anchors bind every NodeId once.
- [x] Exactly 21 Paths bind every EdgeId once with correct endpoints.
- [x] Exactly four BranchTargets bind A/B/C/D initial branch edges.
- [x] Exactly 16 content Hosts bind every non-Navigation node once.
- [x] No Navigation content Host exists.
- [x] All Host node types, payload mirrors, and PersistentIds are exact.
- [x] No generated Debug ownership/tag/identity appears.
- [x] No Level Blueprint gameplay logic exists.

## Enemy Host authoring

- [x] Four prefabs derive from `AWacomBattleEnemyActor`.
- [x] Four exact formal EnemyDefinitions are referenced.
- [x] Eleven formal part identities are synchronized exactly once.
- [x] All four Blueprints compile.
- [x] Eight scene instances use exact Encounter slot mappings.
- [x] No prefab hardcodes a scene slot.
- [x] Non-engine presentation references remain inside controlled placeholders.
- [x] Placeholder references are recorded as release blockers.

## Graybox usability

- [x] World left/right branches match map semantics.
- [x] Host staging does not obstruct traversal splines.
- [x] Trigger volumes are reachable without occupying whole lanes.
- [x] Pickups and Exit marker are visible at target Anchors.
- [x] Battle enemy parts are visible and targetable from viewpoints.
- [x] Event and Shop viewpoints frame content.
- [x] Exit marker clearly communicates non-interactive/no-travel state.

## Validation evidence

- [x] Default Unity WacomEditor build passes.
- [x] Focused Automation passes with exact counts recorded.
- [x] 7/7 AssetRegistry/load/class audit passes.
- [x] 5/5 Blueprint compile audit passes.
- [x] `WacomValidateRunFloorScene` passes locally.
- [x] Forbidden-reference and failed-load checks pass.
- [x] Git diff check, Git/LFS status, and LFS fsck pass.
- [x] MCP session provenance and writer audits are archived.

## Explicit non-readiness

- [x] No Production Journey package was created.
- [x] No Floor 2/3 DataAsset or world was created.
- [x] No FloorEntrance interaction/travel was implemented.
- [x] No full Golden Path PIE success is claimed.
- [x] No approved-art or balance readiness is claimed.
