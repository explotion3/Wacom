# Contract: Floor 1 Production FloorDefinition

## Identity

```text
Package: /Game/Wacom/Data/Map/Production/DA_Floor_Main_01
Class: UWacomFloorMapDefinition
FloorId: Floor.Main.01
DisplayName seed default: 蛇巢浅林
```

DisplayName and MapPosition are tunable. IDs, graph endpoints, node types, payload identity, Camp flags, Guardian boss semantics, and Exit target/credential are structural.

## Canonical nodes

| # | NodeId | Type | MapPosition | Camp | Payload / semantic |
|---:|---|---|---:|:---:|---|
| 1 | `Node.Entry` | Navigation | `(960,1050)` | Yes | safe entry |
| 2 | `Node.Main.01` | Encounter | `(960,990)` | No | `/Game/Wacom/Data/Encounters/SerpentWood/DA_Encounter_Scout` |
| 3 | `Node.Junction.01` | Navigation | `(960,930)` | Yes | first branch |
| 4 | `Node.Route.A.01` | RunEvent | `(650,860)` | No | `/Game/Wacom/Data/Events/SerpentWood/DA_Event_CastSkin` |
| 5 | `Node.Route.A.02` | Encounter | `(520,790)` | No | `/Game/Wacom/Data/Encounters/SerpentWood/DA_Encounter_MoltGuard` |
| 6 | `Node.Route.A.03` | Treasure | `(650,720)` | No | `/Game/Wacom/Data/Pickups/SerpentWood/DA_Pickup_HerbCache` |
| 7 | `Node.Route.B.01` | Encounter | `(1270,860)` | No | `/Game/Wacom/Data/Encounters/SerpentWood/DA_Encounter_Ambush` |
| 8 | `Node.Route.B.02` | Treasure | `(1400,790)` | No | `/Game/Wacom/Data/Pickups/SerpentWood/DA_Pickup_HunterCache` |
| 9 | `Node.Route.B.03` | RunEvent | `(1270,720)` | No | `/Game/Wacom/Data/Events/SerpentWood/DA_Event_HunterTrace` |
| 10 | `Node.Junction.02` | Navigation | `(960,650)` | Yes | first merge / second branch |
| 11 | `Node.Route.C.01` | Shop | `(650,570)` | No | `/Game/Wacom/Data/Shops/SerpentWood/DA_Shop_Wayfarer` |
| 12 | `Node.Route.C.02` | RunEvent | `(720,450)` | No | `/Game/Wacom/Data/Events/SerpentWood/DA_Event_MerchantRumor` |
| 13 | `Node.Route.D.01` | Encounter | `(1270,590)` | No | `/Game/Wacom/Data/Encounters/SerpentWood/DA_Encounter_RootStalker` |
| 14 | `Node.Route.D.02` | RunEvent | `(1400,510)` | No | `/Game/Wacom/Data/Events/SerpentWood/DA_Event_PoisonMarsh` |
| 15 | `Node.Route.D.03` | Treasure | `(1270,430)` | No | `/Game/Wacom/Data/Pickups/SerpentWood/DA_Pickup_MoltCache` |
| 16 | `Node.Key.01` | Treasure | `(960,350)` | No | `/Game/Wacom/Data/Pickups/SerpentWood/DA_Pickup_SerpentSigil` |
| 17 | `Node.Junction.03` | Navigation | `(960,270)` | Yes | Guardian staging Camp |
| 18 | `Node.Main.02` | Encounter | `(960,200)` | No | `/Game/Wacom/Data/Encounters/SerpentWood/DA_Encounter_EliteSentinel` |
| 19 | `Node.Guardian.01` | Encounter | `(960,130)` | No | `/Game/Wacom/Data/Encounters/SerpentWood/DA_Encounter_ShallowGuardian`; `bBoss=true` |
| 20 | `Node.Exit.01` | FloorEntrance | `(960,60)` | No | target `Floor.Main.02`; requires non-consuming `Credential.Run.SerpentSigil` |

Type totals: `4 Navigation / 6 Encounter / 4 RunEvent / 4 Treasure / 1 Shop / 1 FloorEntrance`.

## Canonical edges

| # | EdgeId | From | To |
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

## Structural validation

- Exactly 20 nodes and 21 edges.
- All NodeId and EdgeId values unique within the Floor.
- Every edge endpoint exists.
- Every node is reachable from Entry.
- A/B merge at Junction 2; C/D merge at Key.
- Key dominates Junction 3, Main 2, Guardian, and Exit.
- Guardian is an Encounter with `bBoss=true`.
- Exit is a FloorEntrance with only the frozen target Floor and credential condition.
- All coordinates are finite, in the 1920×1080 map canvas, and unique.
- No payload package contains Debug, Authoring, Test, BadgeDisplayTests, or TrainingWarrior identity.
