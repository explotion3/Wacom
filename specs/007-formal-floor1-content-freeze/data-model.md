# Data Model: 正式 Floor 1 内容设计与稳定身份冻结

## 1. Journey skeleton

| Identity | 默认标题 | 顺序 | 本轮状态 |
|---|---|---:|---|
| `Journey.Main.01` | 蛇巢之路 | — | 稳定身份冻结 |
| `Floor.Main.01` | 蛇巢浅林 | 1 | 完整图与内容槽冻结 |
| `Floor.Main.02` | 蛇蜕洞窟 | 2 | 身份与主题职责冻结，图未设计 |
| `Floor.Main.03` | 毒巢核心 | 3 | 身份与主题职责冻结，图未设计 |

Journey 继续使用现有 `2/6/2/2/1` 时段预算和当前 Decay 曲线；本轮不新建资产，也不把三层骨架视为可通过 Data Validation 的 Journey。

未来预留路径：

```text
/Game/Wacom/Data/Map/Production/DA_Journey_Main_01
/Game/Wacom/Data/Map/Production/DA_Floor_Main_01
/Game/Wacom/Data/Map/Production/DA_Floor_Main_02
/Game/Wacom/Data/Map/Production/DA_Floor_Main_03
/Game/Wacom/Maps/Run/L_Run_Floor_Main_01
```

路径是 authoring 合同，不表示本轮创建资产；正式 Journey 必须等待 Floor 2/3 拥有有效图。

## 2. Floor 1 nodes

| # | NodeId | 默认标题 | NodeType | Content ID / role | Camp | Landmark | MapPosition |
|---:|---|---|---|---|:---:|---|---:|
| 1 | `Node.Entry` | 林地入口 | Navigation | 安全入口 | Yes | None | `(960,1050)` |
| 2 | `Node.Main.01` | 伏蛇草径 | Encounter | `Encounter.SerpentWood.Scout` / 教学伏击 | No | None | `(960,990)` |
| 3 | `Node.Junction.01` | 分藤岔路 | Navigation | 第一处分岔 | Yes | None | `(960,930)` |
| 4 | `Node.Route.A.01` | 遗落蛇蜕 | RunEvent | `Event.SerpentWood.CastSkin` / 蛇蜕事件 | No | None | `(650,860)` |
| 5 | `Node.Route.A.02` | 蛇蜕守地 | Encounter | `Encounter.SerpentWood.MoltGuard` / 蛇蜕守卫 | No | None | `(520,790)` |
| 6 | `Node.Route.A.03` | 采药洼地 | Treasure | `Pickup.SerpentWood.HerbCache` / 草药补给 | No | None | `(650,720)` |
| 7 | `Node.Route.B.01` | 毒雾伏击 | Encounter | `Encounter.SerpentWood.Ambush` / 高风险伏击 | No | None | `(1270,860)` |
| 8 | `Node.Route.B.02` | 猎人遗物 | Treasure | `Pickup.SerpentWood.HunterCache` / 猎人遗物 | No | None | `(1400,790)` |
| 9 | `Node.Route.B.03` | 腐枝祭痕 | RunEvent | `Event.SerpentWood.HunterTrace` / 猎人痕迹 | No | None | `(1270,720)` |
| 10 | `Node.Junction.02` | 旧营汇流 | Navigation | 第一轮汇合 | Yes | None | `(960,650)` |
| 11 | `Node.Route.C.01` | 林下行商 | Shop | `Shop.SerpentWood.Wayfarer` / 补给商店 | No | None | `(650,570)` |
| 12 | `Node.Route.C.02` | 行商密语 | RunEvent | `Event.SerpentWood.MerchantRumor` / 行商情报 | No | None | `(720,450)` |
| 13 | `Node.Route.D.01` | 盘根伏蛇 | Encounter | `Encounter.SerpentWood.RootStalker` / 盘根伏击 | No | None | `(1270,590)` |
| 14 | `Node.Route.D.02` | 毒沼抉择 | RunEvent | `Event.SerpentWood.PoisonMarsh` / 风险事件 | No | None | `(1400,510)` |
| 15 | `Node.Route.D.03` | 蜕壳密藏 | Treasure | `Pickup.SerpentWood.MoltCache` / 高风险奖励 | No | None | `(1270,430)` |
| 16 | `Node.Key.01` | 浅巢蛇印 | Treasure | `Pickup.SerpentWood.SerpentSigil` → `Card.Run.SerpentSigil` | No | None | `(960,350)` |
| 17 | `Node.Junction.03` | 巢门前哨 | Navigation | Boss 前汇合 | Yes | None | `(960,270)` |
| 18 | `Node.Main.02` | 巢道巡猎 | Encounter | `Encounter.SerpentWood.EliteSentinel` / 精英 | No | None | `(960,200)` |
| 19 | `Node.Guardian.01` | 浅巢守卫 | Encounter | `Encounter.SerpentWood.ShallowGuardian`; `bBoss=true` | No | BossOutline | `(960,130)` |
| 20 | `Node.Exit.01` | 深窟入口 | FloorEntrance | Target `Floor.Main.02`; requires `Card.Run.SerpentSigil` | No | FloorEntranceOutline | `(960,60)` |

Type totals:

| NodeType | Count |
|---|---:|
| Navigation | 4 |
| Encounter | 6 |
| RunEvent | 4 |
| Treasure | 4 |
| Shop | 1 |
| FloorEntrance | 1 |
| **Total** | **20** |

## 3. Floor 1 edges

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

Graph invariants:

- Entry is `Node.Entry`.
- Every node is reachable from Entry.
- Route A/B converge only at `Node.Junction.02`.
- Route C/D converge only at `Node.Key.01`.
- `Node.Key.01` dominates `Node.Junction.03`、`Node.Main.02`、`Node.Guardian.01` 和 `Node.Exit.01`。
- Removing `Node.Key.01` makes Guardian and Exit unreachable.
- Graph is directed; Map Travel remains the existing free return path to Resolved nodes.

## 4. Content slot registry

| Category | Reserved production IDs | Count | Detailed values frozen? |
|---|---|---:|:---:|
| Encounter | `Scout`, `MoltGuard`, `Ambush`, `RootStalker`, `EliteSentinel`, `ShallowGuardian` under `Encounter.SerpentWood.*` | 6 | No |
| RunEvent | `CastSkin`, `HunterTrace`, `MerchantRumor`, `PoisonMarsh` under `Event.SerpentWood.*` | 4 | No |
| Treasure | `HerbCache`, `HunterCache`, `MoltCache`, `SerpentSigil` under `Pickup.SerpentWood.*` | 4 | No |
| Shop | `Shop.SerpentWood.Wayfarer` | 1 | No |

The IDs above are frozen production content contracts. Enemy slots, option text/effects, offers, gold/card amounts and visual assets are not frozen in this slice. No ID may resolve to an existing asset whose asset name or internal ID contains `Debug`.

## 5. Stable and mutable fields

| Fact | Status | Future persistence |
|---|---|---|
| `Journey.Main.01` | Frozen | Candidate |
| Three FloorIds and order | Frozen | Candidate |
| 20 NodeIds | Frozen | Candidate |
| 21 EdgeIds | Frozen | Candidate |
| `Card.Run.SerpentSigil` | Frozen | Candidate, blocked |
| Content Host PersistentId formula | Frozen | Runtime identity contract |
| NodeType / edge endpoints / key gate | Frozen design rule | Static DataAsset truth |
| Reserved content IDs | Frozen content contract | Definition identity, not SaveGame commitment |
| DisplayName / description | Mutable | Never identity |
| MapPosition | Mutable within validator bounds | Never identity |
| World Transform / Spline / Actor GUID | Mutable authoring presentation | Never logical identity |
| Event options / rewards / shop offers / enemy composition | Deferred | Not decided |

Content Host PersistentId formula:

```text
PersistentId = FloorId + "." + NodeId

Example:
Floor.Main.01 + Node.Route.A.01
= Floor.Main.01.Node.Route.A.01
```

Navigation nodes have no content Host PersistentId. Path and Branch actors continue using EdgeId within their one-Floor World.

## 6. Action Point model

Existing costs remain authoritative:

- Encounter victory: 1.
- RunEvent normal terminal choice: 1 unless the later option contract explicitly differs.
- Treasure first successful reward: 1.
- Shop browse/leave: 0; first successful transaction during visit: 1.
- Navigation, traversal, branch choice, Map Travel and Floor Transition: 0.

Shortest route example (`A + C`):

```text
Main.01 Encounter                         1
Route A: Event + Encounter + Treasure    3
Route C: Shop + Event                    1 or 2
Key Treasure                             1
Main.02 Elite                            1
Guardian                                 1
Floor Transition                         0
Total                                    8 or 9
```

Full exploration:

```text
Main.01                                  1
Route A                                  3
Route B                                  3
Route C                                  1 or 2
Route D                                  3
Key + Elite + Guardian                   3
Total                                   14 or 15
```

The 1 AP range is solely the Shop purchase outcome. Later Event options may only change this target after a separate design review.

## 7. Production blockers

1. `Card.Run.SerpentSigil` is guaranteed to be acquired because its Treasure dominates the exit, but current rules do not guarantee retention after card removal operations.
2. A completed Pickup cannot be assumed to reissue the card after removal.
3. `Floor.Main.02/03` have no valid node graphs, so the frozen three-Floor Journey cannot yet pass the existing validation contract; Floor 1's entrance specifically requires a valid later `Floor.Main.02` target.
4. Production Journey/Floor assets, formal Host placement and map migration remain forbidden until both blockers are resolved.
