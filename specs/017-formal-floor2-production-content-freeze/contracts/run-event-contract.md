# Contract: MoltCavern RunEvent

## 1. Shared shape

三个 Event 均满足：

```text
StartNodeId=Start
Nodes.Count=1
Choice.ActionPointPolicy=Automatic
Choice.NextNodeId=None
Choice.bMarkEventCompleted=true
Choice.bCloseEventAfterResolve=true
CardPayment=none
```

每个成功选择固定消耗 1 AP。RunFlag 与 Pressure identity 是现有 FName，不新增 GameplayTag；flag 继续是内存态，不承诺 SaveGame restore。

## 2. Exact choices

| Event | Choice | Condition | Ordered effects |
|---|---|---|---|
| CastoffEcho | ReadRitePattern | none | Set `MoltCavern.RitePatternKnown` |
| CastoffEcho | GatherScaleDust | none | Gold +3; Misdeed +2 |
| CastoffEcho | RestAmongCastoffs | none | Fatigue -2 |
| LostDelver | GuideToOldWell | none | Set `MoltCavern.DelverRouteKnown`; Misdeed -2 |
| LostDelver | TakeAbandonedPack | none | Gold +4; Misdeed +3 |
| LostDelver | ShareRations | none | Fatigue -3 |
| MoltingRite | RepeatKnownRite | RitePatternKnown | Fatigue -3 |
| MoltingRite | FollowDelverMarks | DelverRouteKnown | Wound -2 |
| MoltingRite | OfferCoin | MinGold 3 | Gold -3; Misdeed -2 |
| MoltingRite | ForcePassage | none | Fatigue +5; Wound +1 |

Choice 数量精确为 10。有序 effect 顺序是未来 exact-structure validation 的一部分；自然语言文案不是规则来源。

## 3. Route economy

- Route A 的 GatherScaleDust 从 0 得到 3 Gold，可购买一个 3 Gold Offer。
- Route B 的 TakeAbandonedPack 从 0 得到 4 Gold，可购买一个 3/4 Gold Offer。
- ReadRitePattern 与 GuideToOldWell 分别服务 MoltingRite 的两条条件分支；选择情报意味着主动放弃即时购买力。
- ForcePassage 是无前置兜底，不会因未走信息路线造成软锁。

本合同不增加随机 Event pool、多节点 Event、CardPayment、flag 持久化或跨 Floor 事件状态规则。
