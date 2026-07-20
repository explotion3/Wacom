# Quickstart / Verification Ledger: Spec 019

## Baseline

- Worktree: `D:\UE_Project\5.7\WacomWorktrees\formal-floor1-production-scene-baseline\Wacom`
- Branch: `codex/shop-card-upgrade-baseline`
- Base/main/merge-base at start: `bc209e7e1fd89a4bb82c26688d1cccef54819432`
- Target worktree Git/LFS: clean
- Unreal Editor / Live Coding / ShaderCompileWorker at start: none
- New worktree created: no
- Binary mutation authorized: no
- Spec Kit prerequisites: PASS using process-local `SPECIFY_FEATURE=019-shop-card-upgrade-baseline`; the repository script otherwise rejects the project-approved `codex/*` branch naming convention.
- Cross-artifact analysis: 22/22 functional requirements covered; 49 tasks mapped; 0 critical/high findings; both checklists have 0 incomplete items.

## Checkpoint 1 — Spec and Data

- Status: PASS
- Compile: default Unity `WacomEditor Win64 Development` PASS (`2026-07-20`, 72 actions); test-fixture correction rebuild PASS (4 actions).
- Tests: `Wacom.Data.Card` 7/7 PASS (includes all 3 `Wacom.Data.CardUpgrade` cases); `Wacom.Data.Shop` 5/5 PASS; 0 failures.
- Notes: the first automation attempt exposed a test-only fixture error because abstract `UObject` was used as an outer. Replacing it with a concrete transient `UCardDefinition` removed the ensure; no runtime or content contract changed. No binary asset was created or saved.

## Checkpoint 2 — Run transaction

- Status: PASS
- Compile: default Unity `WacomEditor Win64 Development` PASS after the canonical Actor → PlayerController → Router → RunSession request path was connected.
- Tests: `Wacom.Run.Shop.CardUpgrade` 3/3; `Wacom.Run.NodeActivity.Shop` 2/2; `Wacom.Run.NotificationCoalescing` 5/5; `Wacom.Run.SnapshotRevision` 5/5; `Wacom.UI.Shop` 15/15; 0 failures.
- Notes: only the selected card instance changes Definition. InstanceId, physical zone/index, special-zone owner/flag and commerce AP semantics remain stable. Existing Shop screen stays purchase-only; new Quote/Result contracts are passive groundwork for Spec 020.

## Checkpoint 3 — Cross-system compatibility

- Status: PASS
- Compile: default Unity `WacomEditor Win64 Development` PASS. A test-only pointer constness mismatch in the added rejection matrix was caught on the first attempt and corrected; the subsequent build passed.
- Tests: `Wacom.Run.CardUpgradeCompatibility` 3/3; `Wacom.Run.Save` 10/10; `Wacom.Run.CardWorkspace` 2/2; `Wacom.Run.Event.CardPaymentChoice` 1/1; `Wacom.Run.WorldCardInteraction` 9/9; `Wacom.Run.Deck.BuildInitParams` 3/3; `Wacom.Run.Deck.DeleteCardForGoldByRarity` 1/1; `Wacom.Battle.RuleContentMatrix` 6/6; `Wacom.UI.CardPresentation.RuntimeContext` 1/1. The final `Wacom.Run.Shop.CardUpgrade` rejection matrix and existing transaction cases passed 4/4. No runtime test failures remain.
- Notes: family matching applies only to AllowedCardIds; exact Definition filters remain exact. SaveGame stays v5 and roundtrips the upgraded DefinitionAssetPath plus unchanged InstanceId. App recycle display and Run economy now share the `White/Blue/Yellow/Purple/Intrinsic = 1/2/3/4/0` query. Rejected service, missing-price, insufficient-gold, terminal-Run, stale and non-owned requests are silent and atomic.

## Checkpoint 4 — Final validation

- Status: PASS; changes intentionally remain unstaged and uncommitted for user review.
- Compile: explicit Unity `WacomEditor Win64 Development -DisableAdaptiveUnity` PASS. The first explicit Unity attempt caught an anonymous-TU ambiguity between a generic new `FFixture` and an existing scene fixture; both new test fixtures were renamed to domain-unique names and the second Unity build passed. This has no runtime impact and prevents the clean-commit Unity failure that adaptive dirty-worktree builds can hide.
- Tests: 19 focused prefixes, 89/89 PASS, 0 failures: Data Card/Shop/Map validation, Floor 1/2 formal content loads, Run Shop/CardUpgrade/compatibility/NodeActivity/notifications/revisions/Save/workspace/Event payment/world interaction/Battle init/Backpack, Battle rule matrix, existing Shop UI and CardPresentation runtime context.
- AssetRegistry/failed-load/hash: PASS. Read-only registry enumeration loaded `56 CardDefinition + 3 ShopDefinition = 59` assets, with `0 missing / 0 failed load` and no project package load errors. Aggregate SHA-256 over sorted `relative/path<TAB>file-sha256` UTF-8 lines was identical before/after: `AE150725EB34C979FADFA3BC057F84F263469FAD04410444711EA00482E9B22F`.
- Spec Kit: prerequisites PASS; 22/22 functional requirements covered by 49 dependency-ordered tasks; requirements checklist 16/16 and content checklist 13/13 complete; no critical/high cross-artifact finding.
- Git/LFS/range audit: `git diff --check` PASS; `git lfs fsck` PASS; no staged files and no `.uasset/.umap`, `Content/`, GameplayTag, SaveGame schema, `Build.cs` or module dependency changes. The ignored `specs/019-shop-card-upgrade-baseline/` files are present and will require intentional force-add only after user approval.
- Process note: the target worktree had no Editor/Live Coding process. Editors later observed on `C:\Users\ahhh\.codex\worktrees\0b47\Wacom` and `D:\UE_Project\5.7\WacomWorktrees\enemy-next\Wacom` were unrelated and were not stopped or modified.

## Deliberately skipped

- Builder: prohibited; no production content is authored this round.
- Blueprint compile: no Blueprint or WBP changes.
- PIE: no player-accessible upgrade UI or real upgrade chain assets exist; behavior is covered by automation.

## Known follow-up

- Spec 020 owns formal Shop Buy/Upgrade tabs, before/after comparison, confirmation, AppToast, first Production upgrade chains and shop prices.
- The accepted external `DA_Character_BugGirl` StarterDeck pollution remains outside this branch. This round neither modifies that asset nor weakens any Production dependency validator, and does not claim the unrelated global closure issue is fixed.
