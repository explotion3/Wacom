# Contract: Journey summary and main-menu handoff

## Source and timing

- GameMode 只从当前 settlement Resolution 的 `JourneySucceeded` 事件进入成功流程。
- 不检查 Actor label、Encounter ID、Boss class 或硬编码 NodeId。
- Battle HUD/Session 正常清理；现有 Return-to-Run camera staging 双 barrier 完成后才展示总结。
- 成功路径不调用 exploration hand restore 或 interact Toast refresh。

## Passive screen

- 数据源：`FWacomJourneySummaryViewData`，由 completion summary + Journey display name 构建。
- Screen 只渲染并广播单次 continue intent；不访问 RunSession、不 OpenLevel。
- 原生 fallback 始终可构建；Blueprint 可替换表现但不能改变流程所有权。
- activate 时取得 UI focus；destruct/deactivate 清理委托和 transient refs。

## Handoff

- Continue button 与 ESC/Back 合并为同一 idempotent GameMode handler。
- GameMode 拆除 PrimaryLayout，下一帧 travel 到 `/Game/Wacom/Maps/L_MainMenu`。
- Screen push/cast 失败直接调用同一 handoff。
- 重复 intent、重复 event 或 stale callback 不得安排第二次 travel。
