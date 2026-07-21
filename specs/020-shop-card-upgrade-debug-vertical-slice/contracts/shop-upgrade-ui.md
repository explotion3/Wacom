# Shop Upgrade UI Contract

1. `UWacomShopScreen` owns Shop visit presentation lifecycle and current page/selection only.
2. `UWacomShopUpgradePresentationBuilder` converts authoritative Quote into immutable UI facts.
3. Rows emit only InstanceId selection intent; Screen resolves the cached Quote.
4. Upgrade flow sends InstanceId plus expected current/next Definition to `URunSession`.
5. Success applies Exploration Resolution before optional deactivation, then shows success Toast.
6. Failure never mutates local model and maps stable DisabledReason to a Chinese warning Toast.
7. Refresh is event-driven; no Tick or UI-side inventory inference.
8. WBP bindings may replace visuals but cannot bypass Screen flow.
9. Upgrade definitions may keep the same DisplayName. Current/next cards use their own rarity border; only changed numeric badges on the next-card comparison receive UI-only increased/decreased emphasis.
10. Value emphasis is comparison-local and passive. It does not mutate CardDefinition, activate Battle rewrite materials, or persist into ordinary hand/backpack/battle card faces.
