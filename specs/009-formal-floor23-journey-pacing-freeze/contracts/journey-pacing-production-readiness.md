# Contract: Journey Pacing and Production Readiness

## Frozen AP matrix

| Floor | Shortest | Full | Shop variance |
|---|---:|---:|---:|
| `Floor.Main.01` | 8–9 | 14–15 | 0–1 |
| `Floor.Main.02` | 8–9 | 14–15 | 0–1 |
| `Floor.Main.03` | 10 | 16 | 0 |
| **Journey** | **26–28** | **44–46** | **0–2** |

The range comes only from first successful purchases in the Floor 1 and Floor 2 Shops. Navigation, Map Travel and Floor Transition remain free.

## Journey-day targets

- Critical progression target: about 3 Journey days.
- Full exploration normal target: about 5–6 Journey days.
- Aggressive Night Exploration theoretical lower bound: about 4 Journey days.
- Normal Floor target remains about 1.5–2 days; the third exposure day remains recovery/error margin rather than required padding.

These are authoring targets, not a new runtime timer or forced day gate. Morning Planning, Camp, recovery, failed checks and optional actions are not counted in the 44–46 content AP total.

## Asset authority

| Namespace | Authority after this slice |
|---|---|
| Authoring | Existing `Journey.Authoring` / `Floor.Authoring.01` / `L_Exploration`; unchanged |
| Debug | Existing Debug Journey/Floor/map/builder fixtures; unchanged |
| Production | `Journey.Main.01` / `Floor.Main.01/02/03` and 46 content IDs are approved designs only; assets do not yet exist |

## Remaining hard blockers

1. **Journey success**: add a generic success outcome/state, settlement trigger, result/event, summary and return destination without conflating Defeat.
2. **Production definitions**: create and validate all 46 non-Debug Encounter/Event/Shop/Pickup definitions, including both sigil/seal Card+Credential grants.
3. **Asset authority audit**: select Production map/package authority after AssetRegistry, reference and binary hash review.
4. **Production scene implementation**: only then create Journey/Floor DataAssets, Descriptor/Anchor/Path/Branch/Host bindings and run editor/PIE validation.

## Closed blocker

`Floor.Main.02/03` no longer lack approved valid graph designs. Do not retain that obsolete blocker in long-term Docs after this slice.

## Forbidden shortcuts

- Creating minimal/empty Floor assets just to pass Journey validation.
- Reusing Debug Event/Shop/Pickup/Card definitions as Production slots.
- Treating a resolved terminal Encounter as an already implemented Journey success state.
- Using Defeat, `bRunActive=false`, a fake FloorEntrance, Actor label or Level Blueprint as the terminal contract.
- Editing `L_Exploration` or Debug assets before the production asset-authority audit.
