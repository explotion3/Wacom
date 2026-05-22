# Wacom Docs 入口

本文是后续接手者的文档导航。规则真相优先看领域文档，历史过程看 `DevLog/`，短期任务看 `TODO.md`，长期方向、技术债和未决问题分别看 `Roadmap.md`、`TechDebt.md`、`Questions.md`。

## 建议阅读顺序

1. 先读 [`../AGENTS.md`](../AGENTS.md)：确认协作规则、模块边界、允许修改范围和验证要求。
2. 再读 [`Architecture.md`](Architecture.md)：理解模块依赖方向、Public/Private 边界、Battle/Run/App 的长期分工。
3. 需要了解整体玩法背景时读 [`Game_Design.md`](Game_Design.md)：它是全局 GDD 和设计语境；若和领域文档冲突，以领域文档为准。
4. 按任务领域读对应规则文档：
   - 战斗规则：[`WacomBattle.md`](WacomBattle.md)
   - Run / 探索 / 存档 / 背包：[`WacomRun.md`](WacomRun.md)
   - 静态数据、卡牌、敌人、角色、GameplayTag：[`WacomData.md`](WacomData.md)
   - App 编排、输入、GameMode、世界交互：[`WacomApp.md`](WacomApp.md)
   - UI 数据流、Toast、Screen、HUD、ViewData：[`WacomUI.md`](WacomUI.md)
5. 做 WBP 或 UI 资产承接时，再读绑定清单：
   - [`UI_Backpack_WBP_Binding.md`](UI_Backpack_WBP_Binding.md)
   - [`UI_Battle_WBP_Binding.md`](UI_Battle_WBP_Binding.md)
6. 开始实现前扫计划和风险：
   - 短期任务索引：[`TODO.md`](TODO.md)
   - 长期方向、技术债、待确认问题：[`Roadmap.md`](Roadmap.md)、[`TechDebt.md`](TechDebt.md)、[`Questions.md`](Questions.md)
7. 需要理解某个设计为什么变成现在这样时，再查 [`DevLog/`](DevLog/)。

## 常用专题入口

| 主题 | 当前事实入口 | 相关入口 |
|---|---|---|
| 战斗规则 | [`WacomBattle.md`](WacomBattle.md) | 战后结算 [`WacomRun §8`](WacomRun.md#wacomrun-battle-settlement)，战斗 UI [`WacomUI §8`](WacomUI.md#wacomui-battle-ui)，WBP 合约 [`UI_Battle_WBP_Binding.md`](UI_Battle_WBP_Binding.md) |
| 击倒与奖励卡 | [`WacomBattle §12`](WacomBattle.md#wacombattle-result) | 部位奖励字段 [`WacomData §3`](WacomData.md#wacomdata-enemy-part)，Run 入包 [`WacomRun §8`](WacomRun.md#wacomrun-battle-settlement)，后续方向 [`Roadmap: 击倒事件扩展`](Roadmap.md#roadmap-knockdown) |
| 战斗表现 | [`WacomUI §8`](WacomUI.md#wacomui-battle-ui) | UI 技术债 [`TechDebt: UI 层技术债`](TechDebt.md#techdebt-ui)，后续表现 [`Roadmap: 战斗 UI`](Roadmap.md#roadmap-battle-ui) |
| 背包构筑 | [`WacomRun.md`](WacomRun.md) | 背包 UI [`WacomUI.md`](WacomUI.md)，WBP 合约 [`UI_Backpack_WBP_Binding.md`](UI_Backpack_WBP_Binding.md)，后续表现 [`Roadmap: 背包 UI`](Roadmap.md#roadmap-backpack-ui) |
| 商店与探索事件 | [`WacomRun.md`](WacomRun.md) | 静态定义 [`WacomData.md`](WacomData.md)，世界交互 [`WacomApp.md`](WacomApp.md)，后续方向 [`Roadmap: 商店`](Roadmap.md#roadmap-shop) / [`Roadmap: 探索事件`](Roadmap.md#roadmap-runevent) |
| 地图与节点 | 当前节点消耗规则见 [`WacomRun.md`](WacomRun.md)，地图设计语境见 [`Game_Design §10`](Game_Design.md#game-design-run-map) | 待确认问题 [`Questions: Run、探索与地图`](Questions.md#questions-run-map)，后续方向 [`Roadmap: 地图与探索`](Roadmap.md#roadmap-map) |
| 存档恢复 | [`WacomRun.md`](WacomRun.md) | 技术债 [`TechDebt: 数据与存档债`](TechDebt.md#techdebt-data-save)，后续方向 [`Roadmap: 存档恢复`](Roadmap.md#roadmap-save) |

## 文档职责

| 文档 | 职责 |
|---|---|
| [`../AGENTS.md`](../AGENTS.md) | 项目协作规则、模块职责、代码边界、测试命令和文件安全要求。 |
| [`Architecture.md`](Architecture.md) | 全局架构、模块依赖方向、目录结构、Command / Snapshot / Event 等跨领域约定。 |
| [`Game_Design.md`](Game_Design.md) | 全局 GDD、设计背景和玩法语境；与领域文档冲突时，以领域文档为准。 |
| [`WacomBattle.md`](WacomBattle.md) | 单场战斗规则真相，包括战斗流程、手牌区域、效果执行、敌方行动、战斗结果包。 |
| [`WacomRun.md`](WacomRun.md) | 战斗外 Run 状态真相，包括背包、压力、经验、时段、商店、探索事件、存档和战斗回传。 |
| [`WacomData.md`](WacomData.md) | 静态数据和资产契约，包括卡牌、敌人、角色、商店、探索事件、GameplayTag 和效果字段。 |
| [`WacomApp.md`](WacomApp.md) | 游戏主模块和 App 编排约定，包括 GameMode、PlayerController、输入、世界交互和战斗进出流程。 |
| [`WacomUI.md`](WacomUI.md) | UI 表现层当前事实，包括 CommonUI 层级、Toast、Screen、HUD、ViewData Builder 和刷新模型。 |
| [`UI_Backpack_WBP_Binding.md`](UI_Backpack_WBP_Binding.md) | 背包、卡牌详情、卡牌显示等 WBP 的绑定槽位和制作约束。 |
| [`UI_Battle_WBP_Binding.md`](UI_Battle_WBP_Binding.md) | 战斗手牌、BattleHUD、事件日志和敌方部位 fallback UI 的 WBP 绑定协议。 |
| [`TODO.md`](TODO.md) | 短期任务索引，只保留优先级、归属和跳转，不承载长说明。 |
| [`Roadmap.md`](Roadmap.md) | 未实现功能、阶段方向和内容扩展计划。 |
| [`TechDebt.md`](TechDebt.md) | 已存在的临时写法、兼容路径、临时决定和正式替代方案。 |
| [`Questions.md`](Questions.md) | 会影响规则、策划口径或长期架构的待确认问题。 |
| [`DevLog/`](DevLog/) | 历史记录和阶段复盘，用来理解演进过程；不作为最新规则真相。 |

## 判断规则真相

- 规则、数据字段、模块边界发生冲突时，优先以对应领域文档为准。
- `DevLog/` 只解释历史过程，不覆盖当前领域文档。
- `TODO.md`、`Roadmap.md`、`TechDebt.md`、`Questions.md` 记录下一步和风险，不应替代正式规则。
- 改代码导致规则、资产契约或 UI 绑定变化时，同步更新对应领域文档；临时方案同步写入计划/技术债文档。
