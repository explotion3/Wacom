# Contract: SerpentWood RunEvents

## Shared graph contract

Each of the four events has:

- one node with `NodeId=Start` and `StartNodeId=Start`;
- no NextNode transition;
- every Choice `ActionPointPolicy=Automatic`;
- every Choice `bMarkEventCompleted=true` and `bCloseEventAfterResolve=true`;
- no card payment;
- a successful terminal choice cost of exactly 1 AP under the existing RunEvent rule.

There are exactly 13 ChoiceIds across the four events. ChoiceId only needs to be unique within its Event node, but all listed IDs are intentionally distinct for diagnostics.

## `Event.SerpentWood.CastSkin`

| ChoiceId | Conditions | Effects |
|---|---|---|
| `StudyPattern` | none | Set flag `SerpentWood.MoltTrailKnown` |
| `SellSkin` | none | Gold +2; Misdeed +2 |
| `LeaveUntouched` | none | none |

## `Event.SerpentWood.HunterTrace`

| ChoiceId | Conditions | Effects |
|---|---|---|
| `ReadTrail` | none | Set flag `SerpentWood.MarshRouteKnown` |
| `LootPack` | none | Gold +3; Misdeed +3 |
| `BuryRemains` | none | Misdeed -2 |

## `Event.SerpentWood.MerchantRumor`

| ChoiceId | Conditions | Effects |
|---|---|---|
| `TradeMoltClue` | RunFlagSet `SerpentWood.MoltTrailKnown` | Set flag `SerpentWood.MarshRouteKnown` |
| `BuyMap` | MinGold 1 | Gold -1; set flag `SerpentWood.MarshRouteKnown` |
| `Eavesdrop` | none | Misdeed +2; set flag `SerpentWood.MarshRouteKnown` |
| `Decline` | none | none |

## `Event.SerpentWood.PoisonMarsh`

| ChoiceId | Conditions | Effects |
|---|---|---|
| `FollowMarkedRoute` | RunFlagSet `SerpentWood.MarshRouteKnown` | Fatigue -2 |
| `BurnOffering` | MinGold 2 | Gold -2 |
| `WadeThrough` | none | Fatigue +5 |

## Schema mapping

| Design term | Live static type/value |
|---|---|
| Gold requirement | `EWacomRunEventConditionType::MinGold`, `Value=N` |
| Flag requirement | `RunFlagSet`, `FlagId=<FName>` |
| Gold delta | `EWacomRunEventEffectType::AddGold`, signed `Value` |
| Pressure delta | `AddPressure`, signed `Value`, `PressureType=Misdeed/Fatigue` |
| Flag write | `SetRunFlag`, `FlagId=<FName>` |

No new enum, tag, script callback, numeric flag or direct AP effect is permitted.

## Transaction and persistence boundary

- Conditions and effects are interpreted by the existing RunEvent working-state transaction; failed conditions or effects produce zero partial mutation.
- Negative Gold is paired with `MinGold`; no content-side clamp or alternate currency rule is introduced.
- Negative pressure uses the existing pressure add/clamp behavior.
- RunFlags are current-Run FName facts and remain outside SaveGame v5. The design does not promise clue persistence across disk restore.
- Choice completion marks the current scene Event PersistentId through the normal choice contract; content does not use EventId as runtime state key.
