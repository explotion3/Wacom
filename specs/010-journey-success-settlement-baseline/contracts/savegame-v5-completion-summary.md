# Contract: SaveGame v5 outcome and completion summary

## Schema

- `SaveVersion=5`
- `Outcome: ERunOutcome`
- `bHasCompletionSummary: bool`
- `CompletionSummary: FRunCompletionSummarySaveEntry`
- legacy `bRunActive` 保留，仅供 v4→v5 migration 读取

磁盘摘要字段与 runtime summary 一一对应，但类型独立，避免运行时扩展意外改变历史 schema。

## Migration

| Source | Outcome | Summary |
|---|---|---|
| v0-v3 经既有链到 v4 active | InProgress | absent |
| v4 active | InProgress | absent |
| v4 inactive | Failed | absent |

## Validation and apply

- Succeeded 必须带合法摘要。
- InProgress/Failed 不得携带摘要。
- Apply 只接受 InProgress；Succeeded/Failed 在资产解析和 RunState 写入前原子拒绝。
- Build 可以序列化 Succeeded + summary，便于未来 History/summary persistence；本轮不启用实际保存总开关。
