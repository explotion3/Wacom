# Contract: Formal Floor 1 Graph and Content Slots

## Graph shape

`Floor.Main.01` contains exactly 20 nodes and 21 directed edges. The canonical tables are [data-model.md](../data-model.md); this contract defines the acceptance invariants.

```text
Entry -> Main.01 -> Junction.01
  Route A (3 content nodes) -> Junction.02
  Route B (3 content nodes) -> Junction.02
  Route C (2 content nodes) -> Key.01
  Route D (3 content nodes) -> Key.01
Key.01 -> Junction.03 -> Main.02 -> Guardian.01 -> Exit.01
```

## Required counts

| NodeType | Exact count |
|---|---:|
| Navigation | 4 |
| Encounter | 6 |
| RunEvent | 4 |
| Treasure | 4 |
| Shop | 1 |
| FloorEntrance | 1 |

All 20 nodes must be reachable from `Node.Entry`. Route A/B must converge at `Node.Junction.02`; Route C/D must converge at `Node.Key.01`. There is no path from Entry to Guardian or Exit that bypasses Key.

## Camp contract

Exactly these nodes set `bAllowsCamp=true`:

```text
Node.Entry
Node.Junction.01
Node.Junction.02
Node.Junction.03
```

No content node allows Camp in the frozen default. Camp remains the existing Night transaction and is not a NodeType.

## Landmark and entrance contract

- `Node.Guardian.01`: Encounter with `bBoss=true` and `BossOutline` landmark.
- `Node.Exit.01`: FloorEntrance with `FloorEntranceOutline`, target `Floor.Main.02`.
- Entrance requirement: owned `Card.Run.SerpentSigil`, checked non-consumingly under existing rules.
- Floor Transition itself costs 0 AP.

## Production content IDs

Every content node reserves the exact ID in the canonical node table. These IDs describe a production slot and may not resolve to an Authoring/Debug asset.

Later content design must fill, without changing this graph unless separately approved:

- Encounter enemy slots and difficulty values.
- RunEvent choices, costs, flags and outcomes.
- Shop offers and prices.
- Treasure card/gold amounts and interaction visuals.
- Display text and presentation assets.

## AP acceptance

- A shortest valid route through one first fork and Route C costs 8 AP without a purchase and 9 AP with the first Shop purchase.
- Full exploration costs 14 AP without a purchase and 15 AP with the first Shop purchase.
- Navigation, branch selection, Map Travel and Floor Transition remain free.
- An Event option whose future cost changes these ranges requires a new Floor pacing review.

## Default layout acceptance

- All MapPosition values must remain finite and in `[0,1920] × [0,1080]`.
- No two node centers may be identical.
- Defaults in the canonical table are an initial authoring layout, not stable identities.
- MapPosition never determines world placement or route length.
