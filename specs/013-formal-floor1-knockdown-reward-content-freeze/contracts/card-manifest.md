# Contract: Floor 1 Knockdown Branch Card Manifest

## Exact package set

| # | CardId | Package path |
|---:|---|---|
| 1 | `Reward.SerpentWood.BrushSnake.Aid` | `/Game/Wacom/Data/Cards/Rewards/SerpentWood/BrushSnake/DA_Card_BrushSnake_Aid` |
| 2 | `Reward.SerpentWood.BrushSnake.Destroy` | `/Game/Wacom/Data/Cards/Rewards/SerpentWood/BrushSnake/DA_Card_BrushSnake_Destroy` |
| 3 | `Reward.SerpentWood.MoltGuard.Aid` | `/Game/Wacom/Data/Cards/Rewards/SerpentWood/MoltGuard/DA_Card_MoltGuard_Aid` |
| 4 | `Reward.SerpentWood.MoltGuard.Destroy` | `/Game/Wacom/Data/Cards/Rewards/SerpentWood/MoltGuard/DA_Card_MoltGuard_Destroy` |
| 5 | `Reward.SerpentWood.RootStalker.Aid` | `/Game/Wacom/Data/Cards/Rewards/SerpentWood/RootStalker/DA_Card_RootStalker_Aid` |
| 6 | `Reward.SerpentWood.RootStalker.Destroy` | `/Game/Wacom/Data/Cards/Rewards/SerpentWood/RootStalker/DA_Card_RootStalker_Destroy` |
| 7 | `Reward.SerpentWood.ShallowGuardian.Aid` | `/Game/Wacom/Data/Cards/Rewards/SerpentWood/ShallowGuardian/DA_Card_ShallowGuardian_Aid` |
| 8 | `Reward.SerpentWood.ShallowGuardian.Destroy` | `/Game/Wacom/Data/Cards/Rewards/SerpentWood/ShallowGuardian/DA_Card_ShallowGuardian_Destroy` |

Card object name equals the package leaf. CardId remains the stable rule identity; path, display name and description are authoring locators/content.

## Exact authored fields

| CardId suffix | DisplayName | Description template | Cost | Rarity | Keywords | TargetMode | Ordered Effects |
|---|---|---|---:|---|---|---|---|
| `BrushSnake.Aid` | 伏草藏身 | `获得 {Effect.0} 护盾，使一个敌方部位的当前意图延后 {Effect.1} 点先机。` | 1 | White | Tool | SingleEnemyPart | Shield 2 Player; Slow 1 SingleEnemyPart |
| `BrushSnake.Destroy` | 断牙毒刺 | `对一个敌方部位造成 {Effect.0} 点伤害并施加 {Effect.1} 层中毒。` | 1 | White | Weapon | SingleEnemyPart | Damage 3; Poison 1 |
| `MoltGuard.Aid` | 蜕甲壁垒 | `获得 {Effect.0} 护盾。` | 1 | Blue | Tool | None | Shield 7 Player |
| `MoltGuard.Destroy` | 裂壳重击 | `对一个敌方部位造成 {Effect.0} 点伤害。` | 1 | Blue | Weapon | SingleEnemyPart | Damage 6 |
| `RootStalker.Aid` | 盘根护身 | `获得 {Effect.0} 护盾，使一个敌方部位的当前意图延后 {Effect.1} 点先机。` | 1 | Blue | Tool | SingleEnemyPart | Shield 3 Player; Slow 2 SingleEnemyPart |
| `RootStalker.Destroy` | 毒根突袭 | `对一个敌方部位造成 {Effect.0} 点伤害并施加 {Effect.1} 层中毒。` | 1 | Blue | Weapon | SingleEnemyPart | Damage 5; Poison 1 |
| `ShallowGuardian.Aid` | 冠鳞庇护 | `获得 {Effect.0} 护盾。` | 1 | Yellow | Tool | None | Shield 10 Player |
| `ShallowGuardian.Destroy` | 碎冠毒潮 | `对所有存活敌方部位造成 {Effect.0} 点伤害并施加 {Effect.1} 层中毒。` | 2 | Yellow | Weapon | Damage 4 AllEnemyParts; Poison 1 AllEnemyParts |

## Exact tag mapping

- Rarity: `Card.Rarity.White` / `Card.Rarity.Blue` / `Card.Rarity.Yellow`.
- Keywords: `Card.Keyword.Tool` for Aid; `Card.Keyword.Weapon` for Destroy.
- Effects: `Status.Shield`、`Effect.ApplyStatus.Slow`、`Effect.Damage`、`Effect.ApplyStatus.Poison`.
- Targets: `Target.Player`、`Target.SingleEnemyPart`、`Target.AllEnemyParts`.
- All magnitudes are literal; Duration is 0; MagnitudeSource and TargetZone are empty unless required by the current struct default.

## Required empty fields

- `FCardPhysique{}` / all zero values.
- `PerfectReleaseEffects=[]`.
- `ZoneHooks=[]`.
- `Passives=[]`.
- No Swift, Exhaust or additional keyword.
- Art, audio, material and specialized CardView are not frozen by this contract.
