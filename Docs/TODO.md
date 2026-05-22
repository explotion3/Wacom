# TODO 索引

> 本文只保留短期任务入口和拆分后的索引。长期未实现功能看 [Roadmap](./Roadmap.md)，临时写法和技术债看 [TechDebt](./TechDebt.md)，待确认规则问题看 [Questions](./Questions.md)。

---

## 文档分工

| 文档 | 职责 | 维护规则 |
|---|---|---|
| [Roadmap](./Roadmap.md) | 未实现功能、后续方向、可按阶段推进的内容扩展 | 新功能还没有进入短期实现前，先放这里 |
| [TechDebt](./TechDebt.md) | 临时写法、兼容字段、临时决定、正式替代方案 | 如果代码里出现 `TODO(技术债)`，同步到这里 |
| [Questions](./Questions.md) | 会影响规则、策划口径或长期架构的待确认问题 | 不在代码里静默写死这些问题 |
| 本文 | 短期任务索引和跳转 | 不承载长说明，避免重新变成大杂烩 |

---

## 短期任务索引

建议推进顺序：先收口 P0 规则问题，再做地图 / 节点服务；地图口径确定后再接击倒分支、RunEvent / Shop 的节点生成与存档恢复。

| 优先级 | 状态 | 任务 | 归属 | 入口 |
|---|---|---|---|---|
| P0 | Blocked: 策划确认 | 确认减速、暮气、冻结等状态的数值公式和触发时机 | 战斗规则 | [Questions: 状态与触发时机](./Questions.md#questions-status) |
| P0 | Blocked: 策划确认 | 明确击倒事件 Aid / Withdraw / Destroy 的正式分支效果 | 战斗 / Run | [Questions: 击倒与战后结算](./Questions.md#questions-knockdown) |
| P1 | Blocked: P0 击倒口径 | 接入击倒事件实际分支、奖励卡差异化和节点事件联动 | 战斗 / RunEvent | [Roadmap: 击倒事件扩展](./Roadmap.md#roadmap-knockdown) |
| P1 | Ready: 需先定模块边界 | 推进地图系统：节点、通道、迷雾、撤离回路、地图状态 | Run / 地图 | [Roadmap: 地图与探索](./Roadmap.md#roadmap-map) |
| P1 | Blocked: 地图节点口径 | 推进 RunEvent：随机事件池、更多条件效果、地图节点生成、存档 | Run / Data / App | [Roadmap: 探索事件](./Roadmap.md#roadmap-runevent) |
| P1 | Blocked: 地图节点 / 存档口径 | 商店正式化：随机商品池、价格公式、正式 WBP、存档接入 | Run / Data / App | [Roadmap: 商店](./Roadmap.md#roadmap-shop) |
| P1 | Ready: 内部重构 | 继续拆分 `URunSession`：RunEvent executor、Shop transaction、SaveGame serializer | Run 架构 | [TechDebt: RunSession 结构债](./TechDebt.md#techdebt-run-session) |
| P1 | Ready: 美术 / WBP 工作 | 背包正式 WBP、拖拽 polish、必要时做增量刷新 | UI / Run | [Roadmap: 背包 UI](./Roadmap.md#roadmap-backpack-ui) |
| P2 | Blocked: HD-2D 表现方案 | 战斗手牌表现升级：扇形布局、拖拽出牌、3D 目标选择 | UI / 战斗表现 | [Roadmap: 战斗 UI](./Roadmap.md#roadmap-battle-ui) |
| P2 | Blocked: Demo 范围确认 | 存档系统恢复：Bootstrap 读盘、PauseMenu Save、MainMenu Continue | Run / App | [Roadmap: 存档恢复](./Roadmap.md#roadmap-save) |
| P2 | Ready: WBP 化后清理 | 清理 UI MVVM 迁移尾项：WBP ViewBinding、逐步移除 C++ 手动 SetText fallback | UI 架构 | [TechDebt: UI 架构债](./TechDebt.md#techdebt-ui-architecture) |

---

## 迁移说明

- 原 `TODO.md` 的未实现功能已迁入 [Roadmap](./Roadmap.md)。
- 原 `TODO.md` 的临时写法、临时决定、兼容入口已迁入 [TechDebt](./TechDebt.md)。
- 原 `TODO.md` 的待确认规则问题已迁入 [Questions](./Questions.md)。
- 已在 `WacomRun.md`、`WacomBattle.md`、`WacomApp.md`、`WacomUI.md`、`WacomData.md` 中正式化的一版实现，不再作为短期待办重复记录；只保留其后续扩展方向。
- 状态为 `Blocked` 的任务不应直接写死设计口径；先在 `Questions.md` 或对应领域文档中收口。
- 状态为 `Ready` 的任务仍需在开始前复核领域文档和代码，避免 Roadmap 里的描述变成新的事实源。
