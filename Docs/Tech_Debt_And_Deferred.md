# 技术债务与延后项

本文集中记录 Phase 1 + Phase 2 中**未实现的功能**、**临时写法**、**已知需要优化的点**。
后续开发时按优先级逐条消化。每条标注来源切片和后续方向。

---

## 一、未实现的功能（跳过或占位）

### 规则层

| 项                                | 现状                                         | 后续方向                                                       |
| -------------------------------- | ------------------------------------------ | ---------------------------------------------------------- |
| `Status.Slow` 减速数值效果             | 只记录层数，不影响先机或 Cost                          | 等 `Battle_Rules §16` 正式定义减速公式后实现                           |
| `Status.Twilight` 暮气数值效果         | 只记录层数，不触发任何效果                              | 等"暮气生效触发点"规则确认（回合开始？部位行动前？）                                |
| 暮蛉 `OnTwilightTriggered` 真正改中毒层数 | P3.5 只发 `PassiveTriggered` 事件，不改 Magnitude | 需引入 `FRuntimeCardInstance::EffectMagnitudeModifiers` 或等价机制 |
| 治疗移除 10% 中毒层数                    | 治疗效果 `Effect.Heal` 未实现                     | 实现 Heal 时一并加中毒层数衰减                                         |
| 卡牌耐久 `Durability` 消耗             | `FCardPhysique::Durability` 字段存在但不读取       | 等背包系统引入后消耗耐久                                               |
| 左手主动效果 / 完美释放效果                  | 左手 `Effects` / `PerfectReleaseEffects` 留空  | 等背包系统接入后配置                                                 |
| 右手"相邻右方伙伴代打"                     | 未实现                                        | 等 `Target.Adjacent.Right` 的 Executor 分支                    |
| 击倒事件奖励                           | `EnemyKnockdown` 事件只发不处理                   | Run 外层实现奖励分发                                               |
| 蛇部位间联动                           | 无（头被破坏时身体不强化）                              | 等更多敌人设计后按需加                                                |
| 手牌满时 OnCompanionCount 处理         | 强行加入，下回合 EnforceLimit 处理                   | 若规则变更为"满时不触发"，改 `RunOnCompanionCountPassives`              |

### UI / 表现层

| 项 | 现状 | 后续方向 |
|---|---|---|
| UI 动画（P5 整体） | 全部跳过，HP/卡牌/伤害数字无过渡 | 美术资源到位后做事件队列化 + 具体动画 |
| 主题与样式（P6 整体） | Widget Blueprint 纯色块 + 文字 | 美术阶段只改 WBP，C++ 不动 |
| 手牌扇形布局 | HorizontalBox + ScrollBox 线性排列 | 美术阶段替换为自定义 `UHandLayoutPanel` |
| 卡牌拖拽 | 不支持 | 后续交互升级时加 |
| 目标选择 3D 射线 | 点击 EnemyPartWidget 2D 按钮 | HD-2D 表现时改为 3D 部位高亮 + 点击 |
| EventToast 图标/动画 | 纯文字 | 升级为"事件表现调度器" + Niagara + 音效 |
| 锚点左右归属 | 遍历顺序启发式（第一个锚点进 LeftSlot） | 给 `FHandCardSnapshot` 加 `EHandAnchorRole` 字段 |

### 架构层

| 项 | 现状 | 后续方向 |
|---|---|---|
| Run 外层（背包、关卡路径、掉落） | `WacomRun` 模块为空 | 第三阶段核心工作 |
| SaveGame / 存档 | 未实现 | 第三阶段引入 |
| 网络复制 | 未实现 | 远期，单人游戏暂不需要 |
| GameMode / HUD 正式流程 | BattleTestActor 手动创建 UI | 正式流程走 `UGameUIManagerSubsystem` 或 GameMode |
| Enhanced Input 多 Context | 只有 `IMC_Battle` | Run 外层时加 `IMC_Exploration` / `IMC_Menu`，Push/Pop 切换 |
| ViewModel 层 | Widget 直接持有 Session | UI 复杂度上升时抽 `UBattleViewModel` |
| GAS（GameplayAbilitySystem） | 不使用 | 保持不引入，战斗用自研 Resolver/Executor |

---

## 二、临时写法（能用但不够正式）

### 规则层

| 项 | 临时做法 | 正式方案 |
|---|---|---|
| 双手区保留 | 硬编码"左右手都在时双手区普通卡保留"，不区分角色 | 由 `CharacterDefinition` 或 `CardDefinition` 字段驱动 |
| ZoneHook Trigger 种类 | 只支持 `OnPlay` / `OnPerfectReleaseHit` | 扩展 `OnTurnStart` / `OnDiscard` / `OnEnterZone` 等 |
| 费用转移 | 只支持 `ReduceCost(LastShuffled) + AddCost(Self)` 组合 | 更复杂的多点/条件费用转移用 `CostLedger` |
| CompanionPlayedCount | 全局计数，不区分哪张伙伴 | 当前口径正确（BugGirl.md §5），多角色时再评估 |
| 中毒结算穿透护盾 | `PoisonResolver` 直接扣 HP | 已正式化（Battle_Rules §15），不需要改 |
| 中毒触发时机 | 打牌后 + 部位行动后 | 已正式化（Battle_Rules §15），不需要改 |
| 回合结束时保留/弃牌的时序 | 在 `EndTurnResolver` 里放在敌方行动之前（TurnEnded 事件之后、敌方行动之前） | `Battle_Rules §12` 和 `Hand_Zone_Rules §7` 都只说"回合结束时"进弃牌但未明确时序。当前时序和"回合结束时类效果"步骤对齐。若规则后续明确放在敌方行动之后（比如"敌方可对手牌最后一张攻击"），需要调整 |

### UI 层

| 项 | 临时做法 | 正式方案 |
|---|---|---|
| 全量刷新 | 每次命令后 Snapshot 全量重建 Widget 数据 | 加动画时动画系统自己做 diff |
| BattleTestActor 创建 UI | BeginPlay 手动 CreateWidget + AddToViewport | 走 Subsystem 或 GameMode 管理 |
| C++ 硬编码默认布局 | Widget 类 `Blueprintable` 非 Abstract，带 C++ 默认布局 | 后续美术只改 WBP |
| HP 条瞬间跳变 | `SetPercent` 直接设值 | 加 `SetTargetPercent` + Tick Lerp 插值 |

### 架构层

| 项 | 临时做法 | 正式方案 |
|---|---|---|
| BattleState 非反射 | 裸 struct + pImpl，GC 引用靠 Session 的 `ReferencedAssets` | 若需存档/网络，升级为 USTRUCT 或 UObject |
| MSVC 工具链 14.38 | UE 5.7 警告 "not preferred"，不影响功能 | 升级到 14.44+ |

---

## 三、已正式化的临时决定（不需要再改）

以下条目曾是临时决定，现已写入正式规则文档，代码实现与文档一致：

- 中毒穿透护盾 → `Battle_Rules.md §15`
- 中毒触发时机（打牌后 + 行动后） → `Battle_Rules.md §15`
- 晕厥层数模型（每次行动消耗 1 层） → `Battle_Rules.md §10`

---

## 四、使用方式

1. 开新功能前先扫一遍本文，看有没有相关的债务可以顺手消化。
2. 消化后把对应行删掉或标记"已解决 @ 切片号"。
3. 新增临时写法时同步加到本文。
4. 每个大阶段结束时 review 一次，清理已解决项。
