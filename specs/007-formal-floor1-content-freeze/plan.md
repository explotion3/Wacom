# Implementation Plan: 正式 Floor 1 内容设计与稳定身份冻结

**Branch**: `codex/run-level-authoring-baseline` | **Date**: 2026-07-17 | **Spec**: [spec.md](./spec.md)

**Input**: 冻结三层 Journey 骨架、正式 Floor 1 的 20 节点/21 边设计、稳定身份、内容槽、AP 节奏与生产阻塞；本轮只落文档。

## Summary

以当前 Journey/Floor/Map Node 静态合同为基础，建立与 Authoring baseline、Debug fixture 完全分离的正式设计注册表。Floor 1 采用两轮分岔后汇合的 20 节点/21 边有向图，最短推进 8–9 AP、完整探索 14–15 AP；四个 Navigation 节点均允许 Camp。三层 Journey 只完整设计 Floor 1，Floor 2/3 冻结身份和主题职责。

本轮只生成 Spec Kit 工件并同步长期 Docs，不创建 Production DataAsset 或地图。`Card.Run.SerpentSigil` 是非消耗入口凭证，但普通持有卡可被移除会造成软锁；正式 Journey/Floor 资产实现必须等待任务凭证保留/恢复规则与有效 Floor 2/3 图完成。

## Wacom Domain Context

**Primary Domain**: Run-exploration / Data authoring / Architecture / Tests

**Required Docs Read**:

- [x] `AGENTS.md`
- [x] `Docs/Architecture.md`
- [x] `Docs/WacomMap.md`
- [x] `Docs/WacomRun.md`
- [x] `Docs/WacomDataAuthoring.md`
- [x] `Docs/WacomApp.md`
- [x] `specs/006-run-level-authoring-baseline/`

**Docs To Update**:

- [x] `Docs/WacomMap.md`
- [x] `Docs/WacomDataAuthoring.md`
- [x] `Docs/TODO.md`
- [x] `Docs/Questions.md`

**Owning Module(s)**: 无代码改动。未来所有权保持：`WacomData` 静态定义，`WacomRun` 运行规则，`WacomApp` 场景接入，`WacomEditor` 制作校验，`WacomTests` 合同验证。

**Dependency Direction**:

```text
WacomCore <- WacomData <- WacomBattle <- WacomRun <- WacomApp

WacomEditor -> runtime modules only for editor tooling/validation
WacomTests  -> runtime/app/editor validation as test harness
```

本轮不修改任何 `.Build.cs`，因此依赖图不变。

## Technical Context

**Language/Engine**: Markdown design artifacts for Unreal Engine 5.8 project

**Primary UE Systems**: Existing Primary DataAsset map definitions and scene-binding contracts（只读参考）

**Storage/State**: 本轮无运行态或资产写入；输出是稳定身份与未来 DataAsset authoring 合同

**Runtime Contracts**: 不修改 Snapshot / Command / Resolution / SaveGame；只引用现有 Journey/Floor/Node/Edge/PersistentId 语义

**Testing**: 静态身份、计数、图可达性、支配关系、AP 演算、跨工件一致性和 `git diff --check`

**Target Platform**: N/A；纯文档

**Performance Goals**: N/A；不改变运行时

**Constraints**: 不修改源码、Config、GameplayTag、Build.cs、`.uasset/.umap`；不引用 Debug 内容；不解除蛇印软锁；不生成空壳 Floor 2/3

**Scale/Scope**: 1 Journey identity、3 Floor identities、20 Node identities、21 Edge identities、15 content slots、1 key CardId、4 long-term Docs

**Blueprint Exposure Strategy**: 无 Blueprint 或反射改动

**Data/GameplayTag Impact**: 无 DataAsset/schema/tag 改动；只预留未来 Production ID 与路径

**Save/Load Impact**: 无 schema 变化；冻结身份只是未来持久化候选，不表示本轮启用保存

**UI/App Lifecycle Impact**: 无 UI、输入、焦点、镜头或生命周期改动

## Constitution Check

### Pre-research gate

- **Docs and AGENTS Are the Rule Truth — PASS**: 规划以当前 Docs 和 live 数据合同为准；长期事实同步回 Docs。
- **Wacom Module Boundaries Are Mandatory — PASS**: 不修改代码或依赖；未来所有权明确。
- **Domain Rules Before Presentation — PASS**: 不让地图坐标、Host 或 UI 成为规则真相。
- **Data, GameplayTags, and Authoring Stay Explicit — PASS**: 明确区分设计、未来资产和运行时状态；无 GameplayTag。
- **Reusable Systems Over One-Off Work — PASS**: 使用现有 Journey/Floor 图合同，不提出一次性 Actor 或 Level Blueprint。
- **Validation Is Part of the Slice — PASS**: 文档切片使用静态验证并记录 Unreal 验证不适用原因。

## Phase 0: Research

研究结论见 [research.md](./research.md)：

1. 三层骨架只完整设计 Floor 1，避免用空壳 Floor 伪造合法 Journey。
2. 20 节点/21 边双分岔图满足首层节奏，同时保持全部内容可回访清理。
3. 结构化 ID 与可调展示字段分离，减少主题或内容替换对未来持久化的影响。
4. Host PersistentId 由 FloorId 与 NodeId 派生，避免第二套人工身份。
5. Debug 内容不能晋升；Production 内容 ID 只冻结职责，不冻结具体数值。
6. 蛇印“保证获得”不能证明“保证保留”，因此生产实现保持阻塞。

## Phase 1: Design Artifacts

- [data-model.md](./data-model.md)：三层骨架、20 节点、21 边、内容槽、默认布局、身份和 AP 模型。
- [contracts/stable-identity-contract.md](./contracts/stable-identity-contract.md)：冻结字段、派生 Host PersistentId 和未来持久化候选。
- [contracts/floor1-graph-content-contract.md](./contracts/floor1-graph-content-contract.md)：拓扑、节点类型、内容 ID、Camp 和 AP 验收。
- [contracts/production-readiness-gate.md](./contracts/production-readiness-gate.md)：Authoring/Debug/Production 隔离与蛇印/Floor 2/3 阻塞条件。
- [quickstart.md](./quickstart.md)：静态验证、哈希/LFS/Unreal 跳过项与后续资产轮前置检查。

### Post-design Constitution Re-check

PASS。设计没有增加 public API、模块依赖、GameplayTag、SaveGame 身份字段、运行时分支、Actor 规则副本或二进制资产。已冻结身份同步到长期 Docs；蛇印软锁不以临时引用或 Debug 资产绕过。

## Project Structure

### Documentation (this feature)

```text
specs/007-formal-floor1-content-freeze/
├── spec.md
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── checklists/requirements.md
├── contracts/
│   ├── stable-identity-contract.md
│   ├── floor1-graph-content-contract.md
│   └── production-readiness-gate.md
└── tasks.md
```

### Repository changes

```text
AGENTS.md                         # 当前 Spec Kit plan 指针
.specify/feature.json             # 当前 feature 定位
Docs/WacomMap.md                  # 正式 Journey/Floor 1 长期设计事实
Docs/WacomDataAuthoring.md        # Production authoring 与身份合同
Docs/TODO.md                      # 完成项和后续阻塞任务
Docs/Questions.md                 # 关闭已决问题，保留任务凭证问题
specs/007-formal-floor1-content-freeze/*
```

**Structure Decision**: 本轮不修改 `Source/`、`Config/`、`Content/` 或 `Docs/Architecture.md`。Spec Kit 工件记录审阅过程；稳定规则和制作语义写入领域 Docs。

## Validation Plan

### Static validation

- 核对 20 Nodes / 21 Edges 与类型分布。
- 从 `Node.Entry` 执行可达性遍历；验证两轮支路汇合。
- 删除 `Node.Key.01` 后验证 Guardian/Exit 不可达，证明蛇印节点支配出口。
- 检查 Journey/Floor/Node/Edge/Card ID 唯一、非空、不复用 Authoring/Debug。
- 检查默认 MapPosition 有限、位于 `[0,1920] × [0,1080]` 且不重合。
- 演算关键路线 8–9 AP、完整探索 14–15 AP。
- 使用 `rg` 检查 Production 内容表没有 `/Debug`、`DA_*_Debug`、`Journey.Authoring` 或 `Floor.Authoring.01` 引用。
- 运行跨工件一致性审阅与 `git diff --check`。

### Explicitly skipped

- **WacomEditor compile**: 跳过；无 C++、Build.cs、Config 或反射变更。
- **Automation / AssetRegistry / Blueprint compile**: 跳过；无运行时或资产变更。
- **PIE**: 跳过；未创建正式 DataAsset、Host、Path 或关卡。
- **Binary hash migration audit**: 不适用；本轮不写 `.uasset/.umap`，最终 Git diff 必须证明 Content 零变化。

这些跳过项不产生运行时回归风险；剩余风险是设计尚未落地，且蛇印/Floor 2/3 明确阻塞完整 Production Journey 资产实现。

## Complexity Tracking

无 Constitution 违规或临时实现。
