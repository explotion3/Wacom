# Contract: Run Node Activity Settlement

## Common lifecycle

```text
Idle current node
  -> Begin typed activity
  -> Active token (+ optional AP reservation)
  -> Complete typed result / Cancel
  -> One atomic Run commit
```

- Begin 验证 current node/type/content、互斥事务和固定成本。
- App/UI 只持有 opaque token，不直接扣 AP 或推进 lifecycle。
- Complete 以 working state 同时结算内容、奖励、压力、AP、访问状态和 Map Node lifecycle。
- UI push/场景启动失败必须 Cancel，释放预留且不留下 active visit。

## Encounter

- BeginEncounter 需要 Encounter current node 和至少 1 AP，预留 1 并返回 Battle init facts。
- Victory：提交预留，结算 Battle Result，清撤离进度，Resolve node。
- Withdraw：释放预留，保留 destroyed parts、experience、gained cards，node 保持 Visited。
- Defeat：Run 结束，预留不推进已结束旅程的时段。
- 每个有效 Battle Result继续 Fatigue +1；Wound 使用 packet 中 50%/20% threshold 与 mutual-destruction facts。
- 删除 GameMode `ConsumeNode`、Trigger completion 和 no-trigger settlement wrapper。

## RunEvent

新增选项级：

```text
ActionPointPolicy = Automatic | Free | Fixed
FixedActionPointCost >= 0
```

- Automatic：close/complete terminal choice 为 1，否则 0。
- 正成本 choice 必须 terminal；Validator 阻止跨时段 active event。
- `MinNodeCount` 改为 `MinActionPoints`。
- 删除 `ConsumeNode` effect；旧 Debug DataAsset 在迁移命令中重写并重存。
- effect、paid card、AP、event state 与 node resolve 同一 working state 回滚。

## Shop

- Begin/浏览/空手 EndVisit 为 0；Shop node 首次安全进入可 Resolved。
- 第一次成功 purchase 在同一事务中扣金币、发卡、标记 offer、消费 1 AP。
- 同一 visit 后续 purchase 为 0。
- 如果首次 purchase 耗尽当前 phase，结果同时结束 visit，App 关闭 Shop Screen。
- 无效 offer、金币不足、重复 purchase 和过期 visit token 全部无副作用。

## Treasure/Search

- Pickup 或 Card Interaction 校验失败为 0。
- 首次成功奖励与 1 AP 原子提交并 Resolve node。
- 重复完成返回明确失败，不再次消费或发奖。

## Camp

- BeginCamp 仅在 Night/idle/AP>=1，预留 1 并返回 CampActivityRequested。
- 活动 handler 按 Rest/CardUpgrade/SpecialEvent/Backpack/Skill 分型；本轮 production 不实现具体 handler。
- Handler 只能返回 typed outcome，由 Run core 验证后提交；不能获得可写 `FRunState` 或通过 UI 直接修改卡实例。
- Complete 提交预留，放弃 Night 剩余点，跳 Sunrise，进入新 Morning。
- Cancel 释放预留；为 Camp 进行的免费同层 relocation 保留。
