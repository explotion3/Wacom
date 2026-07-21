# Validation Contract

## C++ checkpoints

- Default Unity `WacomEditor Win64 Development`.
- `Wacom.UI.Shop.UpgradePresentation`
- `Wacom.UI.Shop.UpgradeScreen`
- `Wacom.UI.Shop.UpgradePIEValidation`
- `Wacom.Run.Shop.CardUpgrade`
- `Wacom.Run.NodeActivity.Shop`
- `Wacom.Run.NotificationCoalescing`
- `Wacom.Run.SnapshotRevisions.ShopMutationPaths`
- `Wacom.UI.Shop`

## Asset gates

- two CardDefinition assets load, validate and form one legal White→Blue chain;
- Debug Shop has exactly 25 offers and exact 2/3/4 price service;
- Shop WBP loads, compiles, inherits `UWacomShopScreen`, owns required bindings and is registered to `UI.Widget.ShopScreen`;
- AssetRegistry/failed-load and forbidden closure pass;
- second seed pass has zero writes;
- exact four binary changes are LFS, all protected hashes unchanged.

## Manual PIE

User validates Entry seed, Battle return, buy, upgrade, mouse/Back lifecycle and post-Shop Run controls. Any runtime or asset fix after PIE invalidates the previous acceptance and requires rerunning the affected tests and full route.
