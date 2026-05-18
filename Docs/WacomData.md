# WacomData 模块文档

> 本文是 WacomData 模块的数据结构文档。加字段时先改本文，再改代码。

---

## §1 模块职责

WacomData 负责**静态定义和 DataAsset**。

**负责**：
- 卡牌定义（UCardDefinition）
- 敌人定义（UEnemyDefinition + UEnemyPartDefinition）
- 角色定义（UCharacterDefinition）
- 意图定义（FIntentDefinition）
- 效果结构（FCardEffect / FCardZoneHook / FCardPassive）
- 条件结构（FEffectCondition）

**不负责**：
- 运行时实例（属于 WacomBattle）
- 战斗逻辑
- UI

**依赖方向**：`WacomCore ← WacomData ← WacomBattle`

**资产位置**：
```
Content/Wacom/
├── Cards/BugGirl/DA_Card_*.uasset
├── Characters/DA_Character_BugGirl.uasset
└── Enemies/Snake/{DA_Enemy_Snake, DA_Part_Snake_Head/Body/Tail}.uasset
```

---

## §2 UCardDefinition 字段表

```cpp
UCLASS(BlueprintType)
class UCardDefinition : public UPrimaryDataAsset
{
    UPROPERTY(EditDefaultsOnly) FName              CardId;           // 唯一 ID
    UPROPERTY(EditDefaultsOnly) FText              DisplayName;      // 显示名
    UPROPERTY(EditDefaultsOnly) FText              Description;      // 显示文本
    UPROPERTY(EditDefaultsOnly) int32              BaseCost = 0;     // 基础 Cost
    UPROPERTY(EditDefaultsOnly) FGameplayTag       Rarity;           // Card.Rarity.*
    UPROPERTY(EditDefaultsOnly) FGameplayTagContainer Keywords;      // Card.Keyword.*
    UPROPERTY(EditDefaultsOnly) FCardPhysique      Physique;         // 身材（可选）
    UPROPERTY(EditDefaultsOnly) ECardTargetMode    TargetMode;       // None / SingleEnemyPart / AllEnemyParts / Self / HandCard
    UPROPERTY(EditDefaultsOnly) TArray<FCardEffect> Effects;         // 主效果
    UPROPERTY(EditDefaultsOnly) TArray<FCardEffect> PerfectReleaseEffects; // 完美释放效果
    UPROPERTY(EditDefaultsOnly) TArray<FCardZoneHook> ZoneHooks;     // 区域相关效果或修正
    UPROPERTY(EditDefaultsOnly) TArray<FCardPassive> Passives;       // 被动触发
};
```

### FCardPhysique

```cpp
USTRUCT(BlueprintType)
struct FCardPhysique
{
    UPROPERTY(EditDefaultsOnly) int32 MaxHpBonus = 0;     // 入战生命值上限加成（仅带 Companion 关键词的卡计入）
    UPROPERTY(EditDefaultsOnly) int32 Durability = 0;     // 0 = 无耐久限制（第一阶段不使用）
    UPROPERTY(EditDefaultsOnly) int32 Capacity = 0;       // GDD §11.2：容量字段
                                                          //   0：普通卡，不贡献存放空间
                                                          //   >0：容器卡，进入背包时贡献容量
};
```

---

## §3 UEnemyDefinition + UEnemyPartDefinition 字段表

### UEnemyDefinition

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

### UEnemyPartDefinition

```cpp
UCLASS(BlueprintType)
class UEnemyPartDefinition : public UPrimaryDataAsset
{
    UPROPERTY(EditDefaultsOnly) FName             PartId;          // 如 Snake.Head
    UPROPERTY(EditDefaultsOnly) FText             DisplayName;
    UPROPERTY(EditDefaultsOnly) int32             MaxHp = 0;
    UPROPERTY(EditDefaultsOnly) TArray<FIntentDefinition> IntentSequence; // 循环执行
    UPROPERTY(EditDefaultsOnly) int32             InitialIntentIndex = 0;
    UPROPERTY(EditDefaultsOnly) int32             ExperienceReward = 0;  // GDD §3.3 部位被破坏给予玩家的经验值
};
```

蛇默认配置：Head=3 / Body=2 / Tail=2 经验。

### FIntentDefinition

```cpp
USTRUCT(BlueprintType)
struct FIntentDefinition
{
    UPROPERTY(EditDefaultsOnly) FName                   IntentId;
    UPROPERTY(EditDefaultsOnly) FText                   DisplayName;
    UPROPERTY(EditDefaultsOnly) int32                   Initiative = 0;       // 本意图的先机值
    UPROPERTY(EditDefaultsOnly) int32                   ResistanceValue = 0;  // 抵抗比较用值
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

---

## §4 UCharacterDefinition 字段表

```cpp
UCLASS(BlueprintType)
class UCharacterDefinition : public UPrimaryDataAsset
{
    UPROPERTY(EditDefaultsOnly) FName CharacterId;
    UPROPERTY(EditDefaultsOnly) FText DisplayName;

    // HP 上限规则（GDD §3.1 / §3.4）：
    //   PlayerBaseMaxHp = FingerCount × HpPerFinger
    //   战内 MaxHp = PlayerBaseMaxHp + Σ(备战卡组中带 Companion 关键词的卡的 MaxHpBonus)
    UPROPERTY(EditDefaultsOnly) int32 FingerCount = 10;
    UPROPERTY(EditDefaultsOnly) int32 HpPerFinger = 2;
    int32 GetBasePlayerMaxHp() const;     // 返回 FingerCount * HpPerFinger

    UPROPERTY(EditDefaultsOnly) TObjectPtr<UCardDefinition> LeftHandCard;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UCardDefinition> RightHandCard;
    UPROPERTY(EditDefaultsOnly) TArray<TObjectPtr<UCardDefinition>> StarterDeck; // 不含左右手
};
```

虫妹 `FingerCount = 10`，`HpPerFinger = 2`，本体 HP = 20。
算上初始卡组带 Companion 的卡 `1+1+1+6+23 = 32`，战斗开始时最大 HP = `20 + 32 = 52`。
非 Companion 卡（武器/工具/中立）即便填了 `MaxHpBonus` 也不计入战内 MaxHp。

---

## §5 GameplayTag 清单

所有 tag 在 `WacomCore/Public/Tags/WacomGameplayTags.h` 中声明。严禁业务代码里用字符串拼 tag。

### Card.Keyword

| Tag | 代码名 | 说明 |
|---|---|---|
| `Card.Keyword.Swift` | `Card_Keyword_Swift` | 迅捷 |
| `Card.Keyword.Retain` | `Card_Keyword_Retain` | 保留 |
| `Card.Keyword.Combo` | `Card_Keyword_Combo` | 连击 |
| `Card.Keyword.Companion` | `Card_Keyword_Companion` | 伙伴 |
| `Card.Keyword.Weapon` | `Card_Keyword_Weapon` | 武器 |
| `Card.Keyword.Tool` | `Card_Keyword_Tool` | 工具 |
| `Card.Keyword.Hand` | `Card_Keyword_Hand` | 手（左右手专属）|
| `Card.Keyword.Exhaust` | `Card_Keyword_Exhaust` | 临时关键词：本卡打出后进消耗区 |
| `Card.Keyword.BagProvider` | `Card_Keyword_BagProvider` | 容器卡：背包能力提供者（GDD §11.1 / §11.2）|
| `Card.Keyword.DeleteProvider` | `Card_Keyword_DeleteProvider` | 删牌能力提供者（GDD §11.7）。Backpack 至少一张此关键词卡 → 删牌功能可用。第一阶段 UI 不读，接口就位 |

### Card.Rarity

| Tag | 代码名 | 说明 |
|---|---|---|
| `Card.Rarity.White` | `Card_Rarity_White` | 白色 |
| `Card.Rarity.Blue` | `Card_Rarity_Blue` | 蓝色 |
| `Card.Rarity.Intrinsic` | `Card_Rarity_Intrinsic` | 固有 |

### HandZone

| Tag | 代码名 | 说明 |
|---|---|---|
| `HandZone.Left` | `HandZone_Left` | 左手区 |
| `HandZone.Both` | `HandZone_Both` | 双手区 |
| `HandZone.Right` | `HandZone_Right` | 右手区 |

### Effect

| Tag | 代码名 | 说明 |
|---|---|---|
| `Effect.Damage` | `Effect_Damage` | 伤害 |
| `Effect.Heal` | `Effect_Heal` | 治疗（未实现）|
| `Effect.ApplyStatus.Poison` | `Effect_ApplyStatus_Poison` | 施加中毒 |
| `Effect.ApplyStatus.Slow` | `Effect_ApplyStatus_Slow` | 施加减速 |
| `Effect.ApplyStatus.Freeze` | `Effect_ApplyStatus_Freeze` | 施加冻结 |
| `Effect.ApplyStatus.Twilight` | `Effect_ApplyStatus_Twilight` | 施加暮气 |
| `Effect.Shuffle.Random` | `Effect_Shuffle_Random` | 随机腾挪 |
| `Effect.Shuffle.FromBothToOther` | `Effect_Shuffle_FromBothToOther` | 从双手区腾挪到其他区域 |
| `Effect.Shuffle.ToRandomZone` | `Effect_Shuffle_ToRandomZone` | 腾挪到随机区域 |
| `Effect.Card.AddCost` | `Effect_Card_AddCost` | 对目标卡 RuntimeCostModifier 增加 |
| `Effect.Card.ReduceCost` | `Effect_Card_ReduceCost` | 对目标卡 RuntimeCostModifier 减少 |

### Magnitude.Source

| Tag | 代码名 | 说明 |
|---|---|---|
| `Magnitude.Source.Literal` | `Magnitude_Source_Literal` | FinalMagnitude = Magnitude 字段 |
| `Magnitude.Source.RuntimeCost` | `Magnitude_Source_RuntimeCost` | FinalMagnitude = 本卡当前 RuntimeCost |

### Condition

| Tag | 代码名 | 说明 |
|---|---|---|
| `Condition.Self.InZone` | `Condition_Self_InZone` | 本卡当前在指定区域 |
| `Condition.Target.HasStatus` | `Condition_Target_HasStatus` | 目标部位含指定状态 |

### Status

| Tag | 代码名 | 说明 |
|---|---|---|
| `Status.Poison` | `Status_Poison` | 中毒 |
| `Status.Slow` | `Status_Slow` | 减速 |
| `Status.Freeze` | `Status_Freeze` | 冻结 |
| `Status.Twilight` | `Status_Twilight` | 暮气 |
| `Status.Stunned` | `Status_Stunned` | 晕厥 |
| `Status.Shield` | `Status_Shield` | 护盾 |

### Target

| Tag | 代码名 | 说明 |
|---|---|---|
| `Target.Self` | `Target_Self` | 自身（Player 或本卡，视 EffectType）|
| `Target.Player` | `Target_Player` | 玩家 |
| `Target.SingleEnemyPart` | `Target_SingleEnemyPart` | 单个敌方部位 |
| `Target.AllEnemyParts` | `Target_AllEnemyParts` | 所有敌方部位 |
| `Target.RandomHandCard` | `Target_RandomHandCard` | 手牌中随机一张 |
| `Target.ZoneHandCard` | `Target_ZoneHandCard` | 指定区域的手牌 |
| `Target.Adjacent.Right` | `Target_Adjacent_Right` | 相邻右方（未实现）|
| `Target.LastShuffledCard` | `Target_LastShuffledCard` | 最近一次 Shuffle 的被移动卡 |

### ZoneHook.Trigger

| Tag | 代码名 | 说明 |
|---|---|---|
| `ZoneHook.Trigger.OnPlay` | `ZoneHook_Trigger_OnPlay` | 本卡打出时 |
| `ZoneHook.Trigger.OnPerfectReleaseHit` | `ZoneHook_Trigger_OnPerfectReleaseHit` | 完美释放命中时 |

### Passive.Trigger

| Tag | 代码名 | 说明 |
|---|---|---|
| `Passive.Trigger.AfterPlayed` | `Passive_Trigger_AfterPlayed` | 本卡打出完成后 |
| `Passive.Trigger.OnCompanionCount` | `Passive_Trigger_OnCompanionCount` | 全局 Companion 计数达阈值 |
| `Passive.Trigger.OnTwilightTriggered` | `Passive_Trigger_OnTwilightTriggered` | 暮气施加成功时 |
| `Passive.Trigger.OnTurnStart` | `Passive_Trigger_OnTurnStart` | 玩家回合开始时（Handler 注册，调用点未接入）|
| `Passive.Trigger.OnTurnEnd` | `Passive_Trigger_OnTurnEnd` | 玩家回合结束时（Handler 注册，调用点未接入）|
| `Passive.Trigger.OnDraw` | `Passive_Trigger_OnDraw` | 本卡被抽到手牌时（Handler 注册，调用点未接入）|
| `Passive.Trigger.OnDiscard` | `Passive_Trigger_OnDiscard` | 本卡被弃掉时（Handler 注册，调用点未接入）|

### CardLocation

`Effect.Draw` 通过 `MetaTag` 指定源区域（默认 `CardLocation.Draw`）：

| Tag | 代码名 | 说明 |
|---|---|---|
| `CardLocation.Draw` | `CardLocation_Draw` | 抽牌堆 |
| `CardLocation.Discard` | `CardLocation_Discard` | 弃牌堆 |
| `CardLocation.Exhaust` | `CardLocation_Exhaust` | 消耗区 |
| `CardLocation.Hand` | `CardLocation_Hand` | 手牌 |

### SkillSlot

Run 层角色技能池的占位 tag。等技能列表正式定义后按角色添加 `SkillSlot.*`。

| Tag | 代码名 | 说明 |
|---|---|---|
| `SkillSlot.Placeholder` | `SkillSlot_Placeholder` | 占位（满 10 经验入账一个；不挂效果）|

### Card.CapacityEffect

容器卡的容量效果分类（GDD §11.2）。`FCardPhysique::CapacityEffect` 字段的取值，
空 tag = A 类容器卡（无容量效果），有效 tag = B 类容器卡（特殊存放区按效果应用）。

| Tag | 代码名 | 说明 |
|---|---|---|
| `Card.CapacityEffect.Placeholder` | `Card_CapacityEffect_Placeholder` | 占位 tag，早期 B 类骨架使用（已不再分配给具体卡）。具体卡定义后逐步替换为下列具体效果 tag。 |
| `Card.CapacityEffect.WeaponDamagePlus3` | `Card_CapacityEffect_WeaponDamagePlus3` | 蛛茧绒囊（GDD §11.2 / Stage 4.5.2）。SpecialZone 内 `bBattleEnabledInSpecialZone == true` 且带 `Card.Keyword.Weapon` 关键词的入战 instance，其 `Effect.Damage` 最终结算 +3。 |

---

## §6 效果字段使用表

每个 EffectType 对 `FCardEffect` 字段的使用方式不同。配卡时对照本表。"-" 表示该字段对此 EffectType 无效。

| EffectType | Magnitude 语义 | Target（典型值）| TargetZone | Duration | MagnitudeSource | 备注 |
|---|---|---|---|---|---|---|
| `Effect.Damage` | 伤害值 | SingleEnemyPart / AllEnemyParts / Player | - | - | Literal / RuntimeCost | 部位 HP 归零立即破坏 |
| `Effect.Heal` | 治疗量 | Self(→Player) / SingleEnemyPart | - | - | Literal | 未实现 |
| `Effect.ApplyStatus.Poison` | 层数 | Player / SingleEnemyPart / AllEnemyParts | - | - | Literal / RuntimeCost | 层数模型，不用 Duration |
| `Effect.ApplyStatus.Slow` | 层数 | 同上 | - | - | Literal | 第一阶段只记录 |
| `Effect.ApplyStatus.Freeze` | 层数 | SingleEnemyPart | - | 回合数(0=层数模型) | Literal | 按层数消耗实现 |
| `Effect.ApplyStatus.Twilight` | 层数 | SingleEnemyPart | - | - | Literal | 第一阶段只记录 |
| `Status.Shield` | 护盾值 | Player / Self(部位) | - | - | Literal | 直接加到 Shield 字段 |
| `Effect.Shuffle.Random` | - | RandomHandCard | - | - | - | 从手牌随机选一张腾挪 |
| `Effect.Shuffle.FromBothToOther` | - | ZoneHandCard | HandZone.Both | - | - | 从双手区挑一张腾挪到左/右 |
| `Effect.Shuffle.ToRandomZone` | - | Self(本卡) | - | - | - | 把本卡腾挪到随机区域 |
| `Effect.Card.AddCost` | Modifier 增量 | Self(本卡) / LastShuffledCard | - | - | Literal | 修改 RuntimeCostModifier |
| `Effect.Card.ReduceCost` | Modifier 减量 | 同上 | - | - | Literal | 下限由 ComputeRuntimeCost clamp 到 0 |

### Target 字段速查表

| Target | 解析为 | TargetInstanceId 来源 | 是否需要 TargetZone |
|---|---|---|---|
| `Target.Self` | Player 或本卡（视 EffectType）| 本卡 InstanceId 或 Invalid | 否 |
| `Target.Player` | Player | Invalid | 否 |
| `Target.SingleEnemyPart` | EnemyPart | 调用方选中的部位 | 否 |
| `Target.AllEnemyParts` | EnemyPart（循环展开）| 自动遍历存活部位 | 否 |
| `Target.RandomHandCard` | HandCard | HandZoneService 自选 | 否 |
| `Target.ZoneHandCard` | HandCard | 按 TargetZone 过滤后自选 | **是** |
| `Target.LastShuffledCard` | HandCard | `EffectContext::LastShuffledCardId` | 否 |
| `Target.Adjacent.Right` | EnemyPart | 未实现 | 否 |

### Target.Self 的 EffectType 消歧

- `Effect.Shuffle.ToRandomZone` / `Effect.Card.AddCost` / `Effect.Card.ReduceCost` → 指向本卡（HandCard）
- 其他（Damage / Heal / ApplyStatus）→ 指向玩家（Player）

---

## §7 FEffectCondition / FCardPassive 结构

### FEffectCondition

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

| ConditionType | 语义 | ParamTag | ParamInt |
|---|---|---|---|
| Invalid（未设置）| 永真 | - | - |
| `Condition.Self.InZone` | 本卡当前在指定区域 | `HandZone.*` | - |
| `Condition.Target.HasStatus` | 目标部位含指定状态 | `Status.*` | - |

`bNegate = true` 把结果取反。例如"自卡不在左手区" = `InZone(Left) + bNegate=true`。

### FCardZoneHook

```cpp
USTRUCT(BlueprintType)
struct FCardZoneHook
{
    UPROPERTY(EditDefaultsOnly) FGameplayTag Zone;         // HandZone.Left / Both / Right
    UPROPERTY(EditDefaultsOnly) FGameplayTag Trigger;      // ZoneHook.Trigger.*
    UPROPERTY(EditDefaultsOnly) TArray<FCardEffect> ExtraEffects;
};
```

### FCardPassive

```cpp
USTRUCT(BlueprintType)
struct FCardPassive
{
    UPROPERTY(EditDefaultsOnly) FGameplayTag     Trigger;            // Passive.Trigger.*
    UPROPERTY(EditDefaultsOnly) TArray<FCardEffect> Effects;          // 触发后执行
    UPROPERTY(EditDefaultsOnly) FEffectCondition Condition;          // 触发门控，未设置则永真
    UPROPERTY(EditDefaultsOnly) int32            TriggerThreshold = 0;// 仅计数类 trigger 使用
};
```

| Trigger | 触发时机 | 使用 TriggerThreshold? | 典型卡 |
|---|---|---|---|
| `Passive.Trigger.AfterPlayed` | 本卡打出完成后 | 否 | 烁光蝶（自腾挪）|
| `Passive.Trigger.OnCompanionCount` | 全局 Companion 计数达阈值 | 是 | 拂晓飞蛾（回手，阈值 3）|
| `Passive.Trigger.OnTwilightTriggered` | 暮气施加成功时 | 否 | 暮蛉（占位）|

**TriggerThreshold** 只用于计数类 trigger。达到阈值后触发并清零计数。其他 trigger 不读此字段。
