# Implementation Plan: Debug Shop 卡牌强化可玩竖切

## Summary

在 `main@a8d0c3566452a0d5d3304f24fad801c4ad3ea763` 上复用现有 D 盘 worktree。复用 Spec 019 的 Data/Run 强化合同，在 `WacomApp` 增加正式被动双页签 Screen，在 `WacomEditor` 增加四 Package 定向 seeder，并用现有 Debug Run 路径完成人工 PIE。

## Technical Context

- Engine: UE 5.8, C++/UMG/CommonUI, Win64 Development Editor。
- Modules: `WacomData → WacomRun → WacomApp`，`WacomEditor` 仅制作工具，`WacomTests` 覆盖合同。
- Storage: 两个 CardDefinition、一个 ShopDefinition、一个 WidgetBlueprint；全部 Git LFS。
- Runtime: 不改 SaveGame、GameplayTag、Battle contract 或 Shop 交易规则。
- Validation: 默认 Unity build、focused Automation、Blueprint compile、AssetRegistry/failed-load、MCP writer audit、SHA-256、LFS、用户 PIE。

## Constitution Check

- UI 只消费 Snapshot/ViewData 并提交 Command：PASS。
- 规则仍在 WacomRun，静态内容在 WacomData：PASS。
- 小型 presentation/flow/row/refresh helper，避免扩大巨型测试：PASS。
- 资产 mutation 使用准确 MCP session、writer lease 和四 Package allowlist：PASS。
- 不新增模块依赖、tag 或存档 schema：PASS。
- 用户 PIE 之前不提交：PASS。

## Architecture

```text
UShopDefinition / UCardDefinition
          │
          ▼
AWacomShopTriggerActor → FRunShopVisitRequest
          │
          ▼
URunSession → FRunShopSnapshot.CardUpgradeQuotes
          │
          ▼
UWacomShopUpgradePresentationBuilder
          │
          ▼
UWacomShopScreen + Row/Reconciler/RefreshGate
          │ player intent: InstanceId + expected definitions
          ▼
FWacomShopScreenFlow → URunSession::UpgradeOwnedCardAtShop
          │
          └─ Result → Exploration presentation + AppToast + refresh/deactivate
```

## Implementation Phases

1. Spec 020 工件、托管指针、静态合同。
2. 独立 upgrade presentation、row/reconciler、screen flow 与 refresh signature。
3. C++ fallback 双页签和 WBP binding surface；PIE 金币命令。
4. UI/Run checkpoint 编译与自动化。
5. Editor 定向 seeder、manifest/collision/strict compare tests。
6. MCP 四 Package 播种、第二次幂等、最终编译/自动化/资产门禁。
7. 用户 PIE；通过前保持未提交。

## Project Structure

```text
Source/WacomApp/Public/UI/Shop/
  WacomShopScreen.h
  WacomShopUpgradePresentationBuilder.h
  WacomShopUpgradeRowWidget.h
Source/WacomApp/Private/UI/Shop/
  WacomShopScreen.cpp
  WacomShopScreenFlow.*
  WacomShopRefreshGate.*
  WacomShopUpgradePresentationBuilder.cpp
  WacomShopUpgradeRowWidget.cpp
  WacomShopUpgradeRowListReconciler.*
  WacomShopUpgradePIEValidationCommands.cpp
Source/WacomEditor/Private/ContentBuilders/
  DebugShopUpgradeVerticalSlice.*
  DebugShopUpgradeVerticalSliceEditorCommand.cpp
Source/WacomTests/Private/UI/Shop/
  ShopUpgradePresentationSpec.cpp
  ShopUpgradeScreenSpec.cpp
  ShopUpgradePIEValidationSpec.cpp
Source/WacomTests/Private/Editor/
  DebugShopUpgradeVerticalSliceSpec.cpp
```

## Risk Controls

- Refresh reentrancy：Run 广播后只按 revision/signature 应用，selection 以 InstanceId reconcile。
- 访问关闭：结果内 Resolution 先交给 PlayerController，关闭只走一次。
- WBP binding 漂移：seed 后 load/compile/parent/CDO/widget-name 检查。
- 现有 Debug Shop 权威：preflight 精确比较前 24 Offer；不符即拒绝。
- 二进制污染：writer allowlist、实际 dirty paths、保存后哈希和 forbidden dependency closure。
