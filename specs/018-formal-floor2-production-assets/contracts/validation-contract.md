# Contract: Floor 2 Production asset validation

## Source/transient gates

- Default Unity WacomEditor compile after shared core and after Floor 2 profile.
- Floor 1 command/report/manifest/real assets remain compatible and byte-identical.
- Shared service tests cover parsing, dangerous arguments, comparator boundaries, inspect/no-overwrite and report exit semantics.
- Floor 2 transient profile covers exact counts, references, 26 Intent、10 Choice、12 explicit Parts and content-specific invariants.
- Empty-root inspect reports 47 missing and zero Content writes.

## Real asset gates

- `Wacom.Data.FormalFloor2Content`: 47/47 load/class/strict seed defaults.
- Existing Card/EnemyPart FormalProduction/Enemy/Behavior/Encounter/Event/Pickup/Shop validators.
- Battle RuleContent/KnockdownReward and Run reward/shop/event/pickup/notification smoke.
- AssetRegistry exact class/count and failed-load audit.
- Reference closure excludes Debug、Authoring、Test、BadgeDisplayTests、TrainingWarrior、Character、scene/presentation and legacy reward dependencies.

## Binary and idempotence gates

- First run exact `12+20+15=47 created/saved`, zero allowlist-external assets and zero `.umap`.
- Second run each group: zero created/saved/failed.
- 47-file sorted aggregate SHA-256 unchanged across second run.
- Three read-only Shop dependency hashes unchanged.
- All target index blobs are Git LFS pointers and `git lfs fsck` passes.

## Skips

Blueprint compile and PIE are not applicable because no Blueprint/map/Host/Journey changes exist. The skip does not claim Floor 2 Golden Path readiness.
