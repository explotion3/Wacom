# Debug Content Manifest

Writer allowlist is exactly:

1. `/Game/Wacom/Data/Cards/Debug/ShopUpgrade/DA_Card_TestShopUpgrade_VenomProof_White`
2. `/Game/Wacom/Data/Cards/Debug/ShopUpgrade/DA_Card_TestShopUpgrade_VenomProof_Blue`
3. `/Game/Wacom/Data/Shops/DA_Shop_DebugSnake`
4. `/Game/Wacom/UI/Shop/WBP_ShopScreen`

No other package may be saved. The WBP may reference project UI classes/assets such as generic `WBP_CardView`, but must not reference either Debug card or the Debug Shop.

First authorized seed expects `3 created / 1 modified / 4 saved`; immediate second pass expects all zero.

The White and Blue Debug cards both display as `试制毒牙`. The legacy Blue display name `强化试制毒牙` is an exact one-time repair state; the targeted seeder may rename only that asset, then its immediate second pass must save nothing.
