---
type: data-contract
scope: wacom-data
status: active
updated: 2026-05-31
tags:
  - wacom/data
  - wacom/dataasset
  - wacom/gameplay-tags
  - wacom/validation
---

# WacomData 模块文档

> [!info] 本文职责
> 本文是 WacomData 模块的静态数据契约入口。加 DataAsset 字段、GameplayTag 或内容生成规则时先改本文，再改代码。

> [!note] 相关入口
> 运行时规则见 [[WacomBattle]] / [[WacomRun]]，UI 展示见 [[WacomUI]]，编辑器校验实现位于 `WacomEditor`。

## §1 模块职责

WacomData 负责**静态定义和 DataAsset**。

**负责**：
- 卡牌定义（UCardDefinition）
- 敌人定义（UEnemyDefinition + UEnemyPartDefinition）
- 角色定义（UCharacterDefinition）
- 商店定义（UShopDefinition）
- 探索事件定义（UWacomRunEventDefinition）
- 意图定义（FIntentDefinition）
- 效果结构（FCardEffect / FCardZoneHook / FCardPassive）
- 条件结构（FEffectCondition）

**不负责**：
- 运行时实例（属于 WacomBattle）
- 战斗逻辑
- Run 状态、库存、事件进度和存档
- UI 展示、WBP 绑定和 Presentation ViewData
- 编辑器 Validator 注册与 Commandlet 执行

**依赖方向**：`WacomCore ← WacomData ← WacomBattle`。`WacomData` 只能依赖 `WacomCore`，不能反向依赖 `WacomBattle / WacomRun / WacomApp / WacomEditor`。

**资产位置**：
```
Content/Wacom/
└── Data/
    ├── Cards/BugGirl/DA_Card_*.uasset
    ├── Cards/Rewards/DA_Card_PoisonFang.uasset
    ├── Characters/DA_Character_BugGirl.uasset
    ├── Enemies/Snake/{DA_Enemy_Snake, DA_Part_Snake_Head/Body/Tail}.uasset
    ├── Events/{DA_Event_DebugSnakeGift, DA_Event_DebugFlagReward}.uasset
    └── Shops/DA_Shop_DebugSnake.uasset
```

完整内容目录口径见 `Docs/Content_Organization.md`。

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
    UPROPERTY(EditDefaultsOnly) FWacomHandCardTargetFilter HandCardTargetFilter; // HandCard 目标基础筛选
    UPROPERTY(EditDefaultsOnly) TArray<FCardEffect> Effects;         // 主效果
    UPROPERTY(EditDefaultsOnly) TArray<FCardEffect> PerfectReleaseEffects; // 完美释放效果
    UPROPERTY(EditDefaultsOnly) TArray<FCardZoneHook> ZoneHooks;     // 区域相关效果或修正
    UPROPERTY(EditDefaultsOnly) TArray<FCardPassive> Passives;       // 被动触发
};
```

### FWacomHandCardTargetFilter

`HandCardTargetFilter` 只影响 `TargetMode=HandCard` 的主动打牌目标资格，UI 不直接读取它，而是通过 Battle 的 `ValidateTargetWithCard()` / drop resolver 消费结果。

```cpp
USTRUCT(BlueprintType)
struct FWacomHandCardTargetFilter
{
    UPROPERTY(EditDefaultsOnly) bool bUseExplicitHandCardTargetFilter = false;
    UPROPERTY(EditDefaultsOnly) bool bAllowNormalHandCards = true;
    UPROPERTY(EditDefaultsOnly) bool bAllowHandAnchors = true;
    UPROPERTY(EditDefaultsOnly) FGameplayTagContainer RequiredTargetKeywords;
    UPROPERTY(EditDefaultsOnly) FGameplayTagContainer BlockedTargetKeywords;
};
```

- `bUseExplicitHandCardTargetFilter=true`：卡牌直接使用这两个允许开关。
- `bUseExplicitHandCardTargetFilter=false`：保持旧资产兼容推断。普通 `TargetMode=HandCard` 默认允许普通手牌和左右手锚点；包含 `Effect.Card.DiscardSelected / Effect.Card.ExhaustSelected + Target.SelectedHandCard` 的卡默认只允许普通手牌。
- `RequiredTargetKeywords`：目标有效关键词必须全部拥有；空集合表示不要求。
- `BlockedTargetKeywords`：目标有效关键词命中任意一个即拒绝；空集合表示不阻止。
- 目标有效关键词 = `UCardDefinition::Keywords` + 战斗内 `TemporaryKeywords`。左右手锚点如果被允许，也同样参与 keyword 条件。
- self target 永远禁止，不提供配置项。
- 现阶段只筛普通手牌 / 左右手锚点 / keyword；费用、卡牌类型、区域、伙伴 / 食物专用属性等条件后续再扩展。

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
    UPROPERTY(EditDefaultsOnly) FGameplayTag CapacityEffect; // 空=A类容器；有效=B类容器，开 SpecialZone
};
```

`CapacityEffect` 使用 `Card.CapacityEffect.*` 命名空间。当前数据语义：
- 空 tag：A 类容器，容量计入通量容量和备战容量；物理位于 Backpack 时作为通量内容卡显示。
- 有效 tag：B 类容器，容量不计入通量容量，但计入备战容量；每张 B 类主卡独立展开一个 SpecialZone。
- `Card.CapacityEffect.WeaponDamagePlus3`：B 类容器效果。SpecialZone 内已选择入战且带 `Card.Keyword.Weapon` 的卡，伤害结算 +3。

容器运行时规则见 [WacomRun.md](./WacomRun.md)，入战后的容量效果结算见 [WacomBattle.md](./WacomBattle.md)。

---

<a id="wacomdata-enemy-part"></a>
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
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UCardDefinition> KnockdownRewardCard = nullptr; // 击倒后 Aid/Destroy 获得的奖励卡
};
```

蛇默认部位配置：

| 部位 | MaxHp | ExperienceReward | KnockdownRewardCard |
|---|---:|---:|---|
| `Snake.Head` | 16 | 3 | `DA_Card_PoisonFang` |
| `Snake.Body` | 22 | 2 | `DA_Card_PoisonFang` |
| `Snake.Tail` | 10 | 2 | `DA_Card_PoisonFang` |

`KnockdownRewardCard` 是"万物成卡"第一版部位奖励配置：
- 字段含义：该部位被击倒后，Aid / Destroy 共用的奖励卡定义。
- 运行时行为：Battle 在选择 Aid / Destroy 时创建战斗内 runtime card 并写入战后包，见 [WacomBattle §12](./WacomBattle.md#wacombattle-result)。
- 战后入包：Run 在 Victory（含撤离）时结算 `GainedCards[]`，见 [WacomRun §8](./WacomRun.md#wacomrun-battle-settlement)。
- 不同分支不同奖励表留到后续扩展。

当前蛇敌人内容：
- `DA_Card_PoisonFang`（毒牙）是第一张击倒奖励卡样例，临时效果为 0 费、对单个敌方部位施加 1 中毒。
- 蛇头、蛇身、蛇尾当前都配置同一张毒牙，便于验证完整奖励链路；后续可替换为各部位专属奖励。

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

当前生成的虫妹角色内容：

| 字段 | 当前值 |
|---|---|
| `CharacterId` | `BugGirl` |
| `LeftHandCard` | `DA_Card_LeftHand` |
| `RightHandCard` | `DA_Card_RightHand` |
| `StarterDeck` | `ZhaoguangMudie / FuxiaoFeie / ChifuGongyi / ShuoguangDie / Muling / BugGirlBag / ZhujianRongnang / MuseiYinchongdeng` |

`StarterDeck` 中前 5 张是默认参战伙伴卡；`BugGirlBag` 和 `ZhujianRongnang` 默认进入通量 / 特殊存放相关背包区；`MuseiYinchongdeng` 是原型特例，默认固定在备战区，但它仍作为 A 类容器卡贡献通量和备战容量。

---

## §5 UShopDefinition 字段表

```cpp
UCLASS(BlueprintType)
class UShopDefinition : public UPrimaryDataAsset
{
    UPROPERTY(EditDefaultsOnly) FName ShopId;          // 内容 ID，不是运行时库存 key
    UPROPERTY(EditDefaultsOnly) FText DisplayName;     // 商店显示名
    UPROPERTY(EditDefaultsOnly) TArray<FShopOfferDefinition> Offers;
};

USTRUCT(BlueprintType)
struct FShopOfferDefinition
{
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UCardDefinition> CardDefinition;
    UPROPERTY(EditDefaultsOnly) int32 Price = 0;       // 金币价格，0 表示免费
};
```

`UShopDefinition` 只定义静态商品内容，不保存购买状态。当前 Run 内的库存和已购买状态仍由 `AWacomShopTriggerActor.PersistentId` 作为 key 存在 `URunSession` 中。

当前生成内容：
- `DA_Shop_DebugSnake`（蛇巢调试商店）：卖 `毒牙`、`赤腹工蚁`、`朝光暮蝶`、`虫妹的小布袋`。

编辑器侧已接入 `UWacomShopDefinitionValidator` 内容防呆。校验重点：
- `ShopId` 不能为空。
- `Offers` 不能为空；如果需要剧情空商店，后续应加显式字段表达，不用空列表伪装。
- 每个 Offer 必须配置 `CardDefinition`。
- `Price` 不能为负数；`0` 表示免费商品，合法。
- 第一版不禁止同一张卡重复出现在多个 Offer 中，因为后续可能用于多份库存或不同价格变体。

自动化测试 `Wacom.Data.Shop.DebugSnakeAssetValidation` 会验证 `DA_Shop_DebugSnake` 能通过同一套校验规则，避免商店内容生成漂移。

---

## §6 UWacomRunEventDefinition 字段表

`UWacomRunEventDefinition` 是轻量探索事件图 DataAsset：

```cpp
UCLASS(BlueprintType)
class UWacomRunEventDefinition : public UPrimaryDataAsset
{
    UPROPERTY(EditDefaultsOnly) FName EventId;
    UPROPERTY(EditDefaultsOnly) FText DisplayName;
    UPROPERTY(EditDefaultsOnly) FName StartNodeId;
    UPROPERTY(EditDefaultsOnly) TArray<FWacomRunEventNodeDefinition> Nodes;
};
```

Node 包含 `NodeId / TitleText / BodyText / Choices`。Choice 包含 `ChoiceId / LabelText / Conditions / CardPayment / Effects / NextNodeId / bCloseEventAfterResolve / bMarkEventCompleted`。

条件字段：

| 字段 | 用途 |
|---|---|
| `Type` | `None / MinGold / MinNodeCount / MaxPressure / HasCard / MissingCard / EventCompleted / EventNotCompleted / RunFlagSet / RunFlagNotSet`；`None` 会被忽略 |
| `Value` | 金币、节点、压力阈值等数值条件 |
| `PressureType` | `MaxPressure` 使用，稳定 ID：`Hunger / Wound / Fatigue / Burden / Decay / Misdeed / Bloodlust / Disability` |
| `CardDefinition` | `HasCard / MissingCard` 使用 |
| `TargetPersistentId` | `EventCompleted / EventNotCompleted` 使用，填写场景事件 Actor 的 `PersistentId` |
| `FlagId` | `RunFlagSet / RunFlagNotSet` 使用；当前只保存在本次 Run 内存态，不写入 SaveGame |

效果字段：

| 字段 | 用途 |
|---|---|
| `Type` | `None / GainCard / RemoveCard / AddGold / AddPressure / ConsumeNode / MarkEventCompleted / SetRunFlag / ClearRunFlag`；`None` 会被忽略 |
| `CardDefinition` | `GainCard / RemoveCard` 使用 |
| `Value` | 金币变化、压力变化、消耗节点数 |
| `PressureType` | `AddPressure` 使用 |
| `TargetPersistentId` | `MarkEventCompleted` 使用，填写要标记完成的场景事件 Actor `PersistentId` |
| `FlagId` | `SetRunFlag / ClearRunFlag` 使用；当前只保存在本次 Run 内存态，不写入 SaveGame |

压力类型在 DataAsset 中使用稳定 `FName`，由 `WacomRun` 执行时转换为运行时枚举，避免 `WacomData` 反向依赖 `WacomRun`。

卡牌条件/效果使用 `CardDefinition` 字段。`HasCard / MissingCard` 会检查玩家全部拥有区：通量、备战、特殊存放区和负重区。`RemoveCard` 表示永久交出/移除一张拥有的卡，不发金币，并遵守 Run 层现有安全限制（固有卡、最后一张容量来源卡不可移除）。

卡牌支付字段用于“玩家拖入某张已持有卡后才能提交该选项”：

| 字段 | 用途 |
|---|---|
| `bRequiresOwnedCardPayment` | 开启后该选项不能普通点击提交，必须通过 first-person menu lease 卡牌拖入对应 Zone |
| `PaymentZoneId` | 菜单 Zone 目标 ID；为空时运行时使用 `RunEvent.Pay.{ChoiceId}`；同一节点内必须唯一 |
| `AllowedCardDefinitions` | 允许支付的卡牌定义资产 |
| `AllowedCardIds` | 允许支付的 `UCardDefinition::CardId` |
| `RequiredKeywords` | 支付卡必须全部拥有的 `Card.Keyword.*` |
| `BlockedKeywords` | 支付卡命中任意一个即拒绝 |

`AllowedCardDefinitions` 与 `AllowedCardIds` 是 OR 关系；显式 instance 候选由运行时 snapshot / menu lease provider 生成，不写在 DataAsset 中。空筛选非法，本轮不支持“交任意卡”。支付选项不能同时配置 `RemoveCard` effect，避免拖入精确 instance 后又按 Definition 再移除一张卡。

支付选项制作 checklist：

- `ChoiceId` 必填；`PaymentZoneId` 为空时会解析为 `RunEvent.Pay.{ChoiceId}`，缺少 `ChoiceId` 就无法生成默认 Zone。
- 至少配置一种筛选：`AllowedCardDefinitions`、`AllowedCardIds`、`RequiredKeywords` 或 `BlockedKeywords`。
- 同一 Node 内解析后的 `PaymentZoneId` 必须唯一。
- 支付选项不要配置 `RemoveCard` effect；拖卡支付已经移除玩家拖入的精确 instance。
- 支付后剧情结果用 `NextNodeId / bCloseEventAfterResolve / bMarkEventCompleted / Effects` 表达。

事件状态条件/效果使用 `TargetPersistentId` 字段，填写场景事件 Actor 的 `PersistentId`，不是 `EventDefinition.EventId`。`EventCompleted / EventNotCompleted` 读取对应状态；`MarkEventCompleted` 标记指定 `PersistentId` 完成。当前选项自身仍可继续使用 `bMarkEventCompleted` 标记当前事件完成。

RunFlag 条件/效果使用 `FlagId` 字段，适合表达当前 Run 内的轻量事件记忆，例如“已经看过某个分支”“某个事件已解锁额外选项”。`RunFlagSet` 要求 `FlagId` 已存在，`RunFlagNotSet` 要求不存在；`SetRunFlag / ClearRunFlag` 会在 RunEvent 事务内修改 `FRunState::RunFlags`。RunFlag 不是 GameplayTag，不做数值或计数，本轮不写入 SaveGame。

金币支付 / 金币门槛不新增专用字段或效果类型。制作“需要金币后获得奖励”时，使用 `MinGold=N` 条件 + `AddGold=-N` 效果 + 奖励 effects。`AddGold` 运行时会把金币下限 clamp 到 0；为了让 UI preview 和真实扣费清楚，建议负数 `AddGold` 总额与最大 `MinGold` 条件值一致。

编辑器侧已接入 `UWacomRunEventDefinitionValidator` 内容防呆。校验重点：
- `EventId / StartNodeId` 不能为空，`StartNodeId` 必须能找到节点。
- `NodeId` 在事件内唯一，`ChoiceId` 在同一节点内唯一，`NextNodeId` 必须能找到目标节点。
- `HasCard / MissingCard / GainCard / RemoveCard` 必须配置 `CardDefinition`。
- 卡牌支付选项必须有非空支付筛选，同一节点内解析后的 `PaymentZoneId` 不能重复，且不能同时配置 `RemoveCard` effect。
- V0-AT 后支付相关错误会明确带出 `NodeId / ChoiceId / PaymentZoneId / NextNodeId`，用于 Validate Assets 时快速定位配置项；V0-AZ 后条件 / 效果错误还会带 `ConditionIndex / EffectIndex`。
- `EventCompleted / EventNotCompleted / MarkEventCompleted` 必须配置 `TargetPersistentId`。
- `RunFlagSet / RunFlagNotSet / SetRunFlag / ClearRunFlag` 必须配置 `FlagId`；错误会带出 `NodeId / ChoiceId / ConditionIndex / EffectIndex`。
- `MaxPressure / AddPressure` 必须配置有效压力 ID，`ConsumeNode` 不能为负数。
- `AddGold / AddPressure / ConsumeNode` 的 `Value=0` 只给 warning，不阻断资产；`NextNodeId` 与关闭 / 完成事件同时配置也只给 warning，提示事件结束预览会优先。
- 负数 `AddGold` 但没有 `MinGold` 条件只给 warning，提示实际结算会 clamp 到 0；负数 `AddGold` 总额和最大 `MinGold` 条件值不一致也只给 warning，提示门槛和扣费可能不一致。

调试资产：
- `DA_Event_DebugSnakeGift`：蛇巢遗物事件，可获得 `毒牙`、通过 `CardPayment` 拖入已有 `毒牙`、消耗节点、调整金币/劣迹压力。
- `HandOverFang` 是标准单卡支付样例：`AllowedCardDefinitions=DA_Card_PoisonFang`、`PaymentZoneId=RunEvent.Pay.Fang`、`NextNodeId=End`，不配置 `RemoveCard` effect。
- `DA_Event_DebugFlagReward`：标记奖励样例，专门演示 RunFlag 与 `MinGold + AddGold(-N)` 金币门槛奖励组合。`InspectMark` 设置 `DebugFlagReward.Inspected`，`DebugGrantGold` 用于 PIE 自助获得 3 金币并设置 `DebugFlagReward.GoldGranted`，`ClaimGoldReward` 要求已调查、未领取且金币不少于 3，提交后扣 3 金币、获得 `毒牙` 并设置 `DebugFlagReward.RewardClaimed`，`ResetFlags` 清除三个 Debug flag 并回到 `Start`。
- V0-BC 后，关卡中放置 `AWacomRunEventTriggerActor` 后可在 Details 点击 `ConfigureDebugSnakeGiftSample` 或 `ConfigureDebugFlagRewardSample` 自动绑定上述两个样例；不需要手动复制资产路径。
- 金币、压力和节点数值均为原型调试值，不代表正式平衡。
- 自动化测试 `Wacom.Data.RunEvent.DebugSnakeGiftAsset` 会验证该资产的节点、选项、条件、效果和 `毒牙` 引用，避免内容生成漂移。
- 自动化测试 `Wacom.Data.RunEvent.DebugFlagRewardAsset` 会验证 FlagReward 样例的节点、选项、FlagId、`MinGold(3)`、`AddGold(-3)`、`GainCard(PoisonFang)`、重复领取阻止和 reset flags 配置。

---

<a id="wacomdata-content-validation"></a>
## §7 内容生成与校验

### Commandlet

`UWacomRegenerateContentCommandlet` 位于 `Source/WacomEditor/Private/Commandlets/WacomRegenerateContentCommandlet.cpp`，用于重建当前原型 DataAsset。它依次调用：

| Builder | 产物 |
|---|---|
| `BuildSnakeContent()` | 蛇敌人、三部位、奖励卡 `DA_Card_PoisonFang` |
| `BuildBugGirlContent()` | 虫妹角色、左右手、5 张伙伴初始牌、3 张容器 / 功能卡、卡对卡测试卡 |
| `BuildShopContent()` | `DA_Shop_DebugSnake` |
| `BuildRunEventContent()` | `DA_Event_DebugSnakeGift`、`DA_Event_DebugFlagReward` |

命令：

```powershell
& 'E:\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -run=WacomRegenerateContent -NoSplash -Unattended
```

Commandlet 是内容生成辅助，不是运行时规则入口。改 Builder 后应运行它落盘资产，并跑对应 `Wacom.Data.*` 资产验证测试。

### 当前生成内容

| 资产 | 内容 |
|---|---|
| `/Game/Wacom/Data/Characters/DA_Character_BugGirl` | 虫妹角色；左右手 + StarterDeck（含原型测试卡） |
| `/Game/Wacom/Data/Cards/BugGirl/DA_Card_LeftHand` | 固有左手牌 |
| `/Game/Wacom/Data/Cards/BugGirl/DA_Card_RightHand` | 固有右手牌 |
| `/Game/Wacom/Data/Cards/BugGirl/DA_Card_ZhaoguangMudie` | 朝光暮蝶，0 费伙伴，随机腾挪并按 RuntimeCost 施加中毒 |
| `/Game/Wacom/Data/Cards/BugGirl/DA_Card_FuxiaoFeie` | 拂晓飞蛾，0 费伙伴，施加 1 减速，3 次伙伴计数后回手 |
| `/Game/Wacom/Data/Cards/BugGirl/DA_Card_ChifuGongyi` | 赤腹工蚁，0 费伙伴，保留，腾挪双手区卡 |
| `/Game/Wacom/Data/Cards/BugGirl/DA_Card_ShuoguangDie` | 烁光蝶，1 费伙伴武器，连击，造成 7 伤害，打出后自腾挪 |
| `/Game/Wacom/Data/Cards/BugGirl/DA_Card_Muling` | 暮蛉，5 费伙伴，冻结 1 回合，暮气触发被动占位 |
| `/Game/Wacom/Data/Cards/BugGirl/DA_Card_BugGirlBag` | 虫妹的小布袋，A 类容器，`Capacity=12`，带历史兼容 `BagProvider` |
| `/Game/Wacom/Data/Cards/BugGirl/DA_Card_ZhujianRongnang` | 蛛茧绒囊，B 类容器，`Capacity=3`，`WeaponDamagePlus3` |
| `/Game/Wacom/Data/Cards/BugGirl/DA_Card_MuseiYinchongdeng` | 暮色引虫灯，A 类容器，`Capacity=3`，带 `DeleteProvider`，默认固定在备战区 |
| `/Game/Wacom/Data/Cards/BugGirl/DA_Card_Test_AddCostToSelectedHand` | `Test.AddCostToSelectedHand`，拖到另一张手牌上使目标费用 +2 |
| `/Game/Wacom/Data/Cards/BugGirl/DA_Card_Test_ReduceCostToSelectedHand` | `Test.ReduceCostToSelectedHand`，拖到另一张手牌上使目标费用 -1 |
| `/Game/Wacom/Data/Cards/BugGirl/DA_Card_Test_DiscardSelectedHandCard` | `Test.DiscardSelectedHandCard`，拖到另一张普通手牌上使目标进入弃牌堆 |
| `/Game/Wacom/Data/Cards/BugGirl/DA_Card_Test_ExhaustSelectedHandCard` | `Test.ExhaustSelectedHandCard`，拖到另一张普通手牌上使目标进入消耗区 |
| `/Game/Wacom/Data/Cards/BugGirl/DA_Card_Test_TargetCost3` | `Test.TargetCost3`，卡对卡测试目标，基础费用 3 |
| `/Game/Wacom/Data/Cards/Rewards/DA_Card_PoisonFang` | `PoisonFang`，0 费白卡，对单个敌方部位施加 1 中毒 |
| `/Game/Wacom/Data/Enemies/Snake/DA_Enemy_Snake` | 蛇敌人，包含 Head / Body / Tail 三个部位 |
| `/Game/Wacom/Data/Enemies/Snake/DA_Part_Snake_Head` | `Snake.Head`，HP 16，Exp 3，奖励毒牙 |
| `/Game/Wacom/Data/Enemies/Snake/DA_Part_Snake_Body` | `Snake.Body`，HP 22，Exp 2，奖励毒牙 |
| `/Game/Wacom/Data/Enemies/Snake/DA_Part_Snake_Tail` | `Snake.Tail`，HP 10，Exp 2，奖励毒牙 |
| `/Game/Wacom/Data/Shops/DA_Shop_DebugSnake` | 调试商店，固定卖毒牙、赤腹工蚁、朝光暮蝶、小布袋 |
| `/Game/Wacom/Data/Events/DA_Event_DebugSnakeGift` | 蛇巢遗物调试事件，包含获得毒牙、通过卡牌支付交出毒牙、金币/压力/节点效果 |
| `/Game/Wacom/Data/Events/DA_Event_DebugFlagReward` | 标记奖励调试事件，包含 RunFlag 解锁、PIE 自助给金币、`MinGold(3) + AddGold(-3)` 领取毒牙和 reset flags |

### Data Validation

编辑器侧 Validator 位于 `WacomEditor`，通过 `FWacomEditorModule::StartupModule()` 注册到 `UEditorValidatorSubsystem`。

| Validator | 校验对象 | 共享校验函数 |
|---|---|---|
| `UWacomCardDefinitionValidator` | `UCardDefinition` | `FWacomCardDefinitionValidation::Validate()` |
| `UWacomEnemyPartDefinitionValidator` | `UEnemyPartDefinition` | `FWacomEnemyPartDefinitionValidation::Validate()` |
| `UWacomEnemyDefinitionValidator` | `UEnemyDefinition` | `FWacomEnemyDefinitionValidation::Validate()` |
| `UWacomCharacterDefinitionValidator` | `UCharacterDefinition` | `FWacomCharacterDefinitionValidation::Validate()` |
| `UWacomShopDefinitionValidator` | `UShopDefinition` | `FWacomShopDefinitionValidation::Validate()` |
| `UWacomRunEventDefinitionValidator` | `UWacomRunEventDefinition` | `FWacomRunEventDefinitionValidation::Validate()` |

这些 Validator 用于编辑器 Validate Assets 和自动化测试。不要把 Validator 放进 `WacomData`，否则运行时模块会反向依赖编辑器能力。

当前 `UCardDefinition`、`UEnemyPartDefinition`、`UEnemyDefinition`、`UCharacterDefinition`、`UShopDefinition` 和 `UWacomRunEventDefinition` 六类 DataAsset 已接入 Editor Validator。

Card / EnemyPart / Enemy / Character Validator 只做结构防呆，例如必填 ID、基础数值非负或大于 0、必填数组非空、引用非空、GameplayTag 命名空间有效、数组索引有效；Character 还校验 `StarterDeck` 不包含左右手卡。它们不校验文案质量、数值平衡、流派构筑、固定卡组数量、固定部位数量、跨资产唯一性或生成资产路径。

Shop Validator 只校验 `ShopId` 非空、`Offers` 非空、Offer 卡牌非空、价格非负；不校验 `DisplayName`、重复商品、价格平衡、商品池规则或生成资产路径。

RunEvent Validator 只校验事件图结构、必填引用和压力 ID：`EventId / StartNodeId`、Node / Choice ID、`NextNodeId`、卡牌条件 / 效果引用、卡牌支付筛选和 ZoneId、事件状态目标、RunFlag `FlagId`、`ConsumeNode >= 0`。金币门槛 / 扣费组合只做 authoring warning，不阻断资产。不校验标题 / 正文 / 按钮文案非空、节点可达性、选项是否至少一个、金币 / 压力数值平衡或剧情合法性。

---

## §8 GameplayTag 清单

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
| `Card.Keyword.BagProvider` | `Card_Keyword_BagProvider` | 历史 / 兼容关键词。当前背包 UI 可用性、容量和最后容量来源保护都以玩家持有区是否存在 `Physique.Capacity > 0` 的容器卡为真相；`DA_Card_BugGirlBag` 仍带此关键词作为内容标记 |
| `Card.Keyword.DeleteProvider` | `Card_Keyword_DeleteProvider` | 删牌能力提供者。`URunSession::IsDeleteFunctionAvailable()` 会读取玩家持有区是否存在该 tag；第一阶段背包 UI 和 `DeleteCardForGold` 不强制读取该判定 |

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

### Interaction.Target

| Tag | 代码名 | 说明 |
|---|---|---|
| `Interaction.Target.Battle.EnemyPart` | `Interaction_Target_Battle_EnemyPart` | 场景 World target 表示当前战斗敌方部位，由 `UWacomBattleEnemyPartWorldTargetBridgeComponent` 写入通用 `UWacomInteractionTargetComponent` |
| `Interaction.Target.Run.Object` | `Interaction_Target_Run_Object` | Run / 探索中的场景可交互对象，后续拖拽和鼠标交互 resolver 使用 |

### Effect

| Tag | 代码名 | 说明 |
|---|---|---|
| `Effect.Damage` | `Effect_Damage` | 伤害 |
| `Effect.Heal` | `Effect_Heal` | 治疗玩家 HP，并移除治疗量 10% 的中毒层数（向下取整）|
| `Effect.ApplyStatus.Poison` | `Effect_ApplyStatus_Poison` | 施加中毒 |
| `Effect.ApplyStatus.Slow` | `Effect_ApplyStatus_Slow` | 施加减速 |
| `Effect.ApplyStatus.Freeze` | `Effect_ApplyStatus_Freeze` | 施加冻结 |
| `Effect.ApplyStatus.Twilight` | `Effect_ApplyStatus_Twilight` | 施加暮气 |
| `Effect.Shuffle.Random` | `Effect_Shuffle_Random` | 随机腾挪 |
| `Effect.Shuffle.FromBothToOther` | `Effect_Shuffle_FromBothToOther` | 从双手区腾挪到其他区域 |
| `Effect.Shuffle.ToRandomZone` | `Effect_Shuffle_ToRandomZone` | 腾挪到随机区域 |
| `Effect.Card.AddCost` | `Effect_Card_AddCost` | 对目标卡 RuntimeCostModifier 增加 |
| `Effect.Card.ReduceCost` | `Effect_Card_ReduceCost` | 对目标卡 RuntimeCostModifier 减少 |
| `Effect.Card.DiscardSelected` | `Effect_Card_DiscardSelected` | 将 `Target.SelectedHandCard` 指定的普通手牌移入弃牌堆 |
| `Effect.Card.ExhaustSelected` | `Effect_Card_ExhaustSelected` | 将 `Target.SelectedHandCard` 指定的普通手牌移入消耗区 |
| `Effect.Draw` | `Effect_Draw` | 从指定卡牌区域移动卡牌到手牌 |
| `Effect.Discard` | `Effect_Discard` | 随机弃掉手牌中的普通卡 |
| `Effect.ExhaustSelf` | `Effect_ExhaustSelf` | 标记本卡打出后进入消耗区 |
| `Effect.GainKeyword` | `Effect_GainKeyword` | 给目标手牌临时添加关键词 |
| `Effect.RemoveStatus` | `Effect_RemoveStatus` | 移除目标指定状态层数 |
| `Effect.ModifyInitiative` | `Effect_ModifyInitiative` | 修改目标敌方部位当前先机 |

### Magnitude.Source

| Tag | 代码名 | 说明 |
|---|---|---|
| `Magnitude.Source.Literal` | `Magnitude_Source_Literal` | FinalMagnitude = Magnitude 字段 |
| `Magnitude.Source.RuntimeCost` | `Magnitude_Source_RuntimeCost` | FinalMagnitude = 本卡当前 RuntimeCost |
| `Magnitude.Source.HandCount` | `Magnitude_Source_HandCount` | FinalMagnitude = 当前手牌数量 |
| `Magnitude.Source.TargetStatusStacks` | `Magnitude_Source_TargetStatusStacks` | FinalMagnitude = 目标敌方部位上 `TargetZone` 指定 Status tag 的层数；目标或 `TargetZone` 无效时 fallback 到 `Magnitude` |

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
| `Target.Adjacent.Right` | `Target_Adjacent_Right` | 相邻右方（Tag 已声明，解析未实现）|
| `Target.LastShuffledCard` | `Target_LastShuffledCard` | 最近一次 Shuffle 的被移动卡 |
| `Target.SelectedHandCard` | `Target_SelectedHandCard` | 主动打出 `TargetMode=HandCard` 卡牌时，玩家拖拽/选择的目标手牌 |

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
| `Passive.Trigger.OnTurnStart` | `Passive_Trigger_OnTurnStart` | 玩家回合开始时（Dispatcher 方法已就位，调用点未接入）|
| `Passive.Trigger.OnTurnEnd` | `Passive_Trigger_OnTurnEnd` | 玩家回合结束时（Dispatcher 方法已就位，调用点未接入）|
| `Passive.Trigger.OnDraw` | `Passive_Trigger_OnDraw` | 本卡被抽到手牌时（Dispatcher 方法已就位，调用点未接入）|
| `Passive.Trigger.OnDiscard` | `Passive_Trigger_OnDiscard` | 本卡被弃掉时；弃牌效果、手牌上限和回合结束弃牌会触发，打出后自然进弃牌堆不触发 |

### CardLocation

`Effect.Draw` 通过 `FCardEffect::TargetZone` 指定源区域；未设置时默认 `CardLocation.Draw`。

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
| `Card.CapacityEffect.WeaponDamagePlus3` | `Card_CapacityEffect_WeaponDamagePlus3` | 蛛茧绒囊容量效果。SpecialZone 内 `bBattleEnabledInSpecialZone == true` 且带 `Card.Keyword.Weapon` 关键词的入战 instance，其 `Effect.Damage` 最终结算 +3。 |

---

## §9 效果字段使用表

`FCardEffect` 的当前字段：

```cpp
USTRUCT(BlueprintType)
struct FCardEffect
{
    FGameplayTag EffectType;
    int32 Magnitude = 0;
    FGameplayTag Target;
    FGameplayTag TargetZone;
    int32 Duration = 0;
    FGameplayTag MagnitudeSource;
    FEffectCondition Condition;
    TArray<FMagnitudeModifier> MagnitudeModifiers;

    // deprecated：兼容旧资产，运行时在 MagnitudeSource 为空时才回退读取。
    bool bMagnitudeFromRuntimeCost = false;
};

USTRUCT(BlueprintType)
struct FMagnitudeModifier
{
    FEffectCondition Condition;
    EMagnitudeModOp Op = EMagnitudeModOp::Add;
    int32 Value = 0;
};
```

Magnitude 计算顺序：
1. `MagnitudeSource` 有效时按 source handler 计算。
2. `MagnitudeSource` 为空且 `bMagnitudeFromRuntimeCost=true` 时，用当前 RuntimeCost 兼容旧资产。
3. 否则用 `Magnitude` 字段。
4. 之后按数组顺序应用 `MagnitudeModifiers`。

`Condition` 是整条效果的执行门控；`MagnitudeModifiers` 是 FinalMagnitude 计算后的条件修正列表；`bMagnitudeFromRuntimeCost` 仅用于旧资产反序列化兼容，新资产应使用 `MagnitudeSource`。`Magnitude.Source.TargetStatusStacks` 当前借用 `TargetZone` 传 Status tag，这个参数债已记录在 [TechDebt](./TechDebt.md)。

每个 EffectType 对 `FCardEffect` 字段的使用方式不同。配卡时对照本表。"-" 表示该字段对此 EffectType 无效。

| EffectType | Magnitude 语义 | Target（典型值）| TargetZone | Duration | MagnitudeSource | 备注 |
|---|---|---|---|---|---|---|
| `Effect.Damage` | 伤害值 | SingleEnemyPart / AllEnemyParts / Player | - | - | Literal / RuntimeCost | 部位 HP 归零立即破坏 |
| `Effect.Heal` | 治疗量 | Self(→Player) / Player | - | - | Literal | 恢复玩家 HP，并移除治疗量 10% 的中毒层数 |
| `Effect.ApplyStatus.Poison` | 层数 | Player / SingleEnemyPart / AllEnemyParts | - | - | Literal / RuntimeCost | 层数模型，不用 Duration |
| `Effect.ApplyStatus.Slow` | 层数 | 同上 | - | - | Literal | 第一阶段只记录 |
| `Effect.ApplyStatus.Freeze` | 层数 | SingleEnemyPart | - | 回合数(0=层数模型) | Literal | 按层数消耗实现 |
| `Effect.ApplyStatus.Twilight` | 层数 | SingleEnemyPart | - | - | Literal | 第一阶段只记录 |
| `Status.Shield` | 护盾值 | Player / Self(部位) | - | - | Literal | 直接加到 Shield 字段 |
| `Effect.Shuffle.Random` | - | RandomHandCard | - | - | - | 从手牌随机选一张腾挪 |
| `Effect.Shuffle.FromBothToOther` | - | ZoneHandCard | HandZone.Both | - | - | 从双手区挑一张腾挪到左/右 |
| `Effect.Shuffle.ToRandomZone` | - | Self(本卡) | - | - | - | 把本卡腾挪到随机区域 |
| `Effect.Card.AddCost` | Modifier 增量 | Self(本卡) / LastShuffledCard / SelectedHandCard | - | - | Literal | 修改 RuntimeCostModifier |
| `Effect.Card.ReduceCost` | Modifier 减量 | 同上 | - | - | Literal | 下限由 ComputeRuntimeCost clamp 到 0 |
| `Effect.Card.DiscardSelected` | 建议填 1 | SelectedHandCard | - | - | Literal | 指定普通手牌进弃牌堆；建议显式设置 HandCardTargetFilter：允许普通手牌、拒绝左右手锚点；Magnitude 不参与数量判定；触发目标卡 `OnDiscard` |
| `Effect.Card.ExhaustSelected` | 建议填 1 | SelectedHandCard | - | - | Literal | 指定普通手牌进消耗区；建议显式设置 HandCardTargetFilter：允许普通手牌、拒绝左右手锚点；Magnitude 不参与数量判定；不触发 `OnDiscard` |
| `Effect.Draw` | 张数 | Self / Player | CardLocation.* | - | Literal | `TargetZone` 复用为源区域 tag，默认抽牌堆 |
| `Effect.Discard` | 张数 | Self / Player | - | - | Literal | 随机弃掉手牌中普通卡，不弃锚点 |
| `Effect.ExhaustSelf` | - | Self(本卡) | - | - | - | 通过临时 `Card.Keyword.Exhaust` 标记交给打出后去向阶段处理 |
| `Effect.GainKeyword` | - | HandCard | KeywordTag | - | - | `TargetZone` 复用为要添加的 Keyword tag |
| `Effect.RemoveStatus` | 层数 | Player / SingleEnemyPart | StatusTag | - | Literal | `TargetZone` 复用为要移除的 Status tag |
| `Effect.ModifyInitiative` | 先机增量 | SingleEnemyPart | - | - | Literal | 正数增加，负数减少 |

### MagnitudeModifiers

`FCardEffect::MagnitudeModifiers` 是条件数值修正列表。每条包含 `Condition / Op / Value`：

| Op | 语义 |
|---|---|
| `Add` | `FinalMagnitude += Value` |
| `Multiply` | `FinalMagnitude *= Value` |

多条修正按数组顺序依次应用。条件结构沿用 `FEffectCondition`。

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
| `Target.SelectedHandCard` | HandCard | `PlayCard` 命令的 `TargetCardInstanceId` | 否 |
| `Target.Adjacent.Right` | EnemyPart | 未实现 | 否 |

### Target.Self 的 EffectType 消歧

- `Effect.Shuffle.ToRandomZone` / `Effect.Card.AddCost` / `Effect.Card.ReduceCost` → 指向本卡（HandCard）
- `Effect.Card.DiscardSelected` / `Effect.Card.ExhaustSelected` 必须填写 `Target.SelectedHandCard`，不要填写 `Target.Self`
- 其他（Damage / Heal / ApplyStatus）→ 指向玩家（Player）

### HandCard 目标筛选填写建议

- `Effect.Card.AddCost / ReduceCost + Target.SelectedHandCard`：通常允许普通手牌和左右手锚点。
- `Effect.Card.DiscardSelected / ExhaustSelected + Target.SelectedHandCard`：通常只允许普通手牌，拒绝左右手锚点。
- “只作用伙伴”这类卡可在 `RequiredTargetKeywords` 填 `Card.Keyword.Companion`。
- “不能作用武器”这类卡可在 `BlockedTargetKeywords` 填 `Card.Keyword.Weapon`。
- 旧资产没有显式设置时，Battle 会按上述两类兼容推断；新测试卡和后续正式卡建议显式设置，避免效果组合变复杂后语义含混。

---

## §10 FEffectCondition / FCardZoneHook / FCardPassive

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
    UPROPERTY(EditDefaultsOnly) FText            DisplayText;        // UI 详情展示文本，不参与规则
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

**DisplayText** 只用于卡牌详情面板的“被动”区块展示。战斗规则不读取该字段，仍以
`Trigger / Effects / Condition / TriggerThreshold` 为准。若为空，UI 可以根据规则字段生成
fallback 文本，避免旧资产无说明。
