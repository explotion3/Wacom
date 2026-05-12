# TODO（待完善内容、临时决定和技术债）

> 本文集中记录待完善内容、临时决定和技术债。开新功能前先扫一遍。消化后标记已解决。

---

## §1 未实现的功能

### 规则层

| 项 | 现状 | 后续方向 |
|---|---|---|
| `Status.Slow` 减速数值效果 | 只记录层数，不影响先机或 Cost | 等 `Battle_Rules §16` 正式定义减速公式后实现 |
| `Status.Twilight` 暮气数值效果 | 只记录层数，不触发任何效果 | 等"暮气生效触发点"规则确认（回合开始？部位行动前？）|
| 暮蛉 `OnTwilightTriggered` 真正改中毒层数 | P3.5 只发 `PassiveTriggered` 事件，不改 Magnitude | 需引入 `FRuntimeCardInstance::EffectMagnitudeModifiers` 或等价机制 |
| 治疗移除 10% 中毒层数 | 治疗效果 `Effect.Heal` 未实现 | 实现 Heal 时一并加中毒层数衰减 |
| 卡牌耐久 `Durability` 消耗 | `FCardPhysique::Durability` 字段存在但不读取 | 等背包系统引入后消耗耐久 |
| 左手主动效果 / 完美释放效果 | 左手 `Effects` / `PerfectReleaseEffects` 留空 | 等背包系统接入后配置 |
| 右手"相邻右方伙伴代打" | 未实现 | 等 `Target.Adjacent.Right` 的 Executor 分支 |
| 击倒事件奖励 | `EnemyKnockdown` 事件只发不处理 | Run 外层实现奖励分发 |
| 蛇部位间联动 | 无（头被破坏时身体不强化）| 等更多敌人设计后按需加 |
| 手牌满时 OnCompanionCount 处理 | 强行加入，下回合 EnforceLimit 处理 | 若规则变更为"满时不触发"，改 `RunOnCompanionCountPassives` |

### UI / 表现层

| 项 | 现状 | 后续方向 |
|---|---|---|
| UI 动画（P5 整体）| 全部跳过，HP/卡牌/伤害数字无过渡 | 美术资源到位后做事件队列化 + 具体动画 |
| 主题与样式（P6 整体）| Widget Blueprint 纯色块 + 文字 | 美术阶段只改 WBP，C++ 不动 |
| 手牌扇形布局 | HorizontalBox + ScrollBox 线性排列 | 美术阶段替换为自定义 `UHandLayoutPanel` |
| 卡牌拖拽 | 不支持 | 后续交互升级时加 |
| 目标选择 3D 射线 | 点击 EnemyPartWidget 2D 按钮 | HD-2D 表现时改为 3D 部位高亮 + 点击 |
| EventToast 图标/动画 | 纯文字 | 升级为"事件表现调度器" + Niagara + 音效 |
| 锚点左右归属 | 遍历顺序启发式（第一个锚点进 LeftSlot）| 给 `FHandCardSnapshot` 加 `EHandAnchorRole` 字段 |

### 架构层

| 项 | 现状 | 后续方向 |
|---|---|---|
| 网络复制 | 未实现 | 远期，单人游戏暂不需要 |
| ViewModel 层 | Widget 直接持有 Session | UI 复杂度上升时抽 `UBattleViewModel` |
| GAS（GameplayAbilitySystem）| 不使用 | 保持不引入，战斗用自研 Resolver/Executor |

---

## §2 临时写法

### 规则层

| 项 | 临时做法 | 正式方案 |
|---|---|---|
| 双手区保留 | 硬编码"左右手都在时双手区普通卡保留"，不区分角色 | 由 `CharacterDefinition` 或 `CardDefinition` 字段驱动 |
| ZoneHook Trigger 种类 | 只支持 `OnPlay` / `OnPerfectReleaseHit` | 扩展 `OnTurnStart` / `OnDiscard` / `OnEnterZone` 等 |
| 费用转移 | 只支持 `ReduceCost(LastShuffled) + AddCost(Self)` 组合 | 更复杂的多点/条件费用转移用 `CostLedger` |
| CompanionPlayedCount | 全局计数，不区分哪张伙伴 | 当前口径正确（BugGirl.md §5），多角色时再评估 |
| 回合结束时保留/弃牌的时序 | 在 `EndTurnResolver` 里放在敌方行动之前 | 若规则后续明确放在敌方行动之后，需要调整 |

### UI 层

| 项 | 临时做法 | 正式方案 |
|---|---|---|
| 全量刷新 | 每次命令后 Snapshot 全量重建 Widget 数据 | 加动画时动画系统自己做 diff |
| C++ 硬编码默认布局 | Widget 类 `Blueprintable` 非 Abstract，带 C++ 默认布局 | 后续美术只改 WBP |
| HP 条瞬间跳变 | `SetPercent` 直接设值 | 加 `SetTargetPercent` + Tick Lerp 插值 |

### 架构层

| 项 | 临时做法 | 正式方案 |
|---|---|---|
| BattleState 非反射 | 裸 struct + pImpl，GC 引用靠 Session 的 `ReferencedAssets` | 若需存档/网络，升级为 USTRUCT 或 UObject |
| MSVC 工具链 14.38 | UE 5.7 警告 "not preferred"，不影响功能 | 升级到 14.44+ |

---

## §3 临时决定（未正式化的）

### UI 相关

- **[P1] 全量刷新策略**：每次命令后从 Snapshot 重建所有 Widget 数据，不做增量 diff。
  → 后续加动画时，动画系统自己做 diff，数据刷新仍然全量。

- **[P1] 不做 ViewModel 层**：Widget 直接持有 `UBattleSession*`，调用 `BuildSnapshot` 刷新。
  → 后续 UI 复杂度上升时抽 `UBattleViewModel`。

- **[P1] 手牌用 HorizontalBox 线性排列**：不做扇形、不做拖拽。
  → 后续美术阶段替换为自定义 `UHandLayoutPanel`。

- **[P1] 目标选择用"点击 EnemyPartWidget"实现**：不做拖拽到目标、不做射线检测。
  → 后续第一人称 HD-2D 表现时，目标选择可能改为"鼠标悬停 3D 部位 → 高亮 → 点击"。

- **[P1] EventToast 只显示文字**：不做图标、不做动画。
  → 后续加 Niagara 特效和音效时，EventToast 升级为"事件表现调度器"。

- **[P1] Widget Blueprint 纯色块 + 文字**：不做美术。
  → 后续美术阶段只改 WBP，C++ 不动。

### 规则相关

- **[P2] 双手区保留是虫妹专属规则**：当前硬编码"左右手都在时双手区普通卡保留"。
  → 后续多角色时，保留规则应由 `CharacterDefinition` 或 `CardDefinition` 的字段驱动。

- **[P4] 费用转移只支持"被腾挪卡 -1，本卡 +1"**：朝光暮蝶右手区效果。
  → 后续可能有更复杂的费用转移。到时候需要 `CostLedger` 或 `CostTransferEvent`。

- **[P4] ZoneHook 只支持两种 Trigger**：`OnPlay` 和 `OnPerfectReleaseHit`。
  → 后续可能有 `OnTurnStart`、`OnDiscard`、`OnEnterZone` 等。

- **[P5] CompanionPlayedCount 是全局计数**：不区分"哪张伙伴"。
  → 对齐 BugGirl.md §5 拂晓飞蛾的"三张伙伴"是战斗内全局计数。触发后清零。

- **[P3.4] OnCompanionCount 触发时超手牌上限强行加入**：触发时不检查普通卡上限 10，直接加到 Hand 末尾。
  → 后续若有"手牌满时不触发"的规则变更，改 `RunOnCompanionCountPassives` 里加上限检查。

- **[P5] 暮蛉 OnTwilightTriggered 需要暮气"生效"**：第一阶段暮气只记录层数不生效。
  → 需要先定义"暮气生效"的触发点，然后才能触发暮蛉被动。规则未决项。

### 架构相关

- **[P2.3] 锚点左右归属用"遍历顺序"启发式**：`FHandCardSnapshot` 没告诉 UI 某张锚点卡是左手还是右手。
  → 正式方案：给 `FHandCardSnapshot` 加 `EHandAnchorRole AnchorRole`（None/Left/Right）。

---

## §4 已正式化（参考用）

以下条目曾是临时决定，现已写入正式规则文档，代码实现与文档一致：

- 中毒穿透护盾 → `Battle_Rules.md §15`
- 中毒触发时机（打牌后 + 行动后）→ `Battle_Rules.md §15`
- 晕厥层数模型（每次行动消耗 1 层）→ `Battle_Rules.md §10`
- [P3] 中毒穿透护盾 → 已正式化
- [P3] 中毒触发时机 → 已正式化
- [P1] BattleHUD 由 BattleTestActor 创建 → 已迁移到 `UWacomGameUIManagerSubsystem` 管理
- [P6] Enhanced Input 只做战斗快捷键 → 已扩展为 `IMC_Exploration` / `IMC_Battle`，Push/Pop 切换

---

## §5 待确认的规则问题

1. 中毒等状态的触发单位是"每张牌/每部位行动"还是"每次行动批次"？
2. 背包容量不足时，战斗结束获得的掉落卡如何处理？
3. 自由探索 Run 是否继续复用 `RunSession`，还是新建区域探索 session？
4. 突袭的正式规则是什么？
5. 手牌已满时，拂晓飞蛾从非手牌区域回到手牌的效果如何处理？
6. 右手牌被永久删除后是否完全对称处理？
7. 左右手都被永久删除时，是否只剩普通手牌区？
8. 冻结与迅捷、Cost、完美释放的关系。
9. 暮气归属（玩家/卡牌/敌人意图/多处）。
10. 减速/暮气的数值公式。
11. 击倒事件的正式触发条件。
12. `Effect.Shuffle.ToRandomZone` 在手牌锚点缺失时的回退规则。
