# Contract: SerpentWood Cards, Pickups, and Wayfarer Shop

## New card contract

| CardId | Cost | Rarity | Keywords | TargetMode | Effect |
|---|---:|---|---|---|---|
| `Reward.SerpentWood.HerbalPoultice` | 1 | White | Tool | None | Heal 4 → Player |
| `Reward.SerpentWood.HunterSnare` | 1 | White | Tool | SingleEnemyPart | Slow 2 → SingleEnemyPart |
| `Reward.SerpentWood.MoltWard` | 0 | Blue | Tool | None | Shield 3 → Player |
| `Card.Run.SerpentSigil` | 1 | White | empty | None | Draw 1 → Player from `CardLocation.Draw` |

Exact tag values:

- Rarity: `Card.Rarity.White` / `Card.Rarity.Blue`.
- Keyword: `Card.Keyword.Tool`; SerpentSigil adds no keyword.
- Effects: `Effect.Heal`、`Effect.ApplyStatus.Slow`、`Status.Shield`、`Effect.Draw`.
- Targets: `Target.Player` or `Target.SingleEnemyPart` as listed.
- Draw source: `TargetZone=CardLocation.Draw`.

All four cards have zero/default Physique and empty PerfectReleaseEffects, ZoneHooks and Passives. Illustration, depth map, flavor/description, audio and presentation are not frozen.

## Pickup contract

All four use `EWacomRunPickupRewardType::Card` and exactly one fixed CardDefinition:

| PickupId | Fixed card | Credential grant |
|---|---|---|
| `Pickup.SerpentWood.HerbCache` | HerbalPoultice | none |
| `Pickup.SerpentWood.HunterCache` | HunterSnare | none |
| `Pickup.SerpentWood.MoltCache` | MoltWard | none |
| `Pickup.SerpentWood.SerpentSigil` | SerpentSigil | `Credential.Run.SerpentSigil` |

The SerpentSigil card and Credential are granted by the existing atomic Pickup settlement. The Floor entrance only checks the Credential. Card ownership never implies, removes or restores Credential state.

## Wayfarer shop contract

`ShopId=Shop.SerpentWood.Wayfarer`; the authored Offer order and price are:

1. `Starter.ChitinWard` — 2 Gold.
2. `Starter.AntennaSearch` — 2 Gold.
3. `Starter.MoltCut` — 3 Gold.
4. `PoisonFang` — 2 Gold.
5. `Reward.SerpentWood.HerbalPoultice` — 2 Gold.

Existing dependency paths:

```text
/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_ChitinWard
/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_AntennaSearch
/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_MoltCut
/Game/Wacom/Data/Cards/Rewards/DA_Card_PoisonFang
```

`PoisonFang` is the live formal CardId. It is not renamed, duplicated or counted among the four new cards.

## Economy and AP acceptance

- Route A `SellSkin` yields +2 Gold; from initial 0 Gold it can buy any 2-Gold offer.
- Route B `LootPack` yields +3 Gold; from initial 0 Gold it can buy any offer.
- Information/moral choices may yield no Gold; this is an authored tradeoff, not a failed guarantee.
- Browsing and leaving cost 0 AP; the first successful purchase in one visit costs 1 AP; later purchases in the same visit cost 0.
- Therefore Floor 1 remains `8–9 AP` shortest and `14–15 AP` full exploration.

## Rejected content

Wayfarer cannot reference any test-only card, badge-display fixture, debug key or debug reward. It also does not use random pools, weights or a new price formula.
