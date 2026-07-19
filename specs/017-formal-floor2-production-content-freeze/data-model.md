# Data Model: 正式 Floor 2 Production 内容合同冻结

## 1. Scope and exact totals

本模型冻结 47 个未来新 DataAsset：

| Category | Count | Role |
|---|---:|---|
| `UEnemyDefinition` | 4 | MoltCavern 敌人原型 |
| `UEnemyBehaviorDefinition` | 4 | 每个敌人一份行为 |
| `UEnemyPartDefinition` | 12 | 四敌人的全部部位 |
| `UEncounterDefinition` | 7 | Floor 2 的 7 个 Encounter 节点 |
| `UWacomRunEventDefinition` | 3 | Floor 2 的 3 个 RunEvent 节点 |
| `UWacomRunPickupDefinition` | 4 | Floor 2 的 4 个 Treasure 节点 |
| `UShopDefinition` | 1 | Floor 2 的 1 个 Shop 节点 |
| `UCardDefinition` | 12 | 4 张固定 Pickup/Run 卡 + 8 张击倒分支卡 |
| **Total** | **47** | — |

DeepWayfarer 复用三张已有正式卡，不计入 47 个新资产：`Reward.SerpentWood.HerbalPoultice`、`Starter.ChitinWard`、`Starter.MoltCut`。

## 2. Enemy archetypes and parts

所有 Enemy：

- `DefaultPhaseId=Default`。
- `DefaultBehavior` 指向同目录同 Archetype Behavior。
- `Parts[].PartSlotId` 使用下表 Part 名称。
- `Parts[].InitialIntentSetId=<PartId>.Sequence`。
- `BehaviorOverride=null`。

所有 Part：

- `AidRewardCard` 与 `DestroyRewardCard` 指向所属 Archetype 的显式卡对。
- deprecated `KnockdownRewardCard=null`。
- DisplayName 可继续调优，不参与 stable identity。

| EnemyId | PartSlot | PartId | MaxHp | EXP | Aid/Destroy pair |
|---|---|---|---:|---:|---|
| `Enemy.MoltCavern.ScaleCrawler` | Head | `MoltCavern.ScaleCrawler.Head` | 9 | 1 | `Reward.MoltCavern.ScaleCrawler.Aid/Destroy` |
| `Enemy.MoltCavern.ScaleCrawler` | Body | `MoltCavern.ScaleCrawler.Body` | 12 | 1 | `Reward.MoltCavern.ScaleCrawler.Aid/Destroy` |
| `Enemy.MoltCavern.StoneScaleGuard` | Head | `MoltCavern.StoneScaleGuard.Head` | 10 | 1 | `Reward.MoltCavern.StoneScaleGuard.Aid/Destroy` |
| `Enemy.MoltCavern.StoneScaleGuard` | Carapace | `MoltCavern.StoneScaleGuard.Carapace` | 18 | 2 | `Reward.MoltCavern.StoneScaleGuard.Aid/Destroy` |
| `Enemy.MoltCavern.StoneScaleGuard` | Tail | `MoltCavern.StoneScaleGuard.Tail` | 8 | 1 | `Reward.MoltCavern.StoneScaleGuard.Aid/Destroy` |
| `Enemy.MoltCavern.VenomHunter` | Head | `MoltCavern.VenomHunter.Head` | 12 | 2 | `Reward.MoltCavern.VenomHunter.Aid/Destroy` |
| `Enemy.MoltCavern.VenomHunter` | Coil | `MoltCavern.VenomHunter.Coil` | 15 | 2 | `Reward.MoltCavern.VenomHunter.Aid/Destroy` |
| `Enemy.MoltCavern.VenomHunter` | VenomSac | `MoltCavern.VenomHunter.VenomSac` | 7 | 1 | `Reward.MoltCavern.VenomHunter.Aid/Destroy` |
| `Enemy.MoltCavern.CavernGuardian` | Head | `MoltCavern.CavernGuardian.Head` | 16 | 2 | `Reward.MoltCavern.CavernGuardian.Aid/Destroy` |
| `Enemy.MoltCavern.CavernGuardian` | Body | `MoltCavern.CavernGuardian.Body` | 28 | 4 | `Reward.MoltCavern.CavernGuardian.Aid/Destroy` |
| `Enemy.MoltCavern.CavernGuardian` | Tail | `MoltCavern.CavernGuardian.Tail` | 14 | 2 | `Reward.MoltCavern.CavernGuardian.Aid/Destroy` |
| `Enemy.MoltCavern.CavernGuardian` | MoltCore | `MoltCavern.CavernGuardian.MoltCore` | 12 | 2 | `Reward.MoltCavern.CavernGuardian.Aid/Destroy` |

| EnemyId | Total HP | Total EXP | Part count |
|---|---:|---:|---:|
| `Enemy.MoltCavern.ScaleCrawler` | 21 | 2 | 2 |
| `Enemy.MoltCavern.StoneScaleGuard` | 36 | 4 | 3 |
| `Enemy.MoltCavern.VenomHunter` | 34 | 5 | 3 |
| `Enemy.MoltCavern.CavernGuardian` | 70 | 10 | 4 |

## 3. Behavior and Intent model

ID 规则：

```text
BehaviorId = MoltCavern.<Archetype>.Behavior
PartId     = MoltCavern.<Archetype>.<Part>
IntentSet  = MoltCavern.<Archetype>.<Part>.Sequence
IntentId   = MoltCavern.<Archetype>.<Part>.<Intent>
```

每个 Behavior 只有 `Default` phase；每个 IntentSet 使用 `Sequence`、`CooldownSelections=0`、无 selector rules、无 fallback。Damage Intent 使用冻结 Resistance；状态与 Shield 的 Resistance 为 0。Slow 使用当前 `Default / TargetCardCount=1` 手牌投递。Damage/Poison/Slow target 为 `Target.Player`；Shield target 为行动部位自身 `Target.Self`。

| Archetype.Part | Intent suffix | I | R | Effect | Magnitude | Target | HandAffliction |
|---|---|---:|---:|---|---:|---|---|
| ScaleCrawler.Head | StoneBite | 3 | 4 | `Effect.Damage` | 4 | Player | default |
| ScaleCrawler.Head | Venom | 5 | 0 | `Effect.ApplyStatus.Poison` | 1 | Player | default |
| ScaleCrawler.Body | Skitter | 2 | 3 | `Effect.Damage` | 3 | Player | default |
| ScaleCrawler.Body | Castoff | 2 | 0 | `Status.Shield` | 3 | Self | default |
| ScaleCrawler.Body | Coil | 4 | 0 | `Effect.ApplyStatus.Slow` | 1 | Player | Default / 1 card |
| StoneScaleGuard.Head | CrushBite | 3 | 5 | `Effect.Damage` | 5 | Player | default |
| StoneScaleGuard.Head | DustSpit | 5 | 0 | `Effect.ApplyStatus.Poison` | 1 | Player | default |
| StoneScaleGuard.Carapace | LithicHarden | 2 | 0 | `Status.Shield` | 7 | Self | default |
| StoneScaleGuard.Carapace | Ram | 4 | 6 | `Effect.Damage` | 5 | Player | default |
| StoneScaleGuard.Tail | Sweep | 2 | 3 | `Effect.Damage` | 3 | Player | default |
| StoneScaleGuard.Tail | Brace | 2 | 0 | `Status.Shield` | 3 | Self | default |
| VenomHunter.Head | Pounce | 4 | 6 | `Effect.Damage` | 6 | Player | default |
| VenomHunter.Head | Fang | 5 | 0 | `Effect.ApplyStatus.Poison` | 2 | Player | default |
| VenomHunter.Coil | Bind | 4 | 0 | `Effect.ApplyStatus.Slow` | 2 | Player | Default / 1 card |
| VenomHunter.Coil | Crush | 3 | 5 | `Effect.Damage` | 5 | Player | default |
| VenomHunter.Coil | Veil | 2 | 0 | `Status.Shield` | 4 | Self | default |
| VenomHunter.VenomSac | VenomBurst | 5 | 0 | `Effect.ApplyStatus.Poison` | 2 | Player | default |
| VenomHunter.VenomSac | GuardSac | 2 | 0 | `Status.Shield` | 3 | Self | default |
| CavernGuardian.Head | DeepBite | 3 | 7 | `Effect.Damage` | 7 | Player | default |
| CavernGuardian.Head | VenomFlood | 5 | 0 | `Effect.ApplyStatus.Poison` | 2 | Player | default |
| CavernGuardian.Body | CaveCrush | 4 | 8 | `Effect.Damage` | 8 | Player | default |
| CavernGuardian.Body | MoltWall | 2 | 0 | `Status.Shield` | 9 | Self | default |
| CavernGuardian.Tail | RockSweep | 2 | 5 | `Effect.Damage` | 5 | Player | default |
| CavernGuardian.Tail | Pin | 4 | 0 | `Effect.ApplyStatus.Slow` | 2 | Player | Default / 1 card |
| CavernGuardian.MoltCore | CorePulse | 5 | 0 | `Effect.ApplyStatus.Poison` | 2 | Player | default |
| CavernGuardian.MoltCore | ShedWard | 2 | 0 | `Status.Shield` | 6 | Self | default |

Intent 总数：`5 + 6 + 7 + 8 = 26`。

## 4. Encounter definitions

| EncounterDefinitionId | EnemySlots in authored order | Total HP | Enemy count |
|---|---|---:|---:|
| `Encounter.MoltCavern.ScaleScout` | `Scout → Enemy.MoltCavern.ScaleCrawler` | 21 | 1 |
| `Encounter.MoltCavern.StoneScaleGuard` | `Guard → Enemy.MoltCavern.StoneScaleGuard` | 36 | 1 |
| `Encounter.MoltCavern.HatcheryAmbush` | `Left → ScaleCrawler`, `Right → ScaleCrawler` | 42 | 2 |
| `Encounter.MoltCavern.BridgeSentinel` | `Sentinel → Enemy.MoltCavern.StoneScaleGuard` | 36 | 1 |
| `Encounter.MoltCavern.VenomHunter` | `Hunter → Enemy.MoltCavern.VenomHunter` | 34 | 1 |
| `Encounter.MoltCavern.EliteMolter` | `Guard → StoneScaleGuard`, `Scout → ScaleCrawler` | 57 | 2 |
| `Encounter.MoltCavern.CavernGuardian` | `Guardian → Enemy.MoltCavern.CavernGuardian` | 70 | 1 |

战斗梯度按路径阶段为 `21 → 34–42 → 36 → 57 → 70`。`Node.Guardian.01.bBoss=true` 继续只属于 Floor node payload。

## 5. Card definitions

所有十二张卡：`Physique={0,0,0,None}`，`PerfectReleaseEffects`、`ZoneHooks`、`Passives` 为空；无 Swift 或 Exhaust。Card effect 中的 Shield 指向 Player。

### 5.1 Fixed Pickup/Run cards

| CardId | DisplayName | Cost | Rarity | Keyword | TargetMode | Effects in order |
|---|---|---:|---|---|---|---|
| `Reward.MoltCavern.GlowcapPoultice` | 菌光药膏 | 1 | Blue | Tool | None | `Heal 6 → Player` |
| `Reward.MoltCavern.CrystalWard` | 晶脉护符 | 0 | Blue | Tool | None | `Shield 5 → Player` |
| `Reward.MoltCavern.VenomShard` | 毒晶尖刺 | 1 | Blue | Weapon | SingleEnemyPart | `Damage 4 → target`; `Poison 2 → target` |
| `Card.Run.MoltSeal` | 深窟蜕印 | 1 | Blue | empty | None | `Draw 2 → Player`, `TargetZone=CardLocation.Draw` |

Seed description templates：

| CardId | Description template |
|---|---|
| `Reward.MoltCavern.GlowcapPoultice` | `恢复 {Effect.0} 点生命。` |
| `Reward.MoltCavern.CrystalWard` | `获得 {Effect.0} 点护盾。` |
| `Reward.MoltCavern.VenomShard` | `造成 {Effect.0} 点伤害，并施加 {Effect.1} 层中毒。` |
| `Card.Run.MoltSeal` | `从抽牌堆抽取 {Effect.0} 张牌。` |

### 5.2 Knockdown branch cards

| CardId | DisplayName | Cost | Rarity | Keyword | TargetMode | Effects in order |
|---|---|---:|---|---|---|---|
| `Reward.MoltCavern.ScaleCrawler.Aid` | 鳞影潜行 | 1 | Blue | Tool | SingleEnemyPart | `Shield 3 → Player`; `Slow 1 → target` |
| `Reward.MoltCavern.ScaleCrawler.Destroy` | 裂鳞毒牙 | 1 | Blue | Weapon | SingleEnemyPart | `Damage 4 → target`; `Poison 1 → target` |
| `Reward.MoltCavern.StoneScaleGuard.Aid` | 石甲壁垒 | 1 | Blue | Tool | None | `Shield 9 → Player` |
| `Reward.MoltCavern.StoneScaleGuard.Destroy` | 崩岩重击 | 1 | Blue | Weapon | SingleEnemyPart | `Damage 7 → target` |
| `Reward.MoltCavern.VenomHunter.Aid` | 毒泉缠守 | 1 | Blue | Tool | SingleEnemyPart | `Shield 4 → Player`; `Slow 2 → target` |
| `Reward.MoltCavern.VenomHunter.Destroy` | 猎毒突刺 | 1 | Blue | Weapon | SingleEnemyPart | `Damage 6 → target`; `Poison 2 → target` |
| `Reward.MoltCavern.CavernGuardian.Aid` | 洞壳庇护 | 1 | Yellow | Tool | None | `Shield 13 → Player` |
| `Reward.MoltCavern.CavernGuardian.Destroy` | 碎窟毒潮 | 2 | Yellow | Weapon | AllEnemyParts | `Damage 5 → all living enemy parts`; `Poison 2 → all living enemy parts` |

Description templates：

| Card suffix | Description template |
|---|---|
| `ScaleCrawler.Aid` | `获得 {Effect.0} 点护盾，并使一个敌方部位的当前意图延后 {Effect.1} 点先机。` |
| `ScaleCrawler.Destroy` | `造成 {Effect.0} 点伤害，并施加 {Effect.1} 层中毒。` |
| `StoneScaleGuard.Aid` | `获得 {Effect.0} 点护盾。` |
| `StoneScaleGuard.Destroy` | `造成 {Effect.0} 点伤害。` |
| `VenomHunter.Aid` | `获得 {Effect.0} 点护盾，并使一个敌方部位的当前意图延后 {Effect.1} 点先机。` |
| `VenomHunter.Destroy` | `造成 {Effect.0} 点伤害，并施加 {Effect.1} 层中毒。` |
| `CavernGuardian.Aid` | `获得 {Effect.0} 点护盾。` |
| `CavernGuardian.Destroy` | `对所有存活敌方部位造成 {Effect.0} 点伤害，并施加 {Effect.1} 层中毒。` |

## 6. Pickup definitions

全部使用固定 Card reward。MoltSeal 的 Card 与 Credential 在现有 Pickup working-state 事务中共同授予。

| PickupId | CardDefinition | GrantedCredentialIds |
|---|---|---|
| `Pickup.MoltCavern.FungalCache` | `Reward.MoltCavern.GlowcapPoultice` | empty |
| `Pickup.MoltCavern.MineralCache` | `Reward.MoltCavern.CrystalWard` | empty |
| `Pickup.MoltCavern.VenomCrystalCache` | `Reward.MoltCavern.VenomShard` | empty |
| `Pickup.MoltCavern.MoltSeal` | `Card.Run.MoltSeal` | `Credential.Run.MoltSeal` |

## 7. Shop definition

`ShopId=Shop.MoltCavern.DeepWayfarer`，固定五个 Offer，顺序冻结：

| # | CardId | Price | Asset status |
|---:|---|---:|---|
| 1 | `Reward.SerpentWood.HerbalPoultice` | 3 | existing formal Floor 1 card; read-only |
| 2 | `Starter.ChitinWard` | 3 | existing formal Starter card; read-only |
| 3 | `Starter.MoltCut` | 4 | existing formal Starter card; read-only |
| 4 | `Reward.MoltCavern.GlowcapPoultice` | 4 | new in manifest |
| 5 | `Reward.MoltCavern.VenomShard` | 5 | new in manifest |

不配置随机池、权重、动态价格或重复 Offer。浏览/空手离开 0 AP；本次访问首次成功购买 1 AP。Gold、卡组和压力继续跨 Floor 累计。

## 8. RunEvent definitions

所有 Event：

- `StartNodeId=Start`，恰好一个 Node。
- Choice 均为 `ActionPointPolicy=Automatic`、`NextNodeId=None`、`bMarkEventCompleted=true`、`bCloseEventAfterResolve=true`。
- 无 CardPayment，不使用新 condition/effect 类型。
- Flag 是本 Run 内存态 FName，不是 GameplayTag 或 SaveGame 承诺。

| EventId | ChoiceId | Conditions | Effects in order |
|---|---|---|---|
| `Event.MoltCavern.CastoffEcho` | ReadRitePattern | none | `SetRunFlag(MoltCavern.RitePatternKnown)` |
|  | GatherScaleDust | none | `AddGold(+3)`; `AddPressure(Misdeed,+2)` |
|  | RestAmongCastoffs | none | `AddPressure(Fatigue,-2)` |
| `Event.MoltCavern.LostDelver` | GuideToOldWell | none | `SetRunFlag(MoltCavern.DelverRouteKnown)`; `AddPressure(Misdeed,-2)` |
|  | TakeAbandonedPack | none | `AddGold(+4)`; `AddPressure(Misdeed,+3)` |
|  | ShareRations | none | `AddPressure(Fatigue,-3)` |
| `Event.MoltCavern.MoltingRite` | RepeatKnownRite | `RunFlagSet(MoltCavern.RitePatternKnown)` | `AddPressure(Fatigue,-3)` |
|  | FollowDelverMarks | `RunFlagSet(MoltCavern.DelverRouteKnown)` | `AddPressure(Wound,-2)` |
|  | OfferCoin | `MinGold(3)` | `AddGold(-3)`; `AddPressure(Misdeed,-2)` |
|  | ForcePassage | none | `AddPressure(Fatigue,+5)`; `AddPressure(Wound,+1)` |

Choice 总数：`3 + 3 + 4 = 10`。

## 9. Floor node binding registry

| Floor 2 Node | Definition |
|---|---|
| `Node.Main.01` | `Encounter.MoltCavern.ScaleScout` |
| `Node.Route.A.01` | `Event.MoltCavern.CastoffEcho` |
| `Node.Route.A.02` | `Encounter.MoltCavern.StoneScaleGuard` |
| `Node.Route.A.03` | `Pickup.MoltCavern.FungalCache` |
| `Node.Route.B.01` | `Encounter.MoltCavern.HatcheryAmbush` |
| `Node.Route.B.02` | `Pickup.MoltCavern.MineralCache` |
| `Node.Route.B.03` | `Event.MoltCavern.LostDelver` |
| `Node.Route.C.01` | `Shop.MoltCavern.DeepWayfarer` |
| `Node.Route.C.02` | `Encounter.MoltCavern.BridgeSentinel` |
| `Node.Route.D.01` | `Encounter.MoltCavern.VenomHunter` |
| `Node.Route.D.02` | `Event.MoltCavern.MoltingRite` |
| `Node.Route.D.03` | `Pickup.MoltCavern.VenomCrystalCache` |
| `Node.Key.01` | `Pickup.MoltCavern.MoltSeal` |
| `Node.Main.02` | `Encounter.MoltCavern.EliteMolter` |
| `Node.Guardian.01` | `Encounter.MoltCavern.CavernGuardian` |

分类精确为 `7 Encounter / 3 Event / 4 Pickup / 1 Shop`。

## 10. Route economy, reward yield and AP

从 0 Gold 出发：

```text
Route A: CastoffEcho.GatherScaleDust -> 3 Gold -> buy a 3-Gold offer
Route B: LostDelver.TakeAbandonedPack -> 4 Gold -> buy a 3/4-Gold offer
```

选择 `ReadRitePattern` 或 `GuideToOldWell` 会主动放弃即时取金，但分别为 `MoltingRite` 提供减 Fatigue 或减 Wound 的条件。

必经 Encounter 部位：ScaleScout 2 + EliteMolter (3+2) + CavernGuardian 4 = 11。

| Route | Added Parts | Total reward cards |
|---|---:|---:|
| A + C | StoneScaleGuard 3 + BridgeSentinel 3 | 17 |
| B + C | HatcheryAmbush 4 + BridgeSentinel 3 | 18 |
| A + D | StoneScaleGuard 3 + VenomHunter 3 | 17 |
| B + D | HatcheryAmbush 4 + VenomHunter 3 | 18 |
| Full exploration | all seven Encounters | 24 |

所有奖励选择不新增 AP。Floor 2 继续为：

| Route scope | No purchase | First successful purchase |
|---|---:|---:|
| Shortest progression | 8 | 9 |
| Full exploration | 14 | 15 |

## 11. Stable versus tunable facts

| Fact | Status |
|---|---|
| 47 package、class 与 stable content ID | Frozen future authoring contract |
| Enemy/Behavior/Part/Intent/IntentSet IDs 和引用顺序 | Frozen |
| Part HP/EXP、Intent I/R/type/target/order/magnitude | Frozen initial content design; future balance revision must be explicit |
| Encounter slot IDs、引用顺序和组合 | Frozen |
| CardId、keyword、TargetMode、effect type/target/order | Frozen |
| Card cost/rarity/magnitude | Frozen initial content design; future balance revision may tune without identity migration |
| Pickup mapping、Credential、Shop offer identity/order | Frozen |
| Event node/choice/condition/effect type/order/flag/pressure identity | Frozen |
| DisplayName、Description、Event prose | Mutable presentation text; must not contradict effects |
| Art、audio、material、animation、Host、world Transform | Deferred |
| Reward dedup/cap、backpack overflow policy、其它击倒后果 | Not changed; separate design questions |

## 12. Runtime and persistence impact

本切片不创建任何 runtime state 或资产：

- Battle 未来从现有 Definition/Encounter/Part reward 字段解释内容。
- Run 未来从现有 Pickup、Shop、RunEvent 和 Credential 合同执行事务。
- RunFlag 仍是当前 Run 内存态，不承诺 SaveGame 恢复。
- Gold、卡组、压力和 Credential 按现有 Journey 语义跨 Floor 保留。
- 没有新增 Snapshot、Command、Event、Result、SaveGame、GameplayTag、Build.cs 或模块依赖。
