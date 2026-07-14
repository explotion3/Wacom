# Contract: Backpack Workspace Input and Carrying

## Input Owner

`UWacomBackpackScreen` owns the active input lease and coordinates a passive Workspace. Individual card widgets emit pointer intent but do not start independent UMG drag operations or call Run APIs.

## Selection

| Input | Result |
|---|---|
| Left click unselected movable card | Replace selection with that card |
| Left press already-selected movable card (without Ctrl) | Enter carry immediately on pointer-down; no movement threshold |
| Ctrl + left click movable card | Toggle that card |
| Left click blank workspace | Clear selection |
| Left drag blank workspace | Replace selection with cards whose centers enter marquee |
| Ctrl + left drag blank workspace | Toggle marquee hits against drag-start snapshot |
| Ctrl+A | Select all movable physical cards in active zone |

Read-only/projection cards never enter selection, even if their geometry intersects the marquee.

## Enter Carry

- Pressing an already-selected movable card without Ctrl carries the whole selection immediately, before any pointer movement.
- Pressing and releasing an unselected movable card only selects it. Dragging that unselected card past the movement threshold remains a shortcut that first makes it the sole selection and enters carry.
- Ctrl + left click only changes selection and never enters carry.
- Carrying creates a fan following pointer movement.
- The button release that ends pickup is consumed by `InitialReleaseGuard`; it places nothing.
- `InitialReleaseGuard` belongs only to the pickup pointer sequence. If that sequence's pointer-up is lost during mouse-capture transfer, observing any later left/right pointer-down disarms the guard before the new gesture releases.
- No index/count label is shown.

## Current Card and Wheel

- The rightmost/highest-Z fan card is default current and is immediately releasable.
- Default current has no lift.
- Wheel up moves current one card left; wheel down moves one card right.
- Wheel clamps at ends and does not wrap.
- A non-default current card is the only lifted card.

## Release

After the initial guard is consumed:

- Left click/release submits only current card to the pointer target.
- Right click/release submits all remaining cards to the pointer target.
- Workspace target places card(s) and retains fan transforms as manual layout.
- Active same-zone rack collects card(s) into default layout without Run mutation.
- Different-zone rack submits one atomic move request.
- Delete target opens one batch confirmation.
- Invalid/rejected targets keep affected cards in carry and show feedback.

After a successful current-card release, remaining cards keep stable relative order, close the fan, and clamp current index. Carry exits only when no cards remain or a whole-group operation succeeds.

Run-backed partial releases defer synchronous ViewModel/Snapshot refresh until the released identities have been committed out of Carry. The surviving Carry adopts the successful transaction's storage revision so consecutive left releases remain valid.

## Cancellation and Lifecycle

The following cancel active selection/carry, release mouse capture, close transient hover/detail state and return to idle:

- active-zone switch;
- CommonUI Deactivate/Destruct;
- binding to a different Run;
- Snapshot refresh invalidating the source zone or any carried identity;
- explicit Back/Escape cancellation when no modal confirmation owns input.

Delete confirmation is special:

- opening confirmation suspends Workspace input and mouse capture with an immutable restore snapshot, while keeping the fan frozen and visible;
- cancel restores the exact remaining order/current card;
- confirm revalidates; failure restores carry and shows reason;
- success removes only the confirmed identities and resumes the remaining fan; it exits carry only when no cards remain.

## Focus and Device Scope

- Mouse capture belongs to the Workspace/Screen only while marquee or carry is active.
- Confirmation modal owns focus while visible and restores focus to the Workspace on cancel.
- Existing CommonUI Back/close and zone navigation remain keyboard/controller reachable.
- Full controller free placement is outside this slice.

## Required Tests

- Initial pickup release never places a card.
- A new left/right release works on its first click even if the pickup pointer-up was lost.
- Already-selected card press starts carry before pointer movement, while its matching release only consumes the pickup guard.
- Default rightmost card is current, highest Z, not lifted and can be released first.
- Wheel lift/clamp/no-wrap semantics.
- Left releases one; right releases all.
- Synchronous Run refresh cannot clear the unreleased fan during a partial rack move or delete, and the surviving fan can submit another transaction with the refreshed storage revision.
- No index/count presentation field exists or becomes visible.
- Marquee center hit and read-only exclusion.
- Deactivate/zone switch/invalid refresh release capture and clear state.
- Delete cancel/failure restores carry exactly.
