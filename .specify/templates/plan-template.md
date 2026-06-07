# Implementation Plan: [FEATURE]

**Branch**: `[###-feature-name]` | **Date**: [DATE] | **Spec**: [link]

**Input**: Feature specification from `/specs/[###-feature-name]/spec.md`

**Note**: This template is filled in by `$speckit-plan`. Wacom domain docs and
`AGENTS.md` remain authoritative when generated text conflicts with project
rules.

## Summary

[Primary gameplay/UI/data requirement + high-level implementation approach]

## Wacom Domain Context

**Primary Domain**: [Battle rules / Run-exploration / Data-card authoring /
UI-App / Architecture-modules / Save-load / Tests]

**Required Docs Read**:
- [ ] `AGENTS.md`
- [ ] `Docs/Architecture.md`
- [ ] [Domain doc, e.g. `Docs/WacomBattle.md`]
- [ ] [Related doc, e.g. `Docs/WacomUI.md` or `Docs/WacomDataAuthoring.md`]

**Docs To Update**:
- [ ] [Domain doc that changes]
- [ ] `Docs/TODO.md` or `Docs/TechDebt.md` if temporary debt remains
- [ ] [Binding / authoring / GameplayTag doc if applicable]

**Owning Module(s)**: [WacomCore / WacomData / WacomBattle / WacomRun /
WacomApp / WacomEditor / WacomTests]

**Dependency Direction**:

```text
WacomCore <- WacomData <- WacomBattle <- WacomRun <- WacomApp

WacomEditor -> runtime modules only for editor tooling/validation
WacomTests  -> runtime/app/editor validation as test harness
```

## Technical Context

**Language/Engine**: C++17, Unreal Engine 5.7

**Primary UE Systems**: [GameplayTags / CommonUI / Enhanced Input /
ModelViewViewModel / DataAssets / Automation Tests / Niagara / N/A]

**Storage/State**: [BattleState / FRunState / SaveGame schema / DataAsset /
transient ViewModel/ViewData / N/A]

**Runtime Contracts**: [Snapshot / ViewModel / ViewData / Command / ResultPacket
/ DataAsset field / GameplayTag / validation contract]

**Testing**: [Focused automation prefix and compile command, or NEEDS
CLARIFICATION]

**Target Platform**: Windows editor / packaged Windows build unless specified
otherwise

**Performance Goals**: [Frame/input/UI refresh/automation build constraints, or
N/A]

**Constraints**: [Module boundaries, passive UI, deterministic rules,
PersistentId, SaveGame versioning, WBP binding, asset authoring limits]

**Scale/Scope**: [Cards/effects/screens/assets/modules/tests touched]

**Blueprint Exposure Strategy**: [UPROPERTY/UFUNCTION/events exposed to
designers and invariants kept in C++; explain why reflection is needed]

**Data/GameplayTag Impact**: [New tags, DataAsset fields, generated content,
validators, authoring matrix, or N/A]

**Save/Load Impact**: [SaveGame schema version, migration, in-memory only, or
N/A]

**UI/App Lifecycle Impact**: [CommonUI Activate/Deactivate, focus, input lease,
ViewModel/provider refresh, unsubscription, or N/A]

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

- **Docs and AGENTS Are the Rule Truth**: The plan names required Wacom docs,
  states the rule source, and lists docs to update.
- **Wacom Module Boundaries Are Mandatory**: Owning module(s), public/private API
  surface, Build.cs dependency direction, and cross-module contracts are clear.
- **Domain Rules Before Presentation**: Battle/Run/Data rules stay in domain
  modules; UI reads Snapshot/ViewModel/ViewData and emits commands/intents only.
- **Data, GameplayTags, and Authoring Stay Explicit**: Any DataAsset, tag,
  generated content, validator, or authoring matrix change is named.
- **Reusable Systems Over One-Off Work**: No one-off Actor, Widget, Level
  Blueprint, temporary singleton, string-built tag, or Tick polling shortcut is
  used without documented debt and removal path.
- **Validation Is Part of the Slice**: Compile and focused automation/manual
  validation are planned, including skipped validation risk if any.

## Phase 0: Research

[Resolve unknowns before design. Capture assumptions, alternatives, and rejected
shortcuts. Use CodeGraph for flow/impact questions and live files before edits.]

## Phase 1: Design Artifacts

Produce or update:

- `data-model.md`: runtime state, static assets, snapshots/view data, identity
  fields, and SaveGame impact.
- `contracts/`: public module APIs, command/result contracts, WBP binding or
  editor validation contracts.
- `quickstart.md`: compile/test/manual verification steps.
- `research.md`: open questions and decisions affecting rule truth, assets, or
  module boundaries.

## Project Structure

### Documentation (this feature)

```text
specs/[###-feature]/
├── spec.md
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── contracts/
└── tasks.md
```

### Source Code (repository root)

```text
Source/
├── WacomCore/
│   ├── Public/Tags/
│   └── Private/
├── WacomData/
│   ├── Public/
│   └── Private/
├── WacomBattle/
│   ├── Public/
│   └── Private/
├── WacomRun/
│   ├── Public/
│   └── Private/
├── WacomApp/
│   ├── Public/
│   └── Private/
├── WacomEditor/
│   ├── Public/Validation/
│   └── Private/
└── WacomTests/
    ├── Public/Fixtures/
    └── Private/

Docs/
├── Architecture.md
├── WacomBattle.md
├── WacomRun.md
├── WacomData.md
├── WacomDataAuthoring.md
├── WacomGameplayTags.md
├── WacomApp.md
├── WacomUI.md
├── WacomWorldInteraction.md
├── TODO.md
└── TechDebt.md
```

**Structure Decision**: [List exact files/modules expected to change and why
each belongs there.]

## Validation Plan

**Compile**:

```powershell
& 'E:\UE_5.7\Engine\Build\BatchFiles\Build.bat' WacomEditor Win64 Development -Project='D:\UE_Project\5.7\Wacom\Wacom.uproject' -WaitMutex -NoHotReloadFromIDE
```

**Focused Automation**:

```powershell
& 'E:\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -ExecCmds='Automation RunTests [PREFIX]; Quit' -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache
```

**Suggested Prefixes**: `Wacom.UI.Event`, `Wacom.UI.Shop`,
`Wacom.UI.WorldInteraction`, `Wacom.UI.RunMenuDropTarget`,
`Wacom.UI.FirstPersonCardLayer`, `Wacom.UI.RunFirstPersonCardLayer`,
`Wacom.UI.Battle`, or a focused Battle/Run/Data namespace.

**Manual/Editor Validation**: [WBP binding, map actor PersistentId, content
validation, first-person drag/drop, CommonUI focus flow, or N/A]

## Complexity Tracking

> **Fill ONLY if Constitution Check has violations that must be justified**

| Violation | Why Needed | Simpler Alternative Rejected Because | Removal/Validation Path |
|-----------|------------|--------------------------------------|-------------------------|
| [e.g., temporary bespoke Actor] | [current need] | [why component/subsystem insufficient] | [TODO/TechDebt + validation] |
| [e.g., Widget owns transient state] | [specific problem] | [why command flow/ViewModel insufficient] | [replacement plan] |
