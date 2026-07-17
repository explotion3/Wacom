# Implementation Plan: 正式 Floor 1 Production 内容合同冻结

**Branch**: `codex/formal-floor1-production-content-freeze` | **Date**: 2026-07-17 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `/specs/011-formal-floor1-production-content-freeze/spec.md`

## Summary

本轮在更新后的 `main` 基线 `15b067f63a08feb44993e91de7faa96ba68c0d9f` 上完成纯文档内容设计冻结。交付 Floor 1 的 15 个 Production 节点 Definition 合同，以及 4 Enemy、11 EnemyPart、4 Behavior、4 Card 的支持合同和 38 个未来 DataAsset manifest。所有字段严格映射到当前 `WacomData` schema 与 `WacomDataAuthoring` authoring matrix；不修改源码、运行时合同或二进制资产。

## Wacom Domain Context

**Primary Domain**: Data-card authoring / Battle content / Run-exploration / Tests

**Required Docs Read**:

- [x] `AGENTS.md`
- [x] `Docs/AgentIntegrationWorkflow.md`
- [x] `Docs/Architecture.md`
- [x] `Docs/WacomRun.md`
- [x] `Docs/WacomMap.md`
- [x] `Docs/WacomData.md`
- [x] `Docs/WacomDataAuthoring.md`
- [x] `Docs/TODO.md`
- [x] `Docs/Questions.md`

**Docs To Update**:

- [x] `Docs/WacomData.md`：写入 SerpentWood 敌人、卡牌、Encounter、Pickup、Shop 与 RunEvent 内容合同。
- [x] `Docs/WacomDataAuthoring.md`：写入主题路径、38 资产 manifest、ID/路径约定、禁止引用与未来 builder 写集合。
- [x] `Docs/WacomMap.md`：写入 Floor 1 战斗梯度、路线经济、事件与 AP 仍保持冻结区间。
- [x] `Docs/TODO.md`：关闭 Floor 1 内容设计 blocker，新增 38 DataAsset 实现/校验任务。
- [x] `Docs/Questions.md`：保留击倒三分支 P0 与 Production 世界资产权威问题，明确 null 奖励不是最终规则。
- [ ] `Docs/Architecture.md`：不更新；模块图、依赖方向与公共合同没有变化。

**Owning Module(s)**: 本轮无源码 owner。未来静态内容归 `WacomData`，生成/校验归 `WacomEditor`，执行继续复用 `WacomBattle/WacomRun`，自动化归 `WacomTests`。

**Dependency Direction**:

```text
WacomCore <- WacomData <- WacomBattle <- WacomRun <- WacomApp

WacomEditor -> runtime modules only for editor tooling/validation
WacomTests  -> runtime/app/editor validation as test harness
```

本轮不改 `Build.cs`，不产生新的依赖边。

## Technical Context

**Language/Engine**: Markdown design artifacts for an Unreal Engine 5.8 C++ project

**Primary UE Systems**: Existing `UPrimaryDataAsset` schemas, GameplayTags, Editor Data Validation; no UE execution in this slice

**Storage/State**: Future static DataAsset authoring only; no runtime state or SaveGame change

**Runtime Contracts**: Reuse existing Card/Enemy/Behavior/Encounter/Shop/Pickup/RunEvent fields; no public contract changes

**Testing**: Static count, ID, path, schema, economy and AP audits; Spec Kit cross-artifact analysis; `rg`; `git diff --check`; Git/LFS/range audit

**Target Platform**: Documentation is platform-independent; future assets target Windows editor/package flow

**Performance Goals**: N/A; no runtime work

**Constraints**: Text only; no C++, Config, Build.cs, GameplayTag, `uasset/umap`, builder, Blueprint compile, AssetRegistry mutation or PIE

**Scale/Scope**: 38 future DataAssets, 15 node Definition contracts, 4 long-term domain docs plus TODO/Questions, complete Spec Kit bundle

**Blueprint Exposure Strategy**: N/A; no reflected fields or Blueprint assets change

**Data/GameplayTag Impact**: No schema/tag/asset change. Future definitions use only live fields and declared tags.

**Save/Load Impact**: None. Existing RunFlags remain in-memory; Credential behavior remains SaveGame v5 fact.

**UI/App Lifecycle Impact**: None

## Constitution Check

*GATE: passed before Phase 0 and re-checked after Phase 1.*

- **Docs and AGENTS Are the Rule Truth**: PASS — live Docs and headers override obsolete Spec 007 blocker text; updated long-term Docs receive all frozen facts.
- **Wacom Module Boundaries Are Mandatory**: PASS — no module or API changes; future ownership follows existing dependency direction.
- **Domain Rules Before Presentation**: PASS — content fields are static Data contracts; no Host/UI/scene behavior is invented.
- **Data, GameplayTags, and Authoring Stay Explicit**: PASS — every future asset, ID, path, existing dependency and allowed schema value is listed; zero new tags.
- **Reusable Systems Over One-Off Work**: PASS — four enemy archetypes and node definitions use shared schemas; no one-off Actor/Blueprint/Level script.
- **Validation Is Part of the Slice**: PASS — static audits are sufficient for text-only changes; Unreal validation skips and later asset risks are explicit.

No Constitution violation requires complexity justification.

## Phase 0: Research

1. Compare Spec 007/009/010 with updated `main`; mark old serpent-card soft-lock and Journey-success blockers as resolved in live truth.
2. Read live `UCardDefinition`、`UEnemyDefinition`、`UEnemyPartDefinition`、`UEnemyBehaviorDefinition`、`UEncounterDefinition`、`UShopDefinition`、`UWacomRunPickupDefinition`、`UWacomRunEventDefinition` headers.
3. Verify all requested Effect/Target/Rarity/Keyword/Pressure/condition/effect values against the current authoring matrix and tag catalog.
4. Resolve existing asset identities for Starter cards and PoisonFang so future offers do not duplicate content.
5. Record rejected alternatives and binary/runtime boundaries in [research.md](./research.md).

## Phase 1: Design Artifacts

Produce:

- [data-model.md](./data-model.md): exact enemies, parts, behaviors, encounters, cards, pickups, shop, events, manifest and dependency counts.
- [contracts/enemy-encounter-contract.md](./contracts/enemy-encounter-contract.md): battle content invariants and HP curve.
- [contracts/card-pickup-shop-contract.md](./contracts/card-pickup-shop-contract.md): card fields, pickup mapping and fixed shop inventory.
- [contracts/run-event-contract.md](./contracts/run-event-contract.md): 4 events/13 choices, flags, costs and rollback-compatible authoring.
- [contracts/production-asset-manifest.md](./contracts/production-asset-manifest.md): 38 packages, naming and allowed dependencies.
- [contracts/production-readiness-gate.md](./contracts/production-readiness-gate.md): what this design closes and what still blocks binary production.
- [quickstart.md](./quickstart.md): baseline, static validation evidence, skipped Unreal checks and risk.
- [checklists/requirements.md](./checklists/requirements.md) and [checklists/production-content.md](./checklists/production-content.md): specification and reviewer quality gates.

Post-design re-check: all requested fields map to current schema; no public API, tag, asset or runtime semantics are added.

## Phase 2: Long-term Docs Sync

1. Add the frozen SerpentWood content contract to `Docs/WacomData.md`.
2. Replace Floor 1 “slot only” authoring text with theme paths, manifest count, forbidden dependencies and future builder ownership in `Docs/WacomDataAuthoring.md`.
3. Add combat gradient, route reward/economy and unchanged AP evidence to `Docs/WacomMap.md`.
4. Mark Floor 1 content design complete and add a concrete 38 DataAsset implementation task in `Docs/TODO.md`.
5. Keep knockdown effects and world asset authority open in `Docs/Questions.md`; explicitly state null part reward is a temporary authoring gate.
6. Update only this branch's `.specify/feature.json` and the managed Spec Kit pointer in `AGENTS.md`.

## Phase 3: Static Validation and Delivery

1. Audit exact counts and manifest category totals.
2. Audit ID/path uniqueness and existing dependency identities.
3. Audit Intent/Card/Event schema values against live headers/docs.
4. Audit HP curve, enemy count, route Gold purchasing paths and AP preservation.
5. Audit Production tables for forbidden Debug/Authoring/Test/BadgeDisplayTests/TrainingWarrior references.
6. Run read-only Spec Kit cross-artifact consistency analysis and close any findings before delivery.
7. Run `git diff --check`, changed-file range audit, Git/LFS status and binary-extension audit.
8. Commit one independent documentation change; leave worktree clean, do not merge main, do not push.

## Project Structure

### Documentation (this feature)

```text
specs/011-formal-floor1-production-content-freeze/
├── spec.md
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── contracts/
│   ├── enemy-encounter-contract.md
│   ├── card-pickup-shop-contract.md
│   ├── run-event-contract.md
│   ├── production-asset-manifest.md
│   └── production-readiness-gate.md
├── checklists/
│   ├── requirements.md
│   └── production-content.md
└── tasks.md
```

### Repository files changed

```text
AGENTS.md
.specify/feature.json
Docs/WacomData.md
Docs/WacomDataAuthoring.md
Docs/WacomMap.md
Docs/TODO.md
Docs/Questions.md
specs/011-formal-floor1-production-content-freeze/**
```

**Structure Decision**: Spec artifacts hold full review evidence; stable content identities, fields, paths and production gates are mirrored into their long-term domain Docs. No `Source/` or `Content/` path is modified.

## Validation Plan

**Compile**: SKIPPED — no source, schema, Build.cs or binary asset change.

**Focused Automation**: SKIPPED — no runtime/validator/asset instance is changed; static authoring contract only.

**Static validation**:

```powershell
git diff --check
git status --short --branch
git lfs status
rg -n "Debug|Authoring|Test\.|BadgeDisplayTests|TrainingWarrior" specs/011-formal-floor1-production-content-freeze/contracts Docs/WacomData.md Docs/WacomDataAuthoring.md Docs/WacomMap.md
```

The forbidden-reference audit is interpreted against Production manifest/content table rows; explanatory prohibition text is expected to contain those words.

**Manual/Editor Validation**: SKIPPED — no DataAsset, Blueprint or map exists or changes in this slice.

**Later implementation requirement**: Create and validate all 38 DataAssets; run focused `Wacom.Data.*`/`Wacom.Battle.*` tests, AssetRegistry/reference/hash audit, Blueprint/Host validation and Production Floor Golden Path PIE before removing the asset readiness gate.

## Complexity Tracking

No Constitution violations.
