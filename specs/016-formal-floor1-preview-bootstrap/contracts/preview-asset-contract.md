# Preview Asset Contract

## Package manifest

```text
/Game/Wacom/Run/Preview/GM_WacomRunFloorPreview
/Game/Wacom/Maps/Run/L_Run_Floor_Main_01
```

No other package may be marked dirty or saved by the Preview bootstrap command.

## `GM_WacomRunFloorPreview`

- Asset class: `UBlueprint`.
- Direct parent: `AWacomRunFloorPreviewGameMode`.
- Source configuration: `/Game/Wacom/Core/GameModes/GM_Wacom.GM_Wacom_C`.
- Copied fields: `PlayerControllerClass`, `DefaultPawnClass`, `DefaultCharacter`, `BattleHUDClass`, `ExplorationHUDClass`, `JourneySummaryScreenClass`.
- Forced field: `DefaultJourneyDefinition = nullptr`.
- No construction script, event graph startup behavior, saved Preview Journey, or Level travel logic is added.

If the package exists, all fields are inspect-only. Any mismatch rejects the whole operation before map save.

## `L_Run_Floor_Main_01`

Permitted differences from the Spec 015 map baseline:

1. `AWorldSettings::DefaultGameMode` points to `GM_WacomRunFloorPreview_C`.
2. One plain `APlayerStart` exists with object name and actor label `PlayerStart_FloorMain01Preview`, at the exact `Node.Entry` Anchor transform.

No other intentional change is permitted. In particular:

- Descriptor/Floor pointer unchanged.
- 20 Anchor identities/transforms unchanged.
- 21 Path identities/splines/transforms unchanged.
- 4 BranchTarget identities/transforms unchanged.
- 16 content Host identities/classes/payloads/PersistentIds/transforms unchanged.
- 8 enemy Host slot mappings/classes/Definitions/transforms unchanged.
- 11 viewpoint identities/transforms/references unchanged.
- 20 graybox pads and all manual tuning unchanged.
- no Run binding component or generated ownership tag is placed on PlayerStart.

## Collision policy

The bootstrap rejects before saving when:

- Preview Blueprint package exists with any contract mismatch;
- map package is missing or not a `UWorld`;
- Spec 015 scene validation fails before mutation;
- Entry Anchor is missing/duplicated/non-finite;
- any non-Preview PlayerStart exists;
- more than one Preview PlayerStart exists;
- existing Preview PlayerStart has wrong class, transform, binding, name, or label;
- current map override is a class other than null, `GM_Wacom_C`, or the Preview class.

The known user-authored temporary override to `GM_Wacom_C` is an allowed migration input, not a final valid state.
