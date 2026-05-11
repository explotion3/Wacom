# Data Schema Draft

本文作为第一阶段数据 schema 的定稿草案。目标是锁死三件最容易卡住代码的事：
术语、蛇的数值、抵抗比较口径、`CardDefinition` 与 `FCardEffect` 结构。

本文只覆盖第一阶段需要跑通的最小集合。超出第一阶段的内容一律记在「暂不处理」或「占位字段」。
本文与 `Architecture.md §8` 的数据边界保持一致：静态定义进 `WacomData`，运行时实例进 `WacomBattle`。

## 1. 术语表

文档和代码统一使用以下术语，其他别名不再出现在新文档和新代码中。

| 正式术语 | 英文 / 代码名 | 说明 |
| --- | --- | --- |
| 敌方部位 | `EnemyPart` | 敌人由多个部位构成，每个部位拥有独立的 HP、意图和先机。第一阶段「敌人单位」= 敌方部位。 |
| 敌人 | `Enemy` | 一场战斗中由若干部位组成的整体，部位共享击倒和死亡判定。 |
| 意图 | `Intent` | 某个部位的下一次行动定义，自带先机值。 |
| 当前先机 | `CurrentInitiative` | 部位当前剩余的行动倒计时。 |
| 基础费用 | `BaseCost` | 卡牌静态定义上的费用。 |
| 运行时费用 | `RuntimeCost` | 本次打出该卡时实际使用的费用值。完美释放和先机扣减都以 `RuntimeCost` 为准。 |
| 手牌锚点 | `HandAnchor` | 特指左手牌和右手牌。 |
| 手牌区域 | `HandZone` | 左手区 / 双手区 / 右手区。 |
| 手牌队列 | `HandQueue` | 一条从左到右的有序手牌序列。区域判定由此序列推导。 |
| 静态定义 | `Definition` | DataAsset / DataTable 层的不可变配置，例如 `CardDefinition`、`EnemyPartDefinition`。 |
| 运行时实例 | `Runtime` | 战斗进行中的可变状态，例如 `FRuntimeCardInstance`、`FRuntimeEnemyPart`。 |

以下旧称不再使用：`enemy unit`、`敌方单位`、`Unit`、`Part`（单用时）、`HandIndex`（作为唯一手牌位置依据）。
已有文档中出现的旧称在后续修订时替换。

## 2. GameplayTag 规划

第一阶段只预留以下 tag 命名空间。tag 正式注册由 `Wacom/Config/DefaultGameplayTags.ini` 完成，
此处仅锁定命名。

```text
Card.Keyword.Swift           迅捷
Card.Keyword.Retain          保留
Card.Keyword.Combo           连击
Card.Keyword.Companion       伙伴
Card.Keyword.Weapon          武器
Card.Keyword.Tool            工具
Card.Keyword.Hand            手（左右手专属）
Card.Rarity.White
Card.Rarity.Blue
Card.Rarity.Intrinsic        固有

HandZone.Left
HandZone.Both
HandZone.Right

Effect.Damage
Effect.ApplyStatus.Poison
Effect.ApplyStatus.Slow
Effect.ApplyStatus.Freeze
Effect.ApplyStatus.Twilight  暮气
Effect.Shuffle.Random        随机腾挪
Effect.Shuffle.FromBothToOther
Effect.Shuffle.ToRandomZone
Effect.Card.AddCost          对目标卡 RuntimeCostModifier 增加
Effect.Card.ReduceCost       对目标卡 RuntimeCostModifier 减少（下限在 ComputeRuntimeCost 统一 clamp）
Effect.Heal

Status.Poison
Status.Slow
Status.Freeze
Status.Twilight
Status.Stunned               晕厥
Status.Shield

Target.Self
Target.SingleEnemyPart
Target.AllEnemyParts
Target.RandomHandCard
Target.ZoneHandCard
Target.Adjacent.Right
Target.LastShuffledCard      最近一次 Shuffle 效果产生的被移动卡（仅在同一批效果链内有效）

Magnitude.Source.Literal     FinalMagnitude = Effect.Magnitude 字段（默认）
Magnitude.Source.RuntimeCost FinalMagnitude = 本卡当前 RuntimeCost

Condition.Self.InZone        本卡当前在指定区域（ParamTag = HandZone.*）
Condition.Target.HasStatus   目标部位含指定状态（ParamTag = Status.*）
```

第一阶段不实现的 tag（如背包相关、任务相关、夜幕相关）暂不预留。

## 3. 蛇的数值表

第一阶段测试敌人。部位顺序为 `头 -> 身体 -> 尾巴`。意图序列循环执行，部位行动完成后刷新到下一条意图。

### 头

- HP: 16
- 意图序列（循环）:

| Index | Intent | 先机 | 抵抗值 | 说明 |
| --- | --- | --- | --- | --- |
| 0 | `Bite` 造成 6 伤害 | 3 | 6 | 初始意图 |
| 1 | `Venom` 施加 2 中毒 | 5 | 0 | 非攻击意图，抵抗值视为 0 |
| 2 | `Strike` 造成 8 伤害 | 4 | 8 | |

### 身体

- HP: 22
- 意图序列（循环）:

| Index | Intent | 先机 | 抵抗值 | 说明 |
| --- | --- | --- | --- | --- |
| 0 | `Constrict` 施加 1 减速 | 4 | 0 | 初始意图 |
| 1 | `Harden` 自身 +5 护盾 | 2 | 0 | |
| 2 | `Slam` 造成 5 伤害 | 3 | 5 | |

### 尾巴

- HP: 10
- 意图序列（循环）:

| Index | Intent | 先机 | 抵抗值 | 说明 |
| --- | --- | --- | --- | --- |
| 0 | `Sweep` 造成 3 伤害 | 1 | 3 | 初始意图 |
| 1 | `Lash` 造成 5 伤害 | 2 | 5 | |
| 2 | `Whip` 造成 4 伤害 | 3 | 4 | |

### 战斗开始时先机总和

`3 + 4 + 1 = 8`。该值保证第一回合 `Cost 5` 的暮蛉仍满足 `RuntimeCost <= Enemy Initiative Sum`。

### 完美释放命中窗口（第一回合起始）

| 部位 | 当前先机 | 命中该先机的 Cost |
| --- | --- | --- |
| 头 | 3 | 3 |
| 身体 | 4 | 4 |
| 尾巴 | 1 | 1 |

Cost 1（烁光蝶）、Cost 2（右手）、Cost 5（暮蛉）中只有 Cost 1 在第一回合能直接完美释放尾巴。
其他 Cost 需要配合等待或推进使先机滑动到目标值。等待初始值 2 也意味着单次等待后尾巴直接行动，
这是第一阶段期望出现的"等待即触发"风险模型。

### 第一阶段不做

- 蛇的状态抗性。
- 蛇部位间的联动（例如头被破坏时身体得到强化）。
- 蛇的特殊破坏奖励。

## 4. 抵抗比较口径（第一阶段定稿）

### 原则

只在"先机命中成立"时触发抵抗判定。即 `CardRuntimeCost == Part.CurrentInitiativeBeforePlay` 的每一个部位都会进入抵抗。

### 抵抗值来源

第一阶段统一使用以下规则，不再引入卡牌面板上的独立"抵抗值"字段。

| 项 | 抵抗值 | 说明 |
| --- | --- | --- |
| 卡牌 | 主效果中首个 `Effect.Damage` 的 FinalMagnitude（由 `MagnitudeResolver` 计算）；若卡牌不含伤害效果，记为 0 | `MagnitudeSource = Literal` 时为 `Magnitude` 字段；`= RuntimeCost` 时为本次 RuntimeCost |
| 意图 | 意图上的 `ResistanceValue` 字段（见 `FIntentDefinition`）；攻击类意图填伤害值，非攻击类意图填 0 | 已在 §3 每条意图的"抵抗值"列列出 |

### 判定

```text
if CardResistance > IntentResistance:
    该部位进入 Status.Stunned（晕厥）
else:
    不触发晕厥
```

晕厥处理遵循 `Battle_Rules.md §10`：下一次轮到该部位行动时跳过意图，但仍刷新意图并重置先机。

### 第一阶段显式约束

- 卡牌和意图的抵抗值都是整数，不做小数或百分比。
- 卡牌没有伤害时抵抗值为 0，意图没有伤害时抵抗值为 0。`0 > 0` 为假，不晕厥——这是"非攻击 vs 非攻击"的默认结果。
- 抵抗判定先于完美释放（已由 `Battle_Rules.md §9` 规定）。
- 抵抗本身不改变伤害、不改变先机、不阻止本次打牌推进先机。

### 暂不处理

- 护盾对抵抗值的影响。
- 多重意图的抵抗值合并。
- 卡牌面板独立抵抗值字段。
- 非伤害效果之间的抵抗（如"施加中毒 vs 施加减速"）。

## 5. 卡牌数据 Schema

本章是第一阶段 `WacomData` 层的静态定义草案。字段名用于锁定方向，代码落地时保留这些字段。

### 5.1 `UCardDefinition : UPrimaryDataAsset`

```cpp
// Pseudo-UHT. 仅用于说明字段。
UCLASS(BlueprintType)
class UCardDefinition : public UPrimaryDataAsset
{
    UPROPERTY(EditDefaultsOnly) FName              CardId;           // 唯一 ID
    UPROPERTY(EditDefaultsOnly) FText              DisplayName;      // 显示名
    UPROPERTY(EditDefaultsOnly) FText              Description;      // 显示文本
    UPROPERTY(EditDefaultsOnly) int32              BaseCost = 0;     // 基础 Cost
    UPROPERTY(EditDefaultsOnly) FGameplayTag       Rarity;           // Card.Rarity.*
    UPROPERTY(EditDefaultsOnly) FGameplayTagContainer Keywords;      // Card.Keyword.*
    UPROPERTY(EditDefaultsOnly) FCardPhysique      Physique;         // 身材，可选
    UPROPERTY(EditDefaultsOnly) ECardTargetMode    TargetMode;       // None / SingleEnemyPart / AllEnemyParts / Self / HandCard
    UPROPERTY(EditDefaultsOnly) TArray<FCardEffect> Effects;         // 主效果
    UPROPERTY(EditDefaultsOnly) TArray<FCardEffect> PerfectReleaseEffects; // 完美释放效果
    UPROPERTY(EditDefaultsOnly) TArray<FCardZoneHook> ZoneHooks;     // 区域相关效果或修正（占位，见 5.4）
    UPROPERTY(EditDefaultsOnly) TArray<FCardPassive> Passives;       // 被动触发（占位，见 5.5）
};
```

### 5.2 `FCardPhysique`

```cpp
USTRUCT(BlueprintType)
struct FCardPhysique
{
    UPROPERTY(EditDefaultsOnly) int32 MaxHpBonus = 0;     // 入战生命值上限加成
    UPROPERTY(EditDefaultsOnly) int32 Durability = 0;     // 0 = 无耐久限制
    UPROPERTY(EditDefaultsOnly) int32 BagCapacity = 0;    // 背包容量加成，第一阶段不读取
};
```

### 5.3 `FCardEffect`（核心）

第一阶段所有卡牌效果都用这个结构表达。执行器见 `CardEffectExecutor`。

```cpp
USTRUCT(BlueprintType)
struct FCardEffect
{
    UPROPERTY(EditDefaultsOnly) FGameplayTag     EffectType;       // Effect.*
    UPROPERTY(EditDefaultsOnly) int32            Magnitude = 0;    // 伤害值 / 状态层数 / 次数（Source = Literal 时使用）
    UPROPERTY(EditDefaultsOnly) FGameplayTag     Target;           // Target.*
    UPROPERTY(EditDefaultsOnly) FGameplayTag     TargetZone;       // HandZone.*（仅当 Target 涉及区域时使用）
    UPROPERTY(EditDefaultsOnly) int32            Duration = 0;     // 状态回合数
    UPROPERTY(EditDefaultsOnly) FGameplayTag     MagnitudeSource;  // Magnitude.Source.*（见下）
    UPROPERTY(EditDefaultsOnly) FEffectCondition Condition;        // 执行条件（见 §5.3.1）
    // Deprecated: bMagnitudeFromRuntimeCost —— 保留仅用于旧 DataAsset 兼容，新数据用 MagnitudeSource
};
```

**Magnitude.Source 取值**（决定 FinalMagnitude 的计算方式）：

| Source Tag | FinalMagnitude = |
| --- | --- |
| Invalid / `Magnitude.Source.Literal` | `Magnitude` 字段 |
| `Magnitude.Source.RuntimeCost` | 本卡当前 RuntimeCost（朝光暮蝶用） |
| 未来扩展 | 在 `MagnitudeResolver` 注册即可，如 `HandSize` / `DrawPileSize` / `DamageCardsPlayed` |

### 5.3.1 `FEffectCondition`（效果执行条件）

```cpp
USTRUCT(BlueprintType)
struct FEffectCondition
{
    UPROPERTY(EditDefaultsOnly) FGameplayTag ConditionType;  // Condition.*，未设置 = 永真
    UPROPERTY(EditDefaultsOnly) FGameplayTag ParamTag;       // 辅助 tag（Zone / Status / Keyword）
    UPROPERTY(EditDefaultsOnly) int32        ParamInt = 0;   // 辅助数值（阈值）
    UPROPERTY(EditDefaultsOnly) bool         bNegate = false;// 结果取反
};
```

**Condition 取值**：

| ConditionType | 语义 | ParamTag | ParamInt |
| --- | --- | --- | --- |
| Invalid（未设置） | 永真，效果永远执行 | - | - |
| `Condition.Self.InZone` | 本卡当前在指定区域 | `HandZone.Left/Both/Right` | - |
| `Condition.Target.HasStatus` | 目标部位含指定状态（层数 > 0） | `Status.*` | - |
| 未来扩展 | 在 `ConditionResolver` 注册即可，如 `Hand.SizeAtLeast` / `EnemyParts.AliveAtLeast` | | |

`bNegate = true` 把结果取反。例如 "自卡不在左手区" = `InZone(Left) + bNegate=true`。

### 5.3.2 EffectType 字段速查表

每个 EffectType 对 `FCardEffect` 字段的使用方式不同。配卡时对照本表。
"-" 表示该字段对此 EffectType 无效，填写默认值。

| EffectType | Magnitude 语义 | Target（典型值）| TargetZone | Duration | MagnitudeSource | 备注 |
| --- | --- | --- | --- | --- | --- | --- |
| `Effect.Damage` | 伤害值 | `Target.SingleEnemyPart` / `Target.AllEnemyParts` / `Target.Player` | - | - | Literal / RuntimeCost | 部位 HP 归零立即破坏 |
| `Effect.Heal` | 治疗量 | `Target.Self` → 玩家 / `Target.SingleEnemyPart` | - | - | Literal | 第一阶段未实现 |
| `Effect.ApplyStatus.Poison` | 层数 | `Target.Player` / `Target.SingleEnemyPart` / `Target.AllEnemyParts` | - | - | Literal / RuntimeCost（朝光暮蝶）| 层数模型，不用 Duration |
| `Effect.ApplyStatus.Slow` | 层数 | 同上 | - | - | Literal | 层数模型，第一阶段只记录 |
| `Effect.ApplyStatus.Freeze` | 层数 | `Target.SingleEnemyPart` | - | 回合数（0 = 层数模型）| Literal | 当前按层数消耗实现 |
| `Effect.ApplyStatus.Twilight` | 层数 | `Target.SingleEnemyPart` | - | - | Literal | 第一阶段只记录 |
| `Status.Shield`（复用为效果 tag）| 护盾值 | `Target.Player` / `Target.Self`（部位）| - | - | Literal | 直接加到 Shield 字段，不走状态层数 |
| `Effect.Shuffle.Random` | - | `Target.RandomHandCard` | - | - | - | 从手牌随机选一张（排除本卡）腾挪；写 `LastShuffledCardId` |
| `Effect.Shuffle.FromBothToOther` | - | `Target.ZoneHandCard` | `HandZone.Both` | - | - | 从双手区随机挑一张腾挪到左/右手区 |
| `Effect.Shuffle.ToRandomZone` | - | `Target.Self`（本卡）| - | - | - | 把本卡腾挪到随机区域（烁光蝶 AfterPlayed 用）|
| `Effect.Card.AddCost` | Modifier 增量 | `Target.Self`（本卡）/ `Target.LastShuffledCard` | - | - | Literal | 修改 `RuntimeCostModifier`，下次进手牌生效；无上限 |
| `Effect.Card.ReduceCost` | Modifier 减量 | 同上 | - | - | Literal | 下限由 `ComputeRuntimeCost` clamp 到 0 |

### 5.3.3 Target 字段速查表

`Target` 决定 `EffectContext::TargetKind` / `TargetInstanceId`。配卡时对照本表选合适的值。

| Target | 解析为 | TargetInstanceId 来源 | 是否需要 TargetZone |
| --- | --- | --- | --- |
| `Target.Self` | Player 或本卡（视 EffectType）| 本卡 InstanceId（HandCard 类效果）或 Invalid（Player 类效果）| 否 |
| `Target.Player` | Player | Invalid | 否 |
| `Target.SingleEnemyPart` | EnemyPart | 调用方选中的部位（来自 BattleCommand）| 否 |
| `Target.AllEnemyParts` | EnemyPart（循环展开）| CardEffectDispatcher 自动遍历存活部位 | 否 |
| `Target.RandomHandCard` | HandCard | HandZoneService 自选（Shuffle 类效果）| 否 |
| `Target.ZoneHandCard` | HandCard | HandZoneService 按 TargetZone 过滤后自选 | **是**（必须填 `HandZone.*`）|
| `Target.LastShuffledCard` | HandCard | `EffectContext::LastShuffledCardId`（上一条 Shuffle 效果的产物）| 否 |
| `Target.Adjacent.Right` | EnemyPart | 第一阶段未实现（右手"相邻右方伙伴代打"）| 否 |

**Target.Self 的 EffectType 消歧**（由 `CardEffectDispatcher::FillTargetFromCardEffect` 处理）：
- `Effect.Shuffle.ToRandomZone` / `Effect.Card.AddCost` / `Effect.Card.ReduceCost` → 指向本卡（HandCard）
- 其他（Damage / Heal / ApplyStatus）→ 指向玩家（Player）

### 5.3.4 配卡速查示例

以下是几个常见配法，供新卡对照：

**"对单个敌方部位造成 5 伤害"**
```yaml
EffectType: Effect.Damage
Magnitude: 5
Target: Target.SingleEnemyPart
```

**"对目标施加等于本卡 Cost 的中毒"**（朝光暮蝶）
```yaml
EffectType: Effect.ApplyStatus.Poison
Magnitude: 0
MagnitudeSource: Magnitude.Source.RuntimeCost
Target: Target.SingleEnemyPart
```

**"把本卡腾挪到随机区域"**（烁光蝶 AfterPlayed）
```yaml
EffectType: Effect.Shuffle.ToRandomZone
Target: Target.Self
```

**"从双手区腾挪一张到其他区域"**（赤腹工蚁）
```yaml
EffectType: Effect.Shuffle.FromBothToOther
Target: Target.ZoneHandCard
TargetZone: HandZone.Both
```

**"被腾挪卡 -1 Cost，本卡 +1 Cost"**（朝光暮蝶右手区 ZoneHook 组合）
```yaml
[0] EffectType: Effect.Shuffle.Random           Target: Target.RandomHandCard
[1] EffectType: Effect.Card.ReduceCost Mag=1    Target: Target.LastShuffledCard
[2] EffectType: Effect.Card.AddCost    Mag=1    Target: Target.Self
```

**"只有目标中毒时才造成额外伤害"**（示例，未来扩展）
```yaml
EffectType: Effect.Damage
Magnitude: 3
Target: Target.SingleEnemyPart
Condition:
  ConditionType: Condition.Target.HasStatus
  ParamTag: Status.Poison
```

### 5.3.5 现有 EffectType 的完整清单

| EffectType | 现状 |
| --- | --- |
| `Effect.Damage` | 已实现 |
| `Effect.Heal` | 预留，未实现 |
| `Effect.ApplyStatus.Poison` | 已实现（P3.1 真正结算伤害）|
| `Effect.ApplyStatus.Slow` | 占位（仅记录层数）|
| `Effect.ApplyStatus.Freeze` | 占位（行动时跳过，消耗层数）|
| `Effect.ApplyStatus.Twilight` | 占位（仅记录层数，触发 OnTwilightTriggered 事件）|
| `Effect.Shuffle.Random` | 已实现 |
| `Effect.Shuffle.FromBothToOther` | 已实现 |
| `Effect.Shuffle.ToRandomZone` | 已实现 |
| `Effect.Card.AddCost` | 已实现 |
| `Effect.Card.ReduceCost` | 已实现 |
| `Status.Shield` | 已实现（作为效果 tag 直接加 Shield 字段）|

第一阶段不实现：费用转移的显式 tag、`突袭`、`夜幕降临` 等。

### 5.4 `FCardZoneHook`

```cpp
USTRUCT(BlueprintType)
struct FCardZoneHook
{
    UPROPERTY(EditDefaultsOnly) FGameplayTag Zone;         // HandZone.Left / Both / Right
    UPROPERTY(EditDefaultsOnly) FGameplayTag Trigger;      // ZoneHook.Trigger.*
    UPROPERTY(EditDefaultsOnly) TArray<FCardEffect> ExtraEffects;
};
```

**ZoneHook.Trigger 取值**：

| Trigger | 触发时机 | ExtraEffects 执行? | 典型卡 |
| --- | --- | --- | --- |
| `ZoneHook.Trigger.OnPlay` | 本卡打出时（CardPlayed 事件之后、主效果之前）| 是 | 朝光暮蝶右手区（费用转移）|
| `ZoneHook.Trigger.OnPerfectReleaseHit` | 本卡完美释放命中时 | 否，仅作为"跳过先机推进"标记 | 朝光暮蝶左手区（不推先机）|

**触发顺序**：
1. `OnPlay` 发生在 `CardPlayed` 事件之后、"记录出牌前先机"之前
2. `OnPerfectReleaseHit` 的判定紧跟在抵抗判定之后，作为先机推进的 skip 条件
3. 不同区域的 Hook 不会同时命中（卡只在一个区域）

### 5.5 `FCardPassive`

```cpp
USTRUCT(BlueprintType)
struct FCardPassive
{
    UPROPERTY(EditDefaultsOnly) FGameplayTag     Trigger;            // Passive.Trigger.*
    UPROPERTY(EditDefaultsOnly) TArray<FCardEffect> Effects;          // 触发后执行，走 CardEffectDispatcher
    UPROPERTY(EditDefaultsOnly) FEffectCondition Condition;          // 触发门控，未设置则永真
    UPROPERTY(EditDefaultsOnly) int32            TriggerThreshold = 0;// 仅计数类 trigger（OnCompanionCount）使用
};
```

**Passive.Trigger 取值**：

| Trigger | 触发时机 | 使用 TriggerThreshold? | 典型卡 |
| --- | --- | --- | --- |
| `Passive.Trigger.AfterPlayed` | 本卡打出完成后（卡牌去向之后）| 否 | 烁光蝶（自腾挪） |
| `Passive.Trigger.OnCompanionCount` | 全局 Companion 计数达阈值 | 是 | 拂晓飞蛾（回手，阈值 3）|
| `Passive.Trigger.OnTwilightTriggered` | 暮气施加成功时 | 否 | 暮蛉（P3.5 占位）|

**Condition 字段**复用 `FEffectCondition`（§5.3.1）。典型用法：
- "本卡在双手区时 AfterPlayed 才触发" → `Condition.Self.InZone(HandZone.Both)`
- 未设置时永真（所有现有被动的默认行为）

**TriggerThreshold** 只用于计数类 trigger（当前只有 OnCompanionCount）。达到阈值后触发并清零计数。其他 trigger 不读此字段，保留 0 即可。

## 6. 敌人数据 Schema

### 6.1 `UEnemyDefinition : UPrimaryDataAsset`

```cpp
UCLASS(BlueprintType)
class UEnemyDefinition : public UPrimaryDataAsset
{
    UPROPERTY(EditDefaultsOnly) FName            EnemyId;
    UPROPERTY(EditDefaultsOnly) FText            DisplayName;
    UPROPERTY(EditDefaultsOnly) TArray<FEnemyPartSlot> Parts; // 顺序即部位顺序
};

USTRUCT(BlueprintType)
struct FEnemyPartSlot
{
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UEnemyPartDefinition> PartDef;
};
```

### 6.2 `UEnemyPartDefinition : UPrimaryDataAsset`

```cpp
UCLASS(BlueprintType)
class UEnemyPartDefinition : public UPrimaryDataAsset
{
    UPROPERTY(EditDefaultsOnly) FName             PartId;          // 如 Snake.Head
    UPROPERTY(EditDefaultsOnly) FText             DisplayName;
    UPROPERTY(EditDefaultsOnly) int32             MaxHp = 0;
    UPROPERTY(EditDefaultsOnly) TArray<FIntentDefinition> IntentSequence; // 循环执行
    UPROPERTY(EditDefaultsOnly) int32             InitialIntentIndex = 0;
};
```

### 6.3 `FIntentDefinition`

```cpp
USTRUCT(BlueprintType)
struct FIntentDefinition
{
    UPROPERTY(EditDefaultsOnly) FName                   IntentId;
    UPROPERTY(EditDefaultsOnly) FText                   DisplayName;
    UPROPERTY(EditDefaultsOnly) int32                   Initiative = 0;       // 本意图的先机值
    UPROPERTY(EditDefaultsOnly) int32                   ResistanceValue = 0;  // 抵抗比较用值；攻击意图填伤害，非攻击填 0
    UPROPERTY(EditDefaultsOnly) TArray<FIntentEffect>   Effects;              // 行动时产生的效果
};

USTRUCT(BlueprintType)
struct FIntentEffect
{
    UPROPERTY(EditDefaultsOnly) FGameplayTag EffectType;   // 复用 Effect.*
    UPROPERTY(EditDefaultsOnly) int32        Magnitude = 0;
    UPROPERTY(EditDefaultsOnly) FGameplayTag Target;       // Target.Self 或 Target.Player
    UPROPERTY(EditDefaultsOnly) int32        Duration = 0;
};
```

第一阶段意图效果使用的 `EffectType` 与卡牌共用一套。意图打玩家伤害时 `Target = Target.Player`，
给自己上护盾时 `Target = Target.Self`。

### 6.4 蛇的 DataAsset 组织

```text
Content/Wacom/Enemies/
  Snake/
    DA_Enemy_Snake.uasset               UEnemyDefinition
    DA_Part_Snake_Head.uasset           UEnemyPartDefinition
    DA_Part_Snake_Body.uasset
    DA_Part_Snake_Tail.uasset
```

`DA_Enemy_Snake.Parts` 顺序 = `Head, Body, Tail`。具体字段值按 §3 表格填写。

## 7. 运行时实例 Schema

与静态定义分离。运行时类型属于 `WacomBattle`。

```cpp
USTRUCT()
struct FRuntimeCardInstance
{
    FGuid                    InstanceId;
    TObjectPtr<const UCardDefinition> Def;
    int32                    RuntimeCostModifier = 0; // 本场战斗累计修正
    FGameplayTagContainer    TemporaryKeywords;       // 本场战斗内临时关键字
    ECardLocation            Location = ECardLocation::Unknown; // 当前所在容器
};

USTRUCT()
struct FRuntimeEnemyPart
{
    FGuid                    InstanceId;
    TObjectPtr<const UEnemyPartDefinition> Def;
    int32                    CurrentHp = 0;
    int32                    CurrentIntentIndex = 0;
    int32                    CurrentInitiative = 0;
    bool                     bDestroyed = false;
    FGameplayTagContainer    Statuses;                // Status.*
    TMap<FGameplayTag, int32> StatusStacks;           // 层数
    int32                    Shield = 0;
};
```

运行时实例在 `BattleSession::Initialize` 时由静态定义创建。

## 8. 虫妹最小卡组样例（按 Schema）

仅列出与 Schema 直接相关的关键字段。完整显示文本在 DataAsset 里维护。

### 左手

```yaml
CardId: LeftHand
BaseCost: 2
Rarity: Intrinsic
Keywords: [Hand, Weapon, Tool]
TargetMode: None
Effects: []                              # 第一阶段不给左手配主效果
PerfectReleaseEffects: []                # 第一阶段留空
ZoneHooks:
  - Zone: HandZone.Left
    Trigger: ZoneHook.Trigger.OnPlay
    ExtraEffects: []                     # 「左方无牌则抽 1 张非初始牌并赋予迅捷」 —— 第一阶段占位
Passives: []
```

第一阶段左手仅实现为"合法可打出、打出后离开手牌不入任何区域、保留关键字由手牌队列生成阶段应用"，
其主动效果和完美释放效果留空，待背包系统介入后再接。

### 右手

```yaml
CardId: RightHand
BaseCost: 2
Rarity: Intrinsic
Keywords: [Hand, Weapon, Tool]
TargetMode: SingleEnemyPart
Effects:
  - EffectType: Effect.Damage
    Magnitude: 8
    Target: Target.SingleEnemyPart
PerfectReleaseEffects: []
ZoneHooks: []
Passives: []
```

「相邻右方有伙伴则改为使用该伙伴」第一阶段不实现，写在 `ZoneHooks` 后续版本。

### 朝光暮蝶 Zhaoguang Mudie

```yaml
CardId: ZhaoguangMudie
BaseCost: 0
Rarity: White
Keywords: [Companion]
Physique: { MaxHpBonus: 1 }
TargetMode: SingleEnemyPart
Effects:
  - EffectType: Effect.Shuffle.Random
    Target: Target.RandomHandCard
  - EffectType: Effect.ApplyStatus.Poison
    Magnitude: 0
    MagnitudeSource: Magnitude.Source.RuntimeCost  # 层数 = 当前 RuntimeCost
    Target: Target.SingleEnemyPart
    Duration: 0                          # 中毒为层数模型，不使用 Duration
ZoneHooks:
  - Zone: HandZone.Left
    Trigger: ZoneHook.Trigger.OnPerfectReleaseHit
    ExtraEffects: []                     # 占位：本次打出不推进先机。Executor 读到该 hook 时设 bSkipInitiativePush
  - Zone: HandZone.Right
    Trigger: ZoneHook.Trigger.OnPlay
    ExtraEffects: []                     # 占位：费用转移被动。第一阶段不实现
```

### 拂晓飞蛾 Fuxiao Feie

```yaml
CardId: FuxiaoFeie
BaseCost: 0
Rarity: Blue
Keywords: [Companion]
Physique: { MaxHpBonus: 1 }
TargetMode: SingleEnemyPart
Effects:
  - EffectType: Effect.ApplyStatus.Slow
    Magnitude: 1
    Target: Target.SingleEnemyPart
Passives:
  - Trigger: Passive.Trigger.OnCompanionCount
    TriggerThreshold: 3
    Effects: []                          # 占位：从非手牌区域回到手牌。第一阶段不实现
```

### 赤腹工蚁 Chifu Gongyi

```yaml
CardId: ChifuGongyi
BaseCost: 0
Rarity: White
Keywords: [Companion, Retain]
Physique: { MaxHpBonus: 1 }
TargetMode: None
Effects:
  - EffectType: Effect.Shuffle.FromBothToOther
    Target: Target.ZoneHandCard
    TargetZone: HandZone.Both
```

使用前提：双手区存在且非空（由 Resolver 在「特殊条件」阶段校验）。

### 烁光蝶 Shuoguang Die

```yaml
CardId: ShuoguangDie
BaseCost: 1
Rarity: White
Keywords: [Companion, Weapon, Combo]
Physique: { MaxHpBonus: 6 }
TargetMode: SingleEnemyPart
Effects:
  - EffectType: Effect.Damage
    Magnitude: 7
    Target: Target.SingleEnemyPart
Passives:
  - Trigger: Passive.Trigger.AfterPlayed
    Effects:
      - EffectType: Effect.Shuffle.ToRandomZone
        Target: Target.Self               # 此处 Self 指这张卡
```

连击的"回原位"默认由 `Keyword.Combo` 驱动。烁光蝶的 AfterPlayed 被动显式覆盖为「腾挪到随机区域」，
按 `Battle_Rules.md §8` 卡牌离开手牌去向的"卡牌文本优先"处理。

### 暮蛉 Muling

```yaml
CardId: Muling
BaseCost: 5
Rarity: Blue
Keywords: [Companion]
Physique: { MaxHpBonus: 23 }
TargetMode: SingleEnemyPart
Effects:
  - EffectType: Effect.ApplyStatus.Freeze
    Magnitude: 1
    Duration: 1
    Target: Target.SingleEnemyPart
Passives:
  - Trigger: Passive.Trigger.OnTwilightTriggered
    Effects: []                           # 占位：使一张中毒卡效果 +1。第一阶段不实现
```

"突袭" 关键字第一阶段不进 `Keywords`。

## 9. 角色 Schema

```cpp
UCLASS(BlueprintType)
class UCharacterDefinition : public UPrimaryDataAsset
{
    UPROPERTY(EditDefaultsOnly) FName CharacterId;
    UPROPERTY(EditDefaultsOnly) FText DisplayName;
    UPROPERTY(EditDefaultsOnly) int32 BaseMaxHp = 20;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UCardDefinition> LeftHandCard;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UCardDefinition> RightHandCard;
    UPROPERTY(EditDefaultsOnly) TArray<TObjectPtr<UCardDefinition>> StarterDeck; // 不含左右手
};
```

虫妹的 `BaseMaxHp` 第一阶段定为 `20`。算上初始卡组全部带入战斗的身材加成
`1+1+1+6+23 = 32`，战斗开始时最大 HP = `20 + 32 = 52`。

该值仅作为第一阶段测试用，未经平衡。

## 10. 状态 Schema（第一阶段占位）

```cpp
UENUM()
enum class EStatusHost : uint8
{
    Player,
    EnemyPart,
    // 后续扩展：Card / Intent / Enemy
};

USTRUCT()
struct FStatusInstance
{
    FGameplayTag Tag;         // Status.*
    int32        Stacks = 0;  // 层数
    int32        Duration = 0;// 回合数，0 表示按层数模型
};
```

第一阶段状态行为：

| 状态 | 归属 | 行为 |
| --- | --- | --- |
| `Status.Poison` | EnemyPart 或 Player | 触发时机：玩家每打出一张牌后 + 敌方部位每行动一次后，对拥有中毒的一方造成等于层数的伤害。穿透护盾，直接扣生命值。层数不因结算减少 |
| `Status.Slow` | EnemyPart | 仅记录层数，不影响数值。先占位不生效 |
| `Status.Freeze` | EnemyPart | 持有时该部位下一次行动跳过并刷新意图。作用等同晕厥，第一阶段可共用跳过分支 |
| `Status.Twilight` | EnemyPart | 仅记录层数，不触发任何效果。为暮蛉的被动留钩子 |
| `Status.Stunned` | EnemyPart | 由抵抗判定施加，规则见 `Battle_Rules.md §10` |
| `Status.Shield` | EnemyPart 或 Player | `Shield` 字段已在运行时实例中单独存储，不使用层数模型 |

第一阶段状态公式：中毒已正式化（见 `Battle_Rules.md §15`）；减速/暮气的数值效果仍未决。

## 11. Cost 合法性与先机一致性

`Battle_Rules.md §5` 已规定：
- `Enemy Initiative Sum = Sum(Living EnemyParts CurrentInitiative)`
- `if CardRuntimeCost > Enemy Initiative Sum: 卡不可用`

本草案确认：
- 使用 `CurrentInitiative` 的**原值**（不将负数视为 0）。
- 负先机在"敌方部位行动子流程"中会被触发行动，因此参与 Sum 前通常已经归零。

## 12. 随机源

所有腾挪、随机插入、随机目标枚举统一使用 `BattleState` 持有的 `FRandomStream`。`BattleSession::Initialize` 时可选地接收一个 seed，用于自动化测试复现。

## 13. 未决项（不阻塞第一阶段编码）

这些条目在 Resolver / Executor 写到对应位置前必须确认，但不妨碍 S0~S5 开工。

- ~~朝光暮蝶左手区效果的最终文本~~ → 已确定为"本次不推进先机"（P3.3 实现）。
- 右手的"相邻右方伙伴代打"语义。
- ~~拂晓飞蛾回手时手牌已满的落地位置~~ → 临时决定：强行加入（`Phase2_Temporary_Decisions.md`）。
- 暮气触发点的正式时机。
- ~~中毒触发到底是"每张打出后" vs "每次行动批次后"~~ → 已正式化为"每张打出后 + 每次行动后"（`Battle_Rules.md §15`）。
- `Effect.Shuffle.ToRandomZone` 在手牌锚点缺失时的回退规则。
- ~~费用转移的运行时表达~~ → 已确定落到 `RuntimeCostModifier`（P3.3 实现）。

## 14. 第一阶段暂不处理

- 背包 (`Bag`) DataAsset 与运行时。
- 击倒事件奖励。
- 暮色引虫灯的任务系统。
- 非初始牌库。
- 手指系统。
- UI/表现相关的图标、音效字段。
- 完整状态公式。
- 卡牌耐久 (`Durability`) 的消耗分支。
