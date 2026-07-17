# Data Model: 正式 Floor 2/3 图与 Journey 总节奏冻结

## 1. Journey canonical skeleton

| Identity | 默认标题 | 顺序 | 冻结状态 |
|---|---|---:|---|
| `Journey.Main.01` | 蛇巢之路 | — | 三层 Journey 身份与总节奏冻结 |
| `Floor.Main.01` | 蛇巢浅林 | 1 | Spec 007 已冻结 20 Node/21 Edge |
| `Floor.Main.02` | 蛇蜕洞窟 | 2 | 本轮冻结 20 Node/21 Edge 与蜕印门槛 |
| `Floor.Main.03` | 毒巢核心 | 3 | 本轮冻结 20 Node/21 Edge 与 terminal Guardian |

Journey 继续使用现有 `2/6/2/2/1` 时段预算、三天 Floor Exposure 宽限和当前 Decay 曲线。Floor Transition 不消费 AP，进入新 Floor 不重置 JourneyDay、时段、剩余 AP、卡牌或压力。

## 2. Floor 2 nodes — `Floor.Main.02`

| # | NodeId | 默认标题 | NodeType | Content ID / role | Camp | Landmark | MapPosition |
|---:|---|---|---|---|:---:|---|---:|
| 1 | `Node.Entry` | 洞窟入口 | Navigation | 安全入口 | Yes | None | `(960,1050)` |
| 2 | `Node.Main.01` | 鳞岩伏击 | Encounter | `Encounter.MoltCavern.ScaleScout` | No | None | `(960,990)` |
| 3 | `Node.Junction.01` | 裂隙岔口 | Navigation | 第一处分岔 | Yes | None | `(960,930)` |
| 4 | `Node.Route.A.01` | 残蜕回声 | RunEvent | `Event.MoltCavern.CastoffEcho` | No | None | `(650,860)` |
| 5 | `Node.Route.A.02` | 石鳞守地 | Encounter | `Encounter.MoltCavern.StoneScaleGuard` | No | None | `(520,790)` |
| 6 | `Node.Route.A.03` | 菌光补给 | Treasure | `Pickup.MoltCavern.FungalCache` | No | None | `(650,720)` |
| 7 | `Node.Route.B.01` | 孵室伏击 | Encounter | `Encounter.MoltCavern.HatcheryAmbush` | No | None | `(1270,860)` |
| 8 | `Node.Route.B.02` | 矿脉密藏 | Treasure | `Pickup.MoltCavern.MineralCache` | No | None | `(1400,790)` |
| 9 | `Node.Route.B.03` | 失踪探路者 | RunEvent | `Event.MoltCavern.LostDelver` | No | None | `(1270,720)` |
| 10 | `Node.Junction.02` | 旧井汇流 | Navigation | 第一轮汇合 | Yes | None | `(960,650)` |
| 11 | `Node.Route.C.01` | 深窟行商 | Shop | `Shop.MoltCavern.DeepWayfarer` | No | None | `(650,570)` |
| 12 | `Node.Route.C.02` | 断桥守敌 | Encounter | `Encounter.MoltCavern.BridgeSentinel` | No | None | `(720,450)` |
| 13 | `Node.Route.D.01` | 毒泉猎手 | Encounter | `Encounter.MoltCavern.VenomHunter` | No | None | `(1270,590)` |
| 14 | `Node.Route.D.02` | 蜕壳仪式 | RunEvent | `Event.MoltCavern.MoltingRite` | No | None | `(1400,510)` |
| 15 | `Node.Route.D.03` | 毒晶密藏 | Treasure | `Pickup.MoltCavern.VenomCrystalCache` | No | None | `(1270,430)` |
| 16 | `Node.Key.01` | 深窟蜕印 | Treasure | `Pickup.MoltCavern.MoltSeal` → `Card.Run.MoltSeal` + `Credential.Run.MoltSeal` | No | None | `(960,350)` |
| 17 | `Node.Junction.03` | 核门前哨 | Navigation | Guardian 前汇合 | Yes | None | `(960,270)` |
| 18 | `Node.Main.02` | 蜕窟巡猎 | Encounter | `Encounter.MoltCavern.EliteMolter` | No | None | `(960,200)` |
| 19 | `Node.Guardian.01` | 洞窟守卫 | Encounter | `Encounter.MoltCavern.CavernGuardian`; `bBoss=true` | No | BossOutline | `(960,130)` |
| 20 | `Node.Exit.01` | 核心入口 | FloorEntrance | Target `Floor.Main.03`; requires `Credential.Run.MoltSeal` | No | FloorEntranceOutline | `(960,60)` |

Type totals:

| NodeType | Count |
|---|---:|
| Navigation | 4 |
| Encounter | 7 |
| RunEvent | 3 |
| Treasure | 4 |
| Shop | 1 |
| FloorEntrance | 1 |
| **Total** | **20** |

## 3. Floor 2 edges

| # | EdgeId | FromNodeId | ToNodeId |
|---:|---|---|---|
| 1 | `Edge.Main.01` | `Node.Entry` | `Node.Main.01` |
| 2 | `Edge.Main.02` | `Node.Main.01` | `Node.Junction.01` |
| 3 | `Edge.Route.A.01` | `Node.Junction.01` | `Node.Route.A.01` |
| 4 | `Edge.Route.A.02` | `Node.Route.A.01` | `Node.Route.A.02` |
| 5 | `Edge.Route.A.03` | `Node.Route.A.02` | `Node.Route.A.03` |
| 6 | `Edge.Route.A.04` | `Node.Route.A.03` | `Node.Junction.02` |
| 7 | `Edge.Route.B.01` | `Node.Junction.01` | `Node.Route.B.01` |
| 8 | `Edge.Route.B.02` | `Node.Route.B.01` | `Node.Route.B.02` |
| 9 | `Edge.Route.B.03` | `Node.Route.B.02` | `Node.Route.B.03` |
| 10 | `Edge.Route.B.04` | `Node.Route.B.03` | `Node.Junction.02` |
| 11 | `Edge.Route.C.01` | `Node.Junction.02` | `Node.Route.C.01` |
| 12 | `Edge.Route.C.02` | `Node.Route.C.01` | `Node.Route.C.02` |
| 13 | `Edge.Route.C.03` | `Node.Route.C.02` | `Node.Key.01` |
| 14 | `Edge.Route.D.01` | `Node.Junction.02` | `Node.Route.D.01` |
| 15 | `Edge.Route.D.02` | `Node.Route.D.01` | `Node.Route.D.02` |
| 16 | `Edge.Route.D.03` | `Node.Route.D.02` | `Node.Route.D.03` |
| 17 | `Edge.Route.D.04` | `Node.Route.D.03` | `Node.Key.01` |
| 18 | `Edge.Main.03` | `Node.Key.01` | `Node.Junction.03` |
| 19 | `Edge.Main.04` | `Node.Junction.03` | `Node.Main.02` |
| 20 | `Edge.Main.05` | `Node.Main.02` | `Node.Guardian.01` |
| 21 | `Edge.Main.06` | `Node.Guardian.01` | `Node.Exit.01` |

Floor 2 invariants:

- 20 个节点全部从 Entry 可达。
- Route A/B 只在 `Node.Junction.02` 汇合；Route C/D 只在 `Node.Key.01` 汇合。
- `Node.Key.01` 支配 `Node.Junction.03`、`Node.Main.02`、`Node.Guardian.01` 与 `Node.Exit.01`。
- `Node.Exit.01` 只指向 Journey 中更后的 `Floor.Main.03`。

## 4. Floor 3 nodes — `Floor.Main.03`

| # | NodeId | 默认标题 | NodeType | Content ID / role | Camp | Landmark | MapPosition |
|---:|---|---|---|---|:---:|---|---:|
| 1 | `Node.Entry` | 核心外环 | Navigation | 安全入口 | Yes | None | `(960,1050)` |
| 2 | `Node.Main.01` | 巢心先锋 | Encounter | `Encounter.VenomCore.CoreVanguard` | No | None | `(960,990)` |
| 3 | `Node.Junction.01` | 毒脉分流 | Navigation | 第一处分岔 | Yes | None | `(960,930)` |
| 4 | `Node.Route.A.01` | 毒脉共振 | RunEvent | `Event.VenomCore.VeinResonance` | No | None | `(650,860)` |
| 5 | `Node.Route.A.02` | 脉道守卫 | Encounter | `Encounter.VenomCore.VeinGuardian` | No | None | `(520,790)` |
| 6 | `Node.Route.A.03` | 解毒储备 | Treasure | `Pickup.VenomCore.AntidoteCache` | No | None | `(650,720)` |
| 7 | `Node.Route.B.01` | 孵群巡猎 | Encounter | `Encounter.VenomCore.BroodPatrol` | No | None | `(1270,860)` |
| 8 | `Node.Route.B.02` | 仪式密藏 | Treasure | `Pickup.VenomCore.RitualCache` | No | None | `(1400,790)` |
| 9 | `Node.Route.B.03` | 核心低语 | RunEvent | `Event.VenomCore.CoreWhisper` | No | None | `(1270,720)` |
| 10 | `Node.Junction.02` | 巢心汇流 | Navigation | 第一轮汇合 | Yes | None | `(960,650)` |
| 11 | `Node.Route.C.01` | 内环哨卫 | Encounter | `Encounter.VenomCore.InnerSentinel` | No | None | `(650,570)` |
| 12 | `Node.Route.C.02` | 献祭抉择 | RunEvent | `Event.VenomCore.SacrificeChoice` | No | None | `(720,450)` |
| 13 | `Node.Route.D.01` | 毒液潜猎 | Encounter | `Encounter.VenomCore.ToxinStalker` | No | None | `(1270,590)` |
| 14 | `Node.Route.D.02` | 巢心搏动 | RunEvent | `Event.VenomCore.HeartPulse` | No | None | `(1400,510)` |
| 15 | `Node.Route.D.03` | 毒池储备 | Treasure | `Pickup.VenomCore.VenomReservoir` | No | None | `(1270,430)` |
| 16 | `Node.Core.01` | 核心恩赐 | Treasure | `Pickup.VenomCore.CoreBoon` | No | None | `(960,350)` |
| 17 | `Node.Junction.03` | 核心前庭 | Navigation | Guardian 前汇合 | Yes | None | `(960,270)` |
| 18 | `Node.Main.02` | 精英收割者 | Encounter | `Encounter.VenomCore.EliteHarvester` | No | None | `(960,200)` |
| 19 | `Node.Main.03` | 终门先锋 | Encounter | `Encounter.VenomCore.FinalVanguard` | No | None | `(960,130)` |
| 20 | `Node.Guardian.01` | 毒巢核心守卫 | Encounter | `Encounter.VenomCore.CoreGuardian`; `bBoss=true`; terminal design node | No | BossOutline | `(960,60)` |

Type totals:

| NodeType | Count |
|---|---:|
| Navigation | 4 |
| Encounter | 8 |
| RunEvent | 4 |
| Treasure | 4 |
| Shop | 0 |
| FloorEntrance | 0 |
| **Total** | **20** |

## 5. Floor 3 edges

| # | EdgeId | FromNodeId | ToNodeId |
|---:|---|---|---|
| 1 | `Edge.Main.01` | `Node.Entry` | `Node.Main.01` |
| 2 | `Edge.Main.02` | `Node.Main.01` | `Node.Junction.01` |
| 3 | `Edge.Route.A.01` | `Node.Junction.01` | `Node.Route.A.01` |
| 4 | `Edge.Route.A.02` | `Node.Route.A.01` | `Node.Route.A.02` |
| 5 | `Edge.Route.A.03` | `Node.Route.A.02` | `Node.Route.A.03` |
| 6 | `Edge.Route.A.04` | `Node.Route.A.03` | `Node.Junction.02` |
| 7 | `Edge.Route.B.01` | `Node.Junction.01` | `Node.Route.B.01` |
| 8 | `Edge.Route.B.02` | `Node.Route.B.01` | `Node.Route.B.02` |
| 9 | `Edge.Route.B.03` | `Node.Route.B.02` | `Node.Route.B.03` |
| 10 | `Edge.Route.B.04` | `Node.Route.B.03` | `Node.Junction.02` |
| 11 | `Edge.Route.C.01` | `Node.Junction.02` | `Node.Route.C.01` |
| 12 | `Edge.Route.C.02` | `Node.Route.C.01` | `Node.Route.C.02` |
| 13 | `Edge.Route.C.03` | `Node.Route.C.02` | `Node.Core.01` |
| 14 | `Edge.Route.D.01` | `Node.Junction.02` | `Node.Route.D.01` |
| 15 | `Edge.Route.D.02` | `Node.Route.D.01` | `Node.Route.D.02` |
| 16 | `Edge.Route.D.03` | `Node.Route.D.02` | `Node.Route.D.03` |
| 17 | `Edge.Route.D.04` | `Node.Route.D.03` | `Node.Core.01` |
| 18 | `Edge.Main.03` | `Node.Core.01` | `Node.Junction.03` |
| 19 | `Edge.Main.04` | `Node.Junction.03` | `Node.Main.02` |
| 20 | `Edge.Main.05` | `Node.Main.02` | `Node.Main.03` |
| 21 | `Edge.Main.06` | `Node.Main.03` | `Node.Guardian.01` |

Floor 3 invariants:

- 20 个节点全部从 Entry 可达。
- Route A/B 只在 `Node.Junction.02` 汇合；Route C/D 只在 `Node.Core.01` 汇合。
- `Node.Core.01` 支配 `Node.Junction.03`、`Node.Main.02`、`Node.Main.03` 与 `Node.Guardian.01`。
- `Node.Guardian.01` 没有 outgoing Edge，Floor 内没有 FloorEntrance。
- Guardian 的“Journey success”是设计事实；当前 `FRunState`/Battle settlement 尚不能表达成功完成。

## 6. Production content registry

### Floor 2 — 15 slots

```text
Encounter.MoltCavern.ScaleScout
Encounter.MoltCavern.StoneScaleGuard
Encounter.MoltCavern.HatcheryAmbush
Encounter.MoltCavern.BridgeSentinel
Encounter.MoltCavern.VenomHunter
Encounter.MoltCavern.EliteMolter
Encounter.MoltCavern.CavernGuardian

Event.MoltCavern.CastoffEcho
Event.MoltCavern.LostDelver
Event.MoltCavern.MoltingRite

Pickup.MoltCavern.FungalCache
Pickup.MoltCavern.MineralCache
Pickup.MoltCavern.VenomCrystalCache
Pickup.MoltCavern.MoltSeal

Shop.MoltCavern.DeepWayfarer
```

### Floor 3 — 16 slots

```text
Encounter.VenomCore.CoreVanguard
Encounter.VenomCore.VeinGuardian
Encounter.VenomCore.BroodPatrol
Encounter.VenomCore.InnerSentinel
Encounter.VenomCore.ToxinStalker
Encounter.VenomCore.EliteHarvester
Encounter.VenomCore.FinalVanguard
Encounter.VenomCore.CoreGuardian

Event.VenomCore.VeinResonance
Event.VenomCore.CoreWhisper
Event.VenomCore.SacrificeChoice
Event.VenomCore.HeartPulse

Pickup.VenomCore.AntidoteCache
Pickup.VenomCore.RitualCache
Pickup.VenomCore.VenomReservoir
Pickup.VenomCore.CoreBoon
```

三层合计 46 个内容槽：Floor 1 既有 15 + Floor 2 新增 15 + Floor 3 新增 16。具体敌人槽、事件选项、Shop offers、Pickup 奖励数值和视觉未冻结。

## 7. Stable and mutable fields

| Fact | Status | Notes |
|---|---|---|
| `Journey.Main.01` + three FloorIds/order | Frozen | Future persistence candidate |
| Floor 2/3 NodeIds and EdgeIds | Frozen per Floor | Cross-floor handle must include FloorId |
| `Card.Run.MoltSeal` | Frozen content identity | Presentation only |
| `Credential.Run.MoltSeal` | Frozen Run qualification identity | Non-consumable, authoritative gate |
| 31 Production content IDs | Frozen content contracts | Assets and values deferred |
| NodeType / edge endpoints / Camp / gate / terminal role | Frozen design rules | Static DataAsset truth when implemented |
| Host PersistentId formula | Frozen | `<FloorId>.<NodeId>` |
| DisplayName / descriptions | Mutable | Never identity |
| MapPosition | Mutable within validator bounds | Never identity |
| World Transform / Spline / Actor GUID | Mutable authoring presentation | Never logical identity |

Examples:

```text
Floor.Main.02.Node.Key.01
Floor.Main.03.Node.Core.01
Floor.Main.03.Node.Guardian.01
```

## 8. Action Point and Journey-day model

Existing costs remain authoritative:

- Encounter victory: 1.
- normal terminal RunEvent: 1 unless later option contract differs.
- Treasure first successful reward: 1.
- Shop browse/leave: 0; first successful transaction during visit: 1.
- Navigation, traversal, Map Travel and Floor Transition: 0.

Floor 2 shortest (`A or B + C`):

```text
Main.01 Encounter                         1
First branch (3 content nodes)           3
Route C Shop + Encounter                 1 or 2
Key Treasure + Elite + Guardian          3
Floor Transition                         0
Total                                    8 or 9
```

Floor 2 full:

```text
7 Encounter + 3 Event + 4 Treasure      14
Optional first Shop purchase              0 or 1
Total                                    14 or 15
```

Floor 3 shortest (`A or B + C`):

```text
Main.01 Encounter                         1
First branch                              3
Route C Encounter + Event                 2
Core Treasure                             1
Main.02 + Main.03 + Guardian              3
Total                                    10
```

Floor 3 full:

```text
8 Encounter + 4 Event + 4 Treasure      16
```

Journey totals:

| Scope | Shortest | Full |
|---|---:|---:|
| Floor 1 | 8–9 | 14–15 |
| Floor 2 | 8–9 | 14–15 |
| Floor 3 | 10 | 16 |
| **Journey** | **26–28** | **44–46** |

Day target:

- Morning Planning 固定消耗 1；正常 Camp 结束 Night 并跳过 Sunrise。
- 正常 Camp/恢复节奏下，内容吞吐通常约 8–10 AP/天：关键推进约 3 天，完整探索约 5–6 天。
- 积极 Night Exploration 并使用 Sunrise 时，内容上限约 12 AP/天，完整探索理论下界约 4 天。
- 每层目标仍约 1.5–2 天，正常路线不超过三天 Floor Exposure 宽限；路线失误与恢复拥有第 3 天余量。

## 9. Production blockers after this freeze

1. **Journey success runtime contract**: 需要通用成功状态、事件/结果、Guardian settlement 接入、总结页和返回目标；不得复用 Defeat。
2. **Production definitions**: 三层 46 个内容槽必须拥有非 Debug typed definitions 和 Data Validation 覆盖。
3. **Production assets**: Journey/Floor DataAsset、world map 和 Host/Path/Descriptor 必须在 AssetRegistry、引用和哈希权威审计后另案制作。
4. **End-to-end validation**: Production 资产完成后才运行 Map/Journey Data Validation、Scene Binding、Blueprint compile、AssetRegistry 和 PIE。

Floor 2/3 “缺少有效图” blocker 在本轮关闭；图冻结不自动关闭以上三项。
