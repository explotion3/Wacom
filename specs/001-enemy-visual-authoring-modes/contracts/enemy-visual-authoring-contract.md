# Contract: Enemy Visual Authoring Modes

## Scope

This contract covers scene enemy authoring and presentation only:

- `AWacomBattleEnemyActor`
- `AWacomBattleEnemyPartActor`
- `UWacomBattleEnemyPartWorldTargetBridgeComponent`
- BattleHUD SceneEnemyHost registry as an existing consumer

It does not change battle rules, target validation, encounter data, trigger
identity, Run state, SaveGame, or card effects.

## Official Authoring Modes

### Normal Enemy: Host Visual + Hit-only Parts

Authoring shape:

```text
AWacomBattleEnemyActor
├── Host-level full-body visual
├── Head PartActor: PartId + PartSlotId + HitBounds
├── Body PartActor: PartId + PartSlotId + HitBounds
└── Tail PartActor: PartId + PartSlotId + HitBounds
```

Contract:

- Host visual represents the full enemy.
- Child PartActors may have no independent `VisualLayers` or legacy visual.
- Child PartActors remain independently hoverable, clickable, card-targetable,
  and badge/prediction capable through their `HitBounds` and bridge.
- Missing independent child art is not a warning/error when the Host visual is
  present and the PartActor is otherwise valid.
- Host visual has no collision and is never used for target traces.

### Elite/Boss Enemy: PartActor VisualLayers

Authoring shape:

```text
AWacomBattleEnemyActor
├── Head PartActor: HitBounds + VisualLayers
├── Body PartActor: HitBounds + VisualLayers
└── Tail PartActor: HitBounds + VisualLayers
```

Contract:

- Each PartActor may own independent `VisualLayers`.
- Feedback for target confirm, damage, destroyed, hover, and drag preview acts
  on the relevant PartActor visual group.
- `VisualLayersRoot` remains the preferred feedback target for per-part visuals.
- Missing layer asset is a warning for that layer, not a battle rule error.

## Targeting Contract

- `HitBounds` is the only hover, click, and first-person card drag target source.
- Sprite/flipbook size, opacity, alpha, visual sort order, and transparent pixels
  do not alter target resolution.
- Target handles continue to use existing world target bridge fields:
  `EncounterId`, `EnemySlotId`, `PartSlotId`, runtime target id, and stable
  `PartId`.
- BattleHUD continues to accept only targets from the current SceneEnemyHost
  registry.

## Feedback Contract

- Normal hit-only PartActor feedback should remain attached to that PartActor's
  target bridge, prediction, and status badge.
- Per-part target feedback must not scale or move the Host full-body visual by
  default, because that would imply every part target owns the whole body.
- Elite/Boss PartActor feedback continues to scale `VisualLayersRoot`.
- A future explicit Host-wide feedback mode may be added separately, but it is
  not part of this feature.

## Debug / Authoring Status Contract

Host debug view should expose enough facts to distinguish:

- Host visual present / missing.
- Host visual mode: none, static sprite, flipbook, or equivalent classification.
- Generated Host visual component count.
- Existing child part facts: part count, `PartId`, `PartSlotId`,
  stable scene target ids, unknown/missing/duplicate slots, bound runtime facts.

PartActor debug view should expose enough facts to distinguish:

- `VisualLayers`
- legacy prototype visual
- hit-only under Host visual
- no visual resource
- invalid identity
- invalid hit bounds
- missing VisualLayer asset
- duplicate VisualLayer id

Details `Authoring Status` caches should mirror the debug view facts, so makers
do not need to run PIE to understand the current mode.

## Validation Contract

Invalid:

- Host missing required `EnemyDefinition` on placed instances.
- Host has no child PartActors.
- child PartActor missing `PartId` or `PartSlotId`.
- child PartActor has duplicate `PartSlotId` within the same Host.
- child PartActor slot does not correspond to Host `EnemyDefinition.Parts`.
- child PartActor `HitBoundsExtent` has any non-positive axis.
- VisualLayer has empty or duplicate `LayerId`.
- VisualLayer has zero scale on any axis.

Warning:

- Host and all child parts have no visible resource anywhere.
- VisualLayer missing the asset required by its `LayerMode`.
- Existing content mismatch warnings already used for unknown/missing definition
  part ids/slots.

Legal:

- Host has full-body visual and child PartActors have no independent visual.
- Boss PartActors have independent VisualLayers.
- Multiple enemies reuse the same `PartId` under different `EnemySlotId` values.

## Non-impact Contract

This feature must not change:

- `UBattleSession`
- `FBattleSnapshot`
- `FBattleCommand`
- `FBattleResultPacket`
- `UEnemyDefinition`
- `UEncounterDefinition`
- `ABattleTriggerActor.EncounterDefinition`
- `ABattleTriggerActor.SceneEnemyHostSlots`
- Run progress or SaveGame
- card target validation logic
