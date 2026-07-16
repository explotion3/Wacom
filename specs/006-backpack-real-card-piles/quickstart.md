# Quickstart: 背包真实卡牌牌堆与即时携带

## Prerequisites

- Project: `C:\Users\ahhh\.codex\worktrees\0b47\Wacom\Wacom.uproject`
- Engine: `E:\UE_5.8`
- Close the interactive Unreal Editor before commandlet asset generation.
- Preserve unrelated Cowart workspace changes.

## 1. Compile

```powershell
& 'E:\UE_5.8\Engine\Build\BatchFiles\Build.bat' WacomEditor Win64 Development -Project='C:\Users\ahhh\.codex\worktrees\0b47\Wacom\Wacom.uproject' -WaitMutex -NoHotReloadFromIDE -DisableUnity
```

Expected: `WacomApp`, `WacomEditor` and `WacomTests` compile with no UHT or link errors.

## 2. Build formal assets twice

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\Users\ahhh\.codex\worktrees\0b47\Wacom\Wacom.uproject' -run=WacomBuildBackpackUI -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache
```

Run twice. Expected: `WBP_BackpackZonePile` exists with required bindings, `WBP_WacomDeckCardWidget` hosts `WBP_FPCardView`, no PreviewHost/PilePreview dependency remains, and the second run creates no hierarchy/reference drift.

## 3. Focused automation

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\Users\ahhh\.codex\worktrees\0b47\Wacom\Wacom.uproject' -ExecCmds='Automation RunTests Wacom.UI.Backpack; Quit' -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache
```

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\Users\ahhh\.codex\worktrees\0b47\Wacom\Wacom.uproject' -ExecCmds='Automation RunTests Wacom.Run.Backpack; Quit' -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache
```

Also run the focused FirstPersonCardView prefix discovered from current tests.

## 4. PIE layout and interaction matrix

Validate at `1280×720`, `1920×1080`, 16:10 and ultrawide:

1. Open Backpack with 0, 1, 3, 15 and 21 cards in representative piles.
2. Verify collapsed cards are full real faces, front card is rightmost/topmost, click expands the pile, and no card is selected while collapsed.
3. Verify expanded frame grows to contain its fan and chooses left/right direction near edges.
4. Verify physical cards support click/marquee/carry; projection, Special owner and Burden locked cards remain readonly with correct visual identity.
5. Hover an expanded card and confirm only that card uses dynamic feedback.
6. Carry 1, 7, 15 and 21 cards; move slowly, rapidly and outside the original card bounds; confirm no cumulative lag.
7. Use the wheel, release current card, partially release into another pile, trigger atomic rejection and cancel with Escape.
8. Validate carry-hover auto-expand, pile movement/snap, batch delete, B direct close and reopen in the same Run.

## 5. Performance observation

- Compare 1-card and 21-card carry with `stat unit`, `stat slate` and Unreal Insights when available.
- Confirm pointer movement does not trigger layout or visual updates for non-carried cards.
- Confirm at most one Backpack card Retainer is in realtime mode.
- Record any hardware-specific residual spike here and in `Docs/TechDebt.md`; do not report an unmeasured pass.

## 6. Final consistency

```powershell
git diff --check
rg -n "PilePreviewWidgetClass|UWacomBackpackPilePreviewWidget|PreviewHost|WBP_BackpackCardView" Source Docs specs/006-backpack-real-card-piles
git status --short
```

Expected: matches remain only in explicit removed-contract text, not production source or current binding documentation.

## 7. Actual validation record — 2026-07-16

- Baseline: branch/HEAD/status and unrelated Cowart changes were recorded before implementation. The first pre-change command attempt timed out before producing a reliable automation result, so no baseline pass count is claimed.
- Compile: `WacomEditor Win64 Development -DisableUnity` succeeded, 682/682 actions, with only existing DreamShader deprecation warnings.
- Builder migration: the legacy DeckCard child type required one migration save. After migration, two cold commandlet runs returned exit code 0; SHA-256 comparison across the seven formal Backpack assets reported `ChangedAssetCount=0`.
- `Wacom.UI.Backpack`: 68/68 passed.
- `Wacom.Run.Backpack`: 2/2 passed.
- `Wacom.UI.FirstPersonCardLayer`: 282/282 passed.
- Carry follow-up regression: before the fix, 12 pointer updates produced 12 redundant retained-card depth applications (`4 -> 16`). The fixed path keeps the count unchanged, removes the duplicate Workspace pointer dispatch, and passes `Wacom.UI.Backpack` 68/68, `Wacom.Run.Backpack` 2/2 and `Wacom.UI.FirstPersonCardLayer` 282/282.
- Pile expansion follow-up regression: an equivalent expanded-scene reconcile cancelled both in-flight card transitions (`2 -> 0`), producing a partial move followed by a snap to the fan. The fixed path preserves identical-target transitions, interpolates from an immutable start, and passes the focused `RealPileSceneIdentity` test plus `Wacom.UI.Backpack` 68/68 and `Wacom.Run.Backpack` 2/2.
- Collapse/release chain regression: focused tests first reproduced collapse transitions sharing one header-center target, a released card losing CarryLayer ownership before target reconcile, one unintended A-to-B transition, and the card not landing directly at B. The fixed path derives each collapse target from the final collapsed layout and lets the target Scene consume released CarryLayer cards directly. The focused Workspace suite passes 15/15.
- Carry Slate-root regression, first attempt: placing the entire fan under a moving `CarryCache` removed interpolation but did not solve the reported count-dependent hitch. The dynamic current card remained inside that cache, so pointer translation and realtime Retainer invalidation still crossed the whole cached subtree. This attempt was superseded rather than accepted as the final performance contract.
- Carry/cross-zone root-cause regression: focused red tests reproduced all structural defects. For 1/7/15/21 cards, `CarryActiveLayer` held `0` while `CarryCache` held `1/7/15/21`; `CarryCache` moved and the intended outer root did not. A physical A-to-B reconcile created one replacement Widget, changed identity, failed to apply the B zone to the carried instance, and removed it from carry ownership. The fixed hierarchy moves only outer `CarryRoot`, caches `N-1` non-current cards under stationary `CarryCache(CarryLayer)`, and keeps exactly one realtime card in sibling `CarryActiveLayer`. Cross-zone physical reconcile rekeys the protected instance by `InstanceId`, applies target ViewData in place, and leaves target Scene responsible for the single final reparent.
- Follow-up compile and validation: incremental `WacomEditor` compile succeeded. Focused `Workspace.Carry` passed 2/2, formal binding passed 1/1, full `Wacom.UI.Backpack` passed 69/69, and `Wacom.Run.Backpack` passed 2/2. A later `WacomBuildBackpackUI` run returned exit code 0 and SHA-256 remained unchanged for all seven formal Backpack assets.
- Real release-window regression: the earlier tests applied B immediately and therefore skipped the frame the user actually saw. A new red test first observed that `CarryActiveLayer`'s current card was rewritten as static during every Presentation refresh, then reproduced a successful model commit before target reconcile: carry ownership was lost, one A-to-B transition was created, and the card did not land directly at B. The fix recognises both carry branches and adds an explicit pending visual handoff consumed only by the target Scene. The focused test now passes, incremental `WacomEditor` compile succeeds, and the full `Wacom.UI.Backpack` suite remains 69/69. PIE hand-feel/performance is still pending and is not inferred from the automation result.
- Zone-pile lifecycle regression: a focused red test cleared the transient pile registry while retaining Canvas children and reproduced linear orphan growth (`2 -> 4 -> 6 -> 8 -> 10 -> 12`). Reconcile now treats the actual `PileFrameLayer` children as the durable visual owner, reuses matching piles, removes unused/orphan children, and `NativeDestruct` explicitly tears down dynamic pile widgets. `WacomEditor -DisableUnity` succeeded, `Wacom.UI.Backpack` passed 71/71, `Wacom.Run.Backpack` passed 2/2, and the user confirmed 10 consecutive expand/collapse cycles without duplicate or inert pile frames in PIE.
- PIE layout/interaction matrix: pending manual validation.
- 1-card versus 21-card Slate/Insights observation: pending manual measurement; no measured performance pass is claimed yet.
