# Requirements Checklist: 正式 Floor 1 Production 内容合同冻结

**Purpose**: Verify the specification is complete, unambiguous, testable and bounded before long-term Docs sync.
**Created**: 2026-07-17
**Feature**: [spec.md](../spec.md)

## Scope and boundaries

- [x] CHK001 The spec states this is a documentation-only design freeze.
- [x] CHK002 Source, Build.cs, GameplayTag, SaveGame, runtime contracts and binary assets are explicit non-goals.
- [x] CHK003 Art, Host Blueprint, world transform and scene staging are explicitly deferred.
- [x] CHK004 Knockdown Aid/Destroy/Withdraw remains a separate P0 and null rewards are not described as final design.
- [x] CHK005 Formal Journey/Floor/map creation and builder execution are outside this slice.

## Content completeness

- [x] CHK006 The exact 38-new-asset total and category breakdown are specified.
- [x] CHK007 All four enemies, eleven parts, four behaviors and six Encounters have measurable contracts.
- [x] CHK008 All four new cards, four Pickups and five Wayfarer offers have measurable contracts.
- [x] CHK009 All four RunEvents and thirteen Choices have measurable contracts.
- [x] CHK010 Node-to-Definition mapping covers exactly 15 content nodes with 6/4/4/1 distribution.

## Current-schema compatibility

- [x] CHK011 Every requested card/intent Effect and Target is named using a current authoring value.
- [x] CHK012 Rarity and keyword values are current declared GameplayTags; no new tag is implied.
- [x] CHK013 RunEvent conditions/effects, pressure IDs and signed deltas map to current fields.
- [x] CHK014 Pickup Card+Credential uses the current atomic contract.
- [x] CHK015 Existing `PoisonFang` identity/path is explicit and cannot be mistaken for a fifth new card.

## Acceptance and handoff

- [x] CHK016 Encounter HP curve, maximum enemy count and route Gold paths are measurable.
- [x] CHK017 Floor 1 AP remains `8–9 / 14–15` with Shop as the only range source.
- [x] CHK018 Forbidden dependency review distinguishes manifest rows from explanatory prohibition text.
- [x] CHK019 Unreal compile/Automation/AssetRegistry/Builder/Blueprint/PIE skips and later validation obligations are explicit.
- [x] CHK020 Success criteria include changed-file/range/binary and cross-artifact consistency checks.

## Notes

- All requirements-quality items pass before long-term Docs are edited.
- Live Docs and headers override obsolete blocker language in earlier Spec Kit artifacts.
