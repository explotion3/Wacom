---
type: docs-index
scope: wacom-docs
status: active
updated: 2026-06-05
tags:
  - wacom/docs
  - wacom/navigation
---

# Wacom Docs 入口

> [!info] 本文职责
> 本文是项目文档导航。规则真相优先看领域文档，历史过程看 `DevLog/`，短期任务看 [`TODO.md`](TODO.md)，长期方向、技术债和未决问题分别看 [`Roadmap.md`](Roadmap.md)、[`TechDebt.md`](TechDebt.md)、[`Questions.md`](Questions.md)。

> [!warning] 规则真相优先级
> `Game_Design.md` 提供全局 GDD 和设计语境，不保存当前实现规格；若它与领域文档冲突，以 `WacomBattle.md`、`WacomRun.md`、`WacomData.md`、数据专题文档、`WacomApp.md`、`WacomWorldInteraction.md`、`WacomUI.md` 及 UI 专题文档等当前事实文档为准。

## 建议阅读顺序

1. 先读 [`../AGENTS.md`](../AGENTS.md)：确认协作规则、模块边界、允许修改范围和验证要求。
2. 再读 [`Architecture.md`](Architecture.md)：理解模块依赖方向、Public/Private 边界、Battle/Run/App 的长期分工。
3. 需要了解整体玩法背景时读 [`Game_Design.md`](Game_Design.md)：它是全局 GDD 和设计语境，不作为实现规格；若和领域文档冲突，以领域文档为准。
4. 需要了解角色早期方向时读 [`Characters/BugGirl.md`](Characters/BugGirl.md)：它是角色设计语境，不作为当前卡牌或规则实现规格。
5. 按任务领域读对应规则文档：
   - 战斗规则：[`WacomBattle.md`](WacomBattle.md)
   - Run / 探索 / 存档 / 背包：[`WacomRun.md`](WacomRun.md)
   - 静态数据字段：[`WacomData.md`](WacomData.md)
   - 内容生成、资产校验、Battle 制作矩阵：[`WacomDataAuthoring.md`](WacomDataAuthoring.md)
   - GameplayTag 字典：[`WacomGameplayTags.md`](WacomGameplayTags.md)
   - App 编排、输入、GameMode、战斗进出流程：[`WacomApp.md`](WacomApp.md)
   - 世界交互、target handle、Actor authoring / debug / validation：[`WacomWorldInteraction.md`](WacomWorldInteraction.md)
   - UI 总入口：[`WacomUI.md`](WacomUI.md)
   - UI Foundation / Battle UI / 第一人称卡牌层：[`WacomUIFoundation.md`](WacomUIFoundation.md)、[`WacomBattleUI.md`](WacomBattleUI.md)、[`First_Person_Card_Layer_Design.md`](First_Person_Card_Layer_Design.md)
6. 做 WBP 或 UI 资产承接时，再读绑定清单：
   - [`UI_Backpack_WBP_Binding.md`](UI_Backpack_WBP_Binding.md)
   - [`UI_Battle_WBP_Binding.md`](UI_Battle_WBP_Binding.md)
   - [`UI_RunEvent_WBP_Binding.md`](UI_RunEvent_WBP_Binding.md)
7. 开始实现前扫计划和风险：
   - 短期任务索引：[`TODO.md`](TODO.md)
   - 长期方向、技术债、待确认问题：[`Roadmap.md`](Roadmap.md)、[`TechDebt.md`](TechDebt.md)、[`Questions.md`](Questions.md)

## 常用专题入口

| 主题      | 当前事实入口                                                                                                 | 相关入口                                                                                                                                                                             |
| ------- | ------------------------------------------------------------------------------------------------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 战斗规则    | [`WacomBattle.md`](WacomBattle.md)                                                                     | 战后结算 [`WacomRun §10`](WacomRun.md#wacomrun-battle-settlement)，战斗 UI [`WacomBattleUI.md`](WacomBattleUI.md)，WBP 合约 [`UI_Battle_WBP_Binding.md`](UI_Battle_WBP_Binding.md)    |
| 击倒与奖励卡  | [`WacomBattle §12`](WacomBattle.md#wacombattle-result)                                                 | 部位奖励字段 [`WacomData §4`](WacomData.md#wacomdata-enemy-part)，Run 入包 [`WacomRun §10`](WacomRun.md#wacomrun-battle-settlement)，后续方向 [`Roadmap: 击倒事件扩展`](Roadmap.md#roadmap-knockdown) |
| 战斗表现    | [`WacomBattleUI.md`](WacomBattleUI.md)                                                                 | 第一人称卡牌层 [`First_Person_Card_Layer_Design.md`](First_Person_Card_Layer_Design.md)，UI 技术债 [`TechDebt: UI 层技术债`](TechDebt.md#techdebt-ui)，后续表现 [`Roadmap: 战斗 UI`](Roadmap.md#roadmap-battle-ui) |
| 背包构筑    | [`WacomRun.md`](WacomRun.md)                                                                           | 背包 UI [`WacomUI.md`](WacomUI.md)，WBP 合约 [`UI_Backpack_WBP_Binding.md`](UI_Backpack_WBP_Binding.md)，后续表现 [`Roadmap: 背包 UI`](Roadmap.md#roadmap-backpack-ui)                       |
| 商店与探索事件 | [`WacomRun.md`](WacomRun.md)                                                                           | 静态定义 [`WacomData.md`](WacomData.md)，内容制作 [`WacomDataAuthoring.md`](WacomDataAuthoring.md)，世界交互 [`WacomWorldInteraction.md`](WacomWorldInteraction.md)，后续方向 [`Roadmap: 商店`](Roadmap.md#roadmap-shop) / [`Roadmap: 探索事件`](Roadmap.md#roadmap-runevent)             |
| 静态数据制作 | [`WacomData.md`](WacomData.md) | 内容生成 / 校验 [`WacomDataAuthoring.md`](WacomDataAuthoring.md)，GameplayTag [`WacomGameplayTags.md`](WacomGameplayTags.md)，内容目录 [`Content_Organization.md`](Content_Organization.md) |
| 世界交互与目标系统 | [`WacomWorldInteraction.md`](WacomWorldInteraction.md)                                             | App 编排 [`WacomApp.md`](WacomApp.md)，Run 事务 [`WacomRun.md`](WacomRun.md)，战斗目标规则 [`WacomBattle.md`](WacomBattle.md)                                              |
| 地图与节点   | 当前节点消耗规则见 [`WacomRun.md`](WacomRun.md)，地图设计语境见 [`Game_Design §10`](Game_Design.md#game-design-run-map) | 待确认问题 [`Questions: Run、探索与地图`](Questions.md#questions-run-map)，后续方向 [`Roadmap: 地图与探索`](Roadmap.md#roadmap-map)                                                                   |
| 存档恢复    | [`WacomRun.md`](WacomRun.md)                                                                           | 技术债 [`TechDebt: 数据与存档债`](TechDebt.md#techdebt-data-save)，后续方向 [`Roadmap: 存档恢复`](Roadmap.md#roadmap-save)                                                                         |

## 文档职责

| 文档 | 职责 |
|---|---|
| [`../AGENTS.md`](../AGENTS.md) | 项目协作规则、模块职责、代码边界、测试命令和文件安全要求。 |
| [`Architecture.md`](Architecture.md) | 全局架构、模块依赖方向、目录结构、Command / Snapshot / Event 等跨领域约定。 |
| [`Game_Design.md`](Game_Design.md) | 全局 GDD、设计背景和玩法语境；不承载当前实现规格，与领域文档冲突时以领域文档为准。 |
| [`Characters/BugGirl.md`](Characters/BugGirl.md) | 虫妹角色幻想、机制方向和早期卡牌意图；不承载当前 DataAsset、Battle、Run 或 UI 实现规格。 |
| [`WacomBattle.md`](WacomBattle.md) | 单场战斗规则真相，包括命令合同、PlayCard 流程、手牌区域、效果执行、敌方行动、BattleEvent 和战斗结果包。 |
| [`WacomRun.md`](WacomRun.md) | 战斗外 Run 规则真相，包括生命周期、时间 / 节点、压力、卡牌持有区、商店、RunEvent、Pickup、世界卡牌交互、存档和战斗回传。 |
| [`WacomData.md`](WacomData.md) | 静态 DataAsset 契约，包括卡牌、敌人、角色、商店、Pickup、Run world card interaction、RunEvent 和效果字段摘要。 |
| [`WacomDataAuthoring.md`](WacomDataAuthoring.md) | 内容生成、生成资产清单、Data Validation、自动化验证和 Battle rule content authoring matrix。 |
| [`WacomGameplayTags.md`](WacomGameplayTags.md) | GameplayTag 命名空间和 tag catalog；说明 tag 声明与正式可制作范围的区别。 |
| [`WacomApp.md`](WacomApp.md) | 游戏主模块和 App 编排约定，包括 GameMode、PlayerController、输入协调和战斗进出流程。 |
| [`WacomWorldInteraction.md`](WacomWorldInteraction.md) | 世界交互和目标系统，包括 Run world interactable Actor、Battle scene target、Run menu zone target、debug / authoring / validation 约定。 |
| [`WacomUI.md`](WacomUI.md) | UI 表现层总入口，包括 UI 原则、ownership、测试访问原则、Run UI 摘要、卡牌展示和 WBP 文档分工。 |
| [`WacomUIFoundation.md`](WacomUIFoundation.md) | CommonUI shell、PrimaryLayout、UI Settings、Widget registry、Modal / MainMenu、Run MVVM 和 AppToast 例外路径。 |
| [`WacomBattleUI.md`](WacomBattleUI.md) | BattleHUD、命令出口、targeting、presentation flow、Combat Log、Presentation Stack、legacy / fallback Battle UI。 |
| [`First_Person_Card_Layer_Design.md`](First_Person_Card_Layer_Design.md) | 第一人称卡牌层 authoring / runtime contract，包括 Battle / Run source、hover / drag / drop 和 WBP_FirstPersonCardView。 |
| [`Run_Tunnel_Exploration_Spike_V0.md`](Run_Tunnel_Exploration_Spike_V0.md) / [`Run_Tunnel_Presentation_Discussion.md`](Run_Tunnel_Presentation_Discussion.md) | Run Tunnel archived historical background；只用于理解历史 Spike / 表现讨论，当前 movement / input / Run 规则 / world interaction / first-person hand 事实分别看 `WacomApp.md`、`WacomRun.md`、`WacomWorldInteraction.md` 和 `First_Person_Card_Layer_Design.md`。 |
| [`UI_Backpack_WBP_Binding.md`](UI_Backpack_WBP_Binding.md) | 背包、卡牌详情、卡牌显示等 WBP 的绑定槽位和制作约束。 |
| [`UI_Battle_WBP_Binding.md`](UI_Battle_WBP_Binding.md) | BattleHUD、first-person card view、Combat Log、Presentation Stack、shared widgets、legacy / fallback Battle UI 的 WBP 绑定协议。 |
| [`UI_RunEvent_WBP_Binding.md`](UI_RunEvent_WBP_Binding.md) | RunEventScreen、ChoiceButton、PaymentDropTarget 的 WBP 绑定槽位、制作边界和 PIE smoke 检查。 |
| [`TODO.md`](TODO.md) | 短期任务索引，只保留优先级、归属和跳转，不承载长说明。 |
| [`Roadmap.md`](Roadmap.md) | 未实现功能、后续方向和前置依赖索引；不承载当前实现事实。 |
| [`TechDebt.md`](TechDebt.md) | 已存在的临时写法、兼容路径、临时决定和正式替代方案。 |
| [`Questions.md`](Questions.md) | 会影响规则、策划口径或长期架构的开放决策索引；问题收口后同步回领域文档。 |
| [`DevLog/`](DevLog/) | 历史记录和阶段复盘，用来理解演进过程；不作为最新规则真相。公开面和文档重构历史见 [`Wacom_Public_Surface_And_Docs_History.md`](DevLog/Wacom_Public_Surface_And_Docs_History.md)。 |

## 判断规则真相

- 规则、数据字段、模块边界发生冲突时，优先以对应领域文档为准。
- `DevLog/` 只解释历史过程，不覆盖当前领域文档。
- `TODO.md`、`Roadmap.md`、`TechDebt.md`、`Questions.md` 记录下一步和风险，不应替代正式规则。
- 改代码导致规则、资产契约或 UI 绑定变化时，同步更新对应领域文档；临时方案同步写入计划/技术债文档。
