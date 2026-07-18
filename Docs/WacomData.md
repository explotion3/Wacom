---
type: data-contract
scope: wacom-data
status: active
updated: 2026-07-17
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

`WacomData` 负责卡牌、敌人、Encounter、角色、商店、拾取物、Run world card interaction、RunEvent 和 Logical Map Graph 等静态定义。它可以描述“内容是什么”，不能保存“当前 Run / 当前战斗发生了什么”。

**负责：**
- `UCardDefinition`、`UEnemyDefinition`、`UEnemyPartDefinition`、`UCharacterDefinition`
- `UEnemyBehaviorDefinition`
- `UEncounterDefinition`
- `UShopDefinition`
- `UWacomRunPickupDefinition`
- `UWacomRunWorldCardInteractionDefinition`
- `UWacomRunEventDefinition`
- `UWacomJourneyDefinition`、`UWacomFloorMapDefinition`
- `FWacomMapNodeDefinition`、`FWacomMapEdgeDefinition` 和 typed node payload
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
| `UEnemyPartDefinition` | `Source/WacomData/Public/Enemies` | 部位 HP、经验、Aid/Destroy 分支奖励与 legacy 兼容来源 | Battle 执行部位行动和击倒选择 |
| `UEnemyBehaviorDefinition` | `Source/WacomData/Public/Enemies` | 敌人 phase、intent set、selector rule 和意图候选 | Battle 刷新并执行敌方部位当前意图 |
| `UEncounterDefinition` | `Source/WacomData/Public/Encounters` | 单场战斗包含哪些敌人槽，以及敌人槽顺序 | App 的 BattleTrigger 进入战斗前转换为 Battle init params |
| `UCharacterDefinition` | `Source/WacomData/Public/Characters` | 角色基础 HP、左右手固有卡、初始牌组 | Run 初始化角色和玩家卡池；Battle 读取入战卡组 |
| `UShopDefinition` | `Source/WacomData/Public/Shops` | 固定商品列表和价格 | RunSession 按场景 shop visit 保存购买状态 |
| `UWacomRunPickupDefinition` | `Source/WacomData/Public/Pickups` | 数据驱动拾取物奖励配置 | RunSession 使用场景 `PersistentId` 防重复拾取 |
| `UWacomRunWorldCardInteractionDefinition` | `Source/WacomData/Public/Interactions` | Run world card drop 的卡牌筛选、奖励和反馈文案 | RunSession 提交事务，Actor receiver 读取制作定义 |
| `UWacomRunEventDefinition` | `Source/WacomData/Public/Events` | 探索事件图、节点、选项、条件和效果 | RunSession 执行事件事务和回滚 |
| `UWacomJourneyDefinition` | `Source/WacomData/Public/Map` | JourneyId、角色范围、有序 Floor、时段 AP 预算与 Decay 曲线 | RunSession 初始化探索 working state，并在 Morning 读取压力规则 |
| `UWacomFloorMapDefinition` | `Source/WacomData/Public/Map` | FloorId、EntryNodeId、节点与有向边 | RunSession 执行 lifecycle、traversal、MapTravel 和跨层验证 |

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
    TObjectPtr<UTexture2D> CardIllustration;
    TObjectPtr<UTexture2D> CardIllustrationDepthMap;
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
| `DisplayName / Description` | UI 展示文本；规则不从中文自然语言解析效果。`Description` 仍可服务小卡紧凑描述或其它旧 UI；expanded detail 只在没有任何结构化 `Effects / Passives / outcome` section 时把它作为普通正文回退，不解析旧占位。常规详情正文由 `Effects / Passives` 通过 WacomApp 的 semantic explanation document 生成 |
| `CardIllustration` | 卡牌主题插画 `Texture2D`。第一人称卡面复合材质优先使用该纹理；旧卡为空时沿用实际 CardView（第一人称为 `WBP_FirstPersonCardView`）的 authored `CardArt` Brush。稀有度边框不使用本字段，继续由 CardView 的 `RarityBorderSprites` 以 `PaperSprite` 图集区域解析 |
| `CardIllustrationDepthMap` | 可选的纯表现灰度深度图。黑色更深、白色更靠近实体 Frame、中灰为 authored 基准；第一人称卡面量化为约 5 级并限制在 Frame 后方。为空时整张插画仍按统一凹入深度显示，不影响规则、Snapshot 或存档。推荐导入为 `Masks / sRGB=false / Nearest / NoMipmaps / UI` |
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
    TObjectPtr<UCardDefinition> AidRewardCard = nullptr;
    TObjectPtr<UCardDefinition> DestroyRewardCard = nullptr;
    // Deprecated：只供尚未迁移的资产兼容读取。
    TObjectPtr<UCardDefinition> KnockdownRewardCard = nullptr;

    UCardDefinition* ResolveKnockdownRewardCard(EKnockdownChoice Choice) const;
};
```

| 字段 | 语义 |
|---|---|
| `PartId` | 部位 authored id，例如 `Snake.Head`；SceneEnemy PartActor authoring 用它校验静态部位定义。 |
| `MaxHp` | 部位初始生命上限。 |
| `ExperienceReward` | 部位破坏后给玩家的经验记账。 |
| `AidRewardCard` | Aid 分支显式奖励卡；正式 Production Part 必须填写。 |
| `DestroyRewardCard` | Destroy 分支显式奖励卡；正式 Production Part 必须填写。 |
| `KnockdownRewardCard` | deprecated legacy 兼容来源。只有对应新字段为空时 Aid/Destroy 才回退读取；不得与任一新字段混填。 |

Battle 与 UI ViewData 都只能通过 `ResolveKnockdownRewardCard()` 读取奖励：Aid/Destroy 各自优先新字段，空时回退 legacy；Withdraw/None 永远返回空。这个查询是非反射 C++ 合同，Blueprint 不应绕过它自行解释兼容优先级。

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

首个正式动画敌人内容包是 TrainingWarrior，它只使用上述现有 schema：`EnemyId=Enemy.TrainingWarrior`，单一 `Body` 槽引用 `PartId=TrainingWarrior.Body`，HP 24、经验 3，默认行为 `TrainingWarrior.Behavior`。`Default` phase 的 `TrainingWarrior.Body.Sequence` 固定按 Attack（先机 3、抵抗 4、玩家伤害 4）→ Guard（先机 2、自身护盾 4）→ Cleave（先机 4、抵抗 7、玩家伤害 7）循环。单敌人 Encounter 使用 `EncounterDefinitionId=Encounter.TrainingWarrior.Single` 与 `EnemySlotId=Enemy`。

Body 的现有二进制资产仍通过 legacy `KnockdownRewardCard` 给 Aid / Destroy 提供 `Reward.BrokenCleave`（“残缺横斩”）：White、Weapon、1 费、`TargetMode=AllEnemyParts`，对每个存活敌方部位造成 3 伤害。它没有 Physique、被动、ZoneHook、PerfectRelease 或专用插画；CardView 使用既有 fallback。Withdraw 不获得该卡。TrainingWarrior builder 的未来写入已改为同时填写两个显式字段并清空 legacy，但在授权资产迁移前不会执行或重存现有资产。

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
    TArray<FName> GrantedCredentialIds;
};
```

| 字段 | 语义 |
|---|---|
| `PickupId` | 静态内容 ID，只用于内容识别和 debug；不是 `CollectedPickupIds` key |
| `RewardType` | `None / Gold / Card`；`None` 是无效配置 |
| `GoldAmount` | 仅 `RewardType=Gold` 使用，必须大于 0 |
| `CardDefinition` | 仅 `RewardType=Card` 使用，表示固定获得一张卡 |
| `GrantedCredentialIds` | 可为空；与固定主奖励在同一事务中授予的稳定 Run Credential，每项非 `None` 且 Definition 内唯一 |

正式摆放推荐 `BP_WacomRunRewardPickupActor + UWacomRunPickupDefinition`。每个场景实例仍必须有自己的唯一 `PersistentId`。Credential ID 使用 `FName` 数据身份，不是 GameplayTag；静态 Definition 只声明 grant，权威持有状态和写事务属于 `WacomRun`。

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

## §10 Logical Map Graph

Logical Map Graph 的静态真相由 `UWacomJourneyDefinition` 和 `UWacomFloorMapDefinition` 保存。Floor `DisplayName` 与 Node `DisplayName / ShortDescription` 是玩家可读地图文案；`ShortDescription` 可空。`MapPosition` 是 1920×1080 设计画布坐标，只服务地图 UI 排版；世界坐标、Spline、Actor 和 Widget 不进入数据合同。

- `FWacomMapNodeHandle = FloorId + NodeId`，跨 Floor 唯一。
- `FWacomMapEdgeHandle = FloorId + EdgeId`；单独 `EdgeId` 只在所属 Floor 内唯一。
- `FWacomMapEdgeDefinition` 是有向边；允许反向通行必须显式制作另一条边。
- `FWacomMapNodeContent` 使用固定 typed payload。Encounter、RunEvent、Shop、Treasure 和 FloorEntrance 的规则引用只写在匹配字段中。
- Camp 不是节点类型；`bAllowsCamp` 只声明某个已完成节点是否可以成为 Night Camp 的落点。
- Floor Entrance 的 `RequiredCredentialIds` 保存稳定 `FName` 资格要求；`OwnedCardRequirements` 继续保存 Definition/CardId/keyword 筛选。两类条件运行时采用 AND，静态数据不保存持有进度。
- Map validator 要求 Credential requirement 非 `None` 且不重复，并能在入口前置子图中找到不可绕过、配置有效的固定 Pickup grant；它不从实体卡奖励或同名字符串推断 Credential。

这些类型需要 DataAsset、Details 面板和 Blueprint 只读检查，因此使用反射；节点 lifecycle、探索 token、运行时版本和事务结果属于 `WacomRun`。

## §11 RunEvent Definition

`UWacomRunEventDefinition` 是探索事件图的静态定义。RunSession 打开事件时读取当前节点，选择选项时在 working-state 事务中校验条件、应用效果、推进节点或关闭事件。

核心结构：

| 类型 | 语义 |
|---|---|
| `UWacomRunEventDefinition` | `EventId`、标题、起始节点和节点数组 |
| `FWacomRunEventNodeDefinition` | 节点 ID、标题、正文、选项列表 |
| `FWacomRunEventChoiceDefinition` | 选项 ID、按钮文本、条件、效果、下一节点和关闭 / 完成语义 |
| `FWacomRunEventConditionDefinition` | 选项可用条件，例如金币、卡牌、RunFlag、事件完成状态 |
| `FWacomRunEventEffectDefinition` | 选项提交后的效果，例如金币、卡牌、压力和 RunFlag；不再包含节点/行动点效果 |
| `FWacomRunEventCardPaymentDefinition` | 单卡支付筛选和 payment zone authoring |

字段口径：

- `EventId / NodeId / ChoiceId` 是内容 ID，用于事件图和 debug，不替代场景 `PersistentId`。
- RunFlag 使用 `FName FlagId`，不是 Gameplay tag，不做数值或计数。
- 压力类型在 DataAsset 中使用稳定 `FName`，RunSession 执行时转换为运行时压力 ID。
- `CardPayment` 当前是单卡支付合同。支付 UI、drop target 和 menu lease 见 [WacomUI.md](./WacomUI.md) 与 [WacomWorldInteraction.md](./WacomWorldInteraction.md#6-run-menu-zone-target)。
- 选项使用 `ActionPointPolicy = Automatic / Free / Fixed`。Automatic 对 terminal 选项为 1、非 terminal 为 0；`FixedActionPointCost` 只在 Fixed 时使用，正成本选项必须 terminal。
- 条件字段使用 `MinActionPoints`，不再使用旧 NodeCount 术语。行动点属于 Run 事务，不通过 effect 让资产任意扣减。
- 条件和效果的阻断 / warning 口径见 [WacomDataAuthoring.md](./WacomDataAuthoring.md#asset-validation)。

## §12 Battle Effect Structs

卡牌、意图、ZoneHook 和 Passive 使用 Gameplay tag 表达效果类型、目标、条件、区域和触发点。tag 字典见 [WacomGameplayTags.md](./WacomGameplayTags.md)，但 tag 已声明不代表可写入正式资产；可制作范围见 [WacomDataAuthoring.md](./WacomDataAuthoring.md#battle-rule-content-authoring-matrix)。

核心结构：

| 类型 | 静态语义 |
|---|---|
| `FCardEffect` | 卡牌效果条目：EffectType、Magnitude、Target、TargetZone、Duration、MagnitudeSource、Condition、MagnitudeModifiers |
| `FIntentEffect` | 敌人意图效果条目：EffectType、Magnitude、Target、Duration |
| `FEffectCondition` | 效果或 passive 的条件门控 |
| `FCardZoneHook` | 指定手牌区和触发点的额外效果 |
| `FCardPassive` | 被动触发点、旧展示文本、效果、条件和阈值 |

`WacomData` 只定义字段。Battle resolver、dispatcher、validation matrix 和 transient runtime fixture 共同决定这些字段当前是否真正可执行。

`FCardPassive.DisplayText` 是旧展示文本，不是规则真相。正式卡牌详情面板不再把它作为输入；被动详情由 `Trigger / Condition / Effects / TriggerThreshold` 经 WacomApp explanation compiler 生成 semantic blocks/runs。`Passive.Trigger.OnCompanionCount` 的回手说明来自 WacomApp 的 `PassiveOutcomeTemplates`，不要求内容作者在 `Passive.Effects` 里配置不会执行的假效果。

## §13 正式 Floor 1 Production 内容合同

`Floor.Main.01 / 蛇巢浅林` 的 15 个内容节点已完成 Production 内容设计冻结，并已按冻结合同播种为 46 个静态 Production DataAsset。它们继续使用本文件既有 static schema，不新增字段、GameplayTag 或运行时能力。完整资产路径、seed-only 制作边界与验收门禁见 [WacomDataAuthoring.md](./WacomDataAuthoring.md)；本节记录长期规则内容事实。

### SerpentWood 敌人、部位与行为

四个敌人各使用一份 `Default` phase 的 `Sequence` Behavior；每个部位拥有显式 PartSlot/IntentSet。数值记法为 `Damage / Initiative / Resistance`，未写 Resistance 的状态/护盾意图固定为 0，所有 Duration 为 0。

| EnemyId | Part HP / EXP | Sequence Intent contract |
|---|---|---|
| `Enemy.SerpentWood.BrushSnake` | Head `7/1`；Body `9/1` | Head: Bite `3/3/3` → Venom `Poison1/I5`; Body: Rush `2/2/2` → Coil `Slow1/I4` → Hide `Shield2/I2` |
| `Enemy.SerpentWood.MoltGuard` | Head `8/1`；Carapace `14/2`；Tail `6/1` | Head: Snap `4/3/4` → Spit `Poison1/I5`; Carapace: Harden `Shield5/I2` → Slam `4/4/5`; Tail: Sweep `2/2/2` → Brace `Shield2/I2` |
| `Enemy.SerpentWood.RootStalker` | Head `10/2`；Coil `16/2` | Head: Lunge `5/4/5` → Sap `Poison1/I3`; Coil: Tangle `Slow2/I4` → Crush `4/3/4` → RootGuard `Shield3/I2` |
| `Enemy.SerpentWood.ShallowGuardian` | Head `14/2`；Body `22/4`；Tail `10/2`；Crest `6/1` | Head: Bite `6/3/6` → Venom `Poison2/I5`; Body: Crush `6/4/7` → Harden `Shield6/I2`; Tail: Sweep `4/2/4` → Tangle `Slow1/I3`; Crest: Dread `Twilight1/I5` → CrownGuard `Shield4/I2` |

Damage/Poison/Slow/Twilight 均指向 Player，Shield 指向行动部位自身。Slow 使用现有玩家手牌 `Default / TargetCardCount=1` 投递；Twilight 使用现有整手牌语义。所有 11 个正式新部位必须清空 legacy `KnockdownRewardCard`，并按所属敌人显式引用一对 Aid/Destroy 奖励卡。奖励粒度固定为“每个敌人一对”，不是每个部位或节点各建一对；每个部位处理一次击倒选择并获得所选分支的一张独立卡实例，允许同卡重复。

### SerpentWood 击倒分支奖励卡

Aid 固定使用 `Card.Keyword.Tool`，Destroy 固定使用 `Card.Keyword.Weapon`。八张卡都使用零 Physique，无 Swift/Exhaust、PerfectRelease、ZoneHook 或 Passive；Effects 顺序同时决定描述中的 `{Effect.N}`。插画、音效和专用 CardView 表现继续留待资产制作。

| CardId | 名称 | Cost / Rarity | TargetMode | Ordered Effects |
|---|---|---|---|---|
| `Reward.SerpentWood.BrushSnake.Aid` | 伏草藏身 | `1 / White / Tool` | SingleEnemyPart | Shield 2 → Player；Slow 1 → SingleEnemyPart |
| `Reward.SerpentWood.BrushSnake.Destroy` | 断牙毒刺 | `1 / White / Weapon` | SingleEnemyPart | Damage 3；Poison 1 |
| `Reward.SerpentWood.MoltGuard.Aid` | 蜕甲壁垒 | `1 / Blue / Tool` | None | Shield 7 → Player |
| `Reward.SerpentWood.MoltGuard.Destroy` | 裂壳重击 | `1 / Blue / Weapon` | SingleEnemyPart | Damage 6 |
| `Reward.SerpentWood.RootStalker.Aid` | 盘根护身 | `1 / Blue / Tool` | SingleEnemyPart | Shield 3 → Player；Slow 2 → SingleEnemyPart |
| `Reward.SerpentWood.RootStalker.Destroy` | 毒根突袭 | `1 / Blue / Weapon` | SingleEnemyPart | Damage 5；Poison 1 |
| `Reward.SerpentWood.ShallowGuardian.Aid` | 冠鳞庇护 | `1 / Yellow / Tool` | None | Shield 10 → Player |
| `Reward.SerpentWood.ShallowGuardian.Destroy` | 碎冠毒潮 | `2 / Yellow / Weapon` | AllEnemyParts | Damage 4；Poison 1，均作用于所有存活敌方部位 |

描述模板固定为：伏草藏身/盘根护身使用“获得 `{Effect.0}` 护盾，使一个敌方部位的当前意图延后 `{Effect.1}` 点先机”；断牙毒刺/毒根突袭使用“造成 `{Effect.0}` 点伤害并施加 `{Effect.1}` 层中毒”；蜕甲壁垒、冠鳞庇护只描述获得 `{Effect.0}` 护盾；裂壳重击只描述造成 `{Effect.0}` 伤害；碎冠毒潮明确作用于所有存活敌方部位。占位索引不得脱离上表 Effects 顺序。

拟态来源依次为 BrushSnake 的 Hide/Coil 与 Bite/Venom、MoltGuard 的 Harden+Brace 与 Snap/Slam、RootStalker 的 RootGuard+Tangle 与 Lunge/Sap、ShallowGuardian 的 Harden+CrownGuard 与 Sweep/Venom。完整 package、描述模板与十一 Part 映射见 [WacomDataAuthoring.md](./WacomDataAuthoring.md) §4；八张 CardDefinition 作为 Spec 011 的 38 个核心资产之外的增量，已随本组 Production 内容一并落地，总量为 46。

### Encounter 梯度

| EncounterDefinitionId | EnemySlots | Total HP |
|---|---|---:|
| `Encounter.SerpentWood.Scout` | `Scout → BrushSnake` | 16 |
| `Encounter.SerpentWood.MoltGuard` | `Guard → MoltGuard` | 28 |
| `Encounter.SerpentWood.Ambush` | `Left → BrushSnake`, `Right → BrushSnake` | 32 |
| `Encounter.SerpentWood.RootStalker` | `Stalker → RootStalker` | 26 |
| `Encounter.SerpentWood.EliteSentinel` | `Guard → MoltGuard`, `Scout → BrushSnake` | 44 |
| `Encounter.SerpentWood.ShallowGuardian` | `Guardian → ShallowGuardian` | 52 |

战斗梯度固定为 `16 → 26–32 → 44 → 52 HP`，单场最多两个敌人。`bBoss=true` 仍只属于 `Floor.Main.01/Node.Guardian.01` 的 Encounter node payload，不复制到 Encounter 或 Enemy。

### 卡牌、Pickup 与 Shop

四张新卡都使用零 Physique，无 PerfectRelease、ZoneHook 或 Passive；插画和描述不在本轮冻结。

| CardId | Static contract |
|---|---|
| `Reward.SerpentWood.HerbalPoultice` | 草药敷剂；Cost 1；White；Tool；Heal 4 → Player |
| `Reward.SerpentWood.HunterSnare` | 猎人绊索；Cost 1；White；Tool；Slow 2 → SingleEnemyPart |
| `Reward.SerpentWood.MoltWard` | 蜕壳护符；Cost 0；Blue；Tool；Shield 3 → Player |
| `Card.Run.SerpentSigil` | 浅巢蛇印；Cost 1；White；无关键词；Draw 1 → Player from `CardLocation.Draw` |

Pickup 固定映射：

- `Pickup.SerpentWood.HerbCache → Reward.SerpentWood.HerbalPoultice`
- `Pickup.SerpentWood.HunterCache → Reward.SerpentWood.HunterSnare`
- `Pickup.SerpentWood.MoltCache → Reward.SerpentWood.MoltWard`
- `Pickup.SerpentWood.SerpentSigil → Card.Run.SerpentSigil + Credential.Run.SerpentSigil`

`Shop.SerpentWood.Wayfarer` 固定按顺序出售：`Starter.ChitinWard` 2 Gold、`Starter.AntennaSearch` 2、`Starter.MoltCut` 3、现有正式 `PoisonFang` 2、`Reward.SerpentWood.HerbalPoultice` 2。现有毒牙的 live CardId 就是 `PoisonFang`，不重命名、不复制为新的 SerpentWood 卡。

### RunEvent

四个事件均为单节点事件图；全部 13 个选项使用 `ActionPointPolicy=Automatic`，成功时标记当前场景事件完成并关闭，因此每项固定消耗 1 AP。

| EventId | Choice contract |
|---|---|
| `Event.SerpentWood.CastSkin` | `StudyPattern`: Set `SerpentWood.MoltTrailKnown`; `SellSkin`: Gold +2, Misdeed +2; `LeaveUntouched`: none |
| `Event.SerpentWood.HunterTrace` | `ReadTrail`: Set `SerpentWood.MarshRouteKnown`; `LootPack`: Gold +3, Misdeed +3; `BuryRemains`: Misdeed -2 |
| `Event.SerpentWood.MerchantRumor` | `TradeMoltClue`: requires MoltTrailKnown, set MarshRouteKnown; `BuyMap`: MinGold 1, Gold -1, set flag; `Eavesdrop`: Misdeed +2, set flag; `Decline`: none |
| `Event.SerpentWood.PoisonMarsh` | `FollowMarkedRoute`: requires MarshRouteKnown, Fatigue -2; `BurnOffering`: MinGold 2, Gold -2; `WadeThrough`: Fatigue +5 |

`SerpentWood.MoltTrailKnown` 与 `SerpentWood.MarshRouteKnown` 是当前 Run 内的 FName RunFlag，不是 GameplayTag，也不承诺跨 SaveGame 恢复。全部条件、效果、负数恢复和扣费继续由现有 RunEvent working-state 事务原子解释。

上述合同已经落地为 `38 core + 8 knockdown branch reward cards = 46` 个 Production DataAsset：12 Card、4 Behavior、11 EnemyPart、4 Enemy、6 Encounter、4 RunEvent、4 Pickup 与 1 Shop。首次播种通过 exact seed-default、通用 Data Validation、真实加载与 AssetRegistry 计数/类型审计；所有 11 个 Part 使用显式 Aid/Destroy 引用且 legacy 为空。DisplayName、描述、战斗/经济数值与空美术引用仍是可人工调优字段；稳定 ID、class、引用、关键词、TargetMode、effect/condition/choice/slot/intent 的有序结构由制作校验守护。正式世界关卡、背包容量取舍、美术表现和平衡验收继续独立处理。

## §14 修改数据合同时的检查点

修改 DataAsset 字段或新增静态数据能力时，先确认：

- 是否只是静态定义，还是需要 Battle / Run runtime state 新字段。
- 是否需要新增 Gameplay tag；若需要，先在 `WacomCore/Public/Tags/WacomGameplayTags.h` 声明，再更新 [WacomGameplayTags.md](./WacomGameplayTags.md)。
- 是否需要更新 battle content authoring matrix、validator、生成内容 smoke 或 runtime resolver；这些入口见 [WacomDataAuthoring.md](./WacomDataAuthoring.md)。
- 是否影响 Actor authoring / map validation；世界交互侧见 [WacomWorldInteraction.md](./WacomWorldInteraction.md)。
- 是否影响 UI ViewData 或 WBP 绑定；UI 侧见 [WacomUI.md](./WacomUI.md) 和 Binding 文档。

不要为了一个资产临时需求让 `WacomData` 依赖高层模块。需要运行时语义时，在对应领域模块实现，并让 DataAsset 只保存静态配置。
