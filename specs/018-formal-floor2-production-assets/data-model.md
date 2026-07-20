# Data Model: Floor 2 Production 47 DataAsset 播种与校验

## 1. Shared execution model

### Formal production content profile

| Field | Meaning |
|---|---|
| `LogName` | 独立日志前缀，例如 `FormalFloor1Content` |
| `ReportFolder` | 默认 Saved JSON 子目录 |
| `Manifest` | exact writable entries |
| `ExpectedTotal` | profile 总数 |
| `ExpectedGroupCounts` | Cards/EnemyGraph/NodeDefinitions 数量 |
| `ExpectedClassCounts` | 各 UDataAsset class 数量 |
| `ReadOnlyPackages` | 可加载但绝不保存的依赖 |
| `ConfigureExpected` | 根据 entry 构造 transient/新资产完整 seed default |
| `ValidateSpecific` | 主题特有计数与规则 invariant |

### Manifest entry

```text
Group: Cards | EnemyGraph | NodeDefinitions
PackagePath: valid /Game/Wacom/Data/... long package
StableId: non-empty unique FName
AssetClass: exact supported Definition class
```

### Build options and report

```text
Options = Group + bSeedMissing + bCompareSeedDefaults + ReportPath
EntryState = NotProcessed | Missing | Existing | Created | Failed
Report = manifest/selected/created/existing/missing/failed/saved counts
       + exitCode + failureCategory + ordered entry diagnostics
```

状态转换：

```text
Missing --inspect--> Missing/Validation
Missing --seed--> Created (saved and reloadable)
Existing valid --inspect/seed--> Existing (never saved)
Existing wrong class/structure --> Failed
Create/configure/save/reload error --> Failed and stop remaining selected entries
```

## 2. Floor 2 totals

| Group | Classes | Count |
|---|---|---:|
| Cards | `UCardDefinition` | 12 |
| EnemyGraph | 4 Behavior + 12 Part + 4 Enemy | 20 |
| NodeDefinitions | 7 Encounter + 3 Event + 4 Pickup + 1 Shop | 15 |
| **Total** | 8 supported Definition classes | **47** |

完整 package/class/stable-ID 表见 [contracts/asset-manifest.md](contracts/asset-manifest.md)。内容字段复用 Spec 017 [data-model.md](../017-formal-floor2-production-content-freeze/data-model.md)，本轮不得重新解释。

## 3. Content invariants

- 4 Enemy、4 Default Sequence Behavior、12 Part、26 ordered Intent。
- 12 Part 全部显式同敌 Aid/Destroy，legacy 空。
- 7 Encounter authored HP `21/36/42/36/34/57/70`，最多 2 Enemy。
- 12 Card：4 fixed + 8 branch；Guardian Destroy 是 2 费黄色 AllEnemyParts `Damage5 + Poison2`。
- 4 Pickup 固定 Card mapping；MoltSeal 额外授予 `Credential.Run.MoltSeal`。
- 3 Event 共 10 terminal Automatic Choice，ordered effect/condition/flag identity 冻结。
- DeepWayfarer 5 个 ordered offers，前三个外部 package 只读。

## 4. Stable versus tunable

| Stable structural facts | Tunable after first acceptance |
|---|---|
| package、class、stable IDs | DisplayName、Description、Event prose |
| Enemy/Part/Behavior/Intent/Set IDs and order | HP/EXP/I/R/magnitude via explicit balance revision |
| references、slot/order、selector shape | Card cost/rarity/magnitude via explicit balance revision |
| Card keyword/TargetMode/effect type-target-order | Shop price and Event numeric value via explicit revision |
| Pickup/Credential、Shop offer identity/order | Art/presentation remains deferred |
| Event condition/effect type-order and identities | — |

Seeder 永不写已有正确 class 资产，因此 tunable 变更不会被工具恢复。

## 5. Persistence and runtime impact

- 47 个对象是静态 DataAsset，没有新 runtime state。
- Battle/Run 继续读取现有 Definition fields；Snapshot、Command、Result、SaveGame 不变。
- RunFlag 仍是当前 Run 内存态；Credential 使用现有持久合同。
- 没有 GameplayTag、Config、Build.cs 或模块依赖变化。
