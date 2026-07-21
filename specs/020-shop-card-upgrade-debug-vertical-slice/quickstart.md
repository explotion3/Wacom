# Quickstart / Validation Log: Debug Shop 卡牌强化可玩竖切

## Baseline

- Branch: `codex/shop-card-upgrade-debug-vertical-slice`
- Worktree: `D:\UE_Project\5.7\WacomWorktrees\formal-floor1-production-scene-baseline\Wacom`
- Base/main: `a8d0c3566452a0d5d3304f24fad801c4ad3ea763`
- Spec 019 integration: default Unity PASS; 19 prefixes 89/89 PASS; 59 Card/Shop aggregate `AE150725EB34C979FADFA3BC057F84F263469FAD04410444711EA00482E9B22F`; main/source Git/LFS clean and fsck OK.
- Editor/Live Coding at branch creation: none.

## Scope guard

- No GameplayTag, SaveGame, Build.cs, module dependency, map, Floor, Host, Journey or Character mutation.
- No full builder/regenerate.
- Exact binary allowlist: two Debug cards, `DA_Shop_DebugSnake`, `WBP_ShopScreen`.
- User PIE acceptance required before any Spec 020 commit.

## Checkpoints

### Spec and live-source audit

- Status: complete; prerequisite check passed for the complete Spec 020 artifact set.
- CodeGraph MCP was attempted but its transport was closed; live source was read directly with `rg`/PowerShell.
- Existing seam confirmed: Run Quote/Command/Result complete; App Shop Screen purchase-only; Shop tag has fallback but no config WBP; Debug ShopBuilder owns 24-offer defaults.

### UI/source checkpoint

- Default Unity `WacomEditor Win64 Development`: PASS after UI slice; PASS again after the targeted seeder/Editor tests (`2026-07-20`).
- `Wacom.UI.Shop.UpgradePresentation + UpgradeScreen + UpgradePIEValidation`: 5/5 PASS (`Saved/Logs/Spec020-UI-Shop-Upgrade-Retry.log`).
- `Wacom.Editor.DebugShopUpgradeVerticalSlice`: 3/3 PASS in the authoritative pre-seed state (`Saved/Logs/Spec020-Editor-PreSeed.log`). The inspect view reported one existing target (`DA_Shop_DebugSnake`) and three missing targets without mutation.
- Run/UI regressions (`Wacom.Run.Shop.CardUpgrade`, `Wacom.Run.NodeActivity.Shop`, `Wacom.Run.NotificationCoalescing`, `Wacom.Run.SnapshotRevisions.ShopMutationPaths`, `Wacom.UI.Shop`): 32/32 PASS (`Saved/Logs/Spec020-Checkpoint1-Regressions.log`).
- No builder or binary save occurred during this checkpoint.

### MCP asset checkpoint

- `AssertClosedForBuild`: PASS before Editor start.
- Verified session: role `run`, endpoint `ue_wacom_run:8140`, session `39af7316-9b8d-465d-ba77-3c071ca6b50b`, PID `29416`, exact ProjectRoot/branch/base HEAD.
- Writer lease acquired by thread `019f69c0-56e1-7280-b959-6b796da49af0` for exactly the four allowlisted Packages. The recorded baseline contains only the owned Spec020 source/config/spec changes; none of the binary targets were dirty.
- Initial seed report: `3 created / 1 modified / 4 saved / 0 failed`; its internal second pass was `0 created / 0 modified / 0 saved / 0 failed`.
- Initial seed exposed two diagnostics that the first comparator did not promote to failure: UE 5.8 handled ensures for missing WidgetTree GUID entries, plus four Data Validation errors because both cards' Damage/Poison `Effect.Target` was `None`. Assets were preserved; writer audit `20260720-161414-286-run-39af7316-9b8d-465d-ba77-3c071ca6b50b.json` released cleanly.
- Root-cause repair: the WBP builder now populates deterministic GUIDs for every source widget; card effects explicitly use `Target.SingleEnemyPart`; strict inspection checks both contracts and recognizes only the exact prior missing-target state as repairable. A pre-repair regression run failed exactly `RepairRequiredCount=2` (`Saved/Logs/Spec020-Repair-Red.log`).
- Repair seed report: `0 created / 2 modified / 2 saved / 0 failed`; only the White/Blue cards were saved. Its second pass was fully idempotent. Repair writer audit `20260720-162359-719-run-f9fa0aa1-0521-41ec-a8b5-dc1a522410b2.json` released cleanly.
- Final SHA-256:
  - Blue: `CB6FAAE8B1DC52C85D80D142980AD209DAA6E908E551EFEB34F12DDFA1AAF376`
  - White: `555058B6D80CBE002F5EF158A6B93E7B1042CACE8F8022E6F153E4FF940EC374`
  - Debug Shop: `33FE8644359F820B6D622C78BC675D1A554F5AB7FE90AFF5D9DA17E40A0659D5`
  - Shop WBP: `F6C780D3C1615CEF365F274799D3DD47DD716724FA6F28155BAD58A5FDFE9602`
  - Four-file aggregate (sorted repo-relative `path<TAB>UPPER_SHA256`, UTF-8 no BOM, LF including final LF): `4F67DCCA17D50472F9BB78AF8A40698B6109E9E269986EA51BDD68DEF0054689`.
- All four targets are Git LFS; `git lfs fsck` PASS.

### Final checkpoint

- `AssertClosedForBuild`: PASS after both writer sessions closed.
- Final default Unity `WacomEditor Win64 Development`: PASS.
- Focused Automation: 48/48 PASS (`Saved/Logs/Spec020-Final-Focused.log`), covering Editor manifest, Data Card/Shop, Run Shop transaction/revisions/notifications and full `Wacom.UI.Shop`.
- Persisted asset gate: 5/5 PASS (`Saved/Logs/Spec020-Final-AssetRegistry-WBP.log`): four exact AssetRegistry entries/classes, real load, WBP compile, full WidgetTree GUID contract, parent, global `UI.Widget.ShopScreen` registration and command presence. No new ensure.
- Protection audit: 97 tracked LFS assets (Debug/Production maps, BugGirl character, existing CardView and all SerpentWood/MoltCavern DataAssets) match their HEAD LFS SHA-256 exactly; `0 mismatch / 0 non-LFS`.
- Spec Kit analysis: 12 FR, 6 SC and 39 tasks; 100% requirement coverage, no consistency or constitution findings.
- `git diff --check`: PASS (line-ending conversion warnings only); scope audit PASS; no map/character/Production/CardView changes.
- Initial complete PIE: PASS; the later visual refinement below reopens a focused PIE gate.
- Commit: forbidden until PIE passes.

### Post-PIE visual refinement (`2026-07-21`)

- The first complete manual PIE route passed. User then requested a presentation refinement, so the commit gate was reopened and a focused re-PIE is required.
- Both Debug definitions now display as `试制毒牙`. Upgrade identity remains the distinct White/Blue `CardId` pair under the same family; rarity border and changed values carry the visual distinction.
- `FWacomShopUpgradePresentationBuilder` marks only changed next-card effect badges with passive `Increased / Decreased` emphasis. `UWacomCardEffectBadgeWidget` maps that semantic to static digit tint without activating Battle feedback material; ordinary card surfaces stay neutral.
- Same-name success copy is `已强化：试制毒牙（White → Blue）` instead of repeating identical names around an arrow.
- Default Unity `WacomEditor Win64 Development`: PASS before mutation and PASS after Editor shutdown (final target up to date).
- Presentation/Screen regression: 5/5 PASS (`Saved/Logs/Spec020-UpgradeVisualRefinement.log`).
- Final focused regression: 50/50 PASS, 0 failure/ensure signals (`Saved/Logs/Spec020-Final-Focused-Refined.log`). This includes exact persisted asset inspection, Data validators, Run Shop transactions/revisions/notifications and full `Wacom.UI.Shop`.
- After moving same-name success copy into the passive presentation builder and adding its exact assertion, the final incremental Unity build PASS and `Wacom.UI.Shop` is 21/21 PASS with 0 failure/ensure signals (`Saved/Logs/Spec020-Final-UI-Shop-Refined.log`).
- Refinement MCP session: role `run`, endpoint `ue_wacom_run:8140`, session `02755b1d-cfaf-4252-8178-cff38ca61cb3`, PID `21744`, exact ProjectRoot/branch/base HEAD.
- Refinement writer audit: `C:\Users\ahhh\AppData\Local\Wacom\UnrealMcp\Audits\20260721-145345-776-run-02755b1d-cfaf-4252-8178-cff38ca61cb3.json`.
- Targeted migration report: first pass `0 created / 1 modified / 1 saved / 0 failed`, saving only Blue; second pass `0 created / 0 modified / 0 saved / 0 failed`.
- Refined SHA-256:
  - Blue: `0188B26B71B7189D0323FC93C3A03A0018ACAF9D011F6AFB7A78251B1FB78F4E`
  - White: `555058B6D80CBE002F5EF158A6B93E7B1042CACE8F8022E6F153E4FF940EC374` (unchanged)
  - Debug Shop: `33FE8644359F820B6D622C78BC675D1A554F5AB7FE90AFF5D9DA17E40A0659D5` (unchanged)
  - Shop WBP: `F6C780D3C1615CEF365F274799D3DD47DD716724FA6F28155BAD58A5FDFE9602` (unchanged)
  - Four-file aggregate with the same sorted `path<TAB>UPPER_SHA256` UTF-8/LF algorithm: `A6712D2BC7B9186295109A38F521EFBE2B2A40A058BBB4B382829B57437778B4`.
- All four targets resolve to Git LFS; `git lfs fsck` and `git diff --check` PASS. No map, Character, Production or shared CardView binary was saved.
- Final refined PIE: user accepted the refined presentation and explicitly authorized commit. PIE did not rewrite any of the four target asset hashes. Commit gate opened only after Editor shutdown and `AssertClosedForBuild` passed.
