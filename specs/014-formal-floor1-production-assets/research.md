# Research: Floor 1 Production 46 DataAsset 播种与校验

## Decision 1 — 资产权威是“仅首次播种”

**Decision**: Formal Floor 1 工具只创建缺失 package。已存在且 class 正确的资产永远不由 seed 入口保存；class 错误直接失败。

**Rationale**: 46 个资产播种后会进入人工文案、数值和表现迭代。可重复覆盖的 builder 会把初始默认值错误升级为永久权威，并可能抹掉策划调参。

**Rejected**:

- 复用 `BuildDataAsset()`：它会把 expected editable properties 写回 existing asset。
- `-Force/-Replace`：不符合二进制资产权威和人工调参保护。
- 只手工新建 46 个空资产：无法复现、无法证明引用和字段完整。

## Decision 2 — 默认结构校验与 strict seed 对比分层

**Decision**: 默认 inspect 校验稳定结构；`CompareSeedDefaults` 精确比较首次 seed 全部 editable 字段。

**Rationale**: stable identity/引用/规则形状需要长期守护，文案与数值需要持续调优。一个 comparator 无法同时满足两种权威。

**Stable**:

- package/object/class/stable ID；
- Card keywords、TargetMode、ordered Effect type/target/target-zone；
- Behavior phase/set/intent ID、selector mode、ordered Effect type/target；
- Enemy/Encounter slot ID、顺序和引用；
- Part Aid/Destroy 映射、legacy null；
- Event node/choice ID、condition/effect type/order、policy、flag/pressure identity、terminal flags；
- Pickup reward type/card/credential；Shop offer card/order。

**Tunable**: DisplayName/Description/事件文本；Card cost/rarity/magnitude；Part HP/EXP；Intent initiative/resistance/magnitude；Shop price；Event numeric values。任何 tunable 字段仍必须通过通用 schema validator。

## Decision 3 — manifest 是单一制作真相

**Decision**: 每条 manifest 记录包含 package、asset name、class、stable ID、group、dependency、seed configure 与 structural compare；各适配器只调用同一服务。

**Rationale**: 把列表、创建和验证拆成三份很容易形成 45/46、路径拼写和 comparator 漂移。46 条线性规模适合显式 manifest，不需要通用反射配置语言。

## Decision 4 — 不修改共享 ContentBuilderHelpers

**Decision**: FormalFloor1 service 实现私有 create-missing helper；共享 `BuildDataAsset()` 和 `CopyEditedProperties()` 保持不变。

**Rationale**: Snake/TrainingWarrior 与其它已有 builder 依赖“幂等更新 existing”语义。修改共享 helper 会扩大回归和资产重存风险。

## Decision 5 — commandlet 与 Editor bridge 共用服务

**Decision**: `-run=WacomBuildFormalFloor1Content` 提供 CLI/CI；`Wacom.BuildFormalFloor1Content` editor-only console command接受同样参数并调用同一 runner。

**Rationale**: 最新 MCP 工作流要求正式二进制 mutation 在准确 Editor session + writer lease 内发生。Standalone commandlet 无法持有该 Editor lease；console bridge 能由 MCP 在同一 Editor 生命周期触发，又不需要增加 ToolsetRegistry Build.cs 依赖。

**Rejected**:

- 自定义 MCP toolset：需要新增模块依赖，超出批准边界。
- 让 Automation test 写 Production assets：会污染普通全量测试语义。
- acquire lease 后关闭 Editor 再跑 commandlet：破坏 session/audit 因果链。
- 直接用 Python 重写 46 份配置：复制 C++ manifest 和 comparator，长期漂移。

## Decision 6 — 三组依赖与预检

**Decision**:

```text
Cards (12)
  -> EnemyGraph (4 Behavior + 11 Part + 4 Enemy = 19)
  -> NodeDefinitions (6 Encounter + 4 Event + 4 Pickup + 1 Shop = 15)
```

每组 mutation 前先加载/校验所有外部依赖；组内按引用依赖顺序创建。MCP 使用三个独立 writer lease。

**Rationale**: 每组可以独立验证和审计，且不会保存 dangling/null Production 引用。NodeDefinitions 最后才允许读取 existing Starter/PoisonFang。

## Decision 7 — partial failure 保留现场，不自动回滚二进制

**Decision**: 单个 package 保存失败时停止后续创建，报告已保存/失败/未处理项；不删除已成功资产。重跑 `SeedMissing` 只补余项。

**Rationale**: Unreal Package 的补偿删除会制造更危险的资产现场。writer audit、报告和 Git status 比隐式回滚更可审计。

## Decision 8 — 通用 validator + Floor1 exact comparator

**Decision**: 每个真实资产先调用当前 shared validator；11 Part 额外使用 `FormalProduction`。随后 exact comparator 检查 Floor1 stable structure。

**Rationale**: 通用 validator 证明 schema/runtime authoring matrix，exact comparator 证明本内容包没有引用/顺序/身份漂移。二者职责不同，不能互相替代。

## Decision 9 — JSON report 与退出码

**Decision**: schema v1 report 记录 mode/group/strict、expected/present/created/saved/valid counts、每 package state、errors/warnings。退出码为 0/1/2/3。

**Rationale**: 文本日志适合人工诊断，JSON 适合集成、quickstart ledger 和重复运行比较。

## Decision 10 — 哈希只证明“未再保存”

**Decision**: 第二次 `SeedMissing` 期望 0 save，因此 46 文件 SHA-256 必须不变。首次跨机器生成不承诺相同二进制 hash；语义 reload/strict compare 是权威。

**Rationale**: Package serialization 可能包含引擎/环境细节，不能把跨机器 byte determinism 当作内容正确性。相同 worktree 的 no-save double-run 则应完全稳定。

## Decision 11 — 纠正 Spec 013 表格列错位

**Decision**: `ShallowGuardian.Destroy` 的权威配置为 `TargetMode=AllEnemyParts`，Damage 4 与 Poison 1 都使用 `Target.AllEnemyParts`。

**Rationale**: Spec 013 `data-model.md`、长期 `Docs/WacomData.md` 与用户批准计划一致；只有 `contracts/card-manifest.md` 行漏了一个列值。本轮记录纠正，不改变设计。

## Decision 12 — 不做 Blueprint/PIE

**Decision**: 本轮不创建 Blueprint、map、Host 或可进入的 Production Floor，因此不做 Blueprint compile 或 PIE。真实 DataAsset 由 Automation/AssetRegistry/failed-load 覆盖。

**Rationale**: PIE 无法在缺少 Production Journey/Floor/map 的情况下证明 46 个静态资产的场景可玩性。地图与 Golden Path 是明确的下一轮 gate。
