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
3. Authoring baseline assets and main/debug map migration.
4. Structured read-only validator, ToolMenus and standalone validation commandlet.
5. Debug-only builder and renamed commandlet.
6. Long-term docs and regression validation.

Do not run the Debug builder until its write-set tests and target path have been changed away from `L_Exploration`.

## 2. Compile

```powershell
& 'E:\UE_5.8\Engine\Build\BatchFiles\Build.bat' WacomEditor Win64 Development -Project='D:\UE_Project\5.7\WacomWorktrees\run-level-authoring-baseline\Wacom\Wacom.uproject' -WaitMutex -NoHotReloadFromIDE
```

## 3. Focused automation

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\WacomWorktrees\run-level-authoring-baseline\Wacom\Wacom.uproject' -ExecCmds='Automation RunTests Wacom.UI.RunSceneBinding; Automation RunTests Wacom.Editor.RunSceneValidation; Automation RunTests Wacom.Editor.RunExplorationDebugAssets; Automation RunTests Wacom.UI.RunPathTraversal; Automation RunTests Wacom.Run.Map; Quit' -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache -NoDreamShaderEditorBridge
```

Then run affected return-flow tests:

```text
Wacom.UI.Battle
Wacom.UI.Shop
Wacom.UI.Event
```

## 4. Build Debug fixture twice

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\WacomWorktrees\run-level-authoring-baseline\Wacom\Wacom.uproject' -run=WacomBuildRunExplorationDebugAssets -Unattended -NoPause -NoSplash -NullRHI -NoDreamShaderEditorBridge
```

Run the same command a second time. Compare actor counts/references and the forbidden-write SHA-256 set. All forbidden hashes must be unchanged.

## 5. Validate both floors

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\WacomWorktrees\run-level-authoring-baseline\Wacom\Wacom.uproject' -run=WacomValidateRunFloorScene -Map=/Game/Wacom/Maps/L_Exploration -Unattended -NoPause -NoSplash -NullRHI -NoDreamShaderEditorBridge

& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\WacomWorktrees\run-level-authoring-baseline\Wacom\Wacom.uproject' -run=WacomValidateRunFloorScene -Map=/Game/Wacom/Maps/Debug/L_RunExploration_Debug -Unattended -NoPause -NoSplash -NullRHI -NoDreamShaderEditorBridge
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

## 10. Implementation record

### Phase 1 preflight — 2026-07-16

- Worktree: `D:\UE_Project\5.7\WacomWorktrees\run-level-authoring-baseline\Wacom`
- Branch: `codex/run-level-authoring-baseline`
- Base/initial HEAD: `6f5d2a150632758aede1301d3f9b2d01cf6e5164`
- Initial `git status --short --branch`: clean (`## codex/run-level-authoring-baseline`)
- Git LFS: `git-lfs/3.6.1`; `git lfs status` had no staged or unstaged objects.
- Unrelated DreamMaterials protection: the target worktree has no local modification to `Content/DreamMaterials/World/MI_WacomBattleEnemyPartImpactPixel_Default.uasset`; this asset and all card/material work remain outside this slice and must not be regenerated or staged.
- Editor processes: no `UnrealEditor` process command line points at this worktree. PID 7116 opens `C:\Users\ahhh\.codex\worktrees\1171\Wacom\Wacom.uproject`; its Live Coding PID is 28764. They are not treated as holders of the target worktree packages. Before any target asset migration, re-run this process audit.
- Package lock risks: `L_Exploration`, future Authoring/Debug Journey/Floor assets, `GM_Wacom`, future `GM_WacomRunDebug`, and the future Debug map must not be open in an editor that points at this worktree during save/copy/migration.
- Spec Kit prerequisites: requirements checklist 17/17 complete. The prerequisite script rejected only the user-mandated `codex/...` branch naming; feature dir and artifacts were audited manually. `.specify/extensions.yml` has no enabled `before_implement` or `after_implement` hooks.
- LFS attributes: all planned `.uasset`/`.umap` paths resolve to `filter=lfs diff=lfs merge=lfs`.

### Phase 1 live-source audit

- Current Debug builder is not debug-only: it saves shared Path Blueprints, `GM_Wacom`, `BP_WacomPlayerCharacter`, and `L_Exploration` in addition to Debug data.
- Current runtime refresh tears down the installed binding before preflight, tolerates partial registration failure, and has no Scene Descriptor.
- Current editor validator accepts an arbitrary caller-supplied Floor and has text-only diagnostics without Spline geometry checks.
- Existing builder automation directly runs the destructive builder and is therefore intentionally skipped until the Phase 5 write-set guard exists. The live test name is `Wacom.Editor.RunExploration.DebugAssetBuilder.IdempotentAndStable`; the planned final namespace will be `Wacom.Editor.RunExplorationDebugAssets`.

### Phase 1 initial SHA-256

| Asset | SHA-256 |
|---|---|
| `Content/Wacom/Maps/L_Exploration.umap` | `e9c60445ce05748c170751fa8a236a1ac1c31bf99e8c55fa9bd2a44dfa5fb11b` |
| `Content/Wacom/Core/GameModes/GM_Wacom.uasset` | `09c752563bca33b05c1974d548aacdf0e2ebdea5914529bb443dc6ca587e1bab` |
| `Content/Wacom/Core/Player/BP_WacomPlayerCharacter.uasset` | `0fee64d55b7b2be643adce0d3145f6a97cd2a94b92a5149af00eab1659ef59cf` |
| `Content/Wacom/Run/Path/Blueprints/BP_WacomRunMapNodeAnchorActor.uasset` | `ff125e93b6a6e718d50212b50aca5ffa1968df02be9a875d5ee253b06ebc9db5` |
| `Content/Wacom/Run/Path/Blueprints/BP_WacomRunPathBranchTargetActor.uasset` | `977920bef36f24cfb739343a184ac1a5bf0ceaed52e34639a44949da96244ba4` |
| `Content/Wacom/Run/Path/Blueprints/BP_WacomRunPathSegmentActor.uasset` | `947de5ce8ef5f0c900e088993ea34cedfc158bd38645e4609e0d50639624757d` |

Authoring Journey/Floor, Debug map, and Debug GameMode do not yet exist. Existing `DA_Journey_Debug` and `DA_Floor_Debug_01` are present. Authoring asset hashes will be captured immediately after the one-time migration and before the Debug builder is enabled.

### Phase 1 baseline validation

- `WacomEditor`: passed on 2026-07-16 (`145/145`, `Result: Succeeded`, 110.99 s). Only pre-existing DreamShader `FCoreDelegates::OnPostEngineInit` deprecation warnings were emitted; `Saved/Logs/Codex_RunLevel_Phase1_Build.err.log` is empty.
- `Wacom.UI.RunSceneBinding`: passed with `-NoDreamShaderEditorBridge`; 2 discovered, 2 passed, 0 failed, automation exit code 0. The existing transient-World cleanup warning (`UWorld::DestroyActor: World has no context`) remains non-fatal.
- Existing destructive builder automation: skipped by safety gate; must run after T035–T043.

### Phase 1 automation safety incident

- The first `Wacom.UI.RunSceneBinding` launch was aborted before test execution because DreamShader Editor Bridge startup queued a full source scan and persistently regenerated 33 out-of-scope material packages (DreamMaterials, first-person card materials, one Backpack material, and two World materials).
- The command-line editor PID 23188 was stopped. Initial worktree cleanliness, process-local timestamps, `LogDreamShader` generation/save records, and `git lfs status` established that all 33 changes came from this launch.
- Those exact 33 generated packages were restored to `HEAD`; no Spec Kit changes or other binary assets were restored. A follow-up `git status`/`git lfs status` contains only the four expected Spec Kit text changes and no LFS object changes.
- Root cause: `FDreamShaderEditorModule` starts `FDreamShaderEditorBridge`, whose `QueueFullScan()` enqueues all material sources. The plugin-supported `-NoDreamShaderEditorBridge` command-line flag skips that bridge entirely. All subsequent command-line editor automation and Wacom commandlets in this feature use this flag; the quickstart commands above were updated accordingly.
- First launch result: skipped/aborted for asset safety, not counted as a test result. No target Run map/DataAsset/GameMode was modified.
- A second launch verified the DreamShader flag but was aborted because `Start-Process` quoting reduced `-ExecCmds` to the bare `Automation` command; it produced no asset or LFS changes and is not counted as a test result.
- The correctly quoted safe launch queued `Automation RunTests Wacom.UI.RunSceneBinding; Quit`, discovered the two existing tests, completed both successfully, and exited with code 0. Post-run `git status`/`git lfs status` again showed only the four expected Spec Kit text changes and no LFS changes; the log contains zero DreamShader generation/save records.

### Phase 2 Descriptor and atomic binding checkpoint — 2026-07-16

- Added `AWacomRunFloorSceneDescriptorActor`: single-player/non-replicated, no Tick, actor collision disabled, hidden in game, and only an `EditAnywhere, BlueprintReadOnly` Floor reference. No Blueprint setter, Run command, GameplayTag, schema or module dependency was added.
- Added the App-private read-only resolver with stable results for invalid World, missing/duplicate Descriptor, null Floor, empty FloorId, expected Floor mismatch and success.
- The working registry now registers and validates the complete Floor-scoped set of Anchor, Path, required multi-exit BranchTarget and typed content Host identities. Missing, duplicate, invalid and unexpected scene identities reject the working state before installation.
- PlayerController refresh order is now Snapshot → Descriptor → working registry → completeness → version/Floor revalidation → two-phase Coordinator prepare/commit → one installed generation. Registration failure, forced version drift and forced Floor drift preserve the previous registry, coordinator version, traversal state, character transform and Run Session version.
- Automation visibility is a non-reflected `Public/Testing` view plus `WacomTests/Private` adapter under `WITH_DEV_AUTOMATION_TESTS`; no Blueprint test API or scattered production getter was introduced.
- Compile checkpoint 1 (Descriptor only): `11/11`, `Result: Succeeded`, 25.15 s. `Wacom.UI.RunSceneBinding.Descriptor`: 1 discovered, 1 passed, exit 0.
- Compile checkpoint 2 (complete atomic slice): `28/28`, `Result: Succeeded`, 42.92 s. Isolation follow-up compile: `4/4`, `Result: Succeeded`, 6.98 s.
- `Wacom.UI.RunSceneBinding`: 4 discovered, 4 passed, exit 0. This includes the Descriptor failure matrix and `AtomicRefresh.FailuresPreserveInstalledBinding`.
- `Wacom.UI.RunPathTraversal.Coordinator`: 6 discovered, 6 passed, exit 0.
- `Wacom.UI.Shop.ReturnToRunPath`: first follow-up run failed because the legacy test reused the editor global World and the new completeness check correctly rejected unrelated anchors as `SceneNodeAnchorUnexpected`. The test was moved to an isolated transient World and given an explicit Descriptor/Shop host; retry discovered 1, passed 1, exit 0. The failed isolation run is retained as diagnostic evidence and is not a product regression.
- All command-line runs used `-NoDreamShaderEditorBridge`; logs contain no DreamShader generation/save records. Post-checkpoint `git lfs status` has no objects, no target-worktree Editor process remains, and all six protected initial asset SHA-256 values are unchanged.
- Asset migration remains unopened/unstarted at this checkpoint.

### Phase 3 Authoring baseline asset checkpoint — 2026-07-16

- Added the asset-first contract before migration: exact formal scene catalog `8 Anchor / 7 Path / 3 Branch / 6 content hosts`, one Descriptor per map, Authoring/Debug Floor and GameMode/Journey references, ownership separation, stable formal GUID catalog, and cross-map Transform/Spline/identity equivalence.
- `Wacom.UI.RunSceneBinding.AtomicRefresh`: 1 discovered, 1 passed, exit 0. This covers the `Floor.Authoring.01` success path plus wrong-Floor/version/Floor-drift atomic rejection without replacing installed presentation state.
- Pre-migration AssetRegistry audit: `L_Exploration` had 0 Descriptor and 18 Run actors tagged `Wacom.Generated.RunExploration`; `GM_Wacom` referenced `DA_Journey_Debug`, which referenced `DA_Floor_Debug_01`; all four target assets were absent. The six Phase 1 protected hashes were unchanged before writing.
- One-time migration wrote only the authorized package set: formal map, formal GameMode, Authoring Floor/Journey, Debug map and Debug GameMode. The migration script and the temporary read-only GUID audit script were deleted after use; no reusable formal-map overwrite entry remains.
- Migration recovery note: the initial UE Python run returned 1 after creating the Debug World in memory but before saving it. Loading another level in the same process hit UE 5.8 `EditorServer.cpp:2544` World-leak fatal. The formal map/GameMode and three non-map targets had already saved correctly; the Debug map alone was absent. A read-only partial-state audit proved one Authoring Descriptor, 0 Debug ownership and the complete 8/7/3/6 scene catalog before recovery. Recovery used two fresh editor processes: duplicate-and-immediately-save the migrated scene as the Debug map, then configure only its Debug Descriptor/GameMode and restore exactly 18 Debug ownership tags. No existing binary was overwritten during recovery.
- Post-migration AssetRegistry/scene audit: formal map = one Authoring Descriptor, `GM_Wacom`, 0 Debug ownership, 8/7/3/6 identities; Debug map = one Debug Descriptor, `GM_WacomRunDebug`, 18 Debug ownership, 8/7/3/6 identities. All four new asset paths resolve to `filter=lfs diff=lfs merge=lfs`.
- The first asset-contract run correctly exposed a bad test assumption: UE map duplication intentionally regenerates Actor GUIDs in the independent clone. The contract was corrected to compare the formal map against the exact pre-migration GUID catalog and require valid, unique, disjoint Debug GUIDs while comparing NodeId/EdgeId, Transform and Spline across maps. A read-only comparison against `D:\UE_Project\5.7\Wacom\Content\Wacom\Maps\L_Exploration.umap` (same `e9c60445...` baseline hash) found 18 records and 0 formal GUID mismatches. The corrected asset contract discovered 1, passed 1, exit 0.
- Phase 3 compile checkpoints: contract-count update `4/4`, succeeded in 19.92 s; corrected GUID contract `4/4`, succeeded in 13.37 s.
- Directed regressions: `Wacom.UI.RunPathTraversal` 22 discovered / 22 passed / exit 0; `Wacom.Run.Map` 6 discovered / 6 passed / exit 0.
- Stable post-migration SHA-256:

| Asset | SHA-256 |
|---|---|
| `Content/Wacom/Maps/L_Exploration.umap` | `d295ea91da55637d7131ef5cc55d78f311266e58bb64546be60bf0e8cd940029` |
| `Content/Wacom/Core/GameModes/GM_Wacom.uasset` | `0b24f01517cb70fbf4e3c1c36055699ad0cd2fcc8afc8939d1f52abf5c16c74a` |
| `Content/Wacom/Data/Map/Authoring/DA_Floor_LevelAuthoring_01.uasset` | `d1b4f223d4818ad994583c67add3186606b02620c46006d77ac053009b4c2b66` |
| `Content/Wacom/Data/Map/Authoring/DA_Journey_LevelAuthoring.uasset` | `ea044bf14a636fc1dcb4a0d650c5b9648e52d707796126650af7ccb917f929c8` |
| `Content/Wacom/Maps/Debug/L_RunExploration_Debug.umap` | `eb3961cc649b202904ac03280b8be3872d96b121095f0bfd3c959822421eb8e5` |
| `Content/Wacom/Debug/GameModes/GM_WacomRunDebug.uasset` | `d04683c827ba1138139e98a6fc07fa07ec5409770b85cf0809f8078759341fc1` |

- Protected non-write hashes remain unchanged: Player Character `0fee64d5...`, Anchor BP `ff125e93...`, Branch BP `977920be...`, Segment BP `947de5ce...`. No card, backpack, enemy material, DreamShader source/output or other Agent-owned asset is modified.
- Blueprint-wide compile and PIE golden-path checks are deferred to the final validation phase. Current manual risk remains presentation feel and return-flow behavior in PIE; no automated result is being represented as a visual/feel approval.

### Phase 4 read-only Run Floor validation checkpoint — 2026-07-16

- Replaced text-only results with stable `Severity / Code / ObjectPath / Message` diagnostics sorted by severity, code and object path. The validator now accepts only a `World`, resolves the unique Descriptor internally, and performs the complete Anchor/Path/Branch/content-host identity matrix plus Spline shape, finite-transform, direction and 100 cm/300 cm endpoint checks. It has no repair, `Modify`, dirty or save path.
- Added commandlet classification for stable exits: `0` valid or warning-only, `1` resolved scene-contract errors, `2` missing/load/Descriptor-resolution failures. Added `Tools -> Wacom -> Validate Current Run Floor` with symmetric registration/unregistration. Tests access both surfaces only through non-reflected `Public/Testing` views; no Blueprint test API was added.
- First Phase 4 build failed in three mechanical UE 5.8 compatibility points: dynamic `UE_LOG` format strings, unavailable `TArray::CountByPredicate`, and unavailable `TNumericLimits<float>::QuietNaN`. All were corrected without changing validation semantics. A later menu-test build failed because `UToolMenus::ConvertUIAction` is private in UE 5.8; the final test instead proves the actual menu entry exists and calls its same private module callback through a narrow friend automation view, without changing Build.cs or exposing a production API.
- A compile attempt waited behind an unrelated `C:\Users\ahhh\.codex\worktrees\0b47\Wacom` full build holding the global UBT mutex and was timed out while waiting. That external process was not interrupted. After the mutex released, target-worktree builds succeeded: main Phase 4 slice `11/11` in 9.81 s; inactive-map regression `7/7` in 11.54 s; full-map loader fix `4/4` in 6.36 s; final menu slice `6/6` in 8.61 s.
- The first real commandlet run exposed two integration omissions, both fixed in the validation surface: `LoadObject<UWorld>` returns an `EWorldType::Inactive` map asset in commandlet context, and does not expand the level Actor set there. The validator now explicitly supports read-only inactive map assets, while the commandlet uses `UEditorLoadingAndSavingUtils::LoadMap`, matching the established WacomEditor map-loading path.
- `Wacom.Editor.RunSceneValidation`: final run discovered 8, passed 8, failed 0, exit 0. This covers Descriptor/identity matrices, Spline thresholds, stable sorting, read-only dirty invariants, 0/1/2 classification, real menu registration and execution against both maps.
- `Wacom.UI.RunSceneBinding`: discovered 5, passed 5, failed 0, exit 0 after the structured-validator migration.
- `WacomValidateRunFloorScene -Map=/Game/Wacom/Maps/L_Exploration`: exit 0, 0 diagnostics. `-Map=/Game/Wacom/Maps/Debug/L_RunExploration_Debug`: exit 0, 0 diagnostics.
- Menu execution was automated headlessly for both maps by finding the real `LevelEditor.MainMenu.Tools / Wacom / Wacom.ValidateCurrentRunFloor` entry and executing its bound module callback. Both executions passed. Visual notification appearance remains ordinary editor chrome and is not treated as a PIE/gameplay approval.
- All six post-migration SHA-256 values before commandlet/menu validation and afterward are identical to the Phase 3 table. Neither map, GameMode nor Authoring asset was saved or dirtied. `git diff --check` reports no whitespace error (only repository line-ending conversion notices).
- PIE remains deferred to the final validation phase. Phase 4 introduced no new gameplay presentation behavior; the remaining manual risks are still the golden-path feel and Battle/Shop/RunEvent return presentation listed in section 8.

### Phase 5 Debug-only builder checkpoint — 2026-07-16

- Removed `UWacomBuildRunExplorationAssetsCommandlet` and its files. The only build command is now `-run=WacomBuildRunExplorationDebugAssets`; no wrapper or redirect remains. The result contract contains only Debug Journey/Floor/GameMode/World plus dependency, data, scene-validation and save status.
- Builder preflight loads Player/Anchor/Path/Branch shared Blueprints read-only and verifies their native parent classes before creating or modifying any Debug asset. Missing and wrong-parent dependency tests return failure, preserve all four Debug-owned SHA-256/dirty states, and do not create a replacement package.
- Builder writes only `/Game/Wacom/Data/Map/DA_Journey_Debug`, `/Game/Wacom/Data/Map/DA_Floor_Debug_01`, `/Game/Wacom/Debug/GameModes/GM_WacomRunDebug` and `/Game/Wacom/Maps/Debug/L_RunExploration_Debug`. It never compiles, modifies or saves `GM_Wacom`, Player BP or shared Run Path BPs.
- The Debug map rebuild preserves non-generated art/hosts, reuses existing Anchor and Branch transforms plus Path actor transforms/Spline points, rebuilds only generated Anchor/Path/Branch actors, creates a Debug-owned native activity host only when no matching host exists, ensures exactly one Descriptor and sets the dedicated Debug GameMode. The shared descriptor-aware validator runs before any owned package save; any Error returns without saving.
- First builder compile failed only because the WorldSettings header path is `GameFramework/WorldSettings.h`; the second compile exposed only the UE `TObjectPtr<UClass>` dereference form. The corrected full builder slice compiled `5/5` in 9.46 s. SHA test changes compiled `5/5` in 5.73 s; expected-error registration compiled `4/4` in 6.83 s.
- Initial dependency-test launch exited 3 before assertions because this UE Windows build deliberately asserts `No SHA256 Platform implementation` in `FPlatformMisc::GetSHA256Signature`. The tests now use a local dependency-free SHA-256 implementation, verified against the standard empty-input digest. A follow-up launch reached the expected dependency failures but UE Automation counted their Error logs as failures; registering the two exact logs with `AddExpectedError` preserved Error severity and produced the final pass.
- `Wacom.Editor.RunExplorationDebugAssets.DependencyFailuresAreReadOnly`: 1 discovered, 1 passed, exit 0. `Wacom.Editor.RunExplorationDebugAssets.IdempotentAndWriteSet`: 1 discovered, 1 passed, exit 0. Final combined prefix: 2 discovered, 2 passed, failed 0, exit 0.
- Idempotence contract after two in-process builds: `Journey.Debug`, `Floor.Debug.01`, stable Node/Edge/content identities, one Descriptor, 8 Anchors, 7 Paths, 3 BranchTargets, 6 activity hosts, Debug Descriptor → Debug Floor, Debug GameMode → Debug Journey, data/scene validation passed and all four saved owned packages clean.
- The renamed commandlet was then executed in two fresh editor processes. Both runs returned 0 and logged `Start` then `Done`; no builder validation warning/error was emitted.
- Forbidden-write SHA-256 after automation plus both commandlet runs remains exactly: formal map `d295ea91...`, Authoring Floor `d1b4f223...`, Authoring Journey `ea044bf1...`, `GM_Wacom` `0b24f015...`, Player BP `0fee64d5...`, Anchor BP `ff125e93...`, Path BP `947de5ce...`, Branch BP `977920be...`. These equal the Phase 3 stable baselines (and the Phase 1 baselines for the four shared BPs).
- Four owned paths all resolve to `filter=lfs diff=lfs merge=lfs`. Final checkpoint owned hashes after the last automation build are: Debug Journey `7ea99e780946f531bd08155b5cf03630074101836f26ed60b3328e04d2924ea6`, Debug Floor `5913049e10b09b522981a1b94587fcff0ff169f55336695afc3dc70adfdf832b`, Debug GameMode `93c84c7ec65403772c6444e2bb96e949167dc907332143521ca4a98c91b29c30`, Debug map `6b85d40966fc7a013d0ee49ca7e2d37120bda274aba426a8d60010f5b24030b9`. Owned hashes are not promised identical because generated Actor GUIDs/save metadata may change; logical identity/count/reference idempotence is the stable contract.
- T045 directed regressions: `Wacom.Editor.RunExplorationDebugAssets` 2/2, `Wacom.Editor.RunSceneValidation` 8/8, `Wacom.UI.RunSceneBinding` 5/5, all exit 0. The Authoring baseline asset contract still passes after repeated Debug rebuilds, including the formal GUID/Transform/Spline preservation checks.

### Phase 6 documentation and reference audit — 2026-07-16

- Long-term facts were written to `Docs/WacomMap.md`, `Docs/WacomApp.md`, `Docs/WacomDataAuthoring.md` and `Docs/Architecture.md`: Authoring/formal/Debug ownership, unique World Descriptor, descriptor-first working registry and atomic commit, read-only structured validator, 10/100/300 cm Spline thresholds, Debug-only builder write set and unchanged module dependency graph. The obsolete statement that the current Floor Map Screen is unshipped was removed.
- `Docs/TODO.md` and `Docs/Questions.md` explicitly keep formal Floor 1 graph/stable NodeId, content density, cross-floor entrance, formal Camp content and future SaveGame identity open. The current 8-node `Floor.Authoring.01` graph is documented as a migration/authoring baseline, not formal Floor 1 or a stable save identity.
- Old-command audit over `Source`, `Config`, `Scripts`, `Docs` and this feature directory found no runtime entry. `WacomBuildRunExplorationAssets` remains only in `tasks.md`, `plan.md`, `research.md`, `contracts/debug-asset-builder.md` and this quickstart as explicit migration/removal history.
- A one-shot read-only AssetRegistry dependency/referencer audit returned: main map has Authoring Floor and no Debug Floor/Journey/GameMode/map dependency; `GM_Wacom` has Authoring Journey and no Debug dependency; Debug map has Debug Floor and Debug GameMode; Debug map has no referencer; Debug GameMode is referenced only by Debug map; neither is referenced by `GM_Wacom`. The temporary audit script was deleted immediately after exit 0.

### Phase 6 final compile and automation — 2026-07-16

- T052 final `WacomEditor Win64 Development` build passed. The target was already up to date (`0 actions`, `Result: Succeeded`, 2.12 s). The last checkpoint that actually compiled and linked the builder expected-error test slice also passed (`4/4`, 6.83 s).
- T053 final directed automation results, all with `-NoDreamShaderEditorBridge` and exit code 0:

| Prefix | Discovered | Passed | Failed |
|---|---:|---:|---:|
| `Wacom.UI.RunSceneBinding` | 5 | 5 | 0 |
| `Wacom.Editor.RunSceneValidation` | 8 | 8 | 0 |
| `Wacom.Editor.RunExplorationDebugAssets` | 2 | 2 | 0 |
| `Wacom.UI.RunPathTraversal` | 22 | 22 | 0 |
| `Wacom.Run.Map` | 6 | 6 | 0 |
| `Wacom.UI.Battle` | 177 | 177 | 0 |
| `Wacom.UI.Shop` | 15 | 15 | 0 |
| `Wacom.UI.Event` | 12 | 12 | 0 |

- T054 full `Automation RunTests Wacom` discovered 1425 tests and completed the entire queue without fatal/assert/crash. Result: 1423 passed, 2 failed, UE test exit `-1` / process exit 255. Both failures are outside this feature's ownership and point only at unchanged card assets/tests:
  - `Wacom.UI.CardDetail.Assets.DefaultContent`: the configured card detail theme has no Damage/Heal/Shield icon brush or fallback and no Poison/Slow/Freeze/Twilight/Stunned status brush or fallback (`CardDetailContentAssetSpec.cpp:218-232`).
  - `Wacom.UI.CardView.SurfaceParallax.ProductionWidgetAssetActivation`: the production first-person card face has no authored `CardArt` texture and its core surface composite is inactive (`CardSurfaceParallaxSpec.cpp:184,188`).
- `git diff --name-only 6f5d2a15 --` over those two test files and their card/DreamMaterials content roots is empty. This slice did not modify any card UI, card material or referenced test asset, so these failures are recorded rather than repaired under the explicit ownership prohibition. They differ from the older 1302-test TODO baseline's two Battle failures: final directed `Wacom.UI.Battle` is 177/177 and the full run no longer crashes. Because no full suite was run on the untouched base worktree in this task, the two card failures are classified as unchanged/out-of-scope evidence rather than claimed as a newly reproduced historical baseline.

### Phase 6 final Blueprint, commandlet and asset audit — 2026-07-16

- `CompileAllBlueprints -ProjectOnly`: exit 0; commandlet summary `0 errors / 1 warning / 0 failed loads`. The single Blueprint warning is the existing deprecated `SetOwnedRunFirstPersonCardLayerMenuLeaseFromRunCards` node in `Content/Wacom/Test/WBP_MenuTest.uasset`; overall process summary is 0 errors / 6 warnings, including existing plugin config, two disabled DreamShader include-cache and Python-name warnings.
- Final read-only map commandlets both passed with exit 0 and 0 diagnostics: `/Game/Wacom/Maps/L_Exploration` (validator 0.71 s) and `/Game/Wacom/Maps/Debug/L_RunExploration_Debug` (validator 0.76 s). Two earlier launch attempts reached 180/300-second shell timeouts only while another isolated worktree's `WacomEditor` build held the global UBT/`Build.bat` lock; their target commandlet processes were stopped without reaching map validation or save. After that external build released the lock, both retries passed; this is recorded as environment waiting, not a scene-validation failure.
- `WacomBuildRunExplorationDebugAssets` ran in two consecutive fresh editor processes: both logged `Start` then `Done` and exited 0. A final `Wacom.Editor.RunExplorationDebugAssets` audit passed 2/2, proving dependency failures are read-only, logical output is idempotent, the write set is restricted and all four owned packages are clean after save. A final Debug-map commandlet validation after that audit passed with 0 diagnostics.
- Package-dirty audit: the builder automation's in-process package dirty assertions passed; read-only validators introduced no Git/package mutation; no target-worktree Unreal Editor process remained. Only the expected formal migration and Debug-owned LFS files appear in `git status`.
- All formal/Authoring/shared forbidden-write hashes remain unchanged from the Phase 3/Phase 1 baselines:

| Asset | Final SHA-256 |
|---|---|
| `Content/Wacom/Maps/L_Exploration.umap` | `d295ea91da55637d7131ef5cc55d78f311266e58bb64546be60bf0e8cd940029` |
| `Content/Wacom/Core/GameModes/GM_Wacom.uasset` | `0b24f01517cb70fbf4e3c1c36055699ad0cd2fcc8afc8939d1f52abf5c16c74a` |
| `Content/Wacom/Data/Map/Authoring/DA_Floor_LevelAuthoring_01.uasset` | `d1b4f223d4818ad994583c67add3186606b02620c46006d77ac053009b4c2b66` |
| `Content/Wacom/Data/Map/Authoring/DA_Journey_LevelAuthoring.uasset` | `ea044bf14a636fc1dcb4a0d650c5b9648e52d707796126650af7ccb917f929c8` |
| `Content/Wacom/Core/Player/BP_WacomPlayerCharacter.uasset` | `0fee64d55b7b2be643adce0d3145f6a97cd2a94b92a5149af00eab1659ef59cf` |
| `Content/Wacom/Run/Path/Blueprints/BP_WacomRunMapNodeAnchorActor.uasset` | `ff125e93b6a6e718d50212b50aca5ffa1968df02be9a875d5ee253b06ebc9db5` |
| `Content/Wacom/Run/Path/Blueprints/BP_WacomRunPathSegmentActor.uasset` | `947de5ce8ef5f0c900e088993ea34cedfc158bd38645e4609e0d50639624757d` |
| `Content/Wacom/Run/Path/Blueprints/BP_WacomRunPathBranchTargetActor.uasset` | `977920bef36f24cfb739343a184ac1a5bf0ceaed52e34639a44949da96244ba4` |

- Final Debug-owned hashes after the last automation audit are Journey `7ea99e780946f531bd08155b5cf03630074101836f26ed60b3328e04d2924ea6`, Floor `7e6e93bfdc91dfc204ac0d7b15013f28c04303370dbefc9707dc7478d724fa94`, GameMode `93c84c7ec65403772c6444e2bb96e949167dc907332143521ca4a98c91b29c30`, map `f5d5ea12eb23dee865c28af35c15b8cc03973bfa67120962942b795492aea901`. These are an audit snapshot, not a stable binary-hash contract; logical identity/count/reference idempotence is the contract.
- `git check-attr` reports `filter=lfs diff=lfs merge=lfs` for the formal map/GameMode, both new Authoring assets and all four Debug-owned assets. `git lfs status` contains only expected branch assets. `git diff --check` returned 0 with line-ending conversion notices only. The forbidden diff audit found no `Build.cs`, Config, `WacomRun`, GameplayTag, card/DreamMaterials or shared Player/Run Path Blueprint change.

### Phase 6 PIE and consistency closure — 2026-07-16

- T056 visual PIE golden path was not executed: this task ran through headless `UnrealEditor-Cmd` and cannot honestly approve cursor-look, traversal feel, branch-selection input, first-person hand/camera restoration or Battle/Shop/RunEvent transition presentation. Automated contracts cover scene binding, traversal, map travel and all three return-flow namespaces, but they are not substituted for visual/feel approval.
- Manual follow-up risk remains exactly the seven checks in section 8: initial cursor look; first-W single exit; keyboard/mouse multi-exit selection; M-map travel restore; Battle return; Shop/RunEvent return; and post-PIE validator/no-dirty confirmation on `L_Exploration`.
- T057 consistency review covered `spec.md`, `plan.md`, `data-model.md`, all three contracts and the final long-term Docs. The runtime contract now records the actual working Coordinator prepare followed by one teardown/install/commit; the plan's Docs checklist is complete; the quickstart order matches the implemented dependency order. `Docs/WacomMap.md`, `WacomApp.md`, `WacomDataAuthoring.md`, `Architecture.md`, `TODO.md` and `Questions.md` contain the durable ownership, validation and follow-up facts.
- No artifact commits the current 8-node `Floor.Authoring.01` graph as formal Floor 1 or stable save identity. Formal Floor 1 graph/NodeId/content density, Camp, cross-floor entrance and future SaveGame identity remain explicitly out of scope and tracked only as future decisions/work.

### PIE cursor-look follow-up — 2026-07-16

- The first manual check in section 8 initially failed in a fresh `L_Exploration` PIE run: moving the visible cursor before clicking any Actor produced no visible camera response. The Scene Binding itself resolved and anchored correctly; `RunPath_EntryToBattle` did not suspend camera control.
- Bounded diagnostics identified the lifecycle order `AnchorAtTransform (Tick enabled) -> UWacomRunPathTraversalComponent::BeginPlay (Tick disabled)`. The PIE World was not paused, and no Battle or GameMenu staging path had suspended traversal. The root cause was `BeginPlay()` unconditionally disabling Tick after the already-committed pre-BeginPlay Anchor.
- Added `Wacom.UI.RunPathTraversal.PreBeginPlayAnchorPreservesTick`. Before the production fix it failed exactly at `BeginPlay preserves Tick required by anchored cursor look`; after `BeginPlay` restored Tick from the existing `Anchored / Traversing` state, the test passed. Final `Wacom.UI.RunPathTraversal`: 23 discovered, 23 passed, failed 0; final `WacomEditor Win64 Development` build passed.
- The original manual repro then passed: enter PIE, do not click an Actor or the viewport, move the cursor horizontally, and the camera visibly turns. This approves only section 8 item 1. Single/multiple-exit traversal, Map travel, Battle/Shop/RunEvent returns and the post-PIE validator remain separate checks until recorded below.
- All temporary diagnostic logs and the temporary enlarged look-range probe were removed. Every editor/commandlet launch used and verified `-NoDreamShaderEditorBridge`; generated DreamMaterials/Backpack rewrites from earlier malformed launches were restored exactly to HEAD before the fix, and no card, backpack, enemy material, formal map or other binary asset is part of this follow-up.
- The post-PIE `WacomValidateRunFloorScene -Map=/Game/Wacom/Maps/L_Exploration` check passed with exit 0 and 0 diagnostics. Formal map, `GM_Wacom`, Authoring Floor/Journey and all four shared Player/RunPath Blueprint SHA-256 values remain identical to the Phase 6 final table; binary diff and `git lfs status` contain no asset change. Section 8 item 7 is therefore complete; items 2-6 remain manual presentation/flow checks.
- Final follow-up verification used the cleaned production source: `WacomEditor Win64 Development` passed; `Wacom.UI.RunPathTraversal` 23/23, `Wacom.UI.Camera` 6/6, `Wacom.UI.ExplorationHUD` 1/1, `Wacom.UI.InputContext` 6/6 and `Wacom.UI.RunSceneBinding` 5/5 all passed with process exit 0.
- Follow-up `CompileAllBlueprints -ProjectOnly` passed with exit 0, 0 errors and 0 failed loads. Its single Blueprint warning is the existing deprecated `SetOwnedRunFirstPersonCardLayerMenuLeaseFromRunCards` node in `Content/Wacom/Test/WBP_MenuTest.uasset`; no binary asset changed during this check.

### Post-main Golden Path and native menu mouse closure — 2026-07-17

- The feature branch synchronized current `main` with merge commit `5bf069a3b42bdded228e942242388d52aa7c6933`; no additional `main` merge was performed during this closure. The post-merge default Unity `WacomEditor Win64 Development` build passed, and the merged-branch directed Run/UI verification passed 680/680 across Run scene binding, traversal, camera, exploration HUD, input context, Run Map, Battle, Shop, RunEvent, first-person card layers and Backpack. `CompileAllBlueprints -ProjectOnly` remained 0 errors / 1 existing deprecated-node warning / 0 failed loads.
- Manual post-merge Golden Path approval covered initial cursor look, single-exit traversal, multi-exit selection, same-floor Map travel and the Battle/Shop/RunEvent return paths. A final issue remained: authored Map nodes accepted mouse clicks, while the native Map Travel/Close and Pause Resume/Settings/Quit buttons were visible and focusable but mouse-inert.
- Root cause: pure-C++ `UWacomMenuButtonWidget` fallbacks first created their `WidgetTree.RootWidget` in `RebuildWidget()`, after `UCommonButtonBase::Initialize()` had already attempted to wrap and bind the internal `SCommonButton`. Authored Map nodes already had a WBP root at initialization, explaining the path-specific behavior.
- The final fix builds only the pure-native fallback tree in `InitializeNativeClassData()`, before CommonUI wrapping. Blueprint Generated Class instances continue to duplicate their authored widget tree normally. An intermediate `Initialize()`-before-`Super` approach was rejected when `Wacom.UI.Settings.FocusPresentation.AuthoredSkinBindings` caught that it prevented authored WBP tree duplication.
- `Wacom.UI.GameMenu.PointerRouting.NativeButtonsReceiveSlateMouseClicks` now registers and paints a real `SVirtualWindow`, resolves the actual Slate hit path, routes hover plus left-button down/up, and proves one Pause Resume click and one authored Run Map Travel click. Final default Unity build compiled and linked the production/test slice in 18 actions (`Result: Succeeded`, 26.74 s).
- Final directed automation, all with `-NoDreamShaderEditorBridge`: `Wacom.UI.GameMenu` 9/9, `Wacom.UI.RunMap` 7/7 and `Wacom.UI.Settings.FocusPresentation` 3/3; every process exited 0. The full 1425-test `Wacom` suite was not repeated for this localized CommonUI lifecycle fix; its earlier 1423/1425 result and two unchanged card-asset failures remain recorded above. The focused namespaces cover the changed native fallback, authored Settings WBP preservation, menu staging and Run Map flow.
- Final manual PIE recheck passed every affected mouse path: Map node selection, Travel, Close/Back, Pause Resume, Settings entry and return, Quit-to-main confirmation/cancel, plus any visible native action. The user reported `完成：全部正常`; no remaining PIE mouse-click risk is carried from this fix.
- Post-PIE `WacomValidateRunFloorScene` passed for `/Game/Wacom/Maps/L_Exploration` and `/Game/Wacom/Maps/Debug/L_RunExploration_Debug`, both exit 0 with 0 diagnostics. No target-worktree Editor process remained afterward.
- This mouse closure changes only `UWacomMenuButtonWidget` source, one UI foundation document and one automation spec. It changes no `.uasset/.umap`, Run rule, SaveGame schema, GameplayTag, Build.cs/module dependency, card/backpack/enemy material or DreamShader output. No builder was run.
- Final protected SHA-256 values are unchanged from the post-main baseline: formal map `d295ea91da55637d7131ef5cc55d78f311266e58bb64546be60bf0e8cd940029`; Debug map `f5d5ea12eb23dee865c28af35c15b8cc03973bfa67120962942b795492aea901`; formal GameMode `0b24f01517cb70fbf4e3c1c36055699ad0cd2fcc8afc8939d1f52abf5c16c74a`; Debug GameMode `93c84c7ec65403772c6444e2bb96e949167dc907332143521ca4a98c91b29c30`; Authoring Floor `d1b4f223d4818ad994583c67add3186606b02620c46006d77ac053009b4c2b66`; Authoring Journey `ea044bf14a636fc1dcb4a0d650c5b9648e52d707796126650af7ccb917f929c8`; Debug Floor `7e6e93bfdc91dfc204ac0d7b15013f28c04303370dbefc9707dc7478d724fa94`; Debug Journey `7ea99e780946f531bd08155b5cf03630074101836f26ed60b3328e04d2924ea6`; Player BP `5a5b3a2372e061c2f21745d3431b57ef662666a4490657d366308759594ae8a1`; Anchor BP `ff125e93b6a6e718d50212b50aca5ffa1968df02be9a875d5ee253b06ebc9db5`; Path BP `947de5ce8ef5f0c900e088993ea34cedfc158bd38645e4609e0d50639624757d`; Branch BP `977920bef36f24cfb739343a184ac1a5bf0ceaed52e34639a44949da96244ba4`.
- Final `git lfs status` lists no changed binary package; `git diff --check` is clean apart from repository line-ending notices.
