# Contract: Formal Floor 2 Graph and Content Slots

## Exact graph

`Floor.Main.02` contains exactly 20 nodes and 21 directed edges. Canonical tables are in [data-model.md](../data-model.md).

```text
Entry -> Main.01 -> Junction.01
  Route A (3 content nodes) -> Junction.02
  Route B (3 content nodes) -> Junction.02
  Route C (Shop + Encounter) -> Key.01
  Route D (Encounter + Event + Treasure) -> Key.01
Key.01 -> Junction.03 -> Main.02 -> Guardian.01 -> Exit.01
```

## Required counts

| NodeType | Exact count |
|---|---:|
| Navigation | 4 |
| Encounter | 7 |
| RunEvent | 3 |
| Treasure | 4 |
| Shop | 1 |
| FloorEntrance | 1 |

All nodes are reachable. A/B converge at `Node.Junction.02`; C/D converge at `Node.Key.01`. No route reaches Guardian/Exit without Key.

## Camp and landmark contract

- Only `Node.Entry` and the three Junction nodes allow Camp.
- `Node.Guardian.01` uses `bBoss=true` and BossOutline.
- `Node.Exit.01` uses FloorEntranceOutline and targets `Floor.Main.03`.

## Molt Seal gate

- `Node.Key.01` uses `Pickup.MoltCavern.MoltSeal`.
- Future definition grants `Card.Run.MoltSeal` and `Credential.Run.MoltSeal` in one atomic Pickup settlement.
- `Node.Exit.01.RequiredCredentialIds = [Credential.Run.MoltSeal]`.
- `Node.Exit.01.OwnedCardRequirements` does not include the Molt Seal card.
- The requirement is non-consuming and its guaranteed source dominates the entrance.

## AP acceptance

- Shortest route costs 8 AP without a Shop purchase and 9 with the first successful transaction.
- Full exploration costs 14 AP without a Shop purchase and 15 with it.
- Any later Event/Shop design that changes these ranges requires a Floor pacing review.

## Content boundary

The 15 exact Production IDs in the canonical registry reserve typed content slots only. Enemy composition, Event graph, offers, rewards and visuals remain deferred. No slot may resolve to a Debug/Authoring asset.
