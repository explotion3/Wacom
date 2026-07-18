# Contract: Floor 1 Production Scene Validation

## Validation tiers

### Tier A — Manifest/transient

Runs without Production targets present:

- exact 7-package manifest and group counts
- package/object/class uniqueness
- exact 20-node/21-edge graph and node type distribution
- graph reachability, merge, dominance, coordinate, payload, Camp, Guardian, and Exit invariants
- exact scene actor and encounter composition expectations
- argument parsing, missing/existing/wrong-class states, and no-force behavior

### Tier B — Real asset structure

Runs after seeding:

- 7/7 AssetRegistry discovery, load, expected class, and no failed loads
- exact FloorDefinition graph and references
- 5/5 Blueprint compile results
- 4 enemy prefab parent/definition/part sets and controlled placeholder closure
- map descriptor/Anchor/Path/BranchTarget/content Host counts and identities
- payload mirror, PersistentId, viewpoint, and 8-instance Encounter composition checks
- zero Debug/Authoring/Test/legacy-map references

### Tier C — Runtime-adjacent local scene binding

Runs without claiming a complete Journey:

- `WacomValidateRunFloorScene` against `/Game/Wacom/Maps/Run/L_Run_Floor_Main_01`
- existing `Wacom.Editor.RunSceneValidation` and affected Run scene-binding/path contracts
- transient harness may register Floor 1 locally, but must not fabricate a shippable Production Journey package
- verifies Exit marker is locally bound but non-interactive

### Tier D — Manual/read-only graybox review

- left/right world branches match map semantics
- Anchors and content Hosts are visible and do not block splines
- trigger volumes are reachable
- enemy parts are visible/targetable from Battle viewpoints
- Event/Shop viewpoints frame content
- Exit marker clearly communicates “no travel” graybox state
- no asset save during review

## Required Automation prefixes

- `Wacom.Data.Map.Validation`
- `Wacom.Editor.FormalFloor1ProductionScene`
- `Wacom.Editor.RunSceneValidation`
- affected `Wacom.Run.SceneBinding` / Run path prefixes discovered in live tests
- existing Battle scene enemy Host authoring/part tests
- affected Blueprint/AssetRegistry validation tests

The exact discovered test names and pass/fail counts must be recorded in `quickstart.md`; do not assume a prefix exists without enumerating it.

## Build checkpoint

Any C++ change requires default Unity:

```powershell
& 'E:\UE_5.8\Engine\Build\BatchFiles\Build.bat' WacomEditor Win64 Development -Project='<worktree>\Wacom.uproject' -WaitMutex -NoHotReloadFromIDE
```

Unreal commands use `-NoDreamShaderEditorBridge`.

## Idempotence and binary checks

- Second run: `Created=0`, `Saved=0`, no unexpected dirty package.
- Seven `.uasset/.umap` SHA-256 values unchanged after second run.
- Spec 014 Production dependencies selected for pre/post audit remain byte-identical.
- `git lfs status` and `git lfs fsck` pass.
- No binary file outside the seven-package manifest appears in the diff.

## Explicit skips

Full PIE/Golden Path, cross-floor travel, Journey success, actual release-art review, and balance conclusions are skipped. The skip is required, not optional, until a valid Production Journey, Floor 2/3 assets, and FloorId-to-world transition implementation exist.
