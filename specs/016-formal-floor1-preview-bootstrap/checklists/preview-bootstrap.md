# Preview Bootstrap Requirements Checklist: Floor 1 Preview 启动层

**Purpose**: Requirements-quality gate for runtime isolation, binary authority, and PIE acceptance
**Created**: 2026-07-19
**Feature**: [spec.md](../spec.md)

## Requirement completeness

- [x] CHK001 Are direct-map Preview goals and the unchanged main-menu route both explicitly specified? [Completeness, Spec §Frozen Scope/Non-Goals]
- [x] CHK002 Are the exact two mutable packages and every forbidden asset/schema/module category named? [Completeness, Spec §Frozen Scope/Non-Goals]
- [x] CHK003 Are the transient Journey fields, owner, lifetime, and non-persistence semantics fully defined? [Completeness, Spec §FR-004–FR-006]
- [x] CHK004 Are Blueprint source fields and the forced empty default Journey enumerated exactly? [Completeness, Spec §FR-007]
- [x] CHK005 Are PlayerStart identity, count, transform source, and absence of Run identity defined? [Completeness, Spec §FR-008]

## Requirement clarity and consistency

- [x] CHK006 Is “PIE only” defined as both an Editor build boundary and exact PIE world type? [Clarity, Spec §FR-003]
- [x] CHK007 Is same-instance caching consistent with fail-closed Descriptor/Floor drift behavior? [Consistency, Spec §FR-006]
- [x] CHK008 Is the allowed migration from temporary `GM_Wacom` override distinguished from an authority conflict? [Clarity, Spec §US2]
- [x] CHK009 Are “no builder” and the named Preview seed command consistently distinguished? [Consistency, Spec §Non-Goals/FR-009]
- [x] CHK010 Is the branch-continuation exception explicit without implying main was merged or Spec 016 is standalone? [Clarity, Spec §Continuation Constraint]

## Scenario and edge coverage

- [x] CHK011 Are missing, duplicate, invalid, and drifting Descriptor cases all addressed? [Coverage, Spec §Edge Cases]
- [x] CHK012 Are wrong Blueprint authority, unexpected PlayerStart, invalid Entry Anchor, and broken scene preflight addressed before save? [Coverage, Spec §FR-010]
- [x] CHK013 Is partial save/reload failure handled as a reported failure without destructive cleanup? [Coverage, Contract: Seed Command]
- [x] CHK014 Is the post-acceptance change invalidation rule defined for runtime and binary changes? [Coverage, Spec §FR-015]
- [x] CHK015 Are Exit dead-end behavior and absent Journey Summary/transition unambiguous? [Coverage, Spec §US3]

## Acceptance criteria quality

- [x] CHK016 Are scene preservation counts and idempotence counts objectively measurable? [Measurability, Spec §SC-003–SC-004]
- [x] CHK017 Are startup error diagnostics named for the PIE gate? [Measurability, Spec §SC-001]
- [x] CHK018 Are mouse, movement, HUD/hand, M/ESC clicks, battle return, interaction return, and Exit behavior all represented in the manual acceptance? [Completeness, Spec §SC-005]
- [x] CHK019 Is post-PIE hash authority explicit, including stable Preview GM and the acceptance/revalidation gate for intentional user-authored map changes? [Measurability, Spec §SC-007]
- [x] CHK020 Is Preview's release blocker and formal removal condition documented? [Dependencies, Spec §Assumptions]
