# Feature Specification: [FEATURE NAME]

**Feature Branch**: `[###-feature-name]`

**Created**: [DATE]

**Status**: Draft

**Input**: User description: "$ARGUMENTS"

## Wacom Rule Context *(mandatory)*

**Primary Domain**: [Battle rules / Battle UI / Run-exploration / Data-card
authoring / UI-App shell / World interaction / Save-load / Architecture /
Testing]

**Rule Truth Docs**:
- [ ] `AGENTS.md`
- [ ] `Docs/Architecture.md`
- [ ] [Domain doc]
- [ ] [Related UI/data/authoring/binding doc]

**Expected Owning Module(s)**: [WacomCore / WacomData / WacomBattle / WacomRun /
WacomApp / WacomEditor / WacomTests]

**Non-Goals / Boundaries**:
- [What this feature explicitly does not change]
- [Modules, docs, assets, or workflows that must remain untouched]

**Open Rule Questions**:
- [NEEDS CLARIFICATION: Any design truth, asset semantics, UI lifecycle, or
  SaveGame boundary that affects implementation]

## User Scenarios & Testing *(mandatory)*

<!--
  User stories should be prioritized as independently testable Wacom gameplay,
  UI, data-authoring, or editor workflows. Each story should deliver a useful
  slice without relying on later stories.
-->

### User Story 1 - [Brief Title] (Priority: P1)

[Describe the player/designer/developer journey in plain language.]

**Why this priority**: [Explain value and why this is the MVP slice.]

**Independent Test**: [Automation prefix, editor validation, or manual flow that
proves this story works on its own.]

**Acceptance Scenarios**:

1. **Given** [initial Run/Battle/UI/Data state], **When** [player/designer/system
   action], **Then** [observable outcome]
2. **Given** [initial state], **When** [edge action], **Then** [safe outcome]

---

### User Story 2 - [Brief Title] (Priority: P2)

[Describe this user journey in plain language.]

**Why this priority**: [Explain value.]

**Independent Test**: [How this story can be tested independently.]

**Acceptance Scenarios**:

1. **Given** [initial state], **When** [action], **Then** [expected outcome]

---

### User Story 3 - [Brief Title] (Priority: P3)

[Describe this user journey in plain language.]

**Why this priority**: [Explain value.]

**Independent Test**: [How this story can be tested independently.]

**Acceptance Scenarios**:

1. **Given** [initial state], **When** [action], **Then** [expected outcome]

---

[Add more user stories as needed.]

### Edge Cases

- What happens when the relevant `PersistentId`, DataAsset reference, GameplayTag,
  target handle, card instance, or runtime snapshot is invalid?
- How does the flow behave across CommonUI Activate/Deactivate, screen rebuild,
  battle/run transition, or save/load restore if applicable?
- What happens when player resources, cards, capacity, HP, pressure, nodes, or
  shop/event inventory are insufficient?
- What is the rollback behavior for failed RunEvent/shop/backpack/world
  interaction transactions?

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST [specific gameplay/UI/data capability].
- **FR-002**: System MUST preserve Wacom module dependency direction and expose
  only documented public contracts.
- **FR-003**: System MUST keep authoritative Battle/Run/Data rules outside
  widgets, scene-only Actors, and presentation view data.
- **FR-004**: System MUST update relevant Wacom docs when rule truth, authoring
  semantics, binding contracts, or validation expectations change.
- **FR-005**: System MUST include compile and focused validation for the
  implemented slice, or document skipped validation risk.

*Examples of marking unclear requirements:*

- **FR-006**: System MUST persist [NEEDS CLARIFICATION: SaveGame field not
  specified - in-memory only, v4 schema, or deferred?]
- **FR-007**: System MUST expose [NEEDS CLARIFICATION: Blueprint access not
  specified - designer-facing field, protected C++ seam, or no reflection?]

### Wacom-Specific Requirements *(include as applicable)*

- **Docs-first evidence**: [Which `Docs/` files must be read and updated?]
- **Module/API boundary**: [Which public contract is added/changed? What stays
  private?]
- **Data/GameplayTag impact**: [New DataAsset fields, tags, generated content,
  validators, authoring matrix, or N/A]
- **Battle contract impact**: [Command, Snapshot, Event, ResultPacket, Target
  validation, resolver, effect executor, or N/A]
- **Run contract impact**: [URunSession API, FRunState, transaction rollback,
  PersistentId, SaveGame, revision, or N/A]
- **UI/App boundary**: [Snapshot/ViewModel/ViewData source, command/event flow,
  focus/input ownership, lifecycle and unsubscription behavior, or N/A]
- **Testing expectation**: [Automation prefix, test file area, manual editor
  validation, compile command]
- **Temporary debt**: [None, or TODO(技术债) entry and Docs/TODO.md/TechDebt.md
  plan]

### Key Entities *(include if feature involves data/state)*

- **[Runtime State]**: [BattleState/FRunState/transient UI state and owner]
- **[Static Asset]**: [DataAsset, DataTable, map actor, WBP binding, tag, or
  content asset]
- **[Public Contract]**: [Snapshot/ViewData/Command/Result/Event/API]
- **[Identity]**: [InstanceId, PersistentId, GameplayTag, slot identity, asset
  path, or other stable ID]

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: [Player/designer/developer can complete the primary workflow with
  expected observable result.]
- **SC-002**: [Focused automation or manual validation proves the MVP story
  independently.]
- **SC-003**: [No Wacom module boundary violation or undocumented public API is
  introduced.]
- **SC-004**: [Relevant docs and validation commands are updated or explicitly
  marked not applicable.]

## Assumptions

- [Assumption about scope, e.g. "No SaveGame persistence in this slice."]
- [Assumption about current assets/content, e.g. "Existing generated test cards
  can be reused."]
- [Assumption about UI, e.g. "C++ fallback layout remains acceptable until WBP
  binding is updated."]
- [Assumption about validation, e.g. "Focused automation prefix is sufficient
  before full Wacom suite."]
