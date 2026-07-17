# Contract: Formal Floor 3 Terminal Graph

## Exact graph

`Floor.Main.03` contains exactly 20 nodes and 21 directed edges. Canonical tables are in [data-model.md](../data-model.md).

```text
Entry -> Main.01 -> Junction.01
  Route A (3 content nodes) -> Junction.02
  Route B (3 content nodes) -> Junction.02
  Route C (Encounter + Event) -> Core.01
  Route D (Encounter + Event + Treasure) -> Core.01
Core.01 -> Junction.03 -> Main.02 -> Main.03 -> Guardian.01
```

## Required counts

| NodeType | Exact count |
|---|---:|
| Navigation | 4 |
| Encounter | 8 |
| RunEvent | 4 |
| Treasure | 4 |
| Shop | 0 |
| FloorEntrance | 0 |

All nodes are reachable. A/B converge at `Node.Junction.02`; C/D converge at `Node.Core.01`. Core dominates the complete terminal chain.

## Camp and terminal contract

- Only `Node.Entry` and the three Junction nodes allow Camp.
- `Node.Core.01` is the mandatory pre-terminal Treasure and uses `Pickup.VenomCore.CoreBoon`.
- `Node.Guardian.01` uses `Encounter.VenomCore.CoreGuardian`, `bBoss=true` and BossOutline.
- Guardian has no outgoing Edge; the Floor contains no FloorEntrance.
- Successful Guardian settlement is the frozen Journey success design trigger.

## Runtime capability boundary

The current runtime cannot fulfill the final bullet above:

- `FRunState` has no Journey success/outcome state.
- Battle victory resolves the Encounter but does not complete the Journey.
- `bRunActive=false` belongs to Defeat and must not be reused.
- `FWacomMapFloorEntrancePayload` requires a later TargetFloorId and cannot represent Journey completion.

Production activation requires a separate reusable WacomRun success contract plus WacomApp summary/handoff. Actor labels, Level Blueprint branches and hard-coded Guardian NodeId checks are forbidden substitutes.

## AP acceptance

- Shortest route through Route C costs exactly 10 AP.
- Full exploration costs exactly 16 AP.
- There is no Shop purchase range on Floor 3.
