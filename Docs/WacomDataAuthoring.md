---
type: data-authoring-reference
scope: wacom-data-authoring
status: active
updated: 2026-07-14
tags:
  - wacom/data
  - wacom/authoring
  - wacom/validation
---

# WacomData Authoring 文档

> [!info] 本文职责
> 本文记录 Wacom 静态内容制作、内容生成、资产校验和 Battle rule content authoring matrix。字段语义入口见 [WacomData.md](./WacomData.md)，Gameplay tag catalog 见 [WacomGameplayTags.md](./WacomGameplayTags.md)。

> [!warning] 模块边界
> 内容生成和资产校验位于 `WacomEditor`，自动化验证位于 `WacomTests`。`WacomData` 只保存静态类型，不依赖 Editor、Battle、Run 或 UI。

## §1 Authoring 边界

| 入口 | 职责 | 不是 |
|---|---|---|
| DataAsset 字段 | 保存静态配置和内容 ID | 运行时状态、交易记录、UI 状态 |
| `WacomRegenerateContent` | 重建当前示例 / 调试 DataAsset | 运行时加载入口或策划数据库 |
| Editor Validator | 阻断结构错误和当前规则未接入配置 | 数值平衡、文案质量、剧情合法性校验 |
| Battle authoring matrix | 说明当前可写入 DataAsset 并能被 resolver 执行的规则组合 | 自动开放所有已声明 Gameplay tag |
| 自动化测试 | 防止生成资产、validator 和 runtime resolver 漂移 | 正式内容设计审核 |

静态 Definition 的 `EncounterDefinitionId / ShopId / EventId / PickupId / InteractionId` 是内容识别 ID。场景运行时状态使用 Actor `PersistentId`，不要用静态 ID 替代。

Logical Map 使用另一套稳定身份：`JourneyId`、`FloorId`、`NodeId` 和 `EdgeId`。其中 Node/Edge ID 只要求 Floor 内唯一；跨层记录、校验和运行时结果必须使用带 `FloorId` 的 handle。图连接只能在 Floor DataAsset 中制作，不能从场景 Actor 连线反向生成规则图。

## §2 内容生成 Commandlet

`UWacomRegenerateContentCommandlet` 位于 `Source/WacomEditor/Private/Commandlets/WacomRegenerateContentCommandlet.cpp`，用于重建当前示例 DataAsset。

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -run=WacomRegenerateContent -NoSplash -Unattended
```

Builder 当前职责：

| Builder | 产物 |
|---|---|
| `BuildSnakeContent()` | 蛇敌人、`DA_Behavior_Snake`、三部位、奖励卡 `DA_Card_PoisonFang` |
| `BuildEncounterContent()` | `DA_Encounter_SnakeSingle`，正式单蛇战斗入口样例 |
| `BuildBugGirlContent()` | 虫妹角色、左右手、伙伴初始牌、容器 / 功能卡、starter pack、debug key、卡对卡测试卡、badge 测试卡 |
| `BuildShopContent()` | `DA_Shop_DebugSnake`，正式调试商品保留原价，测试 / 调试卡统一 0 金币 |
| `BuildRunEventContent()` | `DA_Event_DebugSnakeGift`、`DA_Event_DebugFlagReward` |
| `BuildRunPickupDefinitionContent()` | `DA_Pickup_DebugGold3`、`DA_Pickup_DebugPoisonFang` |
| `BuildRunWorldCardInteractionDefinitionContent()` | `DA_RunWorldCardInteraction_DebugKeyGold3` |

改 Builder 后应运行 commandlet 落盘资产，并跑对应 `Wacom.Data.*`、`Wacom.Battle.*` 或 Run/UI smoke。Commandlet 只辅助内容制作，不参与运行时规则。

## §3 当前生成内容

核心角色与卡牌：

| 资产组 | 内容 |
|---|---|
| `DA_Character_BugGirl` | 虫妹角色、左右手卡和正式 StarterDeck；测试 / 调试卡不进入初始牌组 |
| 固有手牌 | `DA_Card_LeftHand`、`DA_Card_RightHand` |
| 伙伴初始牌 | 朝光暮蝶、拂晓飞蛾、赤腹工蚁、烁光蝶、暮蛉 |
| 容器 / 功能卡 | 虫妹的小布袋、蛛茧绒囊、暮色引虫灯 |
| Starter pack | 毒针、几丁护片、触须探路、蜕壳切、轻蜕壳、丝线佯攻 |
| 奖励卡 | `DA_Card_PoisonFang`，当前蛇部位击倒奖励样例 |
| Debug / 测试卡 | DebugKey、卡对卡加费 / 减费 / 弃置 / 消耗、关键词筛选目标卡、按当前费用抽牌测试卡 |
| Badge 测试卡 | Damage / Poison / Shield / Heal 的卡面徽章显示测试卡；Burn 只有 UI 预留，不生成正式测试卡 |

核心敌人、商店与 Run 内容：

| 资产组 | 内容 |
|---|---|
| `DA_Enemy_Snake` | 蛇敌人，包含 Head / Body / Tail 三个部位 |
| `DA_Behavior_Snake` | 蛇行为资产，Default phase 下为 Head / Body / Tail 提供三套 `Sequence` intent set |
| 蛇部位 | Head / Body / Tail 配置 HP、经验和毒牙奖励；部位资产不承载行为，行为统一写入 `DA_Behavior_Snake` |
| `DA_Encounter_SnakeSingle` | 正式单蛇 Encounter 样例，`EncounterDefinitionId=Encounter.Snake.Single`，`EnemySlots[0]=Enemy -> DA_Enemy_Snake` |
| `DA_Shop_DebugSnake` | 固定卖毒牙、部分正式卡、starter pack、debug key、卡对卡测试卡、按当前费用抽牌测试卡和 badge 测试卡 |
| `DA_Event_DebugSnakeGift` | 蛇巢遗物调试事件：获得毒牙、单卡支付交出毒牙、金币 / 压力与显式 Action Point policy |
| `DA_Event_DebugFlagReward` | RunFlag 与 `MinGold + AddGold(-N)` 组合样例 |
| `DA_Pickup_DebugGold3` | 数据驱动金币 PickupDefinition，固定获得 3 金币 |
| `DA_Pickup_DebugPoisonFang` | 数据驱动固定卡牌 PickupDefinition，固定获得毒牙 |
| `DA_RunWorldCardInteraction_DebugKeyGold3` | 通用 Run world card interaction 样例，接受 DebugKey，奖励 3 金币并消耗源卡 |

生成内容测试路径集中在 `FWacomGeneratedBattleContentAssets`。该 helper 只属于 `WacomTests`，不作为运行时加载或策划配置来源。

<a id="asset-validation"></a>
## §4 Asset Validation

Editor Validator 由 `WacomEditor` 注册到 `UEditorValidatorSubsystem`。共享校验函数供编辑器 Validate Assets 和自动化测试复用。

| Validator | 校验对象 | 共享校验函数 |
|---|---|---|
| `UWacomCardDefinitionValidator` | `UCardDefinition` | `FWacomCardDefinitionValidation::Validate()` |
| `UWacomEnemyPartDefinitionValidator` | `UEnemyPartDefinition` | `FWacomEnemyPartDefinitionValidation::Validate()` |
| `UWacomEnemyDefinitionValidator` | `UEnemyDefinition` | `FWacomEnemyDefinitionValidation::Validate()` |
| `UWacomEnemyBehaviorDefinitionValidator` | `UEnemyBehaviorDefinition` | `FWacomEnemyBehaviorDefinitionValidation::Validate()` |
| `UWacomCharacterDefinitionValidator` | `UCharacterDefinition` | `FWacomCharacterDefinitionValidation::Validate()` |
| `UWacomEncounterDefinitionValidator` | `UEncounterDefinition` | `FWacomEncounterDefinitionValidation::Validate()` |
| `UWacomShopDefinitionValidator` | `UShopDefinition` | `FWacomShopDefinitionValidation::Validate()` |
| `UWacomRunEventDefinitionValidator` | `UWacomRunEventDefinition` | `FWacomRunEventDefinitionValidation::Validate()` |
| `UWacomRunPickupDefinitionValidator` | `UWacomRunPickupDefinition` | `FWacomRunPickupDefinitionValidation::Validate()` |
| `UWacomRunWorldCardInteractionDefinitionValidator` | `UWacomRunWorldCardInteractionDefinition` | `FWacomRunWorldCardInteractionDefinitionValidation::Validate()` |
| `UWacomMapDefinitionValidator` | `UWacomJourneyDefinition`、`UWacomFloorMapDefinition` | `FWacomMapDefinitionValidationReport`；检查身份、端点、可达性、typed payload、入口条件、必填 DisplayName 与 MapPosition 制作边界 |

当前校验边界：

- Card / EnemyPart / Enemy / EnemyBehavior / Character 校验 ID、基础数值、必填引用、数组索引、Gameplay tag 命名空间和当前 battle rule content contract。它们不校验文案质量、数值平衡、流派构筑、固定卡组数量或生成资产路径。
- Enemy 校验 `PartSlotId` 必填且不重复，并在配置 `DefaultBehavior / BehaviorOverride / InitialIntentSetId` 时检查对应 phase 和 intent set 是否存在。
- Scene Enemy Host 的制作同步属于 `WacomApp` Actor authoring，不属于 `WacomData` schema 或自动 Validator mutation。内容人员先在 Host 配 `EnemyDefinition`，主要维护稳定 `PartSlotId`，再显式执行 `SyncEnemyPartsFromDefinition()`：唯一匹配槽位的 `PartId` 从 `PartDefinition.PartId` 派生，缺失槽位生成零相对变换的 PartActor ChildActorComponent；已有 transform、`HitBoundsExtent`、`ImpactAnchorRelativeLocation` 和 `VisualLayers` 保留。空、未知或重复槽位只标记 surplus，不自动删除；无效定义槽位跳过并报告。
- Host validator 继续只读校验并给出修复方向：重复 `PartSlotId` 为错误；缺失槽位、PartId 与槽位定义不一致、surplus 部位以及制作模式缺少对应视觉资源为可定位 warning。Validator 不在扫描资产或 Validate Map 时生成、删除或改写 PartActor。
- EnemyBehavior 校验 `BehaviorId`、phase、intent set、intent、selector rule、condition、cooldown authoring 和敌人意图 effect contract；可选传入 owning EnemyDefinition 时，会额外校验 `AppliesToPartSlotId / PartDestroyed` 等部位槽引用。
- Character 会校验 `StarterDeck` 不包含左右手卡。
- Shop 校验 `ShopId`、`Offers`、Offer 卡牌和非负价格；不校验重复商品、价格平衡或商品池规则。
- RunEvent 校验事件图结构、ID、引用、NextNode、卡牌条件 / 效果、卡牌支付筛选和 ZoneId、事件状态目标、RunFlag、压力 ID，以及 `Automatic / Free / Fixed` Action Point policy。正成本非 terminal choice 是错误；早期由 effect 单独扣减探索预算的做法已删除。金币门槛 / 扣费组合中的 authoring 风险可以给 warning。
- RunPickupDefinition 校验固定单一奖励配置：`PickupId`、奖励类型、金币数量或卡牌引用。
- RunWorldCardInteractionDefinition 校验 `InteractionId`、至少一个正向卡牌筛选和有效奖励项。
- Actor 摆放实例的 `PersistentId`、重复 ID、receiver 和 facade 配置属于 map / level validation，见 [WacomWorldInteraction.md](./WacomWorldInteraction.md#2-run-world-interactable-actor)。
- Map Floor 与每个 Node 的 `DisplayName` 必填；`ShortDescription` 可空。`MapPosition` 合法闭区间是 `[0,1920] × [0,1080]`，越界或完全重合为 error，节点中心距离小于 `48 px` 为 warning；这些坐标不参与规则距离或合法性。

不要把 Validator 放进 `WacomData`，否则运行时模块会反向依赖编辑器能力。

Map validation 的 report 和执行器归 `WacomEditor`；transient graph fixtures 与自动化归 `WacomTests`。`WacomData` 只提供可反射的静态 authoring types，不依赖 `WacomRun`、关卡 Actor 或 Editor API。

Run 探索 Debug 内容由 `UWacomBuildRunExplorationAssetsCommandlet` 可重复构建：

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -run=WacomBuildRunExplorationAssets -NoSplash -Unattended
```

命令创建或更新 `/Game/Wacom/Data/Map/DA_Journey_Debug`、`DA_Floor_Debug_01` 和 `/Game/Wacom/Run/Path/Blueprints` 下的 Path / Branch / Anchor 蓝图，编译并保存 `GM_Wacom`、`BP_WacomPlayerCharacter` 与 `L_Exploration` 绑定，再执行 Map 和 loaded-world Scene validation。它只迁移可识别的正式内容引用，不从关卡 Actor 连接反向生成第二份 Logical Map Graph。

Run Map UI 资产由独立命令构建，不修改关卡或 Floor 数据：

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -run=WacomBuildRunMapUIAssets -NoSplash -Unattended
```

它幂等生成并编译 `/Game/Wacom/UI/Map/WBP_RunMapNode` 与 `WBP_RunMapScreen`。应先运行探索 Debug builder 迁移 Floor 文案，再运行 UI builder；两者都要求连续运行两次结果稳定。

## §5 生成内容自动化

| 测试入口 | 目的 |
|---|---|
| `Wacom.Data.GeneratedContent.DefinitionAssetValidation` | 读取生成角色、卡牌、Snake 敌人、Snake Behavior、部位、单蛇 Encounter，并统一跑 DataAsset validator |
| `Wacom.Data.BattleStarterContent.StarterPackAssetValidation` | 检查 starter pack 核心字段、BugGirl 初始牌组排除关系、测试卡不进入 StarterDeck 和 Snake Behavior intent set |
| `Wacom.Data.BattleStarterContent.BadgeDisplayTestCardAssetValidation` | 检查 badge 测试卡、DebugSnake 商店 0 金币出售和 CardPresentationBuilder badge view |
| `Wacom.Data.Shop.DebugSnakeAssetValidation` | 验证 DebugSnake 商店能通过 validator |
| `Wacom.Data.Shop.DebugSnakeAsset` | 验证商品顺序和价格，避免内容生成漂移 |
| `Wacom.Data.RunEvent.DebugSnakeGiftAsset` | 验证蛇巢遗物事件节点、选项、条件、效果和毒牙引用 |
| `Wacom.Data.RunEvent.DebugFlagRewardAsset` | 验证 RunFlag、金币门槛 / 扣费、奖励和 reset flags 样例 |
| `Wacom.Battle.GeneratedStarterContent` | 使用真实生成资产进入 `UBattleSession`，验证核心卡牌、辅助卡和 Snake 意图能产生预期 Snapshot/Event |

这些测试证明“生成资产字段仍符合当前合同”，不是正式内容平衡验收。

<a id="battle-rule-content-authoring-matrix"></a>
## §6 Battle Rule Content Authoring Matrix

卡牌和敌人意图的内容制作以 `FWacomBattleRuleContentContract`、本矩阵、validator 和 runtime fixture 为准。Gameplay tag 已声明不代表可写入正式 DataAsset。

`Wacom.Battle.RuleContentMatrix` 使用 transient Definition：每个代表配置先通过 validator，再提交到 `UBattleSession` 验证 Snapshot/Event。`Reserved` / `EventOnly` 条目只记录制作状态，不代表可制作。

`FCardEffect` 当前字段：

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
    bool bMagnitudeFromRuntimeCost = false; // 旧资产兼容，新增资产应使用 MagnitudeSource
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
3. 否则使用 `Magnitude`。
4. 按数组顺序应用 `MagnitudeModifiers`。

`FCardEffect` 保持兼容性的宽资产 schema，但战斗执行不会把 `TargetZone` 继续作为通用 MetaTag 透传。Private Effect Semantics 在 decode seam 按 EffectType 将它转换为 DrawSource、HandZone、Keyword 或 Status typed 参数；`Magnitude.Source.TargetStatusStacks` 的 Status 读取参数单独进入 magnitude plan。制作校验、正式执行和 Target Preview 均读取同一份 semantic definition。

`FIntentEffect` 额外包含窄类型 `FHandAfflictionDelivery`：

```cpp
USTRUCT(BlueprintType)
struct FHandAfflictionDelivery
{
    EHandAfflictionSelection Selection = Default;
    int32 TargetCardCount = 1;
};
```

它只允许用于 `Target.Player` 的 Slow / Freeze / Twilight。`Magnitude=y` 表示每张卡的状态强度；Slow / Freeze 的 `TargetCardCount=x` 表示随机不重复目标数；Twilight 固定 `AllCurrentHandCards`。不要复用 `Duration` 表示目标数。

卡牌效果矩阵：

| EffectType | Magnitude 语义 | Target 典型值 | TargetZone | Duration | MagnitudeSource | 备注 |
|---|---|---|---|---|---|---|
| `Effect.Damage` | 伤害值 | SingleEnemyPart / AllEnemyParts / Player | - | - | Literal / RuntimeCost / TargetStatusStacks | 部位 HP 归零立即破坏 |
| `Effect.Heal` | 治疗量 | Self(->Player) / Player | - | - | Literal | 恢复玩家 HP，并按规则移除中毒 |
| `Effect.ApplyStatus.Poison` | 层数 | Player / SingleEnemyPart / AllEnemyParts | - | - | Literal / RuntimeCost | 层数模型 |
| `Effect.ApplyStatus.Slow` | 强度 | Player / SingleEnemyPart / AllEnemyParts | - | 0 | Literal | 敌方立即延迟当前意图；玩家创建 Pending Hand Affliction |
| `Effect.ApplyStatus.Freeze` | 层数 | Player / SingleEnemyPart / AllEnemyParts | - | 0 | Literal | 敌方拦截后续卡牌推进；玩家创建 Pending Hand Affliction |
| `Effect.ApplyStatus.Twilight` | 层数 | Player / SingleEnemyPart / AllEnemyParts | - | 0 | Literal | 敌方作用于下一意图；玩家污染下回合整手牌 |
| `Status.Shield` | 护盾值 | Player / Self(->Player) | - | - | Literal | 写 Player Shield；敌方自身护盾只在 Enemy Intent 合同中使用 |
| `Effect.Shuffle.Random` | - | RandomHandCard | - | - | - | 从手牌随机选普通卡腾挪 |
| `Effect.Shuffle.FromBothToOther` | - | ZoneHandCard | HandZone.Both | - | - | 从双手区挑一张腾挪到左 / 右 |
| `Effect.Shuffle.ToRandomZone` | - | Self(本卡) | - | - | - | 把本卡腾挪到随机区域 |
| `Effect.Card.AddCost` | Modifier 增量 | Self / LastShuffledCard / SelectedHandCard | - | - | Literal | 修改 RuntimeCostModifier |
| `Effect.Card.ReduceCost` | Modifier 减量 | 同上 | - | - | Literal | 计算时下限 clamp 到 0 |
| `Effect.Card.DiscardSelected` | 建议填 1 | SelectedHandCard | - | - | Literal | 指定普通手牌进弃牌堆，触发目标卡 `OnDiscard` |
| `Effect.Card.ExhaustSelected` | 建议填 1 | SelectedHandCard | - | - | Literal | 指定普通手牌进消耗牌堆，不触发 `OnDiscard` |
| `Effect.Draw` | 张数 | Self / Player | CardLocation.Draw / Discard / Exhaust | - | Literal / RuntimeCost | `TargetZone` 在 decode seam 转换为 DrawSource |
| `Effect.Discard` | 张数 | Self / Player | - | - | Literal | 随机弃掉普通手牌，不弃锚点 |
| `Effect.ExhaustSelf` | - | Self(本卡) | - | - | - | 通过临时 `Card.Keyword.Exhaust` 交给打出后去向阶段 |
| `Effect.GainKeyword` | - | LastShuffledCard / SelectedHandCard | Card.Keyword.* | - | - | `TargetZone` 在 decode seam 转换为 Keyword 参数 |
| `Effect.RemoveStatus` | 层数 | Player / SingleEnemyPart | Status.Poison / Freeze / Twilight / Stunned | - | Literal / TargetStatusStacks | 不支持 Shield；敌方 Slow 是即时操作，没有可移除层数 |
| `Effect.ModifyInitiative` | 先机增量 | SingleEnemyPart | - | - | Literal / TargetStatusStacks | 正数增加，负数减少 |

`Magnitude.Source.RuntimeCost` 当前允许用于 `Effect.Damage`、`Effect.ApplyStatus.Poison` 和 `Effect.Draw`。用于 `Effect.Draw` 时，抽牌数量等于本卡打出时的 RuntimeCost，例如基础费用 2 的测试卡默认抽 2 张，费用被改变后按改变后的费用抽。`Magnitude.Source.TargetStatusStacks` 当前借用 `TargetZone` 指定要读取的 Combatant 状态，只允许 Poison / Freeze / Twilight / Stunned；敌方 Slow 不保留层数，玩家卡牌 Slow 也不通过 actor-target magnitude 读取。`Status.Shield` 不在 `StatusStacks` 中。

敌人 Intent 效果矩阵：

| EffectType | Target | Magnitude | Duration | 备注 |
|---|---|---:|---:|---|
| `Effect.Damage` | `Target.Player` | > 0 | 0 | 对玩家造成伤害 |
| `Effect.ApplyStatus.Poison` | `Target.Player` 或 `Target.Self` | > 0 | >= 0 | 持久 Combatant 层数 |
| `Effect.ApplyStatus.Slow / Freeze` | `Target.Player` | `y > 0` | 0 | `HandAffliction.TargetCardCount=x>0`，默认 RandomUnique |
| `Effect.ApplyStatus.Twilight` | `Target.Player` | `y > 0` | 0 | 固定作用于下回合抽牌后的当前整手牌 |
| `Effect.ApplyStatus.Slow / Freeze / Twilight` | `Target.Self` | > 0 | 0 | Self 表示行动部位；HandAffliction 必须保持默认 |
| `Status.Shield` | `Target.Self` | > 0 | 0 | 当前敌人 Intent 不给玩家加护盾 |

敌人 Intent 当前不支持手牌目标、全体敌方部位目标、伤害自身、给玩家加护盾、抽弃牌、卡牌专用效果、治疗、移除状态或修改先机。不要依赖运行时静默忽略非法目标。

## §7 Target 与 HandCard 筛选

Target 解析速查：

| Target | 解析为 | TargetInstanceId 来源 | 是否需要 TargetZone |
|---|---|---|---|
| `Target.Self` | Player 或本卡，视 EffectType | 本卡 InstanceId 或 Invalid | 否 |
| `Target.Player` | Player | Invalid | 否 |
| `Target.SingleEnemyPart` | EnemyPart | 调用方选中的部位 | 否 |
| `Target.AllEnemyParts` | EnemyPart，循环展开 | 自动遍历存活部位 | 否 |
| `Target.RandomHandCard` | HandCard | HandZoneService 自选 | 否 |
| `Target.ZoneHandCard` | 延迟选择的 HandCard | Adapter 放行后由对应 Shuffle handler 自选；当前 FromBoth 固定从 Both 选择 | 是 |
| `Target.LastShuffledCard` | HandCard | 当前 Effect Chain 的 `LastShuffledCard` scratch | 否 |
| `Target.SelectedHandCard` | HandCard | `PlayCard` 命令的 `TargetCardInstanceId` | 否 |
| `Target.Adjacent.Right` | EnemyPart | 未实现 | 否 |

`Target.Self` 消歧：

- `Effect.Shuffle.ToRandomZone`、`Effect.Card.AddCost`、`Effect.Card.ReduceCost` 指向本卡。
- `Effect.Card.DiscardSelected`、`Effect.Card.ExhaustSelected` 必须使用 `Target.SelectedHandCard`。
- Damage / Heal / ApplyStatus / Shield 等常规数值效果指向玩家。

HandCard 目标筛选建议：

- AddCost / ReduceCost 作用到 `Target.SelectedHandCard` 时，通常允许普通手牌和左右手锚点。
- DiscardSelected / ExhaustSelected 作用到 `Target.SelectedHandCard` 时，通常只允许普通手牌，拒绝左右手锚点。
- “只作用伙伴”使用 `RequiredTargetKeywords=Card.Keyword.Companion`。
- “不能作用武器”使用 `BlockedTargetKeywords=Card.Keyword.Weapon`。
- 新资产建议显式填写 `HandCardTargetFilter`，避免组合效果变复杂后依赖兼容推断。

## §8 Condition / ZoneHook / Passive

`FEffectCondition`：

```cpp
USTRUCT(BlueprintType)
struct FEffectCondition
{
    FGameplayTag ConditionType;
    FGameplayTag ParamTag;
    int32 ParamInt = 0;
    bool bNegate = false;
};
```

| ConditionType | 语义 | ParamTag | ParamInt |
|---|---|---|---|
| Invalid | 永真 | - | - |
| `Condition.Self.InZone` | 本卡当前在指定区域 | `HandZone.*` | - |
| `Condition.Target.HasStatus` | 目标部位含指定持久状态 | `Status.Poison / Freeze / Twilight / Stunned` | - |

`FCardZoneHook`：

```cpp
USTRUCT(BlueprintType)
struct FCardZoneHook
{
    FGameplayTag Zone;
    FGameplayTag Trigger;
    TArray<FCardEffect> ExtraEffects;
};
```

当前制作触发点是 `ZoneHook.Trigger.OnPlay` 和 `ZoneHook.Trigger.OnPerfectReleaseHit`。

`FCardPassive`：

```cpp
USTRUCT(BlueprintType)
struct FCardPassive
{
    FGameplayTag Trigger;
    FText DisplayText;
    TArray<FCardEffect> Effects;
    FEffectCondition Condition;
    int32 TriggerThreshold = 0;
};
```

| Trigger | 当前制作状态 | 使用 TriggerThreshold? | Effects 是否执行 |
|---|---|---|---|
| `Passive.Trigger.AfterPlayed` | 可执行 | 否 | 是 |
| `Passive.Trigger.OnDiscard` | 可执行 | 否 | 是 |
| `Passive.Trigger.OnCompanionCount` | 特殊回手触发 | 是 | 否 |
| `Passive.Trigger.OnTwilightTriggered` | EventOnly / 展示占位 | 否 | 否 |
| `Passive.Trigger.OnTurnStart` | Reserved | 否 | 否 |
| `Passive.Trigger.OnTurnEnd` | Reserved | 否 | 否 |
| `Passive.Trigger.OnDraw` | Reserved | 否 | 否 |

`DisplayText` 是旧展示文本，不再进入正式卡牌详情面板。被动详情由 `Trigger / Effects / Condition / TriggerThreshold` 通过 WacomApp explanation template 生成；`OnCompanionCount` 的回手结果由 `PassiveOutcomeTemplates` 展示，`Passive.Effects` 仍不会执行。无结构化详情的功能卡可以通过 `UCardDefinition::Description` 获得普通正文回退，但该回退不解析 `{Effect.0}`，也不是规则真相。战斗规则仍只读取结构化字段。

## §9 扩展制作矩阵时的检查点

新增 Effect、Target、MagnitudeSource、Condition 或 Passive trigger 时，同步完成：

1. 运行时 resolver / dispatcher / service 接入。
2. `FWacomBattleRuleContentContract` 放开制作范围。
3. Data validator 错误 / warning 更新。
4. `Wacom.Battle.RuleContentMatrix` transient fixture 覆盖。
5. 真实生成资产 smoke 按需补充。
6. WacomApp UI presentation / explanation template 覆盖；新增 `Magnitude.Source.*` 时同步 `MagnitudeSourceTemplates`。
7. [WacomGameplayTags.md](./WacomGameplayTags.md) 和本文矩阵同步。

如果 validator 允许但 resolver 静默无效，应优先收紧合同或补 resolver，再放开正式制作。
