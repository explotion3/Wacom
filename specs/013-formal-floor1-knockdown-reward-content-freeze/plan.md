# Implementation Plan: 正式 Floor 1 击倒分支奖励卡内容冻结

**Branch**: `codex/formal-floor1-knockdown-reward-content-freeze` | **Date**: 2026-07-18 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `specs/013-formal-floor1-knockdown-reward-content-freeze/spec.md`

## Summary

在 Spec 012 已提供 Aid/Destroy 显式字段、统一查询、原子授予与 FormalProduction 校验的基础上，为 Floor 1 四个 SerpentWood Archetype 冻结八张奖励卡的全部可制作字段、package leaf、十一 Part 引用和路线奖励量。所有事实写入 Spec Kit 与长期 Docs；本轮不修改 runtime、schema、builder 源码或二进制资产。

## Wacom Domain Context

**Primary Domain**: Data-card authoring / Battle content / Run-exploration

**Required Docs Read**:

- [x] `AGENTS.md`
- [x] `Docs/AgentIntegrationWorkflow.md`
- [x] `Docs/WacomBattle.md`
- [x] `Docs/WacomData.md`
- [x] `Docs/WacomDataAuthoring.md`
- [x] `Docs/WacomMap.md`
- [x] `Docs/TODO.md`、`Docs/Questions.md`、`Docs/Roadmap.md`
- [x] `.specify/memory/constitution.md`
- [x] Spec 011 与 Spec 012 的规格、数据模型、合同、tasks 和验证台账

**Docs To Update**:

- [x] `Docs/WacomData.md`
- [x] `Docs/WacomDataAuthoring.md`
- [x] `Docs/WacomBattle.md`
- [x] `Docs/WacomMap.md`
- [x] `Docs/TODO.md`
- [x] `Docs/Questions.md`
- [x] `Docs/Roadmap.md`

**Owning Module(s)**: 本轮无模块改动。未来内容归 `WacomData`，生成/校验归 `WacomEditor`，现有击倒解释归 `WacomBattle`，测试归 `WacomTests`。

**Dependency Direction**:

```text
WacomCore <- WacomData <- WacomBattle <- WacomRun <- WacomApp

WacomEditor -> runtime modules only for editor tooling/validation
WacomTests  -> runtime/app/editor validation as test harness
```

## Technical Context

**Language/Engine**: Markdown contracts for an Unreal Engine 5.8 project

**Primary UE Systems**: Future `UCardDefinition` / `UEnemyPartDefinition` authoring; no Unreal process required this slice

**Storage/State**: Future DataAsset fields only; no runtime or persisted state is created

**Runtime Contracts**: Reuses Spec 012 `AidRewardCard`、`DestroyRewardCard`、`ResolveKnockdownRewardCard()` and existing Card Effect semantics unchanged

**Testing**: Spec Kit consistency, exact manifest/count/identity/route arithmetic/schema/reference audits, Markdown links, Git/LFS/range checks

**Target Platform**: Documentation is platform-neutral; future implementation targets Windows editor

**Performance Goals**: N/A; no runtime code

**Constraints**: text-only diff; no Source/Config/Build.cs/GameplayTag/DataAsset/map; no builder; no merge/push

**Scale/Scope**: 8 future cards、11 Part assignments、5 route-yield totals、3 contracts、7 long-term Docs

**Blueprint Exposure Strategy**: N/A; no reflected field or Blueprint change

**Data/GameplayTag Impact**: zero schema/tag change; freezes values expressible by current tags and fields

**Save/Load Impact**: none; repeated cards remain existing independent card instances

**UI/App Lifecycle Impact**: none; existing passive Dialog continues to consume Battle ViewData

## Constitution Check

*GATE: PASS before research; PASS after design.*

- **Docs and AGENTS Are the Rule Truth**: live Docs, Spec 011/012 and constitution were read; finalized facts are assigned to seven long-term Docs.
- **Wacom Module Boundaries Are Mandatory**: no public API, Build.cs or dependency change; future ownership remains Data/Editor/Battle/Tests.
- **Domain Rules Before Presentation**: no UI rule or asset read is introduced; existing Battle query remains the sole runtime interpretation.
- **Data, GameplayTags, and Authoring Stay Explicit**: every future card field, package and Part reference is enumerated; schema/tag impact is zero.
- **Reusable Systems Over One-Off Work**: four enemy-level card pairs are reused by all eleven Parts; no node/part-specific duplicate Definitions.
- **Validation Is Part of the Slice**: static audits are explicit and Unreal validation skips include reason and future gate.

## Phase 0: Research

Conclusions are recorded in [research.md](./research.md):

1. Spec 012 is the live runtime/validation foundation and is already in the base commit.
2. Current Card schema can express all eight cards without new tags or fields.
3. Enemy Intent mimicry defines each effect and progression without inventing mechanics.
4. Part-level reward frequency creates `14–17` critical-path and `20` full-exploration cards.
5. Spec 011's 38-package historical core manifest is extended, not rewritten, by eight package rows.
6. Pure documentation validation is sufficient now; runtime/content proof is deferred to the authorized 46-asset round.

## Phase 1: Design Artifacts

- [data-model.md](./data-model.md): card fields、Part assignments、route yield and Production totals.
- [contracts/card-manifest.md](./contracts/card-manifest.md): exact eight-card content/package contract.
- [contracts/part-assignment-contract.md](./contracts/part-assignment-contract.md): eleven Part mappings and repeat semantics.
- [contracts/production-readiness-contract.md](./contracts/production-readiness-contract.md): 46-asset write set and future validation gate.
- [quickstart.md](./quickstart.md): workspace evidence, static commands, skips and result ledger.

## Project Structure

### Documentation (this feature)

```text
specs/013-formal-floor1-knockdown-reward-content-freeze/
├── spec.md
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── contracts/
│   ├── card-manifest.md
│   ├── part-assignment-contract.md
│   └── production-readiness-contract.md
├── checklists/
│   ├── requirements.md
│   └── content-quality.md
└── tasks.md
```

### Repository files changed

```text
AGENTS.md
.specify/feature.json
Docs/WacomBattle.md
Docs/WacomData.md
Docs/WacomDataAuthoring.md
Docs/WacomMap.md
Docs/TODO.md
Docs/Questions.md
Docs/Roadmap.md
specs/013-formal-floor1-knockdown-reward-content-freeze/**
```

**Structure Decision**: Spec artifacts preserve review evidence; `Docs/` holds durable content and authoring truth. No architecture, Run, App or TechDebt document changes because no corresponding contract changes.

## Validation Plan

**Compile/Automation**: skipped by design because no source, reflected schema, Build.cs, Config or asset changes exist.

**Static validation**:

1. Run Spec Kit prerequisite discovery and read-only cross-artifact analysis.
2. Parse card/Part/route tables and assert exact counts, uniqueness and approved totals.
3. Audit current schema/tag vocabulary and forbidden Production references.
4. Check all changed Markdown relative links.
5. Run `git diff --check`, allowed-path audit, `git lfs status` and `git lfs fsck`.

**Manual/Editor Validation**: Builder、AssetRegistry、Blueprint and PIE are prohibited/skipped. Future 46-asset implementation must run builder twice, Data Validation, focused Battle/Data/Run tests, reference/hash/dirty audits and real PIE.

## Complexity Tracking

No constitution violation or exception is required.
