---
type: data-contract
scope: wacom-data
status: active
updated: 2026-06-08
tags:
  - wacom/data
  - wacom/dataasset
---

# WacomData 模块文档

> [!info] 本文职责
> 本文是 WacomData 模块的静态 DataAsset 契约入口。它只记录数据类型、字段语义和模块边界；内容生成、资产校验和制作矩阵见 [WacomDataAuthoring.md](./WacomDataAuthoring.md)，Gameplay tag 字典见 [WacomGameplayTags.md](./WacomGameplayTags.md)。

> [!warning] 边界
> `WacomData` 只定义静态内容。战斗结算见 [WacomBattle.md](./WacomBattle.md)，Run 状态和事务见 [WacomRun.md](./WacomRun.md)，世界交互 authoring 见 [WacomWorldInteraction.md](./WacomWorldInteraction.md)，UI 表现见 [WacomUI.md](./WacomUI.md)。

## §1 模块职责

`WacomData` 负责卡牌、敌人、Encounter、角色、商店、拾取物、Run world card interaction 和 RunEvent 等静态定义。它可以描述“内容是什么”，不能保存“当前 Run / 当前战斗发生了什么”。

**负责：**
- `UCardDefinition`、`UEnemyDefinition`、`UEnemyPartDefinition`、`UCharacterDefinition`
- `UEnemyBehaviorDefinition`
- `UEncounterDefinition`
- `UShopDefinition`
- `UWacomRunPickupDefinition`
- `UWacomRunWorldCardInteractionDefinition`
- `UWacomRunEventDefinition`
- `FIntentDefinition`、`FCardEffect`、`FCardZoneHook`、`FCardPassive`、`FEffectCondition`

**不负责：**
- `UBattleSession`、`FBattleState`、运行时卡牌实例或战斗命令
- `URunSession`、`FRunState`、库存、事件进度、金币、存档和事务回滚
- Actor 摆放实例的 `PersistentId`、Details authoring facade 或地图校验
- UI ViewData、WBP 绑定、Widget 生命周期或输入路由
- Editor-only 内容生成、资产 Validator 注册和 commandlet 执行

依赖方向：

```text
WacomCore <- WacomData <- WacomBattle <- WacomRun <- WacomApp
                         ^
                         WacomEditor / WacomTests 只在编辑器和测试侧读取数据合同
```

`WacomData` 只能依赖 `WacomCore`。不要让它反向 include `WacomBattle / WacomRun / WacomApp / WacomEditor`。

## §2 资产类型地图

| 类型 | 位置 / 主要头文件 | 静态语义 | 运行时 owner |
|---|---|---|---|
| `UCardDefinition` | `Source/WacomData/Public/Cards` | 卡牌 ID、文案、费用、关键词、目标模式、效果、被动和身材 | Battle 创建 runtime card；Run 保存玩家持有卡牌实例 |
| `UEnemyDefinition` | `Source/WacomData/Public/Enemies` | 敌人由哪些部位组成、默认行为资产和部位行为绑定 | Battle 初始化敌人 runtime state |
| `UEnemyPartDefinition` | `Source/WacomData/Public/Enemies` | 部位 HP、经验、击倒奖励卡 | Battle 执行部位行动和击倒选择 |
| `UEnemyBehaviorDefinition` | `Source/WacomData/Public/Enemies` | 敌人 phase、intent set、selector rule 和意图候选 | Battle 刷新并执行敌方部位当前意图 |
| `UEncounterDefinition` | `Source/WacomData/Public/Encounters` | 单场战斗包含哪些敌人槽，以及敌人槽顺序 | App 的 BattleTrigger 进入战斗前转换为 Battle init params |
| `UCharacterDefinition` | `Source/WacomData/Public/Characters` | 角色基础 HP、左右手固有卡、初始牌组 | Run 初始化角色和玩家卡池；Battle 读取入战卡组 |
| `UShopDefinition` | `Source/WacomData/Public/Shops` | 固定商品列表和价格 | RunSession 按场景 shop visit 保存购买状态 |
| `UWacomRunPickupDefinition` | `Source/WacomData/Public/Pickups` | 数据驱动拾取物奖励配置 | RunSession 使用场景 `PersistentId` 防重复拾取 |
| `UWacomRunWorldCardInteractionDefinition` | `Source/WacomData/Public/Interactions` | Run world card drop 的卡牌筛选、奖励和反馈文案 | RunSession 提交事务，Actor receiver 读取制作定义 |
| `UWacomRunEventDefinition` | `Source/WacomData/Public/Events` | 探索事件图、节点、选项、条件和效果 | RunSession 执行事件事务和回滚 |

静态内容默认位于 `/Game/Wacom/Data`，完整目录约定见 [Content_Organization.md](./Content_Organization.md)。生成内容清单和 commandlet 口径见 [WacomDataAuthoring.md](./WacomDataAuthoring.md)。

## §3 Card Definition

`UCardDefinition` 是卡牌静态规则与显示文本的来源。它不保存 runtime cost modifier、当前位置、临时关键词或是否已被消耗。

```cpp
UCLASS(BlueprintType)
class UCardDefinition : public UPrimaryDataAsset
{
    FName CardId;
    FText DisplayName;
    FText Description;
    int32 BaseCost = 0;
    FGameplayTag Rarity;
    FGameplayTagContainer Keywords;
    FCardPhysique Physique;
    ECardTargetMode TargetMode;
    FWacomHandCardTargetFilter HandCardTargetFilter;
    TArray<FCardEffect> Effects;
    TArray<FCardEffect> PerfectReleaseEffects;
    TArray<FCardZoneHook> ZoneHooks;
    TArray<FCardPassive> Passives;
};
```

字段口径：

| 字段 | 语义 |
|---|---|
| `CardId` | 内容稳定 ID；用于 debug、测试和运行时实例引用来源，不是 UObject path |
| `DisplayName / Description` | UI 展示文本；规则不从描述文本解析效果 |
| `BaseCost` | 基础费用；Battle 会叠加 runtime modifier 后 clamp |
| `Rarity / Keywords` | 静态标签；标签定义见 [WacomGameplayTags.md](./WacomGameplayTags.md) |
| `Physique` | 入战 HP、容量和后续耐久相关静态字段 |
| `TargetMode` | 玩家打出时是否需要敌方部位、玩家自身、手牌或无目标 |
| `HandCardTargetFilter` | 仅 `TargetMode=HandCard` 的目标手牌资格过滤 |
| `Effects / PerfectReleaseEffects` | 主效果与完美释放效果；可制作范围见 [WacomDataAuthoring.md](./WacomDataAuthoring.md) |
| `ZoneHooks / Passives` | 区域触发和被动触发静态配置；执行时机由 Battle 决定 |

`FWacomHandCardTargetFilter` 只影响玩家主动打出 `TargetMode=HandCard` 时的目标资格。UI 不直接解释它，而是读取 Battle validation / drop intent 的结果。

```cpp
USTRUCT(BlueprintType)
struct FWacomHandCardTargetFilter
{
    bool bUseExplicitHandCardTargetFilter = false;
    bool bAllowNormalHandCards = true;
    bool bAllowHandAnchors = true;
    FGameplayTagContainer RequiredTargetKeywords;
    FGameplayTagContainer BlockedTargetKeywords;
};
```

- 显式 filter 优先；未显式设置时，Battle 按当前兼容推断处理普通手牌和左右手锚点。
- `RequiredTargetKeywords` 必须全部满足；`BlockedTargetKeywords` 命中任意一个即拒绝。
- 目标有效关键词 = 卡牌定义关键词 + 战斗内临时关键词。允许锚点时，左右手锚点同样参与关键词条件。
- self target 永远禁止。费用、卡牌类型、伙伴 / 食物等更细条件还不是当前字段合同。

`FCardPhysique` 是卡牌身体 / 容量数据：

```cpp
USTRUCT(BlueprintType)
struct FCardPhysique
{
    int32 MaxHpBonus = 0;
    int32 Durability = 0;
    int32 Capacity = 0;
    FGameplayTag CapacityEffect;
};
```

- `MaxHpBonus` 只对带 `Card.Keyword.Companion` 的入战卡计入玩家战内 MaxHp。
- `Durability` 字段保留，当前规则层未读取；耐久系统接入后再由 Battle / Run resolver 消费。
- `Capacity=0` 表示普通卡；`Capacity>0` 表示容器卡。
- `CapacityEffect` 为空表示 A 类容器；有效 tag 表示 B 类容器并展开 SpecialZone。当前已接入的具体效果见 [WacomGameplayTags.md](./WacomGameplayTags.md#cardcapacityeffect)。

容器在 Run 层的容量与背包规则见 [WacomRun.md](./WacomRun.md)，容量效果入战结算见 [WacomBattle.md](./WacomBattle.md)。

<a id="wacomdata-enemy-part"></a>
## §4 Enemy Definition

`UEnemyDefinition` 描述敌人的部位组成和默认行为资产；部位 HP / 奖励等静态数值在 `UEnemyPartDefinition`，意图选择主合同在 `UEnemyBehaviorDefinition`。

```cpp
UCLASS(BlueprintType)
class UEnemyDefinition : public UPrimaryDataAsset
{
    FName EnemyId;
    FText DisplayName;
    TObjectPtr<UEnemyBehaviorDefinition> DefaultBehavior;
    FName DefaultPhaseId;
    TArray<FEnemyPartSlot> Parts;
};

USTRUCT(BlueprintType)
struct FEnemyPartSlot
{
    FName PartSlotId;
    TObjectPtr<UEnemyPartDefinition> PartDef;
    TObjectPtr<UEnemyBehaviorDefinition> BehaviorOverride;
    FName InitialIntentSetId;
};
```

| 字段 | 语义 |
|---|---|
| `EnemyId` | 敌人 authored id，例如 `Enemy.Snake`。 |
| `DefaultBehavior` | 敌人默认行为资产；正式敌人必须提供可用 `UEnemyBehaviorDefinition`。 |
| `DefaultPhaseId` | 初始 phase；为空时使用 `DefaultBehavior.InitialPhaseId`。 |
| `Parts[].PartSlotId` | 敌人定义内的局部部位槽 ID，必须显式填写；Battle 运行时 key 使用 `EncounterId + EnemySlotId + PartSlotId`。 |
| `Parts[].PartDef` | 部位静态定义，提供 HP、经验和击倒奖励。 |
| `Parts[].BehaviorOverride` | 单部位行为覆盖；为空时使用 `DefaultBehavior`。 |
| `Parts[].InitialIntentSetId` | 该部位初始 intent set；为空时按 `AppliesToPartSlotId == PartSlotId` 匹配，找不到再使用空 `AppliesToPartSlotId` 的 fallback set。 |

`PartDef->PartId` 是可复用部位定义 ID，例如 `Snake.Head`，只保留为内容定义、编辑器校验和 debug 语义；运行时目标匹配不再使用 `PartId`。

```cpp
UCLASS(BlueprintType)
class UEnemyPartDefinition : public UPrimaryDataAsset
{
    FName PartId;
    FText DisplayName;
    int32 MaxHp = 0;
    int32 ExperienceReward = 0;
    TObjectPtr<UCardDefinition> KnockdownRewardCard = nullptr;
};
```

| 字段 | 语义 |
|---|---|
| `PartId` | 部位 authored id，例如 `Snake.Head`；SceneEnemy PartActor authoring 用它校验静态部位定义。 |
| `MaxHp` | 部位初始生命上限。 |
| `ExperienceReward` | 部位破坏后给玩家的经验记账。 |
| `KnockdownRewardCard` | Aid / Destroy 击倒选择共用的奖励卡定义；Battle 内创建 card，战后由 Run 接收。 |

`UEnemyBehaviorDefinition` 是敌人行为的静态主合同：

```cpp
UCLASS(BlueprintType)
class UEnemyBehaviorDefinition : public UPrimaryDataAsset
{
    FName BehaviorId;
    FName InitialPhaseId;
    TArray<FWacomEnemyPhaseDefinition> Phases;
};

USTRUCT(BlueprintType)
struct FWacomEnemyPhaseDefinition
{
    FName PhaseId;
    TArray<FWacomEnemyIntentSetDefinition> IntentSets;
};

USTRUCT(BlueprintType)
struct FWacomEnemyIntentSetDefinition
{
    FName IntentSetId;
    FName AppliesToPartSlotId;
    EWacomEnemyIntentSelectorMode SelectorMode;
    TArray<FWacomEnemyBehaviorIntent> Intents;
    TArray<FWacomEnemyIntentSelectorRule> SelectorRules;
    FName FallbackIntentId;
};
```

`SelectorMode` 当前支持：

| Mode | 语义 |
|---|---|
| `Sequence` | 按 authored intent 顺序选择下一条可用意图，会跳过 rule / cooldown 阻塞的候选。 |
| `Weighted` | 在有效 rule 中按权重使用战斗 RNG 确定性选择。 |
| `PriorityFirst` | 选择有效 rule 中 `Priority` 最高者；并列时沿 authored 顺序。 |

Selector condition 当前支持 `Always`、自身 HP 阈值、同单位任意部位 HP 阈值、部位已破坏、当前 phase、自身状态、玩家状态和冷却可用。冷却以“后续选择次数”为单位，不是回合数。

`FIntentDefinition` 和 `FIntentEffect` 是敌方意图的静态效果描述，只通过 `UEnemyBehaviorDefinition` 进入 Battle 运行时。当前敌人意图字段比卡牌效果更窄；可制作范围见 [WacomDataAuthoring.md](./WacomDataAuthoring.md#battle-rule-content-authoring-matrix)。

```cpp
USTRUCT(BlueprintType)
struct FIntentDefinition
{
    FName IntentId;
    FText DisplayName;
    int32 Initiative = 0;
    int32 ResistanceValue = 0;
    TArray<FIntentEffect> Effects;
};

USTRUCT(BlueprintType)
struct FIntentEffect
{
    FGameplayTag EffectType;
    int32 Magnitude = 0;
    FGameplayTag Target;
    int32 Duration = 0;
};
```
<a id="wacomdata-encounter-definition"></a>
## §5 Encounter Definition

`UEncounterDefinition` 是单场战斗静态敌人组合入口。它只描述“这场战斗有哪些敌人槽”，不保存场景 Actor、运行时进度、视觉 prefab、奖励、阵型、位置或存档状态。

```cpp
UCLASS(BlueprintType)
class UEncounterDefinition : public UPrimaryDataAsset
{
    FName EncounterDefinitionId;
    FText DisplayName;
    TArray<FEncounterEnemySlot> EnemySlots;
};

USTRUCT(BlueprintType)
struct FEncounterEnemySlot
{
    FName EnemySlotId;
    TObjectPtr<UEnemyDefinition> EnemyDefinition;
};
```

| 字段 | 语义 |
|---|---|
| `EncounterDefinitionId` | Encounter 内容稳定 ID；用于内容识别、debug 和后续运行时映射，不从资产名自动回退 |
| `DisplayName` | UI 展示名，可为空；规则层不从文本解析行为 |
| `EnemySlots` | Encounter 敌人槽列表；数组顺序表示战斗敌人槽顺序，不表示场景摆放位置 |
| `EnemySlotId` | Encounter 内稳定敌人槽 ID；后续映射到 Battle `EnemySlotId`，参与多敌人部位身份 |
| `EnemyDefinition` | 敌人槽使用的静态敌人定义；不同槽可以引用同一个敌人定义 |

当前 `UEncounterDefinition` 是静态数据合同，不保存运行态进度。正式场景入口由 `ABattleTriggerActor.EncounterDefinition` 引用它；进入战斗时 App 层把 `EnemySlots` 转换为 `FBattleInitParams.EnemySlots`。Battle 仍只消费 `FBattleInitParams`，Run 仍用场景 Trigger 的 `PersistentId` 作为撤离重入进度 key，不直接持有 Encounter 资产。

当前生成内容包含 `DA_Encounter_SnakeSingle`：`EncounterDefinitionId=Encounter.Snake.Single`，单个 `EnemySlotId=Enemy` 引用 `DA_Enemy_Snake`。`DA_Enemy_Snake` 通过 `DefaultBehavior=DA_Behavior_Snake` 绑定 Head / Body / Tail 三套 `Sequence` intent set，三份 `DA_Part_Snake_*` 只保存 HP、经验和毒牙奖励。关卡 Trigger 应优先引用该 Encounter，再用 `SceneEnemyHostSlots[Enemy]` 绑定场景中的 Snake Host prefab。

## §6 Character Definition

`UCharacterDefinition` 是角色静态入口。Run 初始化时读取它创建玩家初始卡池和角色基准数据。

```cpp
UCLASS(BlueprintType)
class UCharacterDefinition : public UPrimaryDataAsset
{
    FName CharacterId;
    FText DisplayName;
    int32 FingerCount = 10;
    int32 HpPerFinger = 2;
    TObjectPtr<UCardDefinition> LeftHandCard;
    TObjectPtr<UCardDefinition> RightHandCard;
    TArray<TObjectPtr<UCardDefinition>> StarterDeck;
};
```

- `GetBasePlayerMaxHp()` 返回 `FingerCount * HpPerFinger`。
- `LeftHandCard / RightHandCard` 是固有左右手卡，不放进 `StarterDeck`。
- `StarterDeck` 只放正式初始牌。测试卡、debug key 和 badge 显示测试卡不进入初始牌组；需要 PIE 验证时通过调试商店获得。
- 战内 MaxHp = 角色基础 HP + 备战卡组中 Companion 卡的 `Physique.MaxHpBonus` 总和。

## §7 Shop Definition

`UShopDefinition` 只定义静态商品内容，不保存已购买状态。

```cpp
UCLASS(BlueprintType)
class UShopDefinition : public UPrimaryDataAsset
{
    FName ShopId;
    FText DisplayName;
    TArray<FShopOfferDefinition> Offers;
};

USTRUCT(BlueprintType)
struct FShopOfferDefinition
{
    TObjectPtr<UCardDefinition> CardDefinition;
    int32 Price = 0;
};
```

- `ShopId` 是内容 ID，不替代场景 `AWacomShopTriggerActor.PersistentId`。
- `Offers` 不能为空；剧情空商店后续需要显式字段表达。
- `Price=0` 是合法免费商品；负数无效。
- RunSession 使用场景 `PersistentId` 保存本次 Run 内的购买状态和库存访问。

## §8 Pickup Definition

`UWacomRunPickupDefinition` 是 Run world Pickup 的数据驱动奖励定义。它描述“这个拾取物给什么”，不保存“这个场景实例是否已拾取”。

```cpp
UCLASS(BlueprintType)
class UWacomRunPickupDefinition : public UPrimaryDataAsset
{
    FName PickupId;
    EWacomRunPickupRewardType RewardType;
    int32 GoldAmount = 1;
    TObjectPtr<UCardDefinition> CardDefinition;
};
```

| 字段 | 语义 |
|---|---|
| `PickupId` | 静态内容 ID，只用于内容识别和 debug；不是 `CollectedPickupIds` key |
| `RewardType` | `None / Gold / Card`；`None` 是无效配置 |
| `GoldAmount` | 仅 `RewardType=Gold` 使用，必须大于 0 |
| `CardDefinition` | 仅 `RewardType=Card` 使用，表示固定获得一张卡 |

正式摆放推荐 `BP_WacomRunRewardPickupActor + UWacomRunPickupDefinition`。每个场景实例仍必须有自己的唯一 `PersistentId`。

## §9 Run World Card Interaction Definition

`UWacomRunWorldCardInteractionDefinition` 是 Run world card drop receiver 的通用静态制作定义。它描述目标接受什么卡、给什么奖励、是否消耗卡和失败 / 完成反馈文案；不保存目标是否已完成。

```cpp
USTRUCT(BlueprintType)
struct FWacomRunWorldCardInteractionReward
{
    EWacomRunWorldCardInteractionRewardType Type;
    int32 GoldAmount;
    TObjectPtr<UCardDefinition> CardDefinition;
};

UCLASS(BlueprintType)
class UWacomRunWorldCardInteractionDefinition : public UPrimaryDataAsset
{
    FName InteractionId;
    TArray<TObjectPtr<UCardDefinition>> AllowedCardDefinitions;
    TArray<FName> AllowedCardIds;
    FGameplayTagContainer RequiredKeywords;
    FGameplayTagContainer BlockedKeywords;
    TArray<FWacomRunWorldCardInteractionReward> Rewards;
    bool bConsumeCardOnSuccess = true;
    FText PreviewPromptText;
    FText SuccessPromptText;
    FText CompletedPromptText;
    FText RejectedCardPromptText;
    FText ConfigWarningPromptText;
    FText SourceCardUnavailablePromptText;
    FText GenericFailurePromptText;
};
```

- `InteractionId` 是内容 ID，不替代场景 `PersistentId` 和 `CompletedRunWorldInteractionIds` key。
- 正向筛选必须至少包含 `AllowedCardDefinitions / AllowedCardIds / RequiredKeywords` 中一种；单独配置 `BlockedKeywords` 无效。
- `Rewards` 当前支持 Gold 和 Card；成功时按顺序发放。
- `bConsumeCardOnSuccess` 控制成功后是否永久移除源卡。
- Prompt 文本只服务 App 表现反馈，不影响 RunSession 事务判定。

运行时提交流程和 Actor authoring 见 [WacomWorldInteraction.md](./WacomWorldInteraction.md#4-run-world-card-drop)。

## §10 RunEvent Definition

`UWacomRunEventDefinition` 是探索事件图的静态定义。RunSession 打开事件时读取当前节点，选择选项时在 working-state 事务中校验条件、应用效果、推进节点或关闭事件。

核心结构：

| 类型 | 语义 |
|---|---|
| `UWacomRunEventDefinition` | `EventId`、标题、起始节点和节点数组 |
| `FWacomRunEventNodeDefinition` | 节点 ID、标题、正文、选项列表 |
| `FWacomRunEventChoiceDefinition` | 选项 ID、按钮文本、条件、效果、下一节点和关闭 / 完成语义 |
| `FWacomRunEventConditionDefinition` | 选项可用条件，例如金币、卡牌、RunFlag、事件完成状态 |
| `FWacomRunEventEffectDefinition` | 选项提交后的效果，例如金币、卡牌、压力、RunFlag、节点消耗 |
| `FWacomRunEventCardPaymentDefinition` | 单卡支付筛选和 payment zone authoring |

字段口径：

- `EventId / NodeId / ChoiceId` 是内容 ID，用于事件图和 debug，不替代场景 `PersistentId`。
- RunFlag 使用 `FName FlagId`，不是 Gameplay tag，不做数值或计数。
- 压力类型在 DataAsset 中使用稳定 `FName`，RunSession 执行时转换为运行时压力 ID。
- `CardPayment` 当前是单卡支付合同。支付 UI、drop target 和 menu lease 见 [WacomUI.md](./WacomUI.md) 与 [WacomWorldInteraction.md](./WacomWorldInteraction.md#6-run-menu-zone-target)。
- 条件和效果的阻断 / warning 口径见 [WacomDataAuthoring.md](./WacomDataAuthoring.md#asset-validation)。

## §11 Battle Effect Structs

卡牌、意图、ZoneHook 和 Passive 使用 Gameplay tag 表达效果类型、目标、条件、区域和触发点。tag 字典见 [WacomGameplayTags.md](./WacomGameplayTags.md)，但 tag 已声明不代表可写入正式资产；可制作范围见 [WacomDataAuthoring.md](./WacomDataAuthoring.md#battle-rule-content-authoring-matrix)。

核心结构：

| 类型 | 静态语义 |
|---|---|
| `FCardEffect` | 卡牌效果条目：EffectType、Magnitude、Target、TargetZone、Duration、MagnitudeSource、Condition、MagnitudeModifiers |
| `FIntentEffect` | 敌人意图效果条目：EffectType、Magnitude、Target、Duration |
| `FEffectCondition` | 效果或 passive 的条件门控 |
| `FCardZoneHook` | 指定手牌区和触发点的额外效果 |
| `FCardPassive` | 被动触发点、展示文案、效果、条件和阈值 |

`WacomData` 只定义字段。Battle resolver、dispatcher、validation matrix 和 transient runtime fixture 共同决定这些字段当前是否真正可执行。

## §12 修改数据合同时的检查点

修改 DataAsset 字段或新增静态数据能力时，先确认：

- 是否只是静态定义，还是需要 Battle / Run runtime state 新字段。
- 是否需要新增 Gameplay tag；若需要，先在 `WacomCore/Public/Tags/WacomGameplayTags.h` 声明，再更新 [WacomGameplayTags.md](./WacomGameplayTags.md)。
- 是否需要更新 battle content authoring matrix、validator、生成内容 smoke 或 runtime resolver；这些入口见 [WacomDataAuthoring.md](./WacomDataAuthoring.md)。
- 是否影响 Actor authoring / map validation；世界交互侧见 [WacomWorldInteraction.md](./WacomWorldInteraction.md)。
- 是否影响 UI ViewData 或 WBP 绑定；UI 侧见 [WacomUI.md](./WacomUI.md) 和 Binding 文档。

不要为了一个资产临时需求让 `WacomData` 依赖高层模块。需要运行时语义时，在对应领域模块实现，并让 DataAsset 只保存静态配置。
