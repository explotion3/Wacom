# Content Contract Quality Checklist: 正式 Floor 1 击倒分支奖励卡内容冻结

**Purpose**: Review the requirements for exact card authoring, Part assignment, reward volume and Production handoff
**Created**: 2026-07-18
**Feature**: [spec.md](../spec.md)

**Review mode**: Standard pre-implementation author/reviewer gate

## Requirement Completeness

- [x] CHK001 Are all eight CardIds and package paths explicitly specified? [Completeness, Spec §FR-001/FR-009]
- [x] CHK002 Are all authorable card fields named for every card, including required empty fields? [Completeness, Spec §FR-002/FR-007]
- [x] CHK003 Are all eleven FormalProduction Part assignments documented? [Completeness, Spec §FR-010]
- [x] CHK004 Are critical-route and full-exploration reward quantities documented? [Completeness, Spec §FR-012]
- [x] CHK005 Are the 46-asset total and future builder write-set boundary documented? [Completeness, Spec §FR-014]

## Requirement Clarity

- [x] CHK006 Are Effect order, magnitude and Target unambiguous for every card? [Clarity, Spec §FR-006/FR-008]
- [x] CHK007 Are Cost, Rarity and Keyword distributions quantified rather than described as approximate progression? [Clarity, Spec §FR-003/FR-005]
- [x] CHK008 Is “每部位一次” distinguished from creating one card pair per Part? [Clarity, Spec §FR-011]
- [x] CHK009 Is package identity distinguished from stable CardId and display text? [Clarity, Spec §FR-001/FR-009]

## Requirement Consistency

- [x] CHK010 Are card fields consistent among spec, data model and card manifest? [Consistency]
- [x] CHK011 Are Part counts and mappings consistent with Spec 011's `2/3/2/4` enemy topology? [Consistency]
- [x] CHK012 Are repeat semantics consistent with the unchanged Spec 012 runtime contract? [Consistency]
- [x] CHK013 Are route rewards consistent with unchanged `8–9 / 14–15 AP` pacing? [Consistency, Spec §FR-013]

## Scenario and Edge-Case Coverage

- [x] CHK014 Are last-part victory, Withdraw, full-hand and repeated-Archetype cases addressed without inventing new rules? [Coverage, Spec Edge Cases]
- [x] CHK015 Is the mixed Player/enemy targeting requirement for Aid cards explicit? [Coverage, Spec Edge Cases]
- [x] CHK016 Are forbidden Production dependencies and legacy-field requirements explicit? [Coverage, Spec §FR-010/FR-015]
- [x] CHK017 Are backpack-volume and non-card branch effects intentionally deferred rather than silently resolved? [Coverage, Assumption]

## Acceptance and Delivery Quality

- [x] CHK018 Can every count, distribution, route total and forbidden path be objectively audited? [Measurability, Success Criteria]
- [x] CHK019 Are Unreal validation skips tied to the text-only scope and future exit criteria? [Traceability, Spec §FR-018]
- [x] CHK020 Is the allowed changed-file surface explicitly bounded? [Traceability, Spec §FR-017]

## Notes

- `20/20` requirements-quality items pass.
- Focus areas: content authoring precision and Production handoff risk.
- Runtime implementation verification is intentionally outside this checklist.
