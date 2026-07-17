# Data Model: 正式 Floor 1 Production 内容合同冻结

## 1. Scope and exact totals

本模型冻结 38 个未来新 DataAsset：

| Category | Count | Role |
|---|---:|---|
| `UEncounterDefinition` | 6 | Floor 1 的 6 个 Encounter 节点 |
| `UWacomRunEventDefinition` | 4 | Floor 1 的 4 个 RunEvent 节点 |
| `UWacomRunPickupDefinition` | 4 | Floor 1 的 4 个 Treasure 节点 |
| `UShopDefinition` | 1 | Floor 1 的 1 个 Shop 节点 |
| `UCardDefinition` | 4 | 3 张路线奖励卡 + 1 张蛇印表现卡 |
| `UEnemyDefinition` | 4 | SerpentWood 敌人原型 |
| `UEnemyBehaviorDefinition` | 4 | 每个敌人一份行为 |
| `UEnemyPartDefinition` | 11 | 四敌人的全部部位 |
| **Total** | **38** | — |

三张已有 Starter 卡与已有正式 PoisonFang 是 Shop 的只读外部依赖，不计入 38 个新资产。

## 2. Enemy archetypes and parts

所有 Enemy：

- `DefaultPhaseId=Default`。
- `DefaultBehavior` 指向同目录同原型 Behavior。
- 每个 `Parts[].PartSlotId` 精确使用下表 PartSlot。
- 每个 `Parts[].InitialIntentSetId` 使用 `<PartId>.Sequence`。
- `BehaviorOverride=null`。

| EnemyId | PartSlot | PartId | MaxHp | EXP | KnockdownRewardCard |
|---|---|---|---:|---:|---|
| `Enemy.SerpentWood.BrushSnake` | `Head` | `SerpentWood.BrushSnake.Head` | 7 | 1 | null |
| `Enemy.SerpentWood.BrushSnake` | `Body` | `SerpentWood.BrushSnake.Body` | 9 | 1 | null |
| `Enemy.SerpentWood.MoltGuard` | `Head` | `SerpentWood.MoltGuard.Head` | 8 | 1 | null |
| `Enemy.SerpentWood.MoltGuard` | `Carapace` | `SerpentWood.MoltGuard.Carapace` | 14 | 2 | null |
| `Enemy.SerpentWood.MoltGuard` | `Tail` | `SerpentWood.MoltGuard.Tail` | 6 | 1 | null |
| `Enemy.SerpentWood.RootStalker` | `Head` | `SerpentWood.RootStalker.Head` | 10 | 2 | null |
| `Enemy.SerpentWood.RootStalker` | `Coil` | `SerpentWood.RootStalker.Coil` | 16 | 2 | null |
| `Enemy.SerpentWood.ShallowGuardian` | `Head` | `SerpentWood.ShallowGuardian.Head` | 14 | 2 | null |
| `Enemy.SerpentWood.ShallowGuardian` | `Body` | `SerpentWood.ShallowGuardian.Body` | 22 | 4 | null |
| `Enemy.SerpentWood.ShallowGuardian` | `Tail` | `SerpentWood.ShallowGuardian.Tail` | 10 | 2 | null |
| `Enemy.SerpentWood.ShallowGuardian` | `Crest` | `SerpentWood.ShallowGuardian.Crest` | 6 | 1 | null |

Enemy total HP/EXP：

| EnemyId | Total HP | Total EXP |
|---|---:|---:|
| `Enemy.SerpentWood.BrushSnake` | 16 | 2 |
| `Enemy.SerpentWood.MoltGuard` | 28 | 4 |
| `Enemy.SerpentWood.RootStalker` | 26 | 4 |
| `Enemy.SerpentWood.ShallowGuardian` | 52 | 9 |

## 3. Enemy behaviors and intents

行为 ID：

| Enemy | BehaviorId | InitialPhaseId | SelectorMode |
|---|---|---|---|
| BrushSnake | `SerpentWood.BrushSnake.Behavior` | `Default` | all sets `Sequence` |
| MoltGuard | `SerpentWood.MoltGuard.Behavior` | `Default` | all sets `Sequence` |
| RootStalker | `SerpentWood.RootStalker.Behavior` | `Default` | all sets `Sequence` |
| ShallowGuardian | `SerpentWood.ShallowGuardian.Behavior` | `Default` | all sets `Sequence` |

所有 Intent：`CooldownSelections=0`、无 SelectorRules、无 FallbackIntentId、所有 Effect `Duration=0`。伤害项的 `ResistanceValue` 使用表中 R；状态/护盾项 R=0。

| Archetype.Part | IntentId suffix | Display role | I | R | EffectType | Magnitude | Target | HandAffliction |
|---|---|---|---:|---:|---|---:|---|---|
| BrushSnake.Head | `Bite` | Bite | 3 | 3 | `Effect.Damage` | 3 | `Target.Player` | default |
| BrushSnake.Head | `Venom` | Venom | 5 | 0 | `Effect.ApplyStatus.Poison` | 1 | `Target.Player` | default |
| BrushSnake.Body | `Rush` | Rush | 2 | 2 | `Effect.Damage` | 2 | `Target.Player` | default |
| BrushSnake.Body | `Coil` | Coil | 4 | 0 | `Effect.ApplyStatus.Slow` | 1 | `Target.Player` | Default / 1 card |
| BrushSnake.Body | `Hide` | Hide | 2 | 0 | `Status.Shield` | 2 | `Target.Self` | default |
| MoltGuard.Head | `Snap` | Snap | 3 | 4 | `Effect.Damage` | 4 | `Target.Player` | default |
| MoltGuard.Head | `Spit` | Spit | 5 | 0 | `Effect.ApplyStatus.Poison` | 1 | `Target.Player` | default |
| MoltGuard.Carapace | `Harden` | Harden | 2 | 0 | `Status.Shield` | 5 | `Target.Self` | default |
| MoltGuard.Carapace | `Slam` | Slam | 4 | 5 | `Effect.Damage` | 4 | `Target.Player` | default |
| MoltGuard.Tail | `Sweep` | Sweep | 2 | 2 | `Effect.Damage` | 2 | `Target.Player` | default |
| MoltGuard.Tail | `Brace` | Brace | 2 | 0 | `Status.Shield` | 2 | `Target.Self` | default |
| RootStalker.Head | `Lunge` | Lunge | 4 | 5 | `Effect.Damage` | 5 | `Target.Player` | default |
| RootStalker.Head | `Sap` | Sap | 3 | 0 | `Effect.ApplyStatus.Poison` | 1 | `Target.Player` | default |
| RootStalker.Coil | `Tangle` | Tangle | 4 | 0 | `Effect.ApplyStatus.Slow` | 2 | `Target.Player` | Default / 1 card |
| RootStalker.Coil | `Crush` | Crush | 3 | 4 | `Effect.Damage` | 4 | `Target.Player` | default |
| RootStalker.Coil | `RootGuard` | RootGuard | 2 | 0 | `Status.Shield` | 3 | `Target.Self` | default |
| ShallowGuardian.Head | `Bite` | Bite | 3 | 6 | `Effect.Damage` | 6 | `Target.Player` | default |
| ShallowGuardian.Head | `Venom` | Venom | 5 | 0 | `Effect.ApplyStatus.Poison` | 2 | `Target.Player` | default |
| ShallowGuardian.Body | `Crush` | Crush | 4 | 7 | `Effect.Damage` | 6 | `Target.Player` | default |
| ShallowGuardian.Body | `Harden` | Harden | 2 | 0 | `Status.Shield` | 6 | `Target.Self` | default |
| ShallowGuardian.Tail | `Sweep` | Sweep | 2 | 4 | `Effect.Damage` | 4 | `Target.Player` | default |
| ShallowGuardian.Tail | `Tangle` | Tangle | 3 | 0 | `Effect.ApplyStatus.Slow` | 1 | `Target.Player` | Default / 1 card |
| ShallowGuardian.Crest | `Dread` | Dread | 5 | 0 | `Effect.ApplyStatus.Twilight` | 1 | `Target.Player` | Default / all current hand |
| ShallowGuardian.Crest | `CrownGuard` | CrownGuard | 2 | 0 | `Status.Shield` | 4 | `Target.Self` | default |

完整 IntentId 由 `SerpentWood.<Archetype>.<Part>.<Suffix>` 组成；IntentSetId 由 `SerpentWood.<Archetype>.<Part>.Sequence` 组成，`AppliesToPartSlotId=<Part>`。

## 4. Encounter definitions

| EncounterDefinitionId | Display role | EnemySlots in authored order | Total HP | Enemy count |
|---|---|---|---:|---:|
| `Encounter.SerpentWood.Scout` | 教学伏击 | `Scout → Enemy.SerpentWood.BrushSnake` | 16 | 1 |
| `Encounter.SerpentWood.MoltGuard` | 蛇蜕守卫 | `Guard → Enemy.SerpentWood.MoltGuard` | 28 | 1 |
| `Encounter.SerpentWood.Ambush` | 毒雾伏击 | `Left → BrushSnake`, `Right → BrushSnake` | 32 | 2 |
| `Encounter.SerpentWood.RootStalker` | 盘根伏蛇 | `Stalker → Enemy.SerpentWood.RootStalker` | 26 | 1 |
| `Encounter.SerpentWood.EliteSentinel` | 精英巡猎者 | `Guard → MoltGuard`, `Scout → BrushSnake` | 44 | 2 |
| `Encounter.SerpentWood.ShallowGuardian` | 浅巢守卫 | `Guardian → Enemy.SerpentWood.ShallowGuardian` | 52 | 1 |

`Node.Guardian.01` 的 `bBoss=true` 继续属于 `FWacomMapEncounterNodeContent`，不进入 Encounter/Enemy schema。

## 5. Card definitions

所有新卡：`Physique={0,0,0,None}`，`PerfectReleaseEffects/ZoneHooks/Passives` 为空；本轮不冻结插画、深度图或描述文案。

| CardId | DisplayName | Cost | Rarity | Keywords | TargetMode | Single effect |
|---|---|---:|---|---|---|---|
| `Reward.SerpentWood.HerbalPoultice` | 草药敷剂 | 1 | `Card.Rarity.White` | `Card.Keyword.Tool` | `None` | `Effect.Heal`, 4, `Target.Player` |
| `Reward.SerpentWood.HunterSnare` | 猎人绊索 | 1 | `Card.Rarity.White` | `Card.Keyword.Tool` | `SingleEnemyPart` | `Effect.ApplyStatus.Slow`, 2, `Target.SingleEnemyPart`, Duration 0 |
| `Reward.SerpentWood.MoltWard` | 蜕壳护符 | 0 | `Card.Rarity.Blue` | `Card.Keyword.Tool` | `None` | `Status.Shield`, 3, `Target.Player` |
| `Card.Run.SerpentSigil` | 浅巢蛇印 | 1 | `Card.Rarity.White` | empty | `None` | `Effect.Draw`, 1, `Target.Player`, `TargetZone=CardLocation.Draw` |

## 6. Pickup definitions

全部 `RewardType=Card`、`GoldAmount` 不参与运行时；每个 Definition 只配置一张固定 Card。

| PickupId | CardDefinition | GrantedCredentialIds |
|---|---|---|
| `Pickup.SerpentWood.HerbCache` | `Reward.SerpentWood.HerbalPoultice` | empty |
| `Pickup.SerpentWood.HunterCache` | `Reward.SerpentWood.HunterSnare` | empty |
| `Pickup.SerpentWood.MoltCache` | `Reward.SerpentWood.MoltWard` | empty |
| `Pickup.SerpentWood.SerpentSigil` | `Card.Run.SerpentSigil` | `Credential.Run.SerpentSigil` |

## 7. Shop definition

`ShopId=Shop.SerpentWood.Wayfarer`，固定五个 Offer，顺序冻结如下：

| # | CardId | Price | Asset status |
|---:|---|---:|---|
| 1 | `Starter.ChitinWard` | 2 | existing formal Starter card |
| 2 | `Starter.AntennaSearch` | 2 | existing formal Starter card |
| 3 | `Starter.MoltCut` | 3 | existing formal Starter card |
| 4 | `PoisonFang` | 2 | existing formal reward card |
| 5 | `Reward.SerpentWood.HerbalPoultice` | 2 | new in manifest |

Shop 不配置随机池、权重、动态价格或重复 Offer。浏览/空手离开 0 AP；本次访问首次成功购买 1 AP 的运行时规则不变。

## 8. RunEvent definitions

所有 Event：

- `StartNodeId=Start`，`Nodes` 恰好 1 个。
- 每个 Choice `ActionPointPolicy=Automatic`、`NextNodeId=None`、`bMarkEventCompleted=true`、`bCloseEventAfterResolve=true`。
- 无 CardPayment；无 GainCard/RemoveCard/MarkEventCompleted effect 条目。
- Choice 自身完成标记当前场景 PersistentId，不在 effect 中手写 TargetPersistentId。

| EventId | ChoiceId | Conditions | Effects |
|---|---|---|---|
| `Event.SerpentWood.CastSkin` | `StudyPattern` | none | `SetRunFlag(SerpentWood.MoltTrailKnown)` |
|  | `SellSkin` | none | `AddGold(+2)`, `AddPressure(Misdeed,+2)` |
|  | `LeaveUntouched` | none | none |
| `Event.SerpentWood.HunterTrace` | `ReadTrail` | none | `SetRunFlag(SerpentWood.MarshRouteKnown)` |
|  | `LootPack` | none | `AddGold(+3)`, `AddPressure(Misdeed,+3)` |
|  | `BuryRemains` | none | `AddPressure(Misdeed,-2)` |
| `Event.SerpentWood.MerchantRumor` | `TradeMoltClue` | `RunFlagSet(SerpentWood.MoltTrailKnown)` | `SetRunFlag(SerpentWood.MarshRouteKnown)` |
|  | `BuyMap` | `MinGold(1)` | `AddGold(-1)`, `SetRunFlag(SerpentWood.MarshRouteKnown)` |
|  | `Eavesdrop` | none | `AddPressure(Misdeed,+2)`, `SetRunFlag(SerpentWood.MarshRouteKnown)` |
|  | `Decline` | none | none |
| `Event.SerpentWood.PoisonMarsh` | `FollowMarkedRoute` | `RunFlagSet(SerpentWood.MarshRouteKnown)` | `AddPressure(Fatigue,-2)` |
|  | `BurnOffering` | `MinGold(2)` | `AddGold(-2)` |
|  | `WadeThrough` | none | `AddPressure(Fatigue,+5)` |

Choice 总数为 13。`Misdeed` 与 `Fatigue` 是当前 RunEvent executor 接受的稳定 FName pressure IDs。两个 RunFlag 当前仅存在于本次 Run 内存态，不进入 SaveGame v5。

## 9. Node binding registry

| Floor Node | Definition |
|---|---|
| `Node.Main.01` | `Encounter.SerpentWood.Scout` |
| `Node.Route.A.01` | `Event.SerpentWood.CastSkin` |
| `Node.Route.A.02` | `Encounter.SerpentWood.MoltGuard` |
| `Node.Route.A.03` | `Pickup.SerpentWood.HerbCache` |
| `Node.Route.B.01` | `Encounter.SerpentWood.Ambush` |
| `Node.Route.B.02` | `Pickup.SerpentWood.HunterCache` |
| `Node.Route.B.03` | `Event.SerpentWood.HunterTrace` |
| `Node.Route.C.01` | `Shop.SerpentWood.Wayfarer` |
| `Node.Route.C.02` | `Event.SerpentWood.MerchantRumor` |
| `Node.Route.D.01` | `Encounter.SerpentWood.RootStalker` |
| `Node.Route.D.02` | `Event.SerpentWood.PoisonMarsh` |
| `Node.Route.D.03` | `Pickup.SerpentWood.MoltCache` |
| `Node.Key.01` | `Pickup.SerpentWood.SerpentSigil` |
| `Node.Main.02` | `Encounter.SerpentWood.EliteSentinel` |
| `Node.Guardian.01` | `Encounter.SerpentWood.ShallowGuardian` |

分类精确为 `6 Encounter / 4 RunEvent / 4 Pickup / 1 Shop`。

## 10. Economy and AP invariants

Route A 购买路径：

```text
CastSkin.SellSkin: 0 + 2 Gold
Wayfarer: buy ChitinWard/AntennaSearch/PoisonFang/HerbalPoultice for 2
```

Route B 购买路径：

```text
HunterTrace.LootPack: 0 + 3 Gold
Wayfarer: buy any 2-Gold offer or MoltCut for 3
```

所有事件终结选项固定 1 AP。Floor 1 AP 不变：

| Route scope | No purchase | First successful purchase |
|---|---:|---:|
| Shortest progression | 8 | 9 |
| Full exploration | 14 | 15 |

## 11. Frozen vs mutable facts

| Fact | Status |
|---|---|
| 38 asset identities and paths | Frozen future authoring contract |
| Enemy/Part/Behavior/Intent IDs and numbers | Frozen |
| Encounter slots/order and HP totals | Frozen |
| Card fields/effects | Frozen |
| Pickup/Shop/Event fields | Frozen |
| Node-to-Definition mapping | Frozen, inherited from Floor 1 graph |
| Display descriptions and Event prose | Mutable without changing rules |
| Card/enemy art, audio, material, animation style | Deferred |
| Host Blueprint, SceneEnemyHost layout, world Transform | Deferred |
| Knockdown Aid/Destroy/Withdraw effects | Blocked P0, not frozen as empty |
| Balance changes after playtest | Require a separate content revision, not identity migration by default |

## 12. Runtime and persistence impact

This slice creates no runtime state. Future assets will be interpreted by existing contracts:

- Battle builds enemies/intents from definitions and Encounter slots.
- Run executes fixed Pickup, Shop and RunEvent transactions.
- Credential grant remains durable independently of the SerpentSigil card.
- RunFlags remain in-memory and are not promised across SaveGame restore.
- No new Snapshot, Command, Event, Result, SaveGame field or GameplayTag is required.
