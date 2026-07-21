# Data Model: Debug Shop 卡牌强化可玩竖切

## Debug card assets

| Package | CardId | Family | Rarity | Effects | Next |
|---|---|---|---|---|---|
| `/Game/Wacom/Data/Cards/Debug/ShopUpgrade/DA_Card_TestShopUpgrade_VenomProof_White` | `Test.ShopUpgrade.VenomProof.White` | `Test.ShopUpgrade.VenomProof` | White | Damage 3; Poison 1 | Blue |
| `/Game/Wacom/Data/Cards/Debug/ShopUpgrade/DA_Card_TestShopUpgrade_VenomProof_Blue` | `Test.ShopUpgrade.VenomProof.Blue` | `Test.ShopUpgrade.VenomProof` | Blue | Damage 5; Poison 2 | null |

共享字段：DisplayName 均为“试制毒牙”、Cost 1、Weapon、SingleEnemyPart、零 Physique、无 Passive/ZoneHook/PerfectRelease。White/Blue 用现有稀有度边框区分；下一版本预览对变化的数值应用 UI-only `ValueEmphasis`，普通卡面保持 neutral。

## Debug shop mutation

- Package: `/Game/Wacom/Data/Shops/DA_Shop_DebugSnake`
- Existing offers: 前 24 条逐字段保持。
- Appended offer: White test card, 1 Gold。
- Upgrade prices: White 2, Blue 3, Yellow 4；服务 enabled。

## Presentation types

### `FWacomShopCardUpgradePresentationView`

- InstanceId
- CurrentDefinition / NextDefinition
- CurrentCardViewData / NextCardViewData
- Current/Next name、rarity、cost
- PriceText / ActionText / StatusText / ChangeSummaryText
- bCanUpgrade / DisabledReason

### `EWacomShopPage`

- Purchase
- Upgrade

Screen state 仅保存当前页签和选中 InstanceId；任何价格、资格、Definition 仍来自最新 Quote。

## Seeder report

- created / modified / existing / saved / failed
- savedPackages
- diagnostics
- first pass / second pass
- report path / failure category / exit code
