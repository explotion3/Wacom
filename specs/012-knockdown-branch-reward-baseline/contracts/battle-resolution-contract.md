# Battle Resolution Contract

## Selection transaction

Aid/Destroy 继续在当前 `KnockdownChoice` 命令事务中完成：

1. 使用既有 Availability 校验。
2. 记录 `FKnockdownChoice` 并发布 `KnockdownChoiceMade`。
3. 用当前 pending 部位 Definition 的统一查询取得所选奖励。
4. 有奖励时使用既有 `FBattleCardGrantService::GrantCardToHand()`，发布 `CardGained`、记录 `CardGainedResolved` 并追加 `FBattleGainedCard`。
5. `FBattleGainedCard.SourceChoice` 必须等于实际选择。
6. 继续既有 pending queue / BattleEnd 判定。

失败仍由 `UBattleSession::ResolveCommand()` working copy 保证零修改、零事件、零版本变化。

## Empty reward

- 不禁用 Aid/Destroy。
- 命令正常成功。
- 不创建 runtime card。
- 不发布 `CardGained`，不记录 checkpoint，不追加 `GainedCards`。

## Preserved contracts

- Withdraw 不查询奖励、不获得卡，且仍只在有存活部位时可用。
- 最后存活部位仍必须 Aid/Destroy。
- 多部位连续击倒仍逐条 request/resolve。
- 普通手牌上限在 reward grant 后按既有顺序执行。
- 经验/AP/撤离重入/Outcome 不变。
- Victory/Withdraw 由 Run 持久化既得卡，Defeat/Undetermined 不持久化。
- `FBattleResultPacket`、`FBattleGainedCard`、`FRunState`、SaveGame 均不改 schema。
