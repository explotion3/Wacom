---
description: "Dependency-ordered documentation tasks for Floor 1 Production content freeze"
---

# Tasks: 正式 Floor 1 Production 内容合同冻结

**Input**: Design documents from `/specs/011-formal-floor1-production-content-freeze/`

**Prerequisites**: Updated `main` contains the complete Run/Journey integration; this branch/worktree is independent and clean.

**Tests**: This is a documentation-only slice. Static contract analysis, count/identity/schema/economy/AP audits and Git/LFS/range checks replace compile/Automation/PIE; skip reasons remain mandatory.

## Phase 1: Workspace and Rule Truth

**Purpose**: Establish the correct integrated base and live schema before freezing content.

- [x] T001 Confirm branch HEAD and merge-base equal `15b067f63a08feb44993e91de7faa96ba68c0d9f` in `specs/011-formal-floor1-production-content-freeze/quickstart.md`
- [x] T002 Confirm `6f7eb98fb84b0f5d75e248d0d42f7f3ad809542d` is an ancestor and record clean Git/LFS/process state in `specs/011-formal-floor1-production-content-freeze/quickstart.md`
- [x] T003 Read `AGENTS.md`, `Docs/AgentIntegrationWorkflow.md`, `Docs/Architecture.md`, `Docs/WacomRun.md`, `Docs/WacomMap.md`, `Docs/WacomData.md`, `Docs/WacomDataAuthoring.md`, `Docs/TODO.md`, and `Docs/Questions.md`
- [x] T004 Read live Card/Enemy/Part/Behavior/Intent/Encounter/Shop/Pickup/RunEvent headers under `Source/WacomData/Public/` and current tag declarations under `Source/WacomCore/Public/Tags/`
- [x] T005 Reconcile Spec 007/009/010 blockers with integrated live Docs and record superseded assumptions in `specs/011-formal-floor1-production-content-freeze/research.md`

**Checkpoint**: Updated main is the only base; live schema and long-term Docs are authoritative.

---

## Phase 2: Foundational Content Model

**Purpose**: Freeze shared identities, paths, counts and schema boundaries that every content story depends on.

- [x] T006 Define the exact 38-asset category totals in `specs/011-formal-floor1-production-content-freeze/data-model.md`
- [x] T007 Define theme roots, stable ID conventions and package naming in `specs/011-formal-floor1-production-content-freeze/contracts/production-asset-manifest.md`
- [x] T008 Enumerate all 38 exact package paths in `specs/011-formal-floor1-production-content-freeze/contracts/production-asset-manifest.md`
- [x] T009 Record the four allowed existing Shop card dependencies and live `PoisonFang` identity in `specs/011-formal-floor1-production-content-freeze/research.md`
- [x] T010 Define the future builder strict write set and protected read-only dependencies in `specs/011-formal-floor1-production-content-freeze/contracts/production-asset-manifest.md`
- [x] T011 Define frozen versus mutable content/presentation facts in `specs/011-formal-floor1-production-content-freeze/data-model.md`
- [x] T012 Define what the content-design slice closes and the remaining binary/scene/P0 blockers in `specs/011-formal-floor1-production-content-freeze/contracts/production-readiness-gate.md`

**Checkpoint**: Asset identities and production boundary are frozen before individual content values.

---

## Phase 3: User Story 1 — Enemy and Encounter Contract (Priority: P1)

**Goal**: Freeze four reusable enemy archetypes and the six Floor 1 Encounter combinations.

**Independent Test**: 4 Enemy / 11 Part / 4 Behavior / 24 Intent / 6 Encounter counts, unique IDs, current authoring matrix, HP curve and maximum enemy count.

- [x] T013 [US1] Record all 11 PartSlot/PartId/HP/EXP/null reward rows in `specs/011-formal-floor1-production-content-freeze/data-model.md`
- [x] T014 [US1] Record all four Behavior IDs, Default phase, Sequence sets and 24 exact Intents in `specs/011-formal-floor1-production-content-freeze/data-model.md`
- [x] T015 [US1] Record all six Encounter IDs, ordered EnemySlots, HP totals and enemy counts in `specs/011-formal-floor1-production-content-freeze/data-model.md`
- [x] T016 [US1] Freeze behavior/Encounter invariants and Boss payload boundary in `specs/011-formal-floor1-production-content-freeze/contracts/enemy-encounter-contract.md`
- [x] T017 [US1] Audit exact enemy/part/behavior/intent/Encounter counts, IDs, HP and schema values and record results in `specs/011-formal-floor1-production-content-freeze/quickstart.md`

**Checkpoint**: The full battle-content ladder can be implemented without inventing fields or identities.

---

## Phase 4: User Story 2 — Cards, Pickups, Shop and Economy (Priority: P2)

**Goal**: Freeze four new cards, four Pickup mappings, Wayfarer inventory and both route purchase paths.

**Independent Test**: 4 new cards, 4 fixed-card Pickups, 5 Shop offers, existing dependency identities, +2/+3 route Gold and unchanged AP range.

- [x] T018 [US2] Record exact CardId/cost/rarity/keyword/target/effect fields in `specs/011-formal-floor1-production-content-freeze/data-model.md`
- [x] T019 [US2] Record fixed Pickup card mappings and SerpentSigil Credential grant in `specs/011-formal-floor1-production-content-freeze/data-model.md`
- [x] T020 [US2] Record Wayfarer Offer order/prices and Route A/B purchase paths in `specs/011-formal-floor1-production-content-freeze/data-model.md`
- [x] T021 [US2] Freeze card, Pickup, Shop and AP invariants in `specs/011-formal-floor1-production-content-freeze/contracts/card-pickup-shop-contract.md`
- [x] T022 [US2] Audit Card/Effect/Target/rarity/keyword values, fixed Pickup grants, Offer dependencies, route Gold and `8–9 / 14–15 AP` in `specs/011-formal-floor1-production-content-freeze/quickstart.md`

**Checkpoint**: Reward/economy content is implementable using current Card/Pickup/Shop contracts.

---

## Phase 5: User Story 3 — RunEvent Contract (Priority: P3)

**Goal**: Freeze four terminal events and thirteen choices using current RunEvent transactions.

**Independent Test**: 4 Event / 13 Choice counts; all terminal Automatic choices; only current conditions/effects/pressure IDs; required MinGold/RunFlag gates.

- [x] T023 [US3] Define shared one-node terminal event graph and AP semantics in `specs/011-formal-floor1-production-content-freeze/contracts/run-event-contract.md`
- [x] T024 [US3] Record all 13 ChoiceIds, conditions, effects and two RunFlags in `specs/011-formal-floor1-production-content-freeze/data-model.md`
- [x] T025 [US3] Map every design term to live RunEvent enum/field values and persistence boundary in `specs/011-formal-floor1-production-content-freeze/contracts/run-event-contract.md`
- [x] T026 [US3] Audit Event/Choice counts, terminal flags, Automatic policy, negative-delta gates and supported pressure IDs in `specs/011-formal-floor1-production-content-freeze/quickstart.md`

**Checkpoint**: Event content is transaction-compatible and introduces no script or SaveGame promise.

---

## Phase 6: User Story 4 — Long-term Docs and Production Handoff (Priority: P4)

**Goal**: Move the frozen facts into long-term Docs and leave a precise future asset implementation gate.

**Independent Test**: Long-term Docs match Spec 011; Floor 1 content design is Done; 38-asset implementation remains pending; P0/world authority questions remain open.

- [x] T027 [US4] Add the frozen SerpentWood enemy/card/event content contract to `Docs/WacomData.md`
- [x] T028 [US4] Add paths, naming, counts, forbidden references and future builder boundary to `Docs/WacomDataAuthoring.md`
- [x] T029 [US4] Add Floor 1 combat gradient, reward/economy routes and AP evidence to `Docs/WacomMap.md`
- [x] T030 [US4] Close the Floor 1 content-design blocker and add the 38 DataAsset implementation task in `Docs/TODO.md`
- [x] T031 [US4] Preserve and clarify knockdown P0 and Production world asset-authority questions in `Docs/Questions.md`
- [x] T032 [US4] Update `.specify/feature.json` and the managed Spec Kit section in `AGENTS.md` to Spec 011
- [x] T033 [US4] Complete `specs/011-formal-floor1-production-content-freeze/checklists/requirements.md` and `checklists/production-content.md`

**Checkpoint**: Stable facts no longer live only in Spec Kit; remaining production work is not misreported as complete.

---

## Phase 7: Final Static Validation and Delivery

**Purpose**: Verify the documentation bundle, changed-file range and clean handoff.

- [x] T034 Run read-only Spec Kit cross-artifact analysis and resolve all Critical/High/Medium consistency findings
- [x] T035 Run manifest/category/ID/path/schema/HP/economy/AP/forbidden-reference static audits from `specs/011-formal-floor1-production-content-freeze/quickstart.md`
- [x] T036 Run `git diff --check`, changed-file range, binary extension, Git status and Git LFS status audits
- [x] T037 Record final results, documentation diff hash, Unreal skip reasons and remaining production risks in `specs/011-formal-floor1-production-content-freeze/quickstart.md`
- [x] T038 Review the staged diff against `Docs/AgentIntegrationWorkflow.md` and prepare the standard branch/base/final/files/LFS/validation/known-issues handoff

---

## Dependencies & Execution Order

1. Phase 1 establishes the correct integrated base and schema.
2. Phase 2 freezes common identities and package ownership.
3. Phases 3–5 depend on Phase 2 and may be reviewed independently, but all write shared `data-model.md`; execute them in order in the main session.
4. Phase 6 depends on all content contracts so long-term Docs receive final facts once.
5. Phase 7 is read-only until a finding requires a deliberate correction; after correction rerun the full analysis.

## Parallel Opportunities

No subagent is used. The task is documentation-heavy with shared tables and cross-file identities; a single owner avoids conflicting count/ID edits. Static audits can run independently after the documents are stable.

## Completion rule

All tasks through T038 must be checked before the documentation commit. Committing/pushing/merging are delivery operations, not hidden implementation tasks; this branch is committed once, never pushed or merged by this session.
