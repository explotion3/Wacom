# Data Model: Enemy Visual Authoring Modes

## Scene Enemy Host

**Runtime type**: `AWacomBattleEnemyActor`

**Role**: Enemy prefab root. Owns enemy-level identity, child PartActor registry,
badge staggering, Host-level visual presentation, and Host debug/validation.

**Existing fields**:
- `EnemyDefinition: UEnemyDefinition*`
- `EnemySlotId: FName`
- badge stagger config
- Details authoring status/debug cache

**New/updated conceptual fields**:
- `HostVisualMode: FName` or equivalent debug classification:
  `None / StaticSprite / Flipbook`
- Host-level visual asset:
  - static sprite asset for normal enemy whole-body art
  - optional flipbook asset for simple whole-body idle animation
- Host visual transform, scale, tint, visibility, sort order
- generated Host visual component reference(s), kept transient
- Host visual debug facts:
  `bUsingHostVisual`, `HostVisualAssetName`, `HostVisualMode`,
  `GeneratedHostVisualComponentCount`

**Relationships**:
- Has many child `AWacomBattleEnemyPartActor`.
- Child PartActors provide target identity and hit bounds.
- Host visual does not own or infer battle target identity.

**Validation rules**:
- Host still requires valid `EnemyDefinition` and child PartActors.
- Duplicate child `PartSlotId`, unknown slot, missing definition slot, and
  mismatched `PartId` remain invalid/warning per existing Host rules.
- Host visual may satisfy the visual-resource requirement for hit-only child
  PartActors.
- If Host has no visual and all child PartActors lack VisualLayers/legacy art,
  report a clear missing-visual warning.

## Host-level Visual

**Runtime owner**: `AWacomBattleEnemyActor`

**Role**: Whole-enemy presentation for normal enemies.

**Fields**:
- visual type: static sprite or flipbook
- asset pointer for the selected type
- relative transform
- render sort priority
- tint including alpha
- visibility
- optional flipbook playback fields mirroring the existing PartActor layer
  vocabulary: play rate, loop, start time, autoplay

**Behavior**:
- Generates Paper2D visual component(s) under the Host root or a dedicated Host
  visual root.
- Has no collision and no overlap generation.
- Does not participate in hover/click/card-drag target resolution.
- Does not receive per-part feedback by default; per-part target feedback
  remains on the PartActor bridge.

## Scene Enemy PartActor

**Runtime type**: `AWacomBattleEnemyPartActor`

**Role**: One battle target part. Owns identity, hit bounds, interaction target,
world target bridge, prediction/status badge UI, and optional independent
visual layers.

**Existing fields**:
- `PartId: FName`
- `PartSlotId: FName`
- injected `EnemySlotId: FName`
- `HitBoundsExtent: FVector`
- legacy `VisualMesh`, `VisualScale`, `VisualRelativeLocation`
- `VisualLayers: TArray<FWacomBattleEnemyPartVisualLayer>`
- feedback/prediction/status config
- Details authoring status/debug cache

**Updated conceptual classification**:
- `VisualAuthoringMode=VisualLayers`: elite/Boss per-part art.
- `VisualAuthoringMode=LegacyPrototype`: old cube/prototype path.
- `VisualAuthoringMode=HitOnly`: no independent visible art, but Host visual is
  present and identity/hit bounds are valid.
- `VisualAuthoringMode=None`: no independent visible art and no Host visual
  context; still may be target-valid but should be diagnosed as missing visual.

**Validation rules**:
- Missing `PartId` or `PartSlotId` remains invalid.
- Non-positive `HitBoundsExtent` remains invalid.
- Duplicate VisualLayer `LayerId` remains invalid.
- Zero VisualLayer scale axis remains invalid.
- Missing VisualLayer asset remains warning because that layer will not generate.
- Missing independent PartActor visual is legal if Host visual exists.

## PartActor VisualLayers

**Runtime type**: `FWacomBattleEnemyPartVisualLayer`

**Role**: Per-part art layers for elite/Boss enemies.

**Fields**:
- `LayerId`
- `LayerMode=StaticSprite / Flipbook`
- `Sprite`
- `Flipbook`
- flipbook playback config
- relative transform
- `SortOrder`
- `Tint`
- `bVisible`

**Behavior**:
- Generated under `VisualLayersRoot`.
- `SortOrder` maps to `TranslucentSortPriority`.
- `Tint.A` controls opacity.
- Collision stays disabled.
- Does not affect target identity or hit bounds.

## Hit-only PartActor

**Runtime type**: `AWacomBattleEnemyPartActor` with no independent visual asset
and a Host-level visual available.

**Role**: Normal enemy part target overlaying a whole-body Host image.

**Fields**:
- same identity and hit fields as any PartActor
- no required `VisualLayers`
- no required legacy `VisualMesh`

**Behavior**:
- `HitBounds` remains visible/editable in editor and used for target traces.
- In PIE/game, generated visual art can be absent; prediction/status/feedback
  still attach to the part target.
- Per-part feedback should not scale the Host whole-body visual unless a future
  explicit Host-wide feedback mode is added.

## Identity

**Runtime target identity direction**:

```text
EncounterId + EnemySlotId + PartSlotId
```

**Static definition identity**:

```text
PartId == UEnemyPartDefinition::PartId
```

This feature does not change identity fields, target handle payloads, Snapshot
shape, Battle commands, or Run progress.

## State / Refresh

**Refresh triggers**:
- Host `OnConstruction`
- Host Details property changes
- Host `RefreshBattleEnemyPartAuthoringState()`
- PartActor `OnConstruction`
- PartActor Details property changes
- PartActor `RefreshAuthoringState()` / `RefreshVisualLayers()`
- `BeginPlay`

**State transitions**:
- Host visual asset changed -> rebuild Host visual components -> refresh Host
  debug cache -> refresh child authoring status where Host visual affects
  hit-only classification.
- PartActor identity/hit bounds changed -> refresh collision/bridge -> refresh
  PartActor debug cache -> Host debug view reflects updated child facts.
- PartActor VisualLayers changed -> rebuild generated layer components ->
  refresh visual/debug facts.

## SaveGame Impact

None. All visual authoring fields are scene Actor/Blueprint configuration, not
Run persistent state. Battle progress identity and SaveGame schema remain
unchanged.
