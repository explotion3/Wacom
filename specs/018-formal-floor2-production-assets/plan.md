# Implementation Plan: Floor 2 Production 47 DataAsset 播种与校验

**Branch**: `codex/formal-floor2-production-assets` | **Date**: 2026-07-19 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/018-formal-floor2-production-assets/spec.md`

## Summary

在 `WacomEditor/Private` 抽取 Floor-independent seed-only execution service，保留 Floor 1 profile 的完整兼容性，再增加 MoltCavern Floor 2 profile、commandlet、Editor console 与 focused tests。通过 run MCP exact writer allowlist 按 `12/20/15` 串行创建 47 个 DataAsset，使用 strict/structural validation、AssetRegistry、依赖闭包、哈希、幂等和 LFS 证明交付，不触碰 runtime schema、地图或表现资产。

## Wacom Domain Context

**Primary Domain**: Data/card authoring / Battle and Run content / Architecture / Tests

**Required Docs Read**: `AGENTS.md`、Architecture、AgentIntegrationWorkflow、UnrealMCPWorkflow、WacomData、DataAuthoring、Battle、Run、Map、TODO、Questions、Roadmap 与完整 Spec 017。

**Docs To Update**: `Docs/WacomData.md`、`WacomDataAuthoring.md`、`WacomBattle.md`、`WacomRun.md`、`WacomMap.md`、`Architecture.md`、`TODO.md`、`Questions.md`、`Roadmap.md`。

**Owning Module(s)**: `WacomEditor`、`WacomTests`。

**Dependency Direction**:

```text
WacomCore <- WacomData <- WacomBattle <- WacomRun <- WacomApp
WacomEditor -> existing runtime modules for editor-only construction/validation
WacomTests  -> WacomEditor and runtime modules as test harness
```

## Technical Context

**Language/Engine**: C++, Unreal Engine 5.8

**Primary UE Systems**: DataAssets、UObject reflection comparison、Package save/reload、Automation、AssetRegistry、Unreal MCP writer

**Storage/State**: 47 static DataAsset；JSON audit in Saved；无 runtime state

**Runtime Contracts**: 零变化；仅使用现有 Card/Enemy/Part/Behavior/Encounter/Event/Pickup/Shop schema

**Testing**: 默认 Unity WacomEditor compile；FormalProduction/manifest/real asset、Data validators、Battle/Run smoke

**Target Platform**: Windows Editor

**Performance Goals**: N/A；命令按小组串行且无 Tick/runtime overhead

**Constraints**: no overwrite、no Force、exact allowlist、Editor 生命周期不编译/切 branch、提交前用户审阅

**Scale/Scope**: 47 new `.uasset`、共享 private service、2 profiles、2 command surfaces、small tests、Spec/Docs

**Blueprint Exposure Strategy**: 无 Blueprint exposure；commandlet 反射仅由 UE commandlet discovery 要求

**Data/GameplayTag Impact**: 只创建现有 schema DataAsset；零 tag/field change

**Save/Load Impact**: None

**UI/App Lifecycle Impact**: None

## Constitution Check

- **Docs and AGENTS Are the Rule Truth — PASS**: live Docs 与 Spec 017 为内容权威，长期实现事实同步到 Docs。
- **Wacom Module Boundaries Are Mandatory — PASS**: 私有 Editor 工具与 Tests；零 runtime dependency change。
- **Domain Rules Before Presentation — PASS**: 无 UI/Actor；内容只通过 typed Definition 表达。
- **Data, GameplayTags, and Authoring Stay Explicit — PASS**: exact 47 write set、3 read-only dependency、零 schema/tag 已记录。
- **Reusable Systems Over One-Off Work — PASS**: 先抽共享 execution service，不复制 Floor 1 保存管线。
- **Validation Is Part of the Slice — PASS**: 三个 compile checkpoint、Automation、MCP、AssetRegistry、closure、hash、LFS 完整。

## Phase 0: Research

1. 读取并冻结 Spec 017 exact manifest/initial values。
2. 审计 Floor 1 约 2000 行 builder，区分主题 profile 与通用执行管线。
3. 选择 Private profile callbacks，避免公共 runtime API 和 Build.cs 变化。
4. 冻结 partial-save recovery、dirty source MCP、no-subagent console 触发与 BugGirl 外部问题处理。

结果见 [research.md](research.md)，无 unresolved clarification。

## Phase 1: Design Artifacts

- [data-model.md](data-model.md)：profile、manifest、report、状态与 47 asset invariants。
- [contracts/asset-manifest.md](contracts/asset-manifest.md)：exact writable/read-only set。
- [contracts/seeder-command-contract.md](contracts/seeder-command-contract.md)：语法、退出码、no-overwrite 与 recovery。
- [contracts/mcp-write-contract.md](contracts/mcp-write-contract.md)：session、writer、allowlist 与 evidence。
- [contracts/validation-contract.md](contracts/validation-contract.md)：compile、Automation、AssetRegistry/hash/LFS gates。
- [quickstart.md](quickstart.md)：实时验证 ledger 与命令。

## Project Structure

```text
Source/WacomEditor/Private/ContentBuilders/
├── FormalProductionContentSeedService.{h,cpp}
├── FormalFloor1ContentBuilder.{h,cpp}      # profile + compatibility facade
├── FormalFloor2ContentBuilder.{h,cpp}
└── FormalFloor2ContentEditorCommand.cpp

Source/WacomEditor/Private/Commandlets/
└── WacomBuildFormalFloor2ContentCommandlet.{h,cpp}

Source/WacomEditor/Public/Testing/
└── WacomFormalFloor2ContentAutomationTestView.h

Source/WacomEditor/Private/Testing/
└── WacomFormalFloor2ContentAutomationTestView.cpp

Source/WacomTests/Private/Editor/
├── FormalProductionContentSeedServiceSpec.cpp
├── FormalFloor2ContentManifestSpec.cpp
└── FormalFloor2ContentAssetSpec.cpp
```

**Structure Decision**: 通用解析/验证/报告/保存集中到 SeedService；Floor-specific cpp 只保留 tables、expected-object configuration 与特有 invariant。现有 Floor 1 命令/header symbols 保持 facade 兼容。

## Implementation Checkpoints

### Checkpoint 1 — Shared core and Floor 1 parity

- 写共享 types/profile/service；将 Floor 1 parse、comparator、load/run 委托给共享服务。
- 在任何 Editor mutation 前完成编译、共享服务 tests、Floor 1 manifest/real asset tests。
- 记录 Floor 1 46 文件 aggregate hash，证明零二进制变化。

### Checkpoint 2 — Floor 2 profile and transient validation

- 写完整 MoltCavern seed tables/configuration、manifest、commands、test view/spec。
- 编译并运行新前缀与既有 Data validator；执行 empty-root inspect，必须 47 missing/0 writes。
- 冻结源码；Editor 生命周期内不再改 C++ 或 branch/HEAD。

### Checkpoint 3 — MCP asset creation

- `Start -AllowDirty`，记录 dirty source ownership；每次 MCP 前 AssertReady。
- writer lease 与 seed/inspect 按 Cards 12 → EnemyGraph 20 → NodeDefinitions 15。
- 每组释放前检查 Git、hash、LFS、JSON；全组后第二次 0/0 幂等。

### Checkpoint 4 — Final validation and review gate

- ReleaseWriter、正常关闭 Editor、AssertClosedForBuild。
- 最终 Unity compile、focused tests、AssetRegistry/closure/hash/LFS、Docs/quickstart。
- 向用户提交一次性审阅包；未确认不 stage/commit。

## Validation Plan

所有 Unreal 命令带 `-NoDreamShaderEditorBridge`；具体命令、prefix 与 expected counts 见 [quickstart.md](quickstart.md) 和 [validation-contract.md](contracts/validation-contract.md)。不运行 PIE、Blueprint compile 或任何全量 builder。

## Complexity Tracking

无 constitution violation。共享 private service 是对既有重复风险的正式收口，不产生临时债。
