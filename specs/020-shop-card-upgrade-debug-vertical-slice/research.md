# Research: Debug Shop 卡牌强化可玩竖切

## Existing runtime seam

- Spec 019 已实现不可变强化链、Shop 静态价格、精确 Instance Quote/Command/Result 和 working-state 原子交易。
- `FRunShopSnapshot` 已包含 `CardUpgradeService` 与 `CardUpgradeQuotes`；本轮无需扩张 WacomRun 公共合同。
- `UWacomShopScreen` 当前只有购买列表和关闭按钮，但已经具备 RunSession 订阅、revision cache、访问 ownership token 和 AppToast seam。

## UI decision

- 采用一个 Screen、两个页签，而不是第二个顶层 Screen，避免重复 Shop visit ownership 与 CommonUI 生命周期。
- 强化候选 ViewData 独立于购买 Offer ViewData；稳定键是 InstanceId。
- 选中项保存在 Screen presentation state，不进入 RunState；刷新时按 InstanceId 重新解析最新 ViewData。
- C++ fallback 与 WBP 使用相同 binding surface，确保 WBP 缺失时仍可验证完整行为。

## Content decision

- 两张卡放在 Debug/ShopUpgrade 隔离目录；WBP 不引用它们。
- Debug Shop 保留 24 条既有 Offer 完整顺序，只追加一条，以防破坏既有测试用途。
- 定向 seeder 不复用 destructive `ShopBuilder::BuildShopContent()`；只同步其未来默认值。

## PIE command decision

- 命令使用 `FRunExplorationSnapshot` 校验 Journey/Floor/Node 和 `ActiveActivityKind`，只在 PIE 修改。
- 金币使用现有 `AddGold(3-current)`，所以不会降低金币，也不会绕过 Run 通知/revision。
- 命令不授予卡牌，保证真实购买链被验证。

## Rejected alternatives

- 强化事件：会绕开 Shop visit/AP/价格合同，拒绝。
- WBP 内直接遍历 DataAsset 或扣金币：违反 passive UI，拒绝。
- 运行完整 ShopBuilder：会替换 Debug Shop 并可能覆盖人工/其它 Agent 内容，拒绝。
- 临时在角色 StarterDeck 塞测试卡：污染角色和跳过购买路径，拒绝。
