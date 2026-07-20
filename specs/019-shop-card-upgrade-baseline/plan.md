# Implementation Plan: 商店卡牌强化规则与制作合同基线

**Branch**: `codex/shop-card-upgrade-baseline` | **Date**: 2026-07-20 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/019-shop-card-upgrade-baseline/spec.md`

## Summary

在 `WacomData` 为 CardDefinition 增加不可变强化链和 ShopDefinition 可选价格表，在 `WacomRun` 增加精确实例 Quote/Command/Result 与 working-state 原子强化事务，并让买卡/强化共用首次交易 AP settlement。`WacomEditor` 负责局部链、全目录链和结构一致性校验；`WacomApp` 只把 Shop 静态配置传入访问请求并调用 Run 权威回收价值。现有资产默认不启用强化，不改变 Battle 或 SaveGame schema。

## Wacom Domain Context

**Primary Domain**: Data/card authoring / Run-exploration / Shop / Save-load / UI-App seam / Tests

**Required Docs Read**: `AGENTS.md`、Architecture、AgentIntegrationWorkflow、WacomData、DataAuthoring、Run、Battle、App、TODO、Questions、Roadmap、TechDebt。

**Docs To Update**: `Docs/WacomData.md`、`WacomDataAuthoring.md`、`WacomRun.md`、`WacomBattle.md`、`WacomApp.md`、`Architecture.md`、`TODO.md`、`Questions.md`、`Roadmap.md`、`TechDebt.md`。

**Owning Module(s)**: `WacomData`、`WacomRun`、`WacomApp`、`WacomEditor`、`WacomTests`。

**Dependency Direction**:

```text
WacomCore <- WacomData <- WacomBattle <- WacomRun <- WacomApp
WacomEditor -> runtime modules for validation only
WacomTests  -> runtime/app/editor validation as test harness
```

## Technical Context

**Language/Engine**: C++, Unreal Engine 5.8

**Primary UE Systems**: PrimaryDataAsset、GameplayTag values、USTRUCT Snapshot/Result、Automation、AssetRegistry read-only validation

**Storage/State**: Card/Shop DataAsset schema；FRunState in-memory ShopStates；SaveGame v5 existing DefinitionAssetPath

**Runtime Contracts**: CardDefinition fields/helper；Shop visit request/service；FRunShopSnapshot Quote；upgrade Command/Result；URunSession transaction

**Testing**: 默认 Unity WacomEditor compile；Data/Run/Save/Battle/App focused Automation；read-only asset load/hash

**Target Platform**: Windows Editor / packaged single-player runtime

**Performance Goals**: Quote 构建只在 Shop snapshot refresh 时遍历拥有卡；无 Tick、无异步轮询、无额外运行时资产加载

**Constraints**: passive future UI、one commit/one broadcast transaction、no binary mutation、no schema/tag/dependency change、用户审阅前不提交

**Scale/Scope**: 2 DataAsset schemas、Run shop state/API/private helper、4 identity match sites、1 App route seam、editor validators、small focused specs

**Blueprint Exposure Strategy**: DataAsset 制作字段和只读 Quote/Result 使用反射；authoritative upgrade command 仅由 C++ RunSession 入口执行，不给 WBP 直接写 RunState 的 API。

**Data/GameplayTag Impact**: 增加 Card/Shop 字段；复用既有 rarity tags；零新 tag、零资产写入。

**Save/Load Impact**: schema/version 不变；保存当前 DefinitionAssetPath 即保存强化层级，新增 Shop service state 继续与既有 ShopStates 一样不入档。

**UI/App Lifecycle Impact**: 现有 purchase-only Shop Screen 不新增按钮；Actor/PlayerController/Router 只传 visit request；未来 UI 通过 snapshot revision 刷新 Quote。

## Constitution Check

- **Docs and AGENTS Are the Rule Truth — PASS**: live Docs 与源码优先，完成后回写全部长期合同。
- **Wacom Module Boundaries Are Mandatory — PASS**: Data 静态合同、Run 事务、App adapter、Editor validation 分层明确；无 Build.cs 变化。
- **Domain Rules Before Presentation — PASS**: UI 不计算资格/价格，不直接替换 Definition。
- **Data, GameplayTags, and Authoring Stay Explicit — PASS**: 新字段、合法链、结构 comparator、零 tag/资产写入均明确。
- **Reusable Systems Over One-Off Work — PASS**: 族身份 helper 和共享 shop commerce settlement 服务所有卡与商店。
- **Validation Is Part of the Slice — PASS**: 四个可编译 checkpoint、focused Automation、只读资产审计和最终范围/LFS 门禁。

## Phase 0: Research

1. 审计 UCardDefinition、ShopDefinition、FCardInstance、SaveGame v5、Battle init、Shop AP transaction 与 UI revision。
2. 确认 Definition 替换可自然进入 Battle 并由现有 DefinitionAssetPath roundtrip。
3. 确认 AllowedCardIds 当前有四个独立精确匹配点，必须集中到 Data helper。
4. 确认回收价值在 Run 与 App 各维护一份，Run 可作为唯一经济真相且 App 已依赖 Run。
5. 冻结结构比较允许项、global merge audit、legacy shop compatibility 与 Spec 020 UI 边界。

结果见 [research.md](research.md)，无 unresolved clarification。

## Phase 1: Design Artifacts

- [data-model.md](data-model.md)：强化族、Shop service、visit state、Quote/Command/Result 和状态迁移。
- [contracts/card-upgrade-authoring.md](contracts/card-upgrade-authoring.md)：字段、合法链和 comparator。
- [contracts/shop-upgrade-runtime.md](contracts/shop-upgrade-runtime.md)：访问、Quote、事务、AP/revision/rollback。
- [contracts/validation-contract.md](contracts/validation-contract.md)：编译、Automation、AssetRegistry/hash/LFS 门禁。
- [quickstart.md](quickstart.md)：逐 checkpoint 验证 ledger。

## Project Structure

```text
Source/WacomData/
├── Public/Cards/CardDefinition.h
├── Private/Cards/CardDefinition.cpp
└── Public/Shops/ShopDefinition.h

Source/WacomRun/
├── Public/RunState.h
├── Public/RunSession.h
├── Private/RunSession.cpp
└── Private/Shops/RunShopTransaction.{h,cpp}

Source/WacomEditor/{Public,Private}/Validation/
├── CardDefinitionValidation.*
├── CardUpgradeCatalogValidation.*
└── ShopDefinitionValidation.*

Source/WacomTests/Private/
├── Data/CardUpgradeValidationSpec.cpp
├── Run/ShopCardUpgradeSpec.cpp
└── Run/CardUpgradeCompatibilitySpec.cpp
```

App route/header/cpp 和 CardFace builder 只做窄适配；具体 live 路径以 tasks.md 为准。

**Structure Decision**: `URunSession` 继续作为聚合根和单次通知 owner；资格、精确实例替换和 Quote 构建下沉 `FRunShopTransaction`，首次交易 AP 由一个 RunSession private settlement helper 同时供 Purchase/Upgrade 使用。

## Implementation Checkpoints

### Checkpoint 1 — Spec 与 Data 合同

- 添加强化字段/helper、Shop service schema、局部链与 catalog validation。
- 先写 `Wacom.Data.CardUpgrade` 测试，再实现并默认 Unity 编译。
- 运行 Card/Shop/CardUpgrade 定向测试，记录结果到 quickstart。

### Checkpoint 2 — Run 原子强化

- 添加 visit request、service state、Quote/Command/Result、精确实例事务和共享 commerce settlement。
- 保留旧 BeginShopVisit wrapper；现有购买行为和字段名兼容。
- 编译并运行 CardUpgrade、Shop AP、Notification、SnapshotRevision。

### Checkpoint 3 — 跨系统兼容

- 统一四处升级族身份匹配；验证 Battle init 和 Save v5 roundtrip。
- 扩展回收价值到 `1/2/3/4/0`，App card face 调用 Run helper。
- 编译并运行 Save、Backpack、Event、WorldInteraction、Battle init/rule matrix、Card presentation。

### Checkpoint 4 — Docs 与最终门禁

- 同步长期 Docs 和 quickstart。
- 最终 Unity 编译、全部受影响 focused tests、Card/Shop AssetRegistry/failed-load/hash 只读审计。
- `git diff --check`、范围审计、`git lfs fsck`；保持 unstaged/uncommitted 等待用户。

## Validation Plan

所有 Unreal 命令带 `-NoDreamShaderEditorBridge`。不运行 Builder、Blueprint compile 或 PIE；原因和风险记录在 [quickstart.md](quickstart.md)。

## Complexity Tracking

无 constitution violation。新增反射类型只服务 DataAsset 制作与未来被动 UI；算法、事务和 catalog 扫描保持 Private/C++。
