# Production Scene Readiness Checklist

**Purpose**: Gate Floor 1 local graybox scene acceptance without overstating full Journey readiness.

## Package authority

- [ ] Exactly seven new target packages exist.
- [ ] No target path existed before authorized seeding.
- [ ] No existing target was overwritten or resaved.
- [ ] No binary package outside the seven-path allowlist changed.
- [ ] All 46 Spec 014 Production assets remain byte-identical.
- [ ] Writer audit matches actual saved paths.
- [ ] Second seed creates/saves zero and seven hashes remain stable.

## UWacomFloorMapDefinition

- [ ] FloorId is exactly `Floor.Main.01`.
- [ ] Exactly 20 nodes and 21 edges exist.
- [ ] Type distribution is `4/6/4/4/1/1`.
- [ ] All node/edge IDs and MapPositions are unique.
- [ ] Every node is reachable from Entry.
- [ ] A/B and C/D branches merge at the canonical nodes.
- [ ] Key dominates Guardian and Exit.
- [ ] Only four Navigation nodes allow Camp.
- [ ] Fifteen Production Definition references are exact.
- [ ] Guardian has `bBoss=true`.
- [ ] Exit targets Floor 2 and checks non-consuming SerpentSigil Credential.

## Scene binding

- [ ] Exactly one descriptor points to the Production Floor asset.
- [ ] Exactly 20 Anchors bind every NodeId once.
- [ ] Exactly 21 Paths bind every EdgeId once with correct endpoints.
- [ ] Exactly four BranchTargets bind A/B/C/D initial branch edges.
- [ ] Exactly 16 content Hosts bind every non-Navigation node once.
- [ ] No Navigation content Host exists.
- [ ] All Host node types, payload mirrors, and PersistentIds are exact.
- [ ] No generated Debug ownership/tag/identity appears.
- [ ] No Level Blueprint gameplay logic exists.

## Enemy Host authoring

- [ ] Four prefabs derive from `AWacomBattleEnemyActor`.
- [ ] Four exact formal EnemyDefinitions are referenced.
- [ ] Eleven formal part identities are synchronized exactly once.
- [ ] All four Blueprints compile.
- [ ] Eight scene instances use exact Encounter slot mappings.
- [ ] No prefab hardcodes a scene slot.
- [ ] Non-engine presentation references remain inside controlled placeholders.
- [ ] Placeholder references are recorded as release blockers.

## Graybox usability

- [ ] World left/right branches match map semantics.
- [ ] Host staging does not obstruct traversal splines.
- [ ] Trigger volumes are reachable without occupying whole lanes.
- [ ] Pickups and Exit marker are visible at target Anchors.
- [ ] Battle enemy parts are visible and targetable from viewpoints.
- [ ] Event and Shop viewpoints frame content.
- [ ] Exit marker clearly communicates non-interactive/no-travel state.

## Validation evidence

- [ ] Default Unity WacomEditor build passes.
- [ ] Focused Automation passes with exact counts recorded.
- [ ] 7/7 AssetRegistry/load/class audit passes.
- [ ] 5/5 Blueprint compile audit passes.
- [ ] `WacomValidateRunFloorScene` passes locally.
- [ ] Forbidden-reference and failed-load checks pass.
- [ ] Git diff check, Git/LFS status, and LFS fsck pass.
- [ ] MCP session provenance and writer audits are archived.

## Explicit non-readiness

- [ ] No Production Journey package was created.
- [ ] No Floor 2/3 DataAsset or world was created.
- [ ] No FloorEntrance interaction/travel was implemented.
- [ ] No full Golden Path PIE success is claimed.
- [ ] No approved-art or balance readiness is claimed.
