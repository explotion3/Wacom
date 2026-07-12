# Contract: Atomic Backpack Batch Storage Operations

## Purpose

Define the only authoritative path used by the new backpack for multi-card cross-zone movement and multi-card delete-for-gold.

## Ownership

- Public request/result contract: `WacomRun`.
- Validation and mutation algorithm: `WacomRun/Private/Deck`.
- Submission and one final broadcast: `URunSession`.
- Preview text and toast mapping: `WacomApp`.
- Widgets never call this contract directly.

## Batch Move

### Validate

`ValidateMoveInstancesAtomic(Request)` is read-only and returns one result for the entire collection.

It must reject:

- empty or duplicate InstanceIds;
- invalid/missing instances or definitions;
- an instance no longer in `ExpectedSource`;
- a SpecialZone key with a missing/invalid Owner;
- a same-zone request (handled as App collect, not Run move);
- any capacity, ownership, B-owner, burden or existing move-rule violation;
- any storage revision mismatch; Workspace-originated batch requests always use strict revision guarding and cannot opt out.

### Commit

`MoveInstancesAtomic(Request)` must:

1. Copy authoritative Run state into a working state.
2. Re-run full validation against current authoritative facts.
3. Require `ExpectedStorageRevision` to equal the current authoritative storage revision before any working-state mutation.
4. Apply every move to the working state in request order.
5. Recompute dependent storage/burden facts on the working state.
6. Commit the complete working state only if all items succeed.
7. Advance storage/state revision and broadcast exactly once.

### Failure guarantee

On false/failed result, the following remain byte-for-byte or semantically unchanged:

- all physical zone arrays and SpecialZone contents;
- card flags and owner entries;
- gold and pressure/burden;
- storage/state revision;
- `OnRunStateChangedNative` broadcast count.

## Batch Delete for Gold

### Preview

`ValidateDeleteCardsForGoldAtomic(Request)` returns:

- collection allow/deny;
- stable disabled reason;
- card count;
- total reward computed with existing single-card reward rules.

Preview is informational and cannot reserve cards or authority.

### Commit

`DeleteCardsForGoldAtomic(Request)` revalidates after confirmation, requires the request revision to exactly match the current storage revision, removes every card on a working state, recomputes dependent capacity/burden, grants the summed reward once, then commits and broadcasts once.

If any card fails permanent-remove validation at commit time, the whole request fails with zero reward and zero mutation.

## Compatibility

- Existing single-card `MoveInstance` and `DeleteCardForGoldByInstance` remain valid for other call sites.
- They may internally delegate to one-item batch helpers after parity tests pass.
- The new Workspace must never implement multi-card semantics by looping public single-card APIs.

## Required Tests

- Successful 2+ card move emits one broadcast and one revision change.
- Capacity failure on the last item moves zero cards.
- Duplicate, invalid, stale-source and stale-revision requests change nothing.
- Successful 2+ card delete grants exact summed reward once.
- Any invalid delete item deletes zero cards and grants zero gold.
- Removing multiple capacity providers either succeeds with correct final burden or fails atomically under existing rules.
- SpecialZone target/Owner disappearance between preview and commit fails atomically.
