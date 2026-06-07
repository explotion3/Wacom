# Quickstart: Enemy Visual Authoring Modes

## Prerequisites

- UE 5.7 editor toolchain is available.
- Project root is `D:\UE_Project\5.7\Wacom`.
- Use PowerShell.

## Compile

```powershell
& 'E:\UE_5.7\Engine\Build\BatchFiles\Build.bat' WacomEditor Win64 Development -Project='D:\UE_Project\5.7\Wacom\Wacom.uproject' -WaitMutex -NoHotReloadFromIDE
```

Expected result: build succeeds without module dependency or reflection errors.

## Focused Automation

Run the main scene enemy actor contract tests:

```powershell
& 'E:\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -ExecCmds='Automation RunTests Wacom.UI.Battle.BattleSceneEnemyActor; Quit' -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache
```

Run the current Host registry / target routing regression tests:

```powershell
& 'E:\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -ExecCmds='Automation RunTests Wacom.UI.Battle.BattleSceneEnemyTargetRegistry; Quit' -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache
```

Expected result: both prefixes pass.

## Manual Editor Scenario: Normal Enemy

1. Create or open a Host Blueprint based on `AWacomBattleEnemyActor`.
2. Assign a valid `EnemyDefinition`.
3. Configure one Host-level full-body visual.
4. Add child PartActors for at least `Head`, `Body`, and `Tail`.
5. For each child PartActor, configure:
   - `PartId`
   - `PartSlotId`
   - `HitBoundsExtent`
   - no independent VisualLayers
6. Refresh authoring state.

Expected result:

- Host Details show a Host visual mode.
- child PartActors are classified as hit-only or equivalent legal mode.
- no warning/error appears only because child parts lack independent art.
- identity or hit-bound mistakes are still reported.

## Manual PIE Scenario: Normal Enemy Targeting

1. Configure a BattleTrigger with the current EncounterDefinition and
   `SceneEnemyHostSlots` pointing to the Host.
2. Enter PIE and start the battle.
3. Hover each part.
4. Click each part during valid target selection.
5. Drag a card that can target enemy parts onto each part.

Expected result:

- the whole enemy visual remains visible.
- each PartActor remains independently targetable through its `HitBounds`.
- prediction/status badges and drag preview are attached to the relevant part.
- no target is resolved from the Host visual image itself.

## Manual Editor Scenario: Elite/Boss

1. Create or open a Host Blueprint based on `AWacomBattleEnemyActor`.
2. Add multiple child PartActors.
3. Configure each PartActor with `VisualLayers` using static sprite or flipbook
   assets.
4. Refresh authoring state.

Expected result:

- each PartActor generates its visual layer components.
- `SortOrder`, tint, visibility, and flipbook playback fields behave according
  to the existing PartActor VisualLayers contract.
- Host-level visual is optional and does not replace per-part feedback.

## Validate Map

Run editor Validate Map/Level on a map containing:

- one normal Host visual + hit-only parts enemy.
- one Boss-style per-PartActor VisualLayers enemy.
- one intentionally invalid duplicate `PartSlotId` or invalid `HitBounds`
  sample, if available.

Expected result:

- normal Host visual + hit-only parts is legal.
- Boss VisualLayers path remains legal.
- duplicate slot and invalid hit bounds are still invalid.
- no-art-anywhere configuration produces a clear missing visual warning.
