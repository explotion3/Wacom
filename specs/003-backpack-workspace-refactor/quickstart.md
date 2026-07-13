# Quickstart and Validation: Backpack Workspace Refactor

## Scope

Use this guide after each implementation slice. Automated checks validate Run atomicity, App state/lifecycle and stable input semantics. PIE validates visual rhythm, mouse feel, WBP binding and DreamShader presentation.

## Workspace Safety Before Implementation

```powershell
git status --short
git diff -- Source/WacomRun Source/WacomApp Source/WacomTests Docs specs/003-backpack-workspace-refactor
```

The current workspace already contains unrelated Battle/first-person-card/DreamShader work. Do not restore, rewrite or stage those files as part of this feature.

## Slice A — Run Batch Transaction Validation

### Automated contract cases

Add focused cases under `Wacom.Run.Backpack`:

1. Two-card cross-zone success moves both cards and emits one state-change notification.
2. Last-card capacity failure moves zero cards and keeps revision unchanged.
3. Duplicate, invalid and stale-source InstanceIds fail with zero changes.
4. SpecialZone Owner removal between preview and commit fails atomically.
5. Two-card delete grants the exact summed existing reward once.
6. One invalid delete item deletes zero cards and grants zero gold.
7. Multi-capacity-provider deletion produces the correct final burden without exposing intermediate states.
8. Existing single-card APIs retain parity for one-card requests.

Quantitative atomicity gate:

- Repeat rejected batch-move scenarios 50 times and rejected batch-delete scenarios 50 times (100 total transaction runs).
- Every repetition must report zero partial success and preserve cards, gold, burden/pressure, revision and notification count.

### Focused run

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -ExecCmds='Automation RunTests Wacom.Run.Backpack; Quit' -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache
```

Expected: all tests pass; failure-path tests assert zone arrays, SpecialZones, gold, burden/pressure, revision and notification count remain unchanged.

## Slice B/C — Workspace State and Interaction Validation

### Automated interaction cases

Add focused cases under `Wacom.UI.Backpack.Workspace`:

- ZoneKey normalization and stable InstanceId reconcile.
- Same-Run screen reactivation/restoration and new-Run reset.
- Card-center marquee hit; read-only/projection exclusion.
- Click/Ctrl-click/Ctrl-marquee/blank-click/Ctrl+A selection semantics.
- Initial release guard consumes pickup release without placement.
- Repeat the pickup/initial-release transition 50 times in one deterministic automation case; misplaced-card count must remain zero.
- Default rightmost/highest-Z card is current but not lifted.
- Wheel up/down clamps, does not wrap, and only non-default current lifts.
- Later left release removes only current card; right release removes all.
- Remaining fan order and current-index clamp after each release.
- No index/count display field is populated.
- 30% visible clamp and Arrange All layout reset.
- Region switch, Deactivate and invalidating refresh release input/capture and clear transient state.
- Delete cancel and failed confirm restore carry snapshot exactly.

### Focused UI run

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -ExecCmds='Automation RunTests Wacom.UI.Backpack; Quit' -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache
```

## Compile Checkpoint

Run after every source slice and again after final migration:

```powershell
& 'E:\UE_5.8\Engine\Build\BatchFiles\Build.bat' WacomEditor Win64 Development -Project='D:\UE_Project\5.7\Wacom\Wacom.uproject' -WaitMutex -NoHotReloadFromIDE
```

Expected: exit code 0 with no new UHT/Build.cs/public-private include violations.

## PIE Setup

Prepare a test Run with:

- at least 20 physical cards;
- a non-empty BattleDeck;
- flux content cards;
- at least one valid SpecialZone with content;
- a BurdenZone case;
- at least one projected/read-only card;
- enough capacity to test success and a variant that forces rejection.

Open the backpack through the normal PlayerController/CommonUI route, not by constructing a standalone Widget in Level Blueprint.

编辑器内可先用以下命令建立可重复的结构/框选/携带基线：

```text
Wacom.Backpack.SeedPIEValidation
```

该命令只在 Editor build 注册，使用正常 `URunSession::AcquireCardToRun()` 把当前 Run 补到至少 24 张实体牌和 2 个 SpecialZone，不绕过 Run 规则，也不会自动保存。它不会伪造 SpecialZone 内容、Burden 或拒绝条件；事务拒绝、确认恢复和特殊区内容仍需按本节清单准备。

## PIE Checklist — Structure and Layout

- [x] Right-side zone rack remains visible while switching zones.
- [x] Central workspace displays exactly one active zone.
- [ ] With at least 20 cards and 4 available zones, a fresh tester can switch zones and identify the sole central active workspace within 10 seconds; record the measured time.
- [x] Each rack entry shows correct title/count/capacity/active feedback.
- [ ] Manual positions, angles and ZOrder survive zone switch and same-Run close/reopen.
- [ ] New Run starts without the previous Run's manual layout.
- [x] New cards enter a readable default layout; removed/moved cards leave no ghost entry.
- [ ] Backpack cards embed `WBP_FirstPersonCardView`; the automated asset contract confirms fixed `0.75` ScaleBox scaling, pixel snapping and the dedicated feedback overlay, but face content and bleed badges still need final PIE visual comparison against the authored layout.
- [ ] Cards cannot be placed with more than about 70% outside the workspace.
- [x] Arrange All restores upright readable default layout.
- [ ] Dropping onto the active zone rack collects cards without changing Run rule order/revision.

## PIE Checklist — Selection and Carry Feel

- [x] Blank drag draws a selection rectangle and selects cards by center point.
- [ ] Normal click, Ctrl click, Ctrl marquee, blank click and Ctrl+A match the contract.
- [x] Projection/read-only cards never enter the selection.
- [x] Pickup release only enters carry and never drops a card.
- [x] Carried cards form a stable fan following the cursor.
- [ ] Start carry, then move the cursor rapidly outside the pressed card and across the Workspace/rack; the fan continues following without requiring the cursor to re-enter a card.
- [ ] Rightmost/highest-Z card is default current and has no lift.
- [ ] Wheel up/down changes current; only a non-default current card lifts.
- [ ] Wheel stops at both ends without wrapping.
- [x] No `3 / 8`, index or count indicator appears near cursor or screen top.
- [ ] Later left click/release drops one current card and keeps the rest carried.
- [ ] Right click/release drops all remaining cards.
- [ ] Fan closes smoothly and keeps stable order after each single release.

## PIE Checklist — Transactions and Confirmation

- [ ] Valid cross-zone single release moves only current card.
- [ ] Valid cross-zone group release moves all cards in one visible refresh.
- [ ] Capacity/Owner/stale rejection moves zero cards, keeps carry and shows one understandable Toast/feedback.
- [ ] Delete target shows one confirmation with card count and total reward.
- [ ] Cancel restores the exact carry set and current card.
- [ ] Confirm success deletes all and grants exact total gold once.
- [ ] State change during confirmation causes atomic failure and restores carry.

## PIE Checklist — CommonUI Lifecycle

- [ ] Escape/Back closes the screen with no stuck cursor capture.
- [ ] Switching zone during selection/carry cancels safely and preserves completed layouts.
- [ ] Deactivate/reactivate does not duplicate provider subscriptions or state-change callbacks.
- [ ] Detail panel does not block marquee, carry, rack drop or confirmation input.
- [ ] Confirmation modal owns focus and returns focus correctly on cancel.
- [ ] Controller/keyboard can still navigate rack and close/back, even though free placement is mouse-first.

## PIE Checklist — Visual and DreamShader Layer

- [x] Workspace remains fully functional with optional material effects disabled.
- [ ] Selection overlay, current-card lift, valid target and rejected target are visually distinguishable; overlay color never changes card scale or hit geometry.
- [ ] Fake-3D/material response does not move hit geometry or change drop targeting.
- [x] Motion values come from presentation style/WBP defaults rather than Run rules.
- [x] DreamShader source is Wacom-native `.dsm`/`.dsh`; no Godot Demo code, shader or asset was copied.
- [ ] 20–100 cards do not cause obvious idle Tick cost or per-frame Snapshot rebuilds.

## Migration Search

After the new path is complete:

```powershell
rg -n "UWacomCardDragOperation|NativeOnDragDetected|UWacomZoneDropTarget|DeleteZoneHost|BattleDeckZoneHost|FluxContentDropTargetHost|SpecialZonesHost|BurdenZoneHost" Source/WacomApp Source/WacomTests Docs
```

Expected:

- No old class remains as a second input owner.
- Any remaining symbol is a documented migration facade, test of removed behavior, or historical note with an explicit cleanup task.
- `Docs/UI_Backpack_WBP_Binding.md` describes only the new formal binding contract.

## Final Documentation Check

- [x] `Docs/WacomRun.md` describes batch transaction semantics.
- [x] `Docs/WacomUI.md` describes Snapshot → workspace → intent → Screen flow → Run batch command.
- [x] `Docs/WacomApp.md` describes transient Run-scoped layout owner and lifecycle.
- [x] `Docs/UI_Backpack_WBP_Binding.md` describes Workspace/ZoneRack/confirmation assets and passive WBP rules.
- [x] Any deferred WBP/material/controller work is in `Docs/TODO.md`; any unavoidable dual-path debt is in `Docs/TechDebt.md`.

## Full Suite (Optional Final Confidence)

After focused suites and compile pass:

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -ExecCmds='Automation RunTests Wacom; Quit' -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache
```

If skipped, record why and the remaining cross-feature risk in the implementation handoff.
