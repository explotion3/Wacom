# Data Model: 商店卡牌强化规则与制作合同基线

## Card upgrade family

### `UCardDefinition`

| Field | Type | Rule |
|---|---|---|
| `CardId` | `FName` | 每个版本唯一 |
| `UpgradeFamilyId` | `FName` | 链内显式非空且一致；无链卡回退 CardId |
| `NextUpgradeDefinition` | `UCardDefinition*` | 只指向直接下一稀有度；末级为空 |

Runtime helper：`ResolveUpgradeFamilyId()` 和 `MatchesCardIdOrUpgradeFamily()`。Definition 本身始终不可变。

## Shop static service

### `FShopCardUpgradePriceDefinition`

`FromRarity` 只能是 White/Blue/Yellow；`Price >= 0`。

### `FShopCardUpgradeServiceDefinition`

`bEnabled=false` 时价格表被忽略且运行时服务关闭；启用时 price source rarity 唯一。

### `UShopDefinition::CardUpgradeService`

静态制作入口。现有资产默认值关闭。

## Run visit input and state

### `FRunShopVisitRequest`

- `ShopId`
- `Offers`
- `CardUpgradeService`

旧 `BeginShopVisit(ShopId, Offers)` 构造 service-disabled request。

### `FRunShopState`

- 现有 `Offers`
- 第一次访问冻结的 upgrade service runtime copy

重复打开同一 ShopId 继续复用既有库存和服务配置，不接受调用方悄悄替换。

## Quote / Command / Result

### `FRunShopCardUpgradeQuote`

- exact `InstanceId`
- current/next Definition
- `UpgradeFamilyId`
- current/next rarity
- authoritative price
- `bCanUpgrade`
- stable `DisabledReason`

Quote 按 Backpack、BattleDeck、BurdenZone、各 SpecialZone 的稳定物理顺序生成。

### `FRunShopCardUpgradeCommand`

- `InstanceId`
- `ExpectedCurrentDefinition`
- `ExpectedNextDefinition`

命令不包含价格或 AP。

### `FRunShopCardUpgradeResult`

- success/reason
- InstanceId
- previous/upgraded Definition
- gold cost
- first commerce flag
- AP cost
- visit closed flag
- exploration resolution

## State transition

```text
Owned Instance(Current Definition)
  -> validate active run/shop/service/instance/stale guard/chain/price/gold
  -> clone FRunState + activity ticket
  -> deduct gold
  -> replace exact Instance.Definition
  -> set historical bShopVisitHasPurchase=true
  -> if first commerce: spend 1 AP and possibly close visit
  -> commit once + mark Backpack/Shop/Economy dirty + notify once
```

任何验证或 activity settlement 失败都丢弃 working state。

## Identity matching

- `AllowedCardDefinitions`: pointer-exact current version only.
- `AllowedCardIds`: matches `CardId` OR resolved `UpgradeFamilyId`.
- InstanceId remains the only mutable-card-instance identity.

## Save/Battle projection

- Save v5 records the upgraded DefinitionAssetPath without a new field.
- BattleDeck entry copies the instance's current Definition, so no Battle schema changes.
- Shop service state remains transient/in-memory like existing ShopStates.
