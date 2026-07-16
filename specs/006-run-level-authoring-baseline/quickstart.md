# Quickstart: Run 正式关卡制作基线收口

## 0. Safety gate

1. Close editor instances that hold `L_Exploration`, Debug map or affected GameMode/DataAssets.
2. Run `git status --short --branch` and preserve unrelated changes, especially DreamMaterials MI work.
3. Capture SHA-256 for:
   - `Content/Wacom/Maps/L_Exploration.umap`
   - `Content/Wacom/Data/Map/Authoring/*.uasset` after initial migration
   - `Content/Wacom/Core/GameModes/GM_Wacom.uasset`
   - `Content/Wacom/Core/Player/BP_WacomPlayerCharacter.uasset`
   - `Content/Wacom/Run/Path/Blueprints/BP_WacomRun*.uasset`

## 1. Implement in dependency order

1. Descriptor Actor + App-private resolver + focused transient-World tests.
2. Descriptor-first working registry refresh.
3. Structured read-only validator + tests.
4. Authoring baseline assets and main/debug map migration.
5. Debug-only builder and renamed commandlet.
6. ToolMenus and standalone validation commandlet.
7. Long-term docs and regression validation.

Do not run the Debug builder until its write-set tests and target path have been changed away from `L_Exploration`.

## 2. Compile

```powershell
& 'E:\UE_5.8\Engine\Build\BatchFiles\Build.bat' WacomEditor Win64 Development -Project='D:\UE_Project\5.7\Wacom\Wacom.uproject' -WaitMutex -NoHotReloadFromIDE
```

## 3. Focused automation

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -ExecCmds='Automation RunTests Wacom.UI.RunSceneBinding; Automation RunTests Wacom.Editor.RunSceneValidation; Automation RunTests Wacom.Editor.RunExplorationDebugAssets; Automation RunTests Wacom.UI.RunPathTraversal; Automation RunTests Wacom.Run.Map; Quit' -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache
```

Then run affected return-flow tests:

```text
Wacom.UI.Battle
Wacom.UI.Shop
Wacom.UI.Event
```

## 4. Build Debug fixture twice

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -run=WacomBuildRunExplorationDebugAssets -Unattended -NoPause -NoSplash -NullRHI
```

Run the same command a second time. Compare actor counts/references and the forbidden-write SHA-256 set. All forbidden hashes must be unchanged.

## 5. Validate both floors

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -run=WacomValidateRunFloorScene -Map=/Game/Wacom/Maps/L_Exploration -Unattended -NoPause -NoSplash -NullRHI

& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -run=WacomValidateRunFloorScene -Map=/Game/Wacom/Maps/Debug/L_RunExploration_Debug -Unattended -NoPause -NoSplash -NullRHI
```

Expected: exit `0`; warnings may be present only if deliberately accepted. Re-run `git status` and verify validation created no dirty package.

## 6. Editor validation

1. Open `L_Exploration`.
2. Run `Tools -> Wacom -> Validate Current Run Floor`.
3. Confirm success and that the map title has no unsaved marker.
4. Repeat for `L_RunExploration_Debug`.
5. Intentionally test duplicate/missing descriptor only in a disposable test map or transient automation World; do not dirty formal assets.

## 7. Blueprint and content validation

- Compile all project Blueprints: 0 error / 0 failed load.
- AssetRegistry audit:
  - `L_Exploration` → Authoring Floor descriptor.
  - `GM_Wacom` → Authoring Journey.
  - Debug map → Debug GameMode + Debug Floor descriptor.
- Confirm new `.umap/.uasset` files use Git LFS.
- Run `git diff --check`.

## 8. PIE golden path

In `L_Exploration`:

1. Enter PIE; before clicking any actor, cursor look works.
2. Single legal exit: first W begins traversal immediately.
3. Multiple legal exits: branch prompts and keyboard/mouse selection work.
4. M opens map; same-floor travel to completed node restores movement/camera/hand.
5. Enter and exit Battle; Run HUD, viewpoint, hand and W/S movement restore.
6. Enter and exit Shop and RunEvent; the same state restores.
7. Re-run current-floor validation after PIE and confirm no scene mutation.

## 9. Final record

Record command results, test counts, Blueprint result, hash comparison, PIE outcome and any skipped manual risk in this file during implementation. Update long-term facts in `Docs/`; do not leave them only in this Spec Kit directory.
