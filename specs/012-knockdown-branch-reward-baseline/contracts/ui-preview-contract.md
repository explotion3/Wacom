# Knockdown Choice UI Preview Contract

## Data source

`UBattleSession::BuildPendingKnockdownChoiceView()` 是唯一数据入口。Battle Availability helper 对当前 pending 部位调用与结算相同的奖励查询，并只投影：

- `bHasRewardCard`
- `RewardCardId`
- `RewardCardName`

App 不接收或保存 `UCardDefinition*`。

## Display

- Aid 分支：有卡显示 `奖励：<名称>`，无卡显示 `无卡牌奖励`。
- Destroy 分支：同上。
- 名称优先 `DisplayName`，为空回退 `CardId`。
- Withdraw 不显示奖励摘要，不新增占位 CardView。

原生 fallback 和未来 WBP 使用 `AidRewardText` / `DestroyRewardText` 可选绑定锚点。

## Passive and lifecycle rules

- 按钮可用性只读取现有 `bAvailable`，不得因奖励为空而禁用。
- `SetContext` 每次完整覆盖旧文本与按钮状态。
- `NativeConstruct` 支持 context-before-construct。
- Modal layer、Menu input、ESC/Gamepad Back 拦截和 HUD 命令入口不变。
- Widget 不加载卡面、缩略图，不计算 fallback/validation，不订阅 BattleState，不使用 Tick。
