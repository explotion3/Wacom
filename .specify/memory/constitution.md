<!--
Sync Impact Report
Version change: 1.0.0 -> 1.0.1
Modified principles:
- Required Validation Commands: UE 5.7 tool paths -> UE 5.8 tool paths
Added sections:
- None
Removed sections:
- None
Templates requiring updates:
- updated: .specify/templates/plan-template.md (UE 5.8)
- reviewed: .specify/templates/spec-template.md (no toolchain paths)
- updated: .specify/templates/tasks-template.md (UE 5.8)
- reviewed: AGENTS.md remains authoritative; agent-context extension is disabled
Follow-up TODOs:
- None
-->
# Wacom Spec Kit Constitution

## Core Principles

### I. Docs and AGENTS Are the Rule Truth

Every Spec Kit feature MUST start by identifying the Wacom domain, reading the
matching `Docs/` files, and preserving `AGENTS.md` as the collaboration rule
source. Generated specs, plans, and tasks MUST cite the relevant domain docs and
MUST say which docs will be updated. If a generated artifact conflicts with
`AGENTS.md` or an active domain doc, the Wacom document wins and this
constitution or the generated artifact MUST be corrected.

Feature artifacts under `specs/` are temporary planning records, not durable
project truth. When a feature is implemented, any finalized rule behavior,
asset semantics, UI binding contract, authoring workflow, validation expectation,
or technical debt MUST be migrated back into the relevant `Docs/` file,
`Docs/TODO.md`, or `Docs/TechDebt.md`. Long-term facts MUST NOT live only in a
feature spec, plan, or task list.

Rationale: Wacom already has a docs-first rule system. Spec Kit is a planning
helper, not a replacement for project truth.

### II. Wacom Module Boundaries Are Mandatory

Plans MUST declare the owning module and dependency direction before
implementation. Production dependencies MUST preserve:

```text
WacomCore <- WacomData <- WacomBattle <- WacomRun <- WacomApp
```

`WacomEditor` may depend on runtime modules for editor-only tooling and
validation. `WacomTests` may depend on runtime, app, and editor validation code
as a test harness. No production module may depend on `WacomTests`.

Public headers MUST expose only stable contracts. Rules, algorithms, mutable
domain state, resolver logic, command flow helpers, and UI coordinators that are
not cross-module contracts MUST remain in `Private/`.

Rationale: module boundaries are enforced by `Build.cs` and are the main guard
against circular dependencies, slow iteration, and UI leaking into rules.

### III. Domain Rules Before Presentation

Battle rules MUST live in `WacomBattle`, Run and exploration transactions MUST
live in `WacomRun`, static content contracts MUST live in `WacomData`, and
presentation/input orchestration MUST live in `WacomApp`. UI MAY display
Snapshot, ViewModel, ViewData, or PresentationView and MAY emit user intent
through documented commands or app flows. UI MUST NOT own authoritative Battle
or Run state.

Plans involving widgets, CommonUI, HUDs, first-person card layers, world
interaction, shops, RunEvent, backpack screens, or Battle HUD MUST document data
source, refresh timing, focus/input ownership, and unsubscription/lifecycle
behavior.

Rationale: Wacom gameplay must remain testable without a widget tree, map
layout, or animation queue.

### IV. Data, GameplayTags, and Authoring Stay Explicit

DataAsset and content changes MUST be separated from runtime state changes.
Specs and plans MUST say whether a feature changes `WacomData` fields,
generated content, Data Validation, authored assets, map actor authoring, or
runtime rules.

New GameplayTags MUST be declared in
`WacomCore/Public/Tags/WacomGameplayTags.h` and documented in
`Docs/WacomGameplayTags.md`. DataAsset field changes MUST update
`Docs/WacomData.md`, `Docs/WacomDataAuthoring.md`, relevant validators, and
generated-content or smoke tests when applicable.

Rationale: tags and assets are design contracts. A declared tag is not
automatically a valid authored rule until runtime execution and validation
support exist.

### V. Reusable Systems Over One-Off Work

Features MUST prefer reusable services, components, subsystems, data
structures, DataAssets, GameplayTags, ViewModels, providers, validators, or
command flow helpers over bespoke Actor subclasses, one-screen widget logic,
Level Blueprint branches, temporary globals, string-built tags, Tick polling, or
hard-coded scene flows.

If a temporary approach is unavoidable, the plan MUST name its scope, owner,
removal trigger, and formal replacement. The implementation MUST mark it with
`TODO(技术债)` and record it in `Docs/TODO.md` or `Docs/TechDebt.md`.

Rationale: Wacom is a reusable card adventure framework, not a collection of
single-scene prototypes.

### VI. Validation Is Part of the Slice

Every generated task set MUST include the smallest meaningful compile and test
validation for the touched domain. Rules, SaveGame schema, Run state,
DataAsset validation, public contracts, and reproduced bugs require focused
automation tests. UI and App slices require either relevant automation tests or
clear manual validation steps when the flow is not yet automatable.

Skipped validation MUST be explained with reason, remaining risk, and a
follow-up task. A feature is not complete only because the implementation files
exist.

Rationale: Unreal regressions often appear at compile, load, asset validation,
CommonUI lifecycle, or automation runtime rather than at the edited line.

## Wacom Domain Map

| Domain | Rule docs | Primary module | Common tests |
|---|---|---|---|
| Battle rules | `Docs/WacomBattle.md`, `Docs/WacomGameplayTags.md` | `Source/WacomBattle/` | `Source/WacomTests/Private/Battle/` |
| Battle UI | `Docs/WacomBattleUI.md`, `Docs/UI_Battle_WBP_Binding.md` | `Source/WacomApp/` | `Wacom.UI.Battle` |
| Run/exploration | `Docs/WacomRun.md`, `Docs/WacomWorldInteraction.md` | `Source/WacomRun/`, `Source/WacomApp/` | `Source/WacomTests/Private/Run/`, `Wacom.UI.WorldInteraction` |
| Data/card content | `Docs/WacomData.md`, `Docs/WacomDataAuthoring.md` | `Source/WacomData/`, `Source/WacomEditor/` | `Source/WacomTests/Private/Data/` |
| UI/App shell | `Docs/WacomApp.md`, `Docs/WacomUI.md`, `Docs/WacomUIFoundation.md` | `Source/WacomApp/` | `Source/WacomTests/Private/UI/` |
| Architecture/modules | `Docs/Architecture.md` | `Source/*/`, `*.Build.cs` | Compile plus affected automation tests |
| Save/load | `Docs/WacomRun.md`, `Docs/Architecture.md` | `Source/WacomRun/` | SaveGame and Run automation tests |

## Spec Kit Usage Boundaries

Spec Kit SHOULD be used for large or ambiguous changes: new Battle rules,
Effect/Passive/Target expansion, RunEvent or shop systems, backpack/storage
changes, SaveGame schema work, DataAsset authoring changes, major UI/App
flows, or anything that changes module boundaries.

Spec Kit SHOULD NOT be required for narrow fixes, small UI copy/layout changes,
mechanical renames, low-risk documentation edits, or a single isolated bug when
the root cause and validation path are already clear.

The normal flow is:

```text
$speckit-specify -> optional $speckit-clarify -> $speckit-plan
-> $speckit-tasks -> implementation by Codex/main session
-> optional $speckit-analyze/checklist
```

`$speckit-implement` MUST NOT bypass Wacom's requirement to read live files,
respect dirty user changes, update docs, and run validation.

## Required Validation Commands

Compile command:

```powershell
& 'E:\UE_5.8\Engine\Build\BatchFiles\Build.bat' WacomEditor Win64 Development -Project='D:\UE_Project\5.7\Wacom\Wacom.uproject' -WaitMutex -NoHotReloadFromIDE
```

Full automation command:

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -ExecCmds='Automation RunTests Wacom; Quit' -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache
```

Plans SHOULD prefer focused automation prefixes when possible, for example
`Wacom.UI.Event`, `Wacom.UI.Shop`, `Wacom.UI.WorldInteraction`,
`Wacom.UI.RunMenuDropTarget`, `Wacom.UI.FirstPersonCardLayer`,
`Wacom.UI.RunFirstPersonCardLayer`, and `Wacom.UI.Battle`.

## Governance

This constitution governs Spec Kit generated specs, plans, and tasks inside the
Wacom repository. It complements but does not override `AGENTS.md` or active
domain documents in `Docs/`.

Amendments require an explicit constitution update, semantic version bump, Sync
Impact Report, and propagation to affected templates. MAJOR versions remove or
redefine core governance in a backward-incompatible way. MINOR versions add or
materially expand principles. PATCH versions clarify wording without changing
required behavior.

Compliance review is required before planning, before task generation, and at
final implementation review. Reviewers MUST verify docs-first evidence, module
ownership, passive UI boundaries, DataAsset/GameplayTag impacts, reusable
system design, validation results, and any documented temporary debt.

**Version**: 1.0.1 | **Ratified**: 2026-06-07 | **Last Amended**: 2026-07-14
