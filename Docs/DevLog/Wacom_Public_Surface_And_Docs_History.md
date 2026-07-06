---
type: devlog
scope: wacom-public-surface-docs-history
status: archived
updated: 2026-06-05
tags:
  - wacom/devlog
  - wacom/public-surface
  - wacom/docs
  - archive
---

# Wacom Public Surface And Docs History

> [!warning] Archived
> 本文只记录公开面清理和文档重构的历史演进，不是当前规则真相、技术债登记表或待办入口。当前债务看 [TechDebt.md](../TechDebt.md)，短期任务看 [TODO.md](../TODO.md)，领域事实看对应领域文档。

## WacomApp Public UI API surface

这组记录来自 `TechDebt.md` 的历史长段，归档后便于检索 `WacomApp Public UI API surface` 的演进来源。

| 切片 | 历史记录 |
|---|---|
| V0-DY | 只读审计 `Source/WacomApp/Public/UI` 与 `Source/WacomApp/Public/Components` 的反射和 Blueprint 公开面；不删除、不重命名、不移动任何 public / Blueprint API。 |
| V0-DZ | UI / Components 的 `DebugView`、`DebugSummary`、日志开关和 `CallInEditor` 调试入口归到 `Wacom|...|Debug` 并补中文 ToolTip。 |
| V0-EA | `UWacomRunMenuCardLeaseTestMenu` 标成 development/prototype-only 验证入口，保留 public 路径、控制台命令和测试 probe 继承关系。 |
| V0-EB | First-person layer / slot / anchor / Run source 的自动化测试访问收口到少数 test view 和 `WacomTests/Private` access wrapper。 |
| V0-EX | Run card drop router 重构后，`UWacomRunMenuCardLeaseTestMenu` runtime 原型菜单删除，PlayerController / console runtime 入口和测试 probe 继承关系移除；Run card drop coordinator 通过显式 context contract 消费 Controller 能力，不再 friend 读取 Controller 私有状态；first-person layer / slot / anchor / Run source 的 `ForTest` 方法改为 private friend，由 `WacomTests/Private` access wrapper 调用。 |
| V0-EC | Backpack / BattleHUD 的非 Blueprint 测试 / 诊断 helper 收口到 test view + `WacomTests/Private` access / receiver。 |
| V0-ED | `Source/WacomApp/Public/Actors` 中 `ConfigureDebug...Sample` 统一归到 `Wacom|...|Prototype`，并保留 Authoring / Debug 的正式分区。 |
| V0-EE | `Docs/WacomApp.md` 和自动化测试 display name 同步为 Prototype 口径，避免 Prototype 样例入口继续被旧 Authoring 命名误导。 |
| V0-EF | 正式 first-person battle hand interaction 的编辑器分类和新 C++ 调用口径从 Prototype 收口到 Interaction，同时保留旧成员兼容。 |
| V0-EG | 旧 Battle 世界空间手牌 public surface 曾隔离到 prototype 分类；后续已从 runtime / tests 移除，正式主线回到 first-person card layer。 |
| V0-EH | First-person anchor 的 legacy projection / layout、LookInfluence 和静态预览层隔离成 legacy comparison / prototype preview 口径。 |
| V0-EI | 旧 Battle event log drawer / entry / EventToast 公开面归到 legacy compatibility，并标清 DebugBattleHUD 的 debug-only 身份。 |
| V0-EJ | 单条 Battle event presentation builder 标成 compatibility，正式 Combat Log builder 口径补清楚。 |
| V0-EK | 正式 CombatLogFeed / CombatLogBlock / PresentationStack / StackEntry 和 BattleHUD 反馈配置归到正式 authoring 分类。 |
| V0-EL | 旧 2D hand 与旧 2D enemy fallback 公开面归到 fallback / compatibility 分类。 |
| V0-EM | `UBattleHUD` 自身的命令入口、状态查询、表现流、详情、手牌呈现和 C++ fallback layout 分类从泛 UI 口径收口。 |
| V0-EN | `UWacomBattleWidgetBase` 的 Session 注入和 Snapshot WBP 刷新钩子分类收口。 |
| V0-EO | BattleHUD 直接依赖的玩家状态条、牌堆计数和通用进度条分类从根 `Wacom|UI` 收口。 |
| V0-EP | Foundation widget lifecycle / button contract 分类收口到 `Wacom|UI Foundation|...`。 |
| V0-EQ | UI Foundation shell / settings / MVVM 分类收口。 |
| V0-ER | Common Modal / MainMenu category cleanup，Modal Dialog 与 MainMenu authoring 口径收口。 |
| V0-ES | CardView / EffectBadge test hook locality cleanup，自动化计数 getter 收口到 test view + access wrapper。 |
| V0-ET | Shop / RunEvent screen test hook locality cleanup，正式 WBP authoring surface、购买 / 选择 / drop submit 行为和 Blueprint API 不变。 |
| V0-EU | PlayerController Run interaction test access locality cleanup；`AWacomPlayerControllerProbe` 仍作为测试 bridge 保留。 |
| V0-EV | Battle scene target click router test access locality cleanup；生产 PlayerController 输入流和 BattleHUD target selection 不变。 |
| V0-EW | Run world interaction actor test access locality cleanup；生产 Run world Actor、PlayerController、RunSession、drop intent、Toast 和 Blueprint API 不变。 |
| V0-EX | Run menu drop target widget probe test access locality cleanup；生产 Run menu drop target、menu lease、RunEvent 支付和 Blueprint API 不变。 |
| V0-EY | RunEvent choice button class probe test access locality cleanup；生产 RunEvent choice button、payment drop target、menu lease、WBP authoring surface 和 Blueprint API 不变。 |

## Docs Refactor History

| 切片 | 历史记录 |
|---|---|
| V0-EZ | `WacomApp.md` 重构为 App orchestration 当前事实入口；世界交互长内容移入 `WacomWorldInteraction.md`。 |
| V0-FA | `WacomUI.md` 重构为 UI 总入口；CommonUI / Battle UI / first-person card layer 拆入专题文档。 |
| V0-FB | `WacomData.md` 重构为静态 DataAsset 契约入口；内容生成与 GameplayTag 拆入 `WacomDataAuthoring.md` / `WacomGameplayTags.md`。 |
| V0-FC | `WacomRun.md` 重构为 Run 规则当前事实入口；Run Tunnel / input / first-person hand / menu drop / Actor validation 只保留摘要和跳转。 |
| V0-FD | `WacomBattle.md` 重构为单场战斗规则当前事实入口；UI、Data authoring 和长期方向交回专题文档。 |
| V0-FE | `UI_Battle_WBP_Binding.md` 重构为 Battle WBP 绑定清单，不再承载行为时间线。 |
| V0-FF | `Run_Tunnel_Exploration_Spike_V0.md` 和 `Run_Tunnel_Presentation_Discussion.md` 就地归档；`Run_Tunnel` 当前事实继续由 App / Run / WorldInteraction / first-person 文档承接。 |
| V0-FG | `TODO.md 已瘦身` 为短期任务索引，只保留优先级、状态、归属和跳转；大段完成史、规则事实和实现细节不再回填 TODO。 |
| V0-FH | `TechDebt.md` 瘦身为当前债务登记表；公开面与文档重构历史移入本文。 |
| V0-FI | `Game_Design.md` 收口为全局 GDD / 设计语境入口；实现规格、字段级真相和开放问题清单交回领域文档与 `Questions.md`。 |
| V0-FJ | `Questions.md` 收口为开放决策索引；补齐稳定锚点并清理旧时间线口径，跨文档引用改用显式 anchor。 |
| V0-FK | `Roadmap.md` 收口为未来方向和前置依赖索引；当前事实、开放决策、短期任务和技术债交回对应文档。 |
| V0-FL | `Characters/BugGirl.md` 收口为角色设计语境；卡牌字段、生成内容、Battle / Run / UI 当前事实交回领域文档。 |
| V0-FM | `UI_RunEvent_WBP_Binding.md` 收口为 RunEvent WBP 绑定合同；规则、支付事务、menu drop 和 UI 数据流交回领域文档。 |
| V0-FN | `UI_Backpack_WBP_Binding.md` 收口为 Backpack / CardView WBP 绑定合同；背包规则、卡牌字段和 UI 数据流交回领域文档。 |
| V0-FO | `TechDebt.md` 清理剩余旧阶段措辞，保持当前债务登记表职责，不回填实现流水账。 |
| V0-FP | 清理剩余文档漂移：Battle WBP 装备占位、Data Durability、GDD 自引用和索引文档 wikilink。 |

## Reading Notes

- 本文保留历史标签用于 grep 和追溯，不代表这些切片仍是待办。
- 当前剩余债务见 [TechDebt.md](../TechDebt.md)，尤其是 `WacomApp Public UI API surface 当前剩余债务`。
- `DevLog/` 只解释演进过程，不覆盖当前领域文档。
