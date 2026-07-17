# Contract: Atomic Journey success settlement

## Trigger

全部条件同时成立：Run Outcome 为 InProgress；ticket 是当前 active Encounter ticket；ticket Node 精确等于配置终局；packet 为 Victory；`bWithdrawn=false`。

## Atomic order

1. 在 working copy 应用既有战斗奖励、压力、经验和卡牌结果。
2. 通过既有 Node Activity completion 扣 AP、推进时间并 Resolve terminal Node。
3. 从最终 working copy 构建合法 `FRunCompletionSummary`。
4. 设置 `Outcome=Succeeded` 并保存 summary。
5. 追加唯一 `JourneySucceeded` 作为最后事件。
6. 一次性提交 RunState、清除 active ticket、生成 PostSnapshot、广播一次。

任一步失败则不提交 working copy、ticket、版本、事件或广播。

## Priority and exclusions

- 终局非撤离 Victory 优先于同次战后压力失败或 mutual destruction。
- 撤离、普通 Boss、Defeat、Undetermined、invalid/stale/duplicate ticket 不成功。
- 成功后所有玩法写 API 返回其既有失败形态，必须零修改、零事件、零广播；Save build 仍允许。
