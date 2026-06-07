# Feature Specification: RunEvent Card Reward

**Feature Branch**: `001-runevent-card-reward`

**Created**: 2026-06-07

**Status**: Draft

**Input**: User description: "添加一个RunEvent奖励界面，让玩家从三张卡牌奖励中选择一个，预览细节，确认一个奖励，然后返回探索。简单测试不需要太详细"

## Wacom Rule Context *(mandatory)*

**Primary Domain**: Run-exploration / UI-App shell

**Rule Truth Docs**:
- [x] `AGENTS.md`
- [x] `Docs/Architecture.md`
- [x] `Docs/WacomRun.md`
- [x] `Docs/WacomUI.md`
- [x] `Docs/WacomApp.md`
- [x] `Docs/WacomData.md`
- [x] `Docs/UI_RunEvent_WBP_Binding.md`

**Expected Owning Module(s)**: WacomRun, WacomApp, WacomData, WacomTests

**Non-Goals / Boundaries**:
- This feature does not change Battle rules, Battle UI, or Battle reward packets.
- This feature does not introduce SaveGame persistence for RunEvent state.
- This feature does not require a new card reward drafting system outside RunEvent.
- This feature does not let widgets directly mutate Run state.

**Open Rule Questions**:
- None for the initial lightweight spec. Default assumption: the three reward
  cards are authored as RunEvent choice data or equivalent RunEvent payloads,
  and the UI presents them through existing card presentation data.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Choose a RunEvent Card Reward (Priority: P1)

The player opens a reward RunEvent, sees three card reward choices, previews
each card's details, confirms one card, receives that card in the current Run,
and returns to exploration.

**Why this priority**: This is the full player-facing reward loop and is enough
for an MVP validation slice.

**Independent Test**: Run a focused RunEvent UI/manual flow that opens a sample
reward event, selects one reward, verifies the chosen card is added once, closes
the event, and returns to exploration.

**Acceptance Scenarios**:

1. **Given** an active RunEvent with three valid card rewards, **When** the
   player opens the event, **Then** all three rewards are visible and each can
   show card details before confirmation.
2. **Given** the player has previewed a reward, **When** the player confirms
   that reward, **Then** only that selected card is granted to the Run and the
   reward event closes.
3. **Given** the reward event has closed after confirmation, **When** the player
   returns to gameplay, **Then** exploration input and Run UI resume normally.

### Edge Cases

- If fewer than three valid rewards are authored, the screen should show only
  valid rewards and surface a configuration warning rather than granting an
  invalid card.
- If the selected reward cannot be granted, confirmation should fail without
  closing the event or granting any partial result.
- Re-confirming after a successful selection should not grant duplicate cards.
- The screen should behave safely across CommonUI Activate/Deactivate and close
  paths.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST support a RunEvent reward presentation with up to three
  card reward choices.
- **FR-002**: Player MUST be able to preview card details for each visible reward
  before confirming.
- **FR-003**: Player MUST be able to confirm exactly one reward choice.
- **FR-004**: Successful confirmation MUST grant only the selected card to the
  current Run.
- **FR-005**: Successful confirmation MUST close the reward event and return the
  player to exploration.
- **FR-006**: Failed confirmation MUST leave Run state unchanged and keep the
  player in a recoverable UI state.
- **FR-007**: System MUST preserve Wacom module dependency direction and expose
  only documented public contracts.
- **FR-008**: System MUST keep authoritative RunEvent and card grant rules in
  WacomRun/Data contracts, with WacomApp UI acting as passive presentation and
  command flow.
- **FR-009**: System MUST include compile validation and a simple focused
  RunEvent UI/manual validation path for the slice.

### Wacom-Specific Requirements *(include as applicable)*

- **Docs-first evidence**: Update `Docs/WacomRun.md`, `Docs/WacomUI.md`, and
  `Docs/UI_RunEvent_WBP_Binding.md` if the implementation changes the RunEvent
  transaction, UI data, or binding contract.
- **Module/API boundary**: WacomRun owns the reward transaction result; WacomApp
  owns `UWacomRunEventScreen` presentation and confirmation flow; WacomData owns
  authored reward definitions if new data shape is needed.
- **Data/GameplayTag impact**: No new GameplayTag is expected. DataAsset impact
  is limited to RunEvent card reward authoring if existing `GainCard` effects
  are insufficient.
- **Battle contract impact**: None.
- **Run contract impact**: RunEvent choice submission must remain transactional:
  no partial card grant on failure, and no duplicate grant after completion.
- **UI/App boundary**: `UWacomRunEventScreen` reads the current RunEvent snapshot
  or ViewData, uses existing card presentation data for previews, and submits
  confirmation through screen flow into `URunSession`.
- **Testing expectation**: Simple focused validation is enough: `Wacom.UI.Event`
  if automation is practical, otherwise quick manual validation plus compile.
- **Temporary debt**: None expected.

### Key Entities *(include if feature involves data/state)*

- **Reward Choice**: A visible card reward option in the active RunEvent.
- **Card Detail Preview**: UI-only card detail data derived from `UCardDefinition`
  or existing card presentation builders.
- **RunEvent Choice Result**: The Run transaction result that records success,
  failure, event close/completion facts, and card reward effects.
- **Reward Event State**: Active RunEvent state keyed by scene `PersistentId`;
  it remains in-memory only under current SaveGame boundaries.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: In a sample reward event, the player can inspect three card rewards
  and confirm one without ambiguity.
- **SC-002**: After confirmation, exactly one selected card is added to the
  current Run and no unselected reward is added.
- **SC-003**: After the reward is confirmed, the event closes and the player is
  back in exploration.
- **SC-004**: A simple validation pass confirms the reward loop and no Wacom
  module boundary violation is introduced.

## Assumptions

- Existing RunEvent `GainCard` semantics can be reused or lightly adapted for
  the first slice.
- Existing `UWacomCardPresentationBuilder` / card detail ViewData can support
  reward previews.
- No SaveGame persistence is added in this slice.
- A simple test or manual validation is acceptable for this first specification.
