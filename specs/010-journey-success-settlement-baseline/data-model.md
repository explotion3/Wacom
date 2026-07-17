# Data Model: Journey 成功结算与终局交接基线

## 1. Static Journey terminal

`UWacomJourneyDefinition` 新增：

| Field | Type | Owner | Validation |
|---|---|---|---|
| `DisplayName` | `FText` | WacomData | 可空；总结显示时回退 `JourneyId` |
| `SuccessTerminalNode` | `FWacomMapNodeHandle` | WacomData | 可未配置；配置后必须满足终局静态合同 |

终局静态合同：FloorId 是 Journey 最后一个 Floor；Node 存在、`Encounter`、payload `bBoss=true`、从该 Floor Entry 可达、出度为 0；最后一层无 `FloorEntrance`。缺失终局是 legacy warning，不使 Debug Journey invalid；非法已配置终局是 error，Runtime 初始化也拒绝。

## 2. Run outcome state machine

```text
InProgress --terminal non-withdrawn Victory--> Succeeded
InProgress --Defeat / existing pressure-or-finger failure--> Failed
Succeeded --gameplay writes--> rejected
Failed    --restore as active Run--> rejected by Save apply
```

`ERunOutcome` 是权威终态。`IsRunActive()` 在 effective outcome 为 InProgress 时为 true；`IsRunFailed()` 仅在 effective outcome 为 Failed 时为 true。Raw Succeeded 优先于同一战斗产生的压力失败线。

## 3. Completion summary

| Field | Meaning | Source time |
|---|---|---|
| `JourneyId` | 完成的 Journey identity | working state |
| `TerminalNode` | 成功终局 handle | Journey definition |
| `CompletionDay` | 完成后的当前天数 | AP/时间结算后 |
| `EnteredFloorCount` | `EnteredDayNumber > 0` 的 Floor 数 | working progress |
| `TotalFloorCount` | Journey/Floor progress 总数 | working progress |
| `ResolvedNodeCount` | lifecycle 为 Resolved 的 Node 数 | terminal Node resolve 后 |
| `TotalNodeCount` | 全 Floor Node progress 总数 | working progress |
| `FinalPressure` | 战后总压力 | battle settlement 后 |

合法摘要要求 JourneyId/Terminal handle 有效，天数与计数非负，entered≤total floors，resolved≤total nodes，total floor/node 大于 0，最终压力有限。

## 4. Snapshot and event projection

`FRunExplorationSnapshot` 投影 `Outcome`、`bHasCompletionSummary` 和 `CompletionSummary`。成功 Resolution 追加一个 `JourneySucceeded`：

| Event field | Value |
|---|---|
| `Type` | `JourneySucceeded` |
| `Node` | `SuccessTerminalNode` |
| `Detail` | `JourneyId` |
| Ordering | 本次 Resolution 最后一个 event |

Snapshot 不生成成功；它只投影已提交的 RunState。

## 5. SaveGame v5

Save object 保留 legacy v4 `bRunActive` 仅作迁移来源，并新增：

- `Outcome`
- `bHasCompletionSummary`
- `FRunCompletionSummarySaveEntry CompletionSummary`

迁移：v4 active→InProgress；v4 inactive→Failed；摘要清空；version→5。v5 Succeeded 必须有合法摘要；InProgress/Failed 不携带摘要。Build 可以保存终态语义，但 Apply 对 Succeeded/Failed 在触碰活动 RunState 前原子拒绝。

## 6. App ViewData and flow state

`EGameFlowState::JourneySummary` 表示只读终局 UI 阶段。`FWacomJourneySummaryViewData` 包含状态标题“Journey 成功”、Journey 标题、完成天数、Floor 进度、Node 进度、最终压力。来源仅为 Run summary + Journey display name；Screen 不持有 RunSession。

Screen 生命周期：Apply ViewData → CommonUI activate/focus → button 或 Back 广播一次 continue intent → GameMode unbind/teardown → next-tick travel。NativeDestruct 清除绑定和 transient widget refs；push failure 跳过 Screen 并进入相同 handoff。
