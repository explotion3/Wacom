# Contract: Floor 1 Production Scene Binding Manifest

## World identity

```text
World: /Game/Wacom/Maps/Run/L_Run_Floor_Main_01
Descriptor Floor: /Game/Wacom/Data/Map/Production/DA_Floor_Main_01
World-forward: +X
Logical left: -Y
Logical right: +Y
```

World transforms are seed defaults and remain tunable. Bindings and payload identity are structural.

## Anchor/world seed layout

| NodeId | Seed location cm `(X,Y,Z)` | Arrival facing |
|---|---:|---:|
| `Node.Entry` | `(0,0,100)` | `+X` |
| `Node.Main.01` | `(1200,0,100)` | `-X` |
| `Node.Junction.01` | `(2400,0,100)` | `+X` |
| `Node.Route.A.01` | `(3600,-1800,100)` | toward previous Anchor |
| `Node.Route.A.02` | `(4800,-2600,100)` | toward previous Anchor |
| `Node.Route.A.03` | `(6000,-1800,100)` | toward previous Anchor |
| `Node.Route.B.01` | `(3600,1800,100)` | toward previous Anchor |
| `Node.Route.B.02` | `(4800,2600,100)` | toward previous Anchor |
| `Node.Route.B.03` | `(6000,1800,100)` | toward previous Anchor |
| `Node.Junction.02` | `(7200,0,100)` | `+X` |
| `Node.Route.C.01` | `(8400,-1800,100)` | toward previous Anchor |
| `Node.Route.C.02` | `(9600,-1200,100)` | toward previous Anchor |
| `Node.Route.D.01` | `(8400,1800,100)` | toward previous Anchor |
| `Node.Route.D.02` | `(9600,2600,100)` | toward previous Anchor |
| `Node.Route.D.03` | `(10800,1800,100)` | toward previous Anchor |
| `Node.Key.01` | `(12000,0,100)` | toward incoming path |
| `Node.Junction.03` | `(13200,0,100)` | `+X` |
| `Node.Main.02` | `(14400,0,100)` | `-X` |
| `Node.Guardian.01` | `(15600,0,100)` | `-X` |
| `Node.Exit.01` | `(16800,0,100)` | `-X` |

Anchor labels use `Anchor_<NodeId>` with dots converted to underscores. Actor labels are diagnostic, not identity.

## Path and BranchTarget bindings

- One Path for every edge in `floor1-production-floor-contract.md`.
- Path source and target are the corresponding Anchors.
- The traversal spline must leave a clear central lane through every content staging area.
- BranchTargets exist only for:
  - `Edge.Route.A.01 → Node.Route.A.01`
  - `Edge.Route.B.01 → Node.Route.B.01`
  - `Edge.Route.C.01 → Node.Route.C.01`
  - `Edge.Route.D.01 → Node.Route.D.01`

## Content Host manifest

| NodeId | Host class/package | Payload mirror | PersistentId |
|---|---|---|---|
| `Node.Main.01` | `/Game/Wacom/Maps/SceneActor/BP_BattleTriggerActor` | `DA_Encounter_Scout` | `Floor.Main.01.Node.Main.01` |
| `Node.Route.A.01` | `/Game/Wacom/Maps/SceneActor/BP_WacomRunEventTriggerActor` | `DA_Event_CastSkin` | `Floor.Main.01.Node.Route.A.01` |
| `Node.Route.A.02` | `/Game/Wacom/Maps/SceneActor/BP_BattleTriggerActor` | `DA_Encounter_MoltGuard` | `Floor.Main.01.Node.Route.A.02` |
| `Node.Route.A.03` | `/Game/Wacom/Maps/SceneActor/BP_WacomRunRewardPickupActor` | `DA_Pickup_HerbCache` | `Floor.Main.01.Node.Route.A.03` |
| `Node.Route.B.01` | `/Game/Wacom/Maps/SceneActor/BP_BattleTriggerActor` | `DA_Encounter_Ambush` | `Floor.Main.01.Node.Route.B.01` |
| `Node.Route.B.02` | `/Game/Wacom/Maps/SceneActor/BP_WacomRunRewardPickupActor` | `DA_Pickup_HunterCache` | `Floor.Main.01.Node.Route.B.02` |
| `Node.Route.B.03` | `/Game/Wacom/Maps/SceneActor/BP_WacomRunEventTriggerActor` | `DA_Event_HunterTrace` | `Floor.Main.01.Node.Route.B.03` |
| `Node.Route.C.01` | `/Game/Wacom/Maps/SceneActor/BP_WacomShopTriggerActor` | `DA_Shop_Wayfarer` | `Floor.Main.01.Node.Route.C.01` |
| `Node.Route.C.02` | `/Game/Wacom/Maps/SceneActor/BP_WacomRunEventTriggerActor` | `DA_Event_MerchantRumor` | `Floor.Main.01.Node.Route.C.02` |
| `Node.Route.D.01` | `/Game/Wacom/Maps/SceneActor/BP_BattleTriggerActor` | `DA_Encounter_RootStalker` | `Floor.Main.01.Node.Route.D.01` |
| `Node.Route.D.02` | `/Game/Wacom/Maps/SceneActor/BP_WacomRunEventTriggerActor` | `DA_Event_PoisonMarsh` | `Floor.Main.01.Node.Route.D.02` |
| `Node.Route.D.03` | `/Game/Wacom/Maps/SceneActor/BP_WacomRunRewardPickupActor` | `DA_Pickup_MoltCache` | `Floor.Main.01.Node.Route.D.03` |
| `Node.Key.01` | `/Game/Wacom/Maps/SceneActor/BP_WacomRunRewardPickupActor` | `DA_Pickup_SerpentSigil` | `Floor.Main.01.Node.Key.01` |
| `Node.Main.02` | `/Game/Wacom/Maps/SceneActor/BP_BattleTriggerActor` | `DA_Encounter_EliteSentinel` | `Floor.Main.01.Node.Main.02` |
| `Node.Guardian.01` | `/Game/Wacom/Maps/SceneActor/BP_BattleTriggerActor` | `DA_Encounter_ShallowGuardian` | `Floor.Main.01.Node.Guardian.01` |
| `Node.Exit.01` | `BP_WacomRunFloorEntranceMarker_Graybox` | target `Floor.Main.02`; non-interactive | `Floor.Main.01.Node.Exit.01` |

The four Navigation nodes have no content Host.

## Viewpoint and staging rules

- Six Battle, four Event, and one Shop Host each receive one dedicated viewpoint.
- A viewpoint sits behind the arrival-side staging line and faces content; it does not overlap the traversal spline.
- Battle enemy Hosts are placed beyond the interaction Host, visible from its viewpoint, with adequate separation between part hit bounds.
- Trigger volumes remain reachable from the path but do not cover the entire branch lane.
- Pickup actors sit beside the destination Anchor and do not block the outgoing spline.
- The Exit marker is visibly distinct and carries “graybox / no travel” diagnostic labeling.

## Graybox world support

The World may contain non-contract lighting, PlayerStart, floor pads, corridor meshes, signs, and bounds volumes. These actors:

- use Engine or controlled Wacom graybox/placeholder assets;
- carry no Run stable identity;
- do not replace Anchor/Path bindings;
- do not implement Level Blueprint rules;
- may be manually tuned after initial seed.

## Validation invariants

- 1 descriptor, 20 Anchors, 21 Paths, 4 BranchTargets, 16 content Hosts.
- All expected IDs appear exactly once and no unexpected ID appears.
- Host NodeType and payload mirror match the FloorDefinition.
- All 16 PersistentIds equal `Floor.Main.01.<NodeId>`.
- Scene contains 8 battle enemy Host instances with exact slot mappings.
- No `Wacom.Generated.RunExploration`, Debug, Authoring, Test, BadgeDisplayTests, or legacy-map authority.
- No Level Blueprint gameplay logic.
