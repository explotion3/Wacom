# Specification Quality Checklist: Floor 1 Production Scene Baseline

**Purpose**: Verify that Spec 015 requirements are complete, testable, scoped, and consistent before implementation.

**Created**: 2026-07-18

## Requirement completeness

- [x] Scope identifies the exact seven binary packages.
- [x] Floor graph authority and exact 20-node/21-edge contract are specified.
- [x] Scene binding cardinalities and identities are specified.
- [x] Content Host payload and PersistentId contracts are specified.
- [x] Enemy prefab and Encounter instance compositions are specified.
- [x] New-map authority and legacy-map non-modification are explicit.
- [x] Journey/Floor2/3/cross-floor/Golden Path exclusions are explicit.
- [x] Placeholder-art and Exit-marker readiness blockers are explicit.
- [x] Seed-only/no-overwrite/idempotence behavior is explicit.
- [x] Unreal MCP writer identity and package allowlists are explicit.
- [x] Build, Automation, AssetRegistry, Blueprint, scene, hash, LFS, and viewport validation are explicit.
- [x] Durable documentation updates and handoff evidence are required.

## Requirement clarity

- [x] Stable identity is distinguished from tunable presentation.
- [x] Local scene validity is distinguished from Journey runtime readiness.
- [x] Content Hosts are distinguished from battle enemy scene Hosts.
- [x] FloorEntrance marker is explicitly non-interactive.
- [x] “Existing package” behavior forbids configure/save and requires inspection.
- [x] Full PIE skip has a precise blocker rather than an ambiguous “not needed”.

## Testability

- [x] Every P1 story has an independent test.
- [x] Exact count, identity, reference, and topology assertions are enumerable.
- [x] Visual-only questions are isolated to viewport review.
- [x] Second-run zero-save and hashes make idempotence measurable.
- [x] Unexpected binary changes fail closed and are not automatically cleaned.

## Scope control

- [x] No GameplayTag, SaveGame, Snapshot, Command, Resolution, Build.cs, or module dependency change is allowed.
- [x] No existing DataAsset/map/Blueprint/material/card/backpack/enemy-art asset is writable.
- [x] No incomplete Journey or fake floor transition is allowed.
- [x] No full-content builder is allowed.
- [x] No Level Blueprint gameplay behavior is allowed.

## Approval

- [x] User selected new empty map.
- [x] User selected Floor-only/no incomplete Journey.
- [x] User selected four reusable enemy Host prefabs.
- [x] Spec status is Approved for implementation.

## Notes

The remaining implementation observations in `research.md` concern presentation-only placeholder selection and live API compatibility. Any discovered mismatch requiring runtime rules, shared asset modification, or scope expansion must stop for user confirmation.
