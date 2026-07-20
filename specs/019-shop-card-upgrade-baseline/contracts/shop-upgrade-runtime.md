# Shop Card Upgrade Runtime Contract

## Visit initialization

- Canonical visit input contains ShopId, offers and upgrade service.
- Existing two-argument BeginShopVisit wrappers create a disabled service.
- First visit freezes offers and service into `ShopStates[ShopId]`; repeat visits reuse both.

## Stable failure reasons

The runtime distinguishes at minimum: run not active, shop inactive, service disabled, invalid InstanceId, card not owned, missing Definition, stale current/next Definition, ineligible card, invalid chain, no next version, price missing and insufficient gold.

## Quote

- Built from authoritative current RunState; UI cannot supply or mutate price.
- One quote per physically owned valid InstanceId.
- A disabled quote still includes current identity when available and a stable reason.

## Atomic upgrade

- Finds the exact card in Backpack, BattleDeck, BurdenZone or SpecialZone.
- Revalidates the authored direct transition and service price.
- Replaces only Definition; InstanceId, index, zone owner and special-zone battle flag remain unchanged.
- Marks historical `bShopVisitHasPurchase` true to share existing visit semantics.

## Commerce/AP settlement

- Purchase and upgrade call the same private settlement after their state mutation succeeds in a working copy.
- First successful commerce costs 1 AP; later commerce costs 0 AP.
- A phase-ending first commerce closes the shop and invalidates visit ownership.
- Failure before commit has no reward, cost, AP, revision or notification.

## Revisions

- Upgrade success dirties BackpackStorage, Shop and Economy exactly once.
- Quote-only reads do not change revisions.
- Existing purchase result field names remain unchanged; new upgrade result uses an explicit first-commerce field.
