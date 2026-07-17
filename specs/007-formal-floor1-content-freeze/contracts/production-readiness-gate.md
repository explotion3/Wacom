# Contract: Formal Floor Production Readiness Gate

## Asset namespaces

| Namespace | Current authority | This slice |
|---|---|---|
| Authoring | `Journey.Authoring`, `Floor.Authoring.01`, `L_Exploration` | Unchanged; remains playable authoring baseline |
| Debug | Debug Journey/Floor/GameMode/map and Debug builder | Unchanged; remains isolated fixture |
| Production | Reserved `Journey.Main.01` / `Floor.Main.*` and Production content IDs | Design only; no assets created |

Future Production paths are reserved under `/Game/Wacom/Data/Map/Production` and `/Game/Wacom/Maps/Run`. Creating those packages is not authorized by this slice.

## Hard blockers

Production Journey/Floor DataAssets and formal Floor 1 map work must not start until all blockers are closed:

1. **Serpent Sigil retention**: `Card.Run.SerpentSigil` cannot be permanently lost before the unlocked entrance state is safely established, or the system must provide a deterministic recovery path.
2. **Floor 2/3 validity**: `Floor.Main.02` and `Floor.Main.03` must have approved valid graphs so the frozen three-Floor Journey contains no placeholder content; `Node.Exit.01` specifically targets `Floor.Main.02`.
3. **Production definitions**: all 15 content slot IDs must resolve to reviewed non-Debug definitions with valid typed payloads.
4. **Asset authority audit**: identify whether the production map is authored as a new package or through an explicitly reviewed migration of the authoring scene; no blind binary replacement.

## Serpent Sigil risk

The current graph proves acquisition, not retention:

```text
All routes -> Node.Key.01 -> Guardian -> Exit
```

Existing card removal flows can potentially remove a normal owned card. A completed Pickup is not a guaranteed reissue mechanism. Therefore a non-consuming owned-card entrance requirement can become permanently unsatisfied.

Acceptable future resolutions require a separate rule decision and tests, for example:

- protected task-card semantics with removal operations rejecting the card;
- a durable non-card entrance credential;
- deterministic credential recovery/reissue;
- removal of the card gate by an explicit design revision.

This document does not choose among those alternatives.

## Forbidden shortcuts

- Referencing `DA_Card_DebugKey` or any Debug Event/Shop/Pickup as Production content.
- Renaming Authoring/Debug assets or identities and treating that as migration.
- Creating empty/minimal Floor 2/3 assets solely to satisfy validation.
- Editing `L_Exploration` or `L_RunExploration_Debug` before the asset-authority audit.
- Adding SaveGame fields, GameplayTags or runtime special cases under this documentation feature.

## Exit criteria for the next implementation round

- Task credential decision is approved and implemented with focused removal/recovery tests.
- Floor 2/3 graphs have their own approved design freezes.
- Production content definitions have asset IDs, typed payloads and Data Validation coverage.
- Target production map/package authority is explicitly selected after AssetRegistry/reference/hash audit.
- Only then may a new Spec Kit implementation slice authorize DataAsset/map changes.
