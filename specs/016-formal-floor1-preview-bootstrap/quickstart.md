# Quickstart / Verification Ledger: Floor 1 Preview Bootstrap

This ledger is updated continuously. It is not a substitute for long-term `Docs/` truth.

## 1. Workspace preflight

**Date**: 2026-07-19

```text
Worktree: D:\UE_Project\5.7\WacomWorktrees\formal-floor1-production-scene-baseline\Wacom
Branch: codex/formal-floor1-production-scene-baseline
Spec 015 HEAD / current initial HEAD: 75db25f9352fb0112c616792481622ccf2414924
Common base with current main: 9fbdb74858b9b4b705a216c8dd7ff539fe44b19e
Observed main HEAD: 5c9ab11e78281a6971ef41bba4c6730513e9df11
Spec 015 in main at preflight: no
```

User-approved execution correction:

- continue in the existing Spec 015 worktree/branch to avoid another hydrated/Binaries/DDC copy;
- treat Spec 016 as dependent on the complete Spec 015 chain;
- do not merge/rebase main into the branch;
- preserve the current dirty map and correct it through the Preview bootstrap;
- do not use a subagent for Unreal MCP;
- do not create any commit before explicit user PIE acceptance.

Initial Git/LFS state:

```text
git status:
 M Content/Wacom/Maps/Run/L_Run_Floor_Main_01.umap

git lfs status:
L_Run_Floor_Main_01.umap: committed oid 352944f... -> worktree file 3f12102...

Committed Spec 015 map SHA-256:
352944F4FC53234CD3EE6E7A7D52BCFF76B1BF20CA7C0CE4D796E109EC579ECF

Current preserved user map SHA-256:
3F121021CAB235B50AF092EBE753242043916E16ABF2A7B4F6E560DA61EC145A
```

Read-only binary string audit shows the dirty map references `/Game/Wacom/Core/GameModes/GM_Wacom` and `GM_Wacom_C`. This matches the user's manual attempt and explains `DescriptorFloorMismatch`: that GameMode supplies a Journey whose current Floor is not `Floor.Main.01`. No PlayerStart identity was found in the audit.

Relevant process audit:

- no Unreal Editor process was open for this target worktree at preflight;
- an Editor on `D:\UE_Project\5.7\Wacom` and an unrelated automation process may be running and are out of scope; they must not be stopped by this task.

`git lfs fsck`: pending final re-run; previous Spec 015 ledger reports OK.

## 2. Spec Kit artifacts

Created:

- `spec.md`
- `plan.md`
- `research.md`
- `data-model.md`
- `contracts/runtime-preview-contract.md`
- `contracts/preview-asset-contract.md`
- `contracts/preview-seed-command-contract.md`
- `contracts/preview-pie-contract.md`
- `tasks.md`
- `checklists/requirements.md`
- `checklists/preview-bootstrap.md`

No unresolved clarification remains. Managed pointers now select Spec 016.

Non-destructive cross-artifact analysis:

```text
Functional requirements: 16
Buildable success criteria: 7
User stories: 3
Tasks: 58
Requirement coverage: 100%
CRITICAL findings: 0
HIGH findings: 0
Unresolved placeholders: 0 (the checklist sentence mentioning the marker is not a marker)
Constitution gate: pass
```

The standard prerequisite script cannot derive a feature from a mandatory
`codex/` branch prefix by itself. With explicit non-mutating overrides
`SPECIFY_FEATURE=016-formal-floor1-preview-bootstrap` and
`SPECIFY_FEATURE_DIRECTORY=specs/016-formal-floor1-preview-bootstrap`, it
resolved the expected feature and all required artifacts.

## 3. Source checkpoint

Completed on 2026-07-19 before any Preview asset mutation:

```text
AssertClosedForBuild: PASS for the target worktree
Default Unity WacomEditor Win64 Development: PASS
Build mode: default Unity, Development, -NoHotReloadFromIDE
Final source build after collision-policy coverage: PASS, 9.51 s
Wacom.App.RunFloorPreviewBootstrap: 3/3 PASS, exit 0
Wacom.UI.RunSceneBinding: 5/5 PASS, exit 0
Final App automation log SHA-256: 571FAD78371268042F6454B32DE272DB2EF58BF41B90D64FB55F541F2306E0A9
```

The runtime prefix covers base resolver compatibility, non-PIE rejection,
missing character/Descriptor, duplicate Descriptor, empty Floor identity,
exact transient Journey fields/defaults, same-instance caching, and drift
rejection. No binary mutation occurred during this checkpoint.

## 4. Editor bootstrap checkpoint

Completed against the preserved pre-seed map without saving any asset:

```text
Wacom.Editor.FormalFloor1PreviewBootstrap: 3/3 PASS, exit 0
Wacom.Data.Map.Validation: 7/7 PASS, exit 0
Wacom.Run.Map.Traversal: 2/2 PASS, exit 0
Wacom.UI.RunPathTraversal: 23/23 PASS, exit 0
Wacom.Editor.RunSceneValidation: 8/8 PASS, exit 0
Focused Editor/regression subtotal: 43/43 PASS
Combined Preview/runtime/regression total: 51/51 PASS
```

Per-prefix `Saved/Logs/Wacom.log` SHA-256 captured after process exit:

```text
Wacom.Editor.FormalFloor1PreviewBootstrap: C1A3DC89ADEFCE2C76EAB2BBBC51B59CB687D72520146CE93EDDD4C54FBA9EED
Wacom.Data.Map.Validation: E6E6490C1E0759C81A9CAD161517511B7A2C016911B73C46BF0308E772B66312
Wacom.Run.Map.Traversal: 2F841C744EBA1BC0CAAE7D78D630CCDB5CC85545C54AED1FC7EFB5845F8AC8F1
Wacom.UI.RunSceneBinding: 66D3DFFDCD3DEA541A6202655061FD32F1777DE641FF36B22826B7CD6755CD5D
Wacom.UI.RunPathTraversal: 8906DBFF76603A0343A6B352711F03D0DF3F4B345BCCEB16AC1946C735AA24A0
Wacom.Editor.RunSceneValidation: 263318171C71CF05A6E11356DA5E7C5FE458ED5EDEA03E6D0132C4E44DDF6840
```

The Editor prefix now includes a pure collision-policy matrix for wrong parent,
compile/config drift, invalid Entry, scene drift, unexpected/duplicate/dirty
PlayerStart, and unknown GameMode. Read-only preflight kept the map at
`3F121021CAB235B50AF092EBE753242043916E16ABF2A7B4F6E560DA61EC145A`.

## 5. Binary pre-hash manifest

Captured after the final source build/tests and before Editor start:

```text
Current dirty map:
3F121021CAB235B50AF092EBE753242043916E16ABF2A7B4F6E560DA61EC145A

Preview Blueprint exists: no

Spec 014 SerpentWood file count: 46
Spec 014 deterministic aggregate method: sorted relative-path=SHA256 records, LF, UTF-8
Spec 014 deterministic aggregate:
162E761AF4678DB1442675DE18651E55E02D1A9E7E74F4A712C03DF909A3ABF8
Content/Wacom/Data diff against HEAD: none

DA_Floor_Main_01:
8391FCFC362D7F4B3B7168D38B3B5746917470BCF335C9089EAFEE4C4C86BA43
BP_EnemyHost_BrushSnake_Graybox:
00AEE67043990FC2644D5437753EF0BCE5F79D4CE868A95D7E50B39E3B6595B8
BP_EnemyHost_MoltGuard_Graybox:
E0E779190E1121FAF9E5115E5A355D3C1C0C128C92609E4BBD0A31EA5BA059BA
BP_EnemyHost_RootStalker_Graybox:
74387C768B30553CF267976A84AE3FC8371F361E3548350860649EC40B1B6BCB
BP_EnemyHost_ShallowGuardian_Graybox:
1ABDFFE0DAE1043DC90C04DA15B53FF3B97316A8FED3B737A8DC1A12CF0674A0
BP_WacomRunFloorEntranceMarker_Graybox:
905193ACB8CF509FF1FDF1ECF1F110C80EA3E6BF8CAD6601D4B07A1B37B62A0D
```

All six non-map Spec 015 hashes exactly match the committed Spec 015 ledger.
`git lfs fsck` passed. Git/LFS reports only the already-owned dirty map among
binary assets; no Preview Blueprint exists yet.

The only allowed save packages are:

```text
/Game/Wacom/Run/Preview/GM_WacomRunFloorPreview
/Game/Wacom/Maps/Run/L_Run_Floor_Main_01
```

## 6. Unreal MCP / writer evidence

```text
Role / endpoint / port: run / ue_wacom_run / 8140
ThreadId: 019f69c0-56e1-7280-b959-6b796da49af0
ProjectRoot: D:\UE_Project\5.7\WacomWorktrees\formal-floor1-production-scene-baseline\Wacom
Branch / HEAD: codex/formal-floor1-production-scene-baseline / 75db25f9352fb0112c616792481622ccf2414924
SessionId: 7e28d402-90e6-4b7a-b306-f10ea77e4cd8
Editor PID / start UTC: 12052 / 2026-07-18T19:18:05.9142895Z
AssertReady: PASS before writer acquisition and release
Writer release: PASS
Writer audit JSON:
C:\Users\ahhh\AppData\Local\Wacom\UnrealMcp\Audits\20260718-192121-288-run-7e28d402-90e6-4b7a-b306-f10ea77e4cd8.json
Writer audit SHA-256:
2A1228D7F7340449D293847AF272CA64A85EB3E596E69044659EEECAAA645504
Seed report: Saved/FormalFloor1PreviewBootstrap/Spec016-preview-bootstrap.json
Seed report SHA-256:
FE4FB9BAA2D0105B1876487DC3744C01C9BEDBD0B63E81354ADE066E14D3DB71
```

The user entered `WacomSeedFormalFloor1PreviewBootstrap` exactly once after
identity and writer verification. No builder was run:

```text
First pass: created=1 modified=1 existing=0 saved=2 failed=0
Second pass: created=0 modified=0 existing=2 saved=0 failed=0
Actual saved packages:
/Game/Wacom/Run/Preview/GM_WacomRunFloorPreview
/Game/Wacom/Maps/Run/L_Run_Floor_Main_01
```

The writer explicitly took over the already-dirty map at pre-write SHA
`3F121021...145A`. The audit reports only the new Preview Blueprint as a new
dirty path and no out-of-allowlist binary change.

## 7. Asset validation

Passed after writer release and normal Editor close:

- AssetRegistry returns exactly one `UBlueprint` and one `UWorld` for the two packages.
- Preview Blueprint loads, recompiles without `BS_Error`, has the exact parent/copied CDO configuration, and has null `DefaultJourneyDefinition`.
- Map selects Preview GameMode and has exactly one plain `PlayerStart_FloorMain01Preview` at Entry.
- The reused Spec 015 validator accepts exact `1 Descriptor / 20 Anchors / 21 Paths / 4 BranchTargets / 16 content Hosts / 8 enemy Hosts / 11 viewpoints` and all bindings.
- The 46 Spec 014 aggregate remains `162E761AF4678DB1442675DE18651E55E02D1A9E7E74F4A712C03DF909A3ABF8`.
- Floor Definition, four enemy Blueprints, and Exit marker retain their six pre-write hashes.
- Preview Blueprint final SHA-256: `4FF24B329D1B8FF1AE891BBB1C0872B653D9866DAE2498EC5B114A3B25942AD0`.
- Map post-bootstrap/pre-PIE SHA-256: `0412EC2CB59DC747FD545DD0965164AAE7445E03214D80A4C252EA8B7F87BB3F`.
- Both targets use LFS; `git lfs fsck` passes.
- Before PIE, Editor close, Blueprint compile automation, and all regressions left both target hashes unchanged.

Final post-asset verification:

```text
Default Unity WacomEditor: PASS (post-asset test-source checkpoint, 9.25 s)
Wacom.App.RunFloorPreviewBootstrap: 3/3 PASS
Wacom.Editor.FormalFloor1PreviewBootstrap: 3/3 PASS
Wacom.Data.Map.Validation: 7/7 PASS
Wacom.Run.Map.Traversal: 2/2 PASS
Wacom.UI.RunSceneBinding: 5/5 PASS
Wacom.UI.RunPathTraversal: 23/23 PASS
Wacom.Editor.RunSceneValidation: 8/8 PASS
Total: 51/51 PASS, 0 failed
```

Final log SHA-256 values:

```text
Preview App: B46EED1D573A1EEDC5F4DE393846E62EAB656C9028723E94FE43087D0324C30D
Preview Editor/AssetRegistry/Blueprint: 3B83FAEF3A0CA6BA691273174DAE024AE6840D7710C28687088CF1B835DE2AD0
Data Map: 864469101BF53177584A5885E9E0778465908ADA25BA0638E03F18604188C46B
Run Traversal: 7E22038F2095BFE5C63C02D1ECFB1800DE4FC9D0F1153D436F89D1F036F29541
Scene Binding: 6F0B645AF1AC84DED955A171D09C4C59597981B5B2AA53F55D6D16C52CE6E37F
Path Traversal: 9BBE38CBB0719DE4FA27260F44278ECC9AF98DF4117A25D7C02AD1A83869A3AA
Scene Validation: F717BDD4A26CFF624F6A76CE9BE16581D74B7A4AA9A3105CFEEE09FD1662DE44
```

Two post-asset test startups overlapped another worktree's
`WacomBuildEnemyPack` and stopped before test discovery at engine
initialization. Only the two owned stalled commandlet PIDs were terminated;
the unrelated worktree was untouched. After its command sequence ended and a
10-second quiet window was observed, the complete 51-test set passed above.

## 8. User PIE gate

No commit may be created until every item passes and the user explicitly confirms it.

Open `/Game/Wacom/Maps/Run/L_Run_Floor_Main_01` directly and start PIE:

1. Log contains Preview Journey `Journey.Preview.Floor.Main.01` and `Floor.Main.01`.
2. No `MissingJourney`, `DescriptorFloorMismatch`, or `SceneBindingSnapshotInvalid`.
3. Player starts/anchors at Entry.
4. Mouse look, W/S, HUD, and hand work.
5. M map opens/closes and buttons click.
6. ESC menu opens/closes and buttons click.
7. Complete one path movement.
8. Complete one battle and return.
9. Complete/close at least one Event, Reward, or Shop interaction and return.
10. After returns, mouse, W/S, M, ESC, HUD, and hand still work.
11. Exit is reachable but has no interaction, transition, or Journey Summary.
12. Stop PIE; the Preview GameMode hash remains unchanged. If the map was intentionally adjusted and saved, explicitly accept the new hash and rerun focused validation before commit.

```text
PIE result: PASS
User acceptance: "没有问题，整理提交" (2026-07-19)
Preview Blueprint post-PIE SHA-256:
4FF24B329D1B8FF1AE891BBB1C0872B653D9866DAE2498EC5B114A3B25942AD0
Final user-authored map SHA-256:
2F22B9956FB05D0F1249D121010109208526873072B61BAE6224096888E921C1
```

PIE log evidence contains the exact Preview startup:

```text
[WacomRunFloorPreviewGameMode] Preview Journey ready:
Journey=Journey.Preview.Floor.Main.01 Floor=Floor.Main.01
Descriptor=WacomRunFloorSceneDescriptorActor_0 Character=DA_Character_BugGirl
```

The same session records a non-terminal Battle victory and two Aid reward cards
returning through `Floor.Main.01/Node.Main.01`. Log audit found zero
`MissingJourney`, `DescriptorFloorMismatch`, or
`SceneBindingSnapshotInvalid`, and no Journey Summary path.

The map hash changed from `0412...` because the user deliberately adjusted and
saved the map during the accepted PIE session. The user explicitly selected
the resulting `2F22...` file as the authoritative version; it was not reverted.
That exact on-disk version then passed `Wacom.Editor.FormalFloor1PreviewBootstrap`
3/3, `Wacom.UI.RunSceneBinding` 5/5, and
`Wacom.Editor.RunSceneValidation` 8/8. Their final log hashes are respectively
`80E5790E0743E4AF42B3F9A6F998AC74F9FC644659EC7392AF0D1CE7A0E9BBB7`,
`6842FEC9BE68CB61F9E4029D695D084D0835FD04D9EC82A76524FAA323D5922F`,
and `5070578CE22FFD5522CCB68367FEAD6D1B2AC7C1C5B5BE56BDAC4D4CF1B18AC8`;
the map and Preview Blueprint hashes remained stable across those checks.

Any later source or target asset correction resets this gate.

## 9. Continuous risks and skips

- **History risk**: Spec 016 depends on the full Spec 015 chain because the user chose to continue in the old branch. Integration must preserve order and binary authority.
- **Dirty binary risk**: the starting map is a user-owned uncommitted file. It must not be restored, replaced, or accidentally included as the Spec 015 authority.
- **Release risk**: a Production map referencing the Preview GameMode is a release blocker until the full three-Floor Production Journey and startup path exist.
- **Exit risk**: Floor 1 Exit remains a non-interactive graybox dead end.
- **Save risk**: Preview has no success terminal and is not a supported saved/continued Run.
- **Viewport tooling risk**: Spec 015 observed viewport capture dirtying/auto-saving a map. Avoid programmatic viewport capture in this slice.
- **Builder skip**: all content builders remain forbidden; only the named Preview command may save the exact targets.
- **Main-menu skip**: main-menu flow is intentionally unchanged and is not part of Preview PIE.
- **Floor2/3 skip**: no asset exists for cross-floor or full Journey validation.

## 10. Deferred commit gate

Final Spec Kit and repository audit before staging:

```text
Spec Kit cross-artifact analysis: PASS
Functional requirements: 16/16 covered
Success criteria: 7/7 covered
Tasks: 58 total; 55 complete before the three commit/handoff finalization tasks
CRITICAL/HIGH findings: 0
Unresolved clarification markers: 0
git diff --check: PASS (line-ending warnings only)
Forbidden-scope audit: PASS
git lfs status: expected map plus new Preview Blueprint only
git lfs fsck: PASS
Target AssertClosedForBuild: PASS
```

PID 7052 was identified as an unrelated Editor for
`D:\UE_Project\5.6\Youyu\Ylll.uproject`; it was not touched. The target Wacom
worktree had no Editor/Live Coding process and passed the project-specific
closed-for-build gate.

Planned only after explicit user PIE success:

```text
feat(app): add run floor preview bootstrap
feat(content): enable floor1 preview startup
```

Authorization and finalization state:

```text
User PIE authorization: received 2026-07-19
Authoritative map hash: 2F22B9956FB05D0F1249D121010109208526873072B61BAE6224096888E921C1
Commit count for Spec 016: 2
Source/Spec/Docs commit: 3c4c22fd2722020df2a358c439f999b8c56e2391
Content commit: this ledger's final HEAD; exact hash is reported in the handoff
Merge main: forbidden
Push: forbidden
```
