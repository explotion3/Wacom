# Implementation Plan: 正式 Floor 2 Production 内容合同冻结

**Branch**: `codex/formal-floor2-production-content-freeze` | **Date**: 2026-07-19 | **Spec**: [spec.md](./spec.md)

**Input**: 在 `Floor.Main.02` 已冻结的 20 Node/21 Edge 图上，以纯文档方式补齐 MoltCavern 的 47-package Production 内容合同、路线经济、击倒奖励和后续资产门禁。

## Summary

冻结四个 MoltCavern 敌人原型、十二部位、四个 Behavior、二十六个 Intent、七个 Encounter、十二张 Card、四个 Pickup、三个 RunEvent 与一个 Shop。所有内容复用当前 schema，不增加运行时能力。47 个未来 DataAsset 只获得稳定 ID、精确 package、结构字段和初始设计数值；本轮不创建资产、不运行 builder，也不触碰 Floor/map/Host。

Floor 2 继续保持 `8–9 / 14–15 AP`，四条关键路线的击倒奖励为 `17 / 18 / 17 / 18`，完整探索为 24。A/B 取金选项各自支持从 0 Gold 完成至少一次购买；情报 flag 服务 D 路事件。蜕印 Pickup 同时预留表现卡和独立 Credential。

## Wacom Domain Context

**Primary Domain**: Data-card authoring / Battle content / Run-exploration / Map content

**Required Docs Read**:

- [x] `AGENTS.md`
- [x] `Docs/AgentIntegrationWorkflow.md`
- [x] `Docs/Architecture.md`
- [x] `Docs/WacomData.md`
- [x] `Docs/WacomDataAuthoring.md`
- [x] `Docs/WacomBattle.md`
- [x] `Docs/WacomMap.md`
- [x] `Docs/TODO.md`
- [x] `Docs/Questions.md`
- [x] `Docs/Roadmap.md`
- [x] Spec 009、011、013、014 与当前 Spec Kit 模板/constitution

**Docs To Update**:

- [ ] `Docs/WacomData.md`
- [ ] `Docs/WacomDataAuthoring.md`
- [ ] `Docs/WacomBattle.md`
- [ ] `Docs/WacomMap.md`
- [ ] `Docs/TODO.md`
- [ ] `Docs/Questions.md`
- [ ] `Docs/Roadmap.md`

**Owning Module(s)**: 本轮无模块改动。未来静态资产属于 `WacomData`，既有规则解释属于 `WacomBattle/WacomRun`，制作服务和校验属于 `WacomEditor`，自动化属于 `WacomTests`。

**Dependency Direction**:

```text
WacomCore <- WacomData <- WacomBattle <- WacomRun <- WacomApp

WacomEditor -> runtime modules only for editor tooling/validation
WacomTests  -> runtime/app/editor validation as test harness
```

本轮不修改 `.Build.cs`、公共 API 或依赖方向。

## Technical Context

**Language/Engine**: Markdown/JSON design artifacts for Unreal Engine 5.8

**Primary UE Systems**: 只引用现有 DataAsset schema、GameplayTag authoring matrix、RunEvent/Shop/Pickup/Credential 与 FormalProduction Part validation

**Storage/State**: 无 runtime state；stable ID 与 package 是未来静态资产制作合同

**Runtime Contracts**: 不修改 Snapshot、Command、Resolution、ResultPacket、SaveGame、AP、RunFlag、Credential 或击倒事务

**Testing**: Spec Kit 跨工件分析、47-package 计数/唯一性、schema/目标、Encounter HP、路线奖励/AP、禁止引用、Markdown、`git diff --check`、Git/LFS/range 静态审计

**Target Platform**: N/A；纯文档

**Performance Goals**: N/A；零运行时改动

**Constraints**: 不修改 Source、Config、Content、GameplayTag、Build.cs、SaveGame、二进制资产或其它 Agent 资产；不运行 builder；用户审阅前不提交

**Scale/Scope**: 47 个未来 package、26 Intent、7 Encounter、10 Event Choice、5 Shop Offer、7 个长期 Docs、1 套 Spec 017 工件

**Blueprint Exposure Strategy**: 无 Blueprint 或反射改动

**Data/GameplayTag Impact**: 无 schema/tag/资产变更；只冻结现有字段的未来值与路径

**Save/Load Impact**: 无 schema 变化；两个 RunFlag 继续是内存态，`Credential.Run.MoltSeal` 复用当前通用持久集合

**UI/App Lifecycle Impact**: 无 UI、输入、焦点、镜头、Host 或场景生命周期改动

## Constitution Check

### Pre-research gate

- **Docs and AGENTS Are the Rule Truth — PASS**: live Docs、Spec 009 与当前制作范式已审计，长期事实将在本轮同步。
- **Wacom Module Boundaries Are Mandatory — PASS**: 零代码/依赖；未来职责保持 Data→Battle/Run、Editor 制作、Tests 验证。
- **Domain Rules Before Presentation — PASS**: 不修改 UI/Actor，不让表现层推导内容规则。
- **Data, GameplayTags, and Authoring Stay Explicit — PASS**: 精确列出 47 package、stable ID、结构字段、只读依赖和零 tag/schema 变化。
- **Reusable Systems Over One-Off Work — PASS**: 复用现有 typed Definition 与 FormalProduction 合同，不新增 Floor 2 特例类或 builder。
- **Validation Is Part of the Slice — PASS**: 静态验证完整；所有 Unreal 验证跳过原因与未来补证据门禁写入 quickstart。

## Phase 0: Research

研究结论见 [research.md](./research.md)：

1. 当前 Enemy/Behavior/Part/Intent schema 能表达四个 Archetype，无需 selector、cooldown 或新 target。
2. Floor 2 的 47 个资产按 Floor 1 exact-manifest 范式组织，但本轮只冻结，不提前复制 seeder。
3. Shield 的敌方自指与卡牌 Player target 必须在合同中分别写全，避免自然语言“Shield”造成错误 target。
4. 每敌人一对 Aid/Destroy 卡继续复用 Spec 012 的统一查询与 FormalProduction profile；12 Part 全部显式映射，legacy 为空。
5. 两个事件 flag 均可用现有 FName RunFlag 表达，不新增 GameplayTag 或 SaveGame 承诺。
6. A/B 各自的 Gold 选项与 3 Gold Shop 下界形成可购买路径；情报路线是主动放弃即时购买力换 D 路减压/减伤。
7. 关键路线奖励由必经 11 Part 加两轮支路得到 17/18/17/18，完整探索 24；不增加 AP、去重或替代奖励。
8. 已知 BugGirl StarterDeck 污染不属于本 manifest，继续保留 validator 失败事实而不修改资产或放宽门禁。

## Phase 1: Design Artifacts

- [data-model.md](./data-model.md)：47 资产分类、敌人/Intent、Encounter、Card、Pickup、Event、Shop、节点映射、路线模型与可调字段。
- [contracts/production-asset-manifest.md](./contracts/production-asset-manifest.md)：47 条精确 package/class/stable ID manifest。
- [contracts/enemy-encounter-contract.md](./contracts/enemy-encounter-contract.md)：敌人图、Intent target 与 Encounter 编组合同。
- [contracts/card-pickup-shop-contract.md](./contracts/card-pickup-shop-contract.md)：十二卡、四 Pickup、Shop 与 Part reward 引用合同。
- [contracts/run-event-contract.md](./contracts/run-event-contract.md)：三个 Event、十 Choice、条件/效果和经济合同。
- [contracts/production-readiness-gate.md](./contracts/production-readiness-gate.md)：未来资产播种/校验/场景前置与禁止依赖。
- [quickstart.md](./quickstart.md)：基线、静态命令、跳过项、已知风险与提交门禁。

### Post-design constitution re-check

- **PASS**: Design artifacts 只描述现有 schema 的内容实例，没有引入公共 API、运行时状态或一次性场景路径。
- **PASS**: stable structure 与 tunable presentation/balance 字段分离；文档冻结不冒充资产交付。
- **PASS**: 后续 47 资产需要独立 writer allowlist、真实加载、Data Validation、AssetRegistry、引用/哈希、幂等与 LFS 证据。

## Project Structure

### Documentation (this feature)

```text
specs/017-formal-floor2-production-content-freeze/
├── spec.md
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── contracts/
│   ├── production-asset-manifest.md
│   ├── enemy-encounter-contract.md
│   ├── card-pickup-shop-contract.md
│   ├── run-event-contract.md
│   └── production-readiness-gate.md
├── checklists/
│   ├── requirements.md
│   └── content-quality.md
└── tasks.md
```

### Repository files changed

```text
AGENTS.md
.specify/feature.json
Docs/WacomData.md
Docs/WacomDataAuthoring.md
Docs/WacomBattle.md
Docs/WacomMap.md
Docs/TODO.md
Docs/Questions.md
Docs/Roadmap.md
specs/017-formal-floor2-production-content-freeze/**
```

**Structure Decision**: 所有改动都是规划与长期文档。`Docs/Architecture.md`、`Docs/WacomRun.md`、`Docs/WacomApp.md` 和 `Docs/TechDebt.md` 不修改，因为没有架构、结算、UI 或临时代码变化。

## Validation Plan

**Compile**: 跳过。本轮没有 C++、Build.cs、Config、UHT 或二进制资产变化。

**Focused Automation**: 跳过。本轮没有可执行规则、DataAsset 或测试夹具变化。

**Asset/Editor/PIE**: 跳过 AssetRegistry、Builder、Blueprint 与 PIE；47 个资产和 Floor 2 场景尚不存在，运行这些流程不能为纯文档合同提供有效增量证据。

**Static validation**:

1. 校验 47-package 分类、package/stable ID 唯一性与禁止引用。
2. 校验 4 Enemy/12 Part/4 Behavior/26 Intent 与 7 Encounter HP/slot order。
3. 校验 12 Card/4 Pickup/3 Event/10 Choice/1 Shop 的 schema 值和有序结构。
4. 校验 Part reward 12/12、legacy 0、路线奖励 `17/18/17/18/24` 与 AP `8–9 / 14–15`。
5. 运行 Spec Kit prerequisite/analyze、Markdown link、`git diff --check`、Git/LFS status/fsck 与变更范围审计。

**Known external issue**: `DA_Character_BugGirl` StarterDeck 污染是已接受的外部问题。本轮不修改该资产、不削弱 validator，也不声称 Production dependency closure 已通过。

## Complexity Tracking

无 constitution violation。
