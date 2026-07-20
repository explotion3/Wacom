# Validation Contract

## Compile checkpoints

Each C++ checkpoint runs default Unity `WacomEditor Win64 Development` after Editor/Live Coding are closed. Unreal invocations include `-NoDreamShaderEditorBridge`.

## Focused automation

1. Data: `Wacom.Data.Card`, `Wacom.Data.Shop`, `Wacom.Data.CardUpgrade`.
2. Run: `Wacom.Run.Shop.CardUpgrade`, `Wacom.Run.NodeActivity.Shop`, `Wacom.Run.NotificationCoalescing`, `Wacom.Run.SnapshotRevision`.
3. Compatibility: `Wacom.Run.Save`, Backpack/workspace, Event payment, WorldInteraction, Battle initialization/rule content and App card presentation.

## Asset gate

- Read-only load/AssetRegistry/failed-load audit for existing CardDefinition and ShopDefinition assets.
- Record before/after aggregate SHA-256 and require zero tracked binary changes.
- No builder, no package save, no Blueprint compile and no PIE.

## Repository gate

- `git diff --check`.
- Scope audit: no Build.cs, GameplayTag declaration/config, SaveGame schema/version or binary path.
- `git lfs fsck` and Git/LFS status.
- Changes remain unstaged/uncommitted until user approval.
