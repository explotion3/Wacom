# Implementation Plan: 正式 Floor 2/3 图与 Journey 总节奏冻结

**Branch**: `codex/run-level-authoring-baseline` | **Date**: 2026-07-17 | **Spec**: [spec.md](./spec.md)

**Input**: 在已冻结并实现通用 Credential 的正式三层 Journey 骨架上，以纯文档方式补齐 Floor 2/3 的 canonical graph、Production 内容槽、终局语义和整段 AP/天数目标。

## Summary

为 `Floor.Main.02`（蛇蜕洞窟）和 `Floor.Main.03`（毒巢核心）各冻结 20 节点/21 边双分岔图。Floor 2 通过必经 `Pickup.MoltCavern.MoltSeal` 同时预留 `Card.Run.MoltSeal` 与 `Credential.Run.MoltSeal`，入口只检查 Credential；Floor 3 不设置 Shop/FloorEntrance，最终 `Node.Guardian.01` 无出边并被指定为 Journey 成功终点。三层 Journey 的最短推进冻结为 26–28 AP，完整探索为 44–46 AP，正常节奏目标约 5–6 天。

本轮只生成 Spec Kit 工件并同步长期 Docs，不创建或修改任何运行时、DataAsset、Blueprint、地图或二进制资产。当前运行时不能表达 Journey 成功完成，Production 激活必须继续被通用 success state/event/UI handoff 阻塞。

## Wacom Domain Context

**Primary Domain**: Run-exploration / Data authoring / Content design / Testing

**Required Docs Read**:

- [x] `AGENTS.md`
- [x] `Docs/AgentIntegrationWorkflow.md`
- [x] `Docs/Architecture.md`
- [x] `Docs/WacomMap.md`
- [x] `Docs/WacomRun.md`
- [x] `Docs/WacomDataAuthoring.md`
- [x] `Docs/TODO.md`
- [x] `Docs/Questions.md`
- [x] `specs/007-formal-floor1-content-freeze/`
- [x] `specs/008-run-credential/`
- [x] 当前 Map validator、FloorEntrance payload 与 Battle settlement live 源码

**Docs To Update**:

- [ ] `Docs/WacomMap.md`
- [ ] `Docs/WacomDataAuthoring.md`
- [ ] `Docs/TODO.md`
- [ ] `Docs/Questions.md`

**Owning Module(s)**: 本轮无模块改动。未来静态定义归 `WacomData`，Journey 成功规则归 `WacomRun`，终局表现归 `WacomApp`，静态/场景校验归 `WacomEditor`。

**Dependency Direction**:

```text
WacomCore <- WacomData <- WacomBattle <- WacomRun <- WacomApp

WacomEditor -> runtime modules only for editor tooling/validation
WacomTests  -> runtime/app/editor validation as test harness
```

本轮不修改 `.Build.cs` 或任何依赖。

## Technical Context

**Language/Engine**: Markdown design artifacts for an Unreal Engine 5.8 project

**Primary UE Systems**: 只引用现有 `UWacomJourneyDefinition`、`UWacomFloorMapDefinition`、typed Map Node payload、Run Credential 和 Editor Data Validation 合同

**Storage/State**: 无运行时状态改动；冻结的 ID 是未来静态资产和持久化候选

**Runtime Contracts**: 不修改 Snapshot / Command / Resolution / SaveGame；Floor 3 terminal Guardian 只作为设计合同，当前 runtime 不具备成功状态

**Testing**: 静态 Node/Edge/type/reachability/dominance/layout/AP 审计、Spec Kit 跨工件一致性、`rg` 边界检查、`git diff --check` 和 Git/LFS 状态

**Target Platform**: N/A；纯文档

**Performance Goals**: N/A；不改变运行时

**Constraints**: 不修改 Source、Config、Content、GameplayTag、Build.cs、SaveGame 或二进制资产；不引用 Debug 内容；不运行 builder；不伪造 Journey success

**Scale/Scope**: 2 个 Floor、40 个 Node、42 个 Edge、31 个新增 Production 内容槽、2 个蜕印身份、4 个长期 Docs、1 套 Spec 009 工件

**Blueprint Exposure Strategy**: 无 Blueprint 或反射改动

**Data/GameplayTag Impact**: 无 DataAsset/schema/tag 改动；只冻结未来 Production ID、路径和 typed payload 职责

**Save/Load Impact**: 无 schema 变化；`Credential.Run.MoltSeal` 复用已落地的通用 v4 Credential 集合，但本轮不创建或写入资产/存档

**UI/App Lifecycle Impact**: 无 UI、焦点、输入、镜头或生命周期改动；Journey 总结页与返回目标另案设计

## Constitution Check

### Pre-research gate

- **Docs and AGENTS Are the Rule Truth — PASS**: 已读取长期 Docs、Spec 007/008 与 live 源码；已决事实同步回长期 Docs。
- **Wacom Module Boundaries Are Mandatory — PASS**: 本轮零代码/依赖；未来 success ownership 明确为 Run 规则与 App 表现分层。
- **Domain Rules Before Presentation — PASS**: 不让 Guardian Actor、Widget 或关卡蓝图直接写成功状态。
- **Data, GameplayTags, and Authoring Stay Explicit — PASS**: 明确 31 个内容槽、两项蜕印身份、零资产/GameplayTag 修改和 Production 门禁。
- **Reusable Systems Over One-Off Work — PASS**: Floor 2 复用通用 Credential，Floor 3 使用现有 NodeType，不增加一次性节点或特殊 Actor。
- **Validation Is Part of the Slice — PASS**: 文档切片使用完整静态审计，并记录所有 Unreal 跳过项及剩余风险。

## Phase 0: Research

研究结论见 [research.md](./research.md)：

1. 两层沿用 20/21 双分岔骨架，保持制作预算和认知一致，通过类型密度与终局链形成递进。
2. NodeId/EdgeId 继续 Floor-scoped；跨层身份必须携带 FloorId，Host PersistentId 继续派生。
3. Floor 2 用一个 Encounter 替换 Floor 1 的 Event 槽，保持 14–15 AP 并提高中层压力。
4. 蜕印使用 Card + Credential 双层合同，入口只认 Credential，避免普通卡软锁。
5. Floor 3 移除 Shop/FloorEntrance，使用必经核心恩赐与三段终局 Encounter 链，形成 10/16 AP。
6. Floor 3 无入口在当前静态图合同下可表达，但运行时胜利只会 Resolve Encounter，不能声明 Journey 已完成。
7. 四个 Navigation 均允许 Camp；正常内容吞吐约 8–10 AP/天，支持 5–6 天目标。
8. Production 继续被终局 success contract、46 个内容 definitions 和资产权威审计阻塞。

## Phase 1: Design Artifacts

- [data-model.md](./data-model.md)：三层骨架、Floor 2/3 完整 Node/Edge 表、内容槽、默认布局、身份矩阵与 AP 模型。
- [contracts/stable-identity-extension.md](./contracts/stable-identity-extension.md)：两层 Floor-scoped identity、蜕印双身份与 Host PersistentId。
- [contracts/floor2-graph-content-contract.md](./contracts/floor2-graph-content-contract.md)：Floor 2 拓扑、类型、Camp、蜕印和 AP 验收。
- [contracts/floor3-terminal-graph-contract.md](./contracts/floor3-terminal-graph-contract.md)：Floor 3 拓扑、终局 Guardian、无入口和运行时 blocker。
- [contracts/journey-pacing-production-readiness.md](./contracts/journey-pacing-production-readiness.md)：总 AP/天数模型与 Production readiness gate。
- [quickstart.md](./quickstart.md)：静态验证、跨工件审计、Git/LFS/哈希边界和 Unreal 跳过项记录。

### Post-design Constitution Re-check

PASS。设计没有新增 public API、模块依赖、GameplayTag、SaveGame 字段、运行时分支、Actor 规则副本或二进制资产；终局能力缺口被登记为后续阻塞，没有用失败状态或伪入口绕过。

## Project Structure

### Documentation (this feature)

```text
specs/009-formal-floor23-journey-pacing-freeze/
├── spec.md
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── checklists/requirements.md
├── contracts/
│   ├── stable-identity-extension.md
│   ├── floor2-graph-content-contract.md
│   ├── floor3-terminal-graph-contract.md
│   └── journey-pacing-production-readiness.md
└── tasks.md
```

### Repository changes

```text
AGENTS.md                         # 当前 Spec Kit plan 指针
.specify/feature.json             # 当前 feature 定位
Docs/WacomMap.md                  # 三层完整图、跨层门槛、终局与节奏
Docs/WacomDataAuthoring.md        # Floor 2/3 Production IDs 与制作门禁
Docs/TODO.md                      # 完成图冻结，登记 success/definitions/assets 后续
Docs/Questions.md                 # 关闭图/时长问题，新增终局交接问题
specs/009-formal-floor23-journey-pacing-freeze/*
```

**Structure Decision**: 不修改 `Source/`、`Config/`、`Content/`、`Docs/Architecture.md` 或 `Docs/WacomRun.md`。Spec Kit 记录设计审阅；长期规则和制作合同写入 WacomMap/DataAuthoring/TODO/Questions。

## Validation Plan

### Static validation

- 核对 Floor 2/3 各 20 Nodes / 21 Edges 及精确类型分布。
- 从各层 `Node.Entry` 执行可达性遍历，验证 A/B 与 C/D 两轮汇合。
- 删除 Floor 2 `Node.Key.01` 后验证 Guardian/Exit 不可达；删除 Floor 3 `Node.Core.01` 后验证终局链不可达。
- 检查 Floor 2 Exit 只指向 `Floor.Main.03`，只要求 `Credential.Run.MoltSeal`，且前置必经 Pickup 预留 grant。
- 检查 Floor 3 Guardian 无出边、无 FloorEntrance、`bBoss=true`，并显式标记 runtime success blocker。
- 检查两层 Floor/Node/Edge/content/Card/Credential ID 非空、作用域合法且不映射 Authoring/Debug。
- 检查默认 MapPosition 有限、位于 `[0,1920] × [0,1080]` 且 Floor 内不重合。
- 演算 Floor 2 `8–9 / 14–15`、Floor 3 `10 / 16`、Journey `26–28 / 44–46`。
- 运行 Spec Kit 跨工件一致性分析、模板残留检查、`git diff --check`、Git/LFS 与非目标文件审计。

### Explicitly skipped

- **WacomEditor compile**: 跳过；无 C++、Build.cs、Config 或反射变更。
- **Automation**: 跳过；无运行时、validator 或资产行为变更。
- **AssetRegistry / Blueprint compile**: 跳过；无资产创建、加载、绑定或保存。
- **PIE**: 跳过；没有 Production Floor 2/3 world，且 Journey success runtime 尚未实现。
- **Binary hash migration audit**: 不适用；不写 `.uasset/.umap`，最终必须证明 Content diff 为零。

跳过项不会引入运行时回归；剩余风险是设计尚未编码为资产，终局成功结算仍不可运行。

## Complexity Tracking

无 Constitution 违规或临时实现。
