# Research: Enemy Visual Authoring Modes

## Decision: Host-level visual lives on `AWacomBattleEnemyActor`

**Rationale**: The Host is already the enemy prefab root, owns
`EnemyDefinition`, injects `EnemySlotId`, scans child PartActors, applies badge
stagger, and exposes Host debug summary. A whole-body normal-enemy visual is an
enemy-level presentation concept, so the Host is the correct owner.

**Alternatives considered**:
- Put the full-body sprite on one special PartActor. Rejected because it makes a
  visual-only body part look like a rule target and can confuse identity,
  feedback, and destroyed-state semantics.
- Put the full-body sprite in BattleHUD. Rejected because scene target authoring
  belongs to world Actors, while HUD should stay passive and registry-driven.
- Use a Level Blueprint or bespoke snake Actor. Rejected as one-off authoring.

## Decision: Hit-only PartActor is a legal mode when Host visual exists

**Rationale**: Normal enemies need multiple battle targets over one authored
image. Child PartActors already own `PartId`, `PartSlotId`, `HitBounds`,
interaction target, world target bridge, prediction widget, status badge, and
debug state. They can be visible only as editor/debug hit boxes and still be
valid battle targets.

**Alternatives considered**:
- Require every PartActor to have a transparent or duplicated sprite. Rejected
  because it adds pointless asset work and makes normal enemy authoring brittle.
- Derive hit zones from sprite bounds or alpha. Rejected because the existing
  contract says `HitBounds` is the only source of hover/click/drag detection.

## Decision: Keep PartActor `VisualLayers` as elite/Boss mode

**Rationale**: The current `AWacomBattleEnemyPartActor` already supports
`StaticSprite` and `Flipbook` layers, generated Paper2D components,
`SortOrder`, tint, visibility, missing asset diagnostics, and feedback scaling
through `VisualLayersRoot`. That is the correct per-part path for enemies that
need depth, occlusion, or local idle animation.

**Alternatives considered**:
- Move all visual layers to Host. Rejected because Boss parts need independent
  feedback and local visuals tied to each target.
- Introduce PaperZD now. Rejected for this slice because the user only needs the
  authoring-mode contract; state-machine animation can be a later extension.

## Decision: No Battle/Data/Run runtime contract change

**Rationale**: Both visual modes map to the same existing target identity:
`EncounterId + EnemySlotId + PartSlotId`, with `PartId` as static part
definition ID. Battle rules consume snapshot enemy parts and target handles, not
sprite components. Therefore `UBattleSession`, `UEnemyDefinition`,
`UEncounterDefinition`, `ABattleTriggerActor`, Run progress, and SaveGame do not
need changes.

**Alternatives considered**:
- Add an enemy visual mode to `UEnemyDefinition`. Rejected for this first slice:
  the mode is currently scene prefab authoring, not static battle rules.
- Add a new Battle Snapshot field. Rejected because UI/targeting behavior is
  unchanged.

## Decision: Host visual uses PaperSprite/PaperFlipbook component generation

**Rationale**: `WacomApp` already depends on `Paper2D`, and PartActor
VisualLayers already use `UPaperSpriteComponent` and
`UPaperFlipbookComponent`. Reusing the same component vocabulary avoids new
module dependencies and keeps normal/Boss art paths understandable.

**Alternatives considered**:
- Use only `UStaticMeshComponent` or material planes. Rejected because the
  project has already chosen Paper2D for enemy visual layers.
- Add a new renderer subsystem. Rejected as unnecessary for an authoring
  contract slice.

## Decision: Validation distinguishes visual source from target validity

**Rationale**: A PartActor can be target-valid without independent art, and a
Host can have a valid whole-body visual while child identity is wrong. Debug and
validation must therefore report:

- Host whole-body visual present.
- PartActor VisualLayers present.
- legacy prototype visual present.
- hit-only PartActor under Host visual.
- missing visual resources everywhere.
- identity, duplicate slot, and hit-bound errors independently.

**Alternatives considered**:
- Treat missing PartActor visual as warning in all cases. Rejected because it
  would keep noisy false positives for normal enemies.
- Suppress all visual warnings when Host visual exists. Rejected because Boss
  part layer mistakes and no-art configurations still need diagnostics.
