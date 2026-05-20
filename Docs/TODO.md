# TODO（待完善内容、临时决定和技术债）

> 本文集中记录待完善内容、临时决定和技术债。开新功能前先扫一遍。消化后标记已解决。

---

## §1 未实现的功能

### 规则层

| 项                                                     | 现状                                                                                 | 后续方向                                                                           |
| ----------------------------------------------------- | ---------------------------------------------------------------------------------- | ------------------------------------------------------------------------------ |
| `Status.Slow` 减速数值效果                                  | 只记录层数，不影响先机或 Cost                                                                  | 等 `WacomBattle.md` 正式定义减速公式后实现                                                 |
| `Status.Twilight` 暮气数值效果                              | 只记录层数，不触发任何效果                                                                      | 等"暮气生效触发点"规则确认（回合开始？部位行动前？）                                                    |
| 暮蛉 `OnTwilightTriggered` 真正改中毒层数                      | P3.5 只发 `PassiveTriggered` 事件，不改 Magnitude                                         | 需引入 `FRuntimeCardInstance::EffectMagnitudeModifiers` 或等价机制                     |
| 卡牌耐久 `Durability` 消耗                                  | `FCardPhysique::Durability` 字段存在但不读取                                               | 等耐久系统设计（暮色引虫灯 1 耐久 = 打出一次进消耗区）                                                 |
| 左手主动效果 / 完美释放效果                                       | 左手 `Effects` / `PerfectReleaseEffects` 留空                                          | 等具体卡牌设计                                                                        |
| 右手"相邻右方伙伴代打"                                          | 未实现                                                                                | 等 `Target.Adjacent.Right` 的 Executor 分支                                        |
| 击倒事件三选一具体效果                                           | Stage 7 已搭好"撤离/援助/破坏"框架 + dialog UI + BattleProgress 持久化撤离破坏部位；援助/破坏不依赖当前手牌区左右手是否存在；最后存活部位击倒后撤离不可选；Run 层第一阶段仅记日志       | Stage 9 节点事件接入时按 `FKnockdownChoice::Choice` 分支触发实际效果（左手 buff / 永久强化部位 / 特殊节点等） |
| 击倒事件左右手永久缺失可用性                                      | `FKnockdownChoiceView` 已预留 `LeftHandMissing / RightHandMissing` reason；当前 Aid/Destroy 不看手牌区锚点是否存在，也不处理角色永久失去左/右手 | 等手指/事件导致永久失去左/右手牌的 Run/Battle 字段确定后，在击倒可用性 helper 中禁用对应分支 |
| 蛇部位间联动                                                | 无（头被破坏时身体不强化）                                                                      | 等更多敌人设计后按需加                                                                    |
| 手牌满时 OnCompanionCount 处理                              | 随机插入当前手牌后立即执行普通卡上限，超限卡进弃牌区                                                           | 若规则变更为"满时不触发"，改 `RunOnCompanionCountPassives`                                  |
| 存档系统恢复                                                | Stage 0.1 暂停（`bSaveSystemEnabled = false`），底层 UWacomSaveGame / FRunState 拷贝/迁移机制保留 | demo 完善后恢复：Bootstrap 读盘 / PauseMenu Save 按钮 / MainMenu Continue                |
| `IsDeleteFunctionAvailable` 接入 `DeleteCardForGold` 校验 | 接口就位但 DeleteCardForGold 不读（GDD §11.7 第一阶段始终允许删牌）                                   | 等 GDD 切换为"按需可用"后接入                                                             |

### 卡牌扩展（按需做，未来卡牌出现时再实现）

| 项                                                      | 现状  | 触发实现的条件                            |
| ------------------------------------------------------ | --- | ---------------------------------- |
| `Effect.CopyCard` 复制手牌临时副本                             | 未做  | 出现需要复制机制的卡                         |
| `Magnitude.Source.DiscardCount` 弃牌堆数量 Magnitude        | 未做  | 出现按弃牌堆数量调整数值的卡                     |
| `Magnitude.Source.DestroyedPartCount` 已破坏部位数 Magnitude | 未做  | 出现按破坏部位数加伤的卡                       |
| `Target.AllHandCards` 手牌全展开                            | 未做  | 出现"对所有手牌生效"的卡                      |
| `Target.Adjacent.Left` 本卡左相邻                           | 未做  | 出现按相邻位置定位的卡（与右手代打类卡共用 Executor 分支） |
| `Target.Adjacent.Right` 本卡右相邻                          | 未做  | 同上；右手代打也依赖此                        |
| `Target.RandomEnemyPart` 随机存活部位                        | 未做  | 出现随机选部位的卡                          |

### 卡牌扩展（已注册 Handler 但调用点未接入）

| 项 | 现状 | 接入要求 |
|---|---|---|
| `Passive.Trigger.OnTurnStart` | Dispatcher 方法已就位，无调用点 | 出现需要回合开始触发的被动卡时，在 `BattleTurnFlow` 起始阶段加调用 |
| `Passive.Trigger.OnTurnEnd` | Dispatcher 方法已就位，无调用点 | 出现回合结束触发的被动卡时，在 `EndTurnResolver` 加调用（注意时序） |
| `Passive.Trigger.OnDraw` | Dispatcher 方法已就位，无调用点 | 出现入手触发的被动卡时，在 `DeckService::DrawCards` / 手牌编排路径加调用 |
| `Passive.Trigger.OnDiscard` | Dispatcher 方法已就位，无调用点 | 出现弃牌触发的被动卡时，在 `DeckService::DiscardFromHand` 与回合结束弃牌路径加调用 |

### 卡牌扩展（新 GDD 触发的依赖项）

| 项 | 现状 | 依赖 GDD 章节 |
|---|---|---|
| `Passive.Trigger.OnEnemyPartDestroyed` | 未做 | GDD §6 / §3.3。Stage 7 已让"玩家三选一"在 `RecordPartDestroyed` 路径有挂载点，被动触发可一并接入 |
| `Passive.Trigger.OnPlayerDamaged` | 未做 | 战内"玩家受扣血"事件可由战内伤口阈值跨越（GDD §3.2 / §9.2）的 flag 维护承接，不一定要走 Passive trigger。先观察 |
| B 类容器卡容量效果扩展 | `WeaponDamagePlus3` 已实现；其他 CapacityEffect 尚无通用扩展框架 | 等具体容量效果设计（cost-1 / 关键词加成 / 数值修正）落地后逐个接入 |
| 暮色引虫灯战斗主动效果 | 当前 Cost=0 无效果，打出无意义 | GDD §4.4 定义了 1 耐久 / 打出一次进消耗区，等耐久系统接入 |
| 暮色引虫灯任务后升级 | 未做 | 远期，等任务系统 |
| 击倒事件 UI dialog 美术 | Stage 7 已落地：C++ 硬编码 CanvasPanel + Border + Button 布局，BindWidget 锚点 PartNameText/AidButton/WithdrawButton/DestroyButton 就位 | 美术阶段配 WBP 即接 |
| 地图系统（Stage 8）| 节点/通道/迷雾/撤离回路规则已在 GDD §10 确认，代码未开始 | 新建 WacomMap 模块或放 WacomRun 下 |
| 节点事件（Stage 9）| 露营/野炊/商店/探险/事件规则已在 GDD §10 确认，代码未开始 | 强依赖 Stage 8 地图系统完成 |

### UI / 表现层

| 项                            | 现状                                                                                                                                  | 后续方向                                                                                                                                                                                                                                                           |
| ---------------------------- | ----------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| UI 动画（P5 整体）                 | 全部跳过，HP/卡牌/伤害数字无过渡                                                                                                                  | 美术资源到位后做事件队列化 + 具体动画                                                                                                                                                                                                                                           |
| 主题与样式（P6 整体）                 | Widget Blueprint 纯色块 + 文字                                                                                                           | 美术阶段只改 WBP，C++ 不动                                                                                                                                                                                                                                              |
| 手牌扇形布局                       | `UHandPanel` 已先将 Snapshot 转成 `FHandCardVisualEntry[]`，当前默认 renderer 是统一水平手牌带；`BattleHUD` 可加载 `WBP_HandPanel`，WBP 只绑定 `UnifiedHandSlot`，并支持间距/边距/居中参数；`UCardWidget` 已支持基础 hover 上浮/缩放反馈和 hover 详情；点击目标牌进入目标选择，再点同一张牌可取消选择                    | 后续新增更强选中突出、详情样式美术化和扇形 renderer；必要时替换为自定义 `UHandLayoutPanel`，但继续消费 VisualEntry，不重读规则 Snapshot                                                                                                                         |
| 战斗卡牌拖拽                       | BattleHUD 仍是点击手牌再点敌方部位，不支持把战斗手牌拖到目标                                                                                                 | HD-2D 表现阶段评估是否改为拖拽到 3D 部位 / 悬停高亮 / 点击确认                                                                                                                                                                                                                        |
| 目标选择 3D 射线                   | 点击 EnemyPartWidget 2D 按钮；`BattleHUD::BuildTargetSelectionView()` 已作为只读表现桥，当前 2D UI 和未来 HD-2D/PaperZD 部位表现都可按 `PartInstanceId` 消费                                                                                                            | HD-2D 表现时改为 3D 部位高亮 + 点击；正式 Actor/Component 继续读取 TargetSelectionView，不重复解析 HUD 内部状态                                                                                                                                                                                                                                       |
| EventToast 图标/动画             | 纯文字                                                                                                                                 | 升级为"事件表现调度器" + Niagara + 音效                                                                                                                                                                                                                                    |
| 锚点左右归属                       | 遍历顺序启发式（第一个锚点进 LeftSlot）                                                                                                            | 给 `FHandCardSnapshot` 加 `EHandAnchorRole` 字段                                                                                                                                                                                                                   |
| 背包 UI 删牌区与 DeleteProvider 联动 | 删牌区始终显示（GDD §11.7 第一阶段约定），`IsDeleteFunctionAvailable()` 接口就位但 UI 不读                                                                 | 等 GDD 切换为"按需可用"后，BackpackScreen 根据 `IsDeleteFunctionAvailable()` 显示/隐藏删牌区                                                                                                                                                                                      |
| 探索 HUD 压力阈值警示色               | 压力值纯数字白色                                                                                                                            | 压力 >50% 黄色 / >80% 红色                                                                                                                                                                                                                                           |
| 背包存放区主卡槽/内容槽重构               | 通量区接口已拆出 `FluxMainCardsHost / FluxContentDropTargetHost`；SpecialZone 已抽成 `UWacomSpecialZoneWidget`，接口层按 OwnerCard + ContentCards 渲染 | 正式 WBP 按美术草图落主卡槽/内容槽视觉；后续可为 `UWacomSpecialZoneWidget` 创建独立 WBP                                                                                                                                                                                                 |
| 背包 UI WBP 美术落地               | 暂缓继续推进。`BackpackScreen` 已拆为三大区 Host，并删除旧通量混合布局，只保留通量主卡/内容槽接口；WBP 绑定清单已落在 `Docs/UI_Backpack_WBP_Binding.md`；C++ fallback 可运行         | 后续在编辑器中创建/调整正式 `WBP_BackpackScreen`，按绑定清单接 `DeleteZoneHost / BattleDeckZoneHost / FluxMainCardsHost / FluxContentDropTargetHost / SpecialZonesHost / BurdenZoneHost`，并按 `Docs/Image/背包界面.png` 调整外层结构和样式；规则仍通过 `RunSession::MoveInstance` / `DeleteCardForGold` |
| 背包 UI 拖拽手感                   | 已接入 UMG DragDropOperation，但当前仍是 C++ 默认布局与全量重建，缺少悬停高亮、失败提示、动效反馈                                                                      | 后续做交互 polish；真实规则继续以 `RunSession::MoveInstance` / `DeleteCardForGold` 为准                                                                                                                                                                                       |
| 背包 UI 增量刷新                   | `BackpackScreen::RebuildAll()` 每次从 RunState 全量重建所有区块                                                                                | 卡牌数量明显增加或需要动画时，迁 ListView/TileView 或做 instance diff                                                                                                                                                                                                            |

### 架构层

| 项 | 现状 | 后续方向 |
|---|---|---|
| 网络复制 | 未实现 | 远期，单人游戏暂不需要 |
| GAS（GameplayAbilitySystem）| 不使用 | 保持不引入，战斗用自研 Resolver/Executor |
| UI 架构迁移 MVVM | M1+M2 已落地：Run 域走 ViewModel + Provider 订阅模型；C++ 父类硬编码布局 + 订阅粗粒度多播 + 手动 SetText；FieldNotify 字段就位但未被 WBP ViewBinding 消费 | 美术阶段切 WBP：ViewModel 加到 WBP 配 Global Collection Identifier `WacomRunViewModel`，View Bindings 绑字段到 TextBlock/ProgressBar；C++ 父类 SetText 路径作 fallback 保留 → 全 WBP 后逐步删 |
| 战斗 UI 接 ViewModel | 保留 Snapshot 推送模型（BattleHUD 作 Controller 递归 RefreshFromSnapshot） | 第一阶段不动。如果将来非战斗 widget 需要"看战斗状态"（如击倒事件 UI / 探索期小窗），加 `UWacomBattleViewModel` 作外部观察入口；子 widget 内部仍用 Snapshot |
| 卡牌 UI 展示数据复用 | `UWacomCardPresentationBuilder` 已从 `UWacomCardView` 抽出；背包卡牌、拖拽预览、详情面板、战斗手牌已走统一构建入口；旧 `UWacomCardView::Build*` 静态函数仅作兼容转发 | 后续奖励、商店、事件预览等卡牌显示统一接入 Builder；确认无蓝图/代码依赖后再考虑移除旧兼容入口 |
| 战斗手牌 WBP 承接 | `UCardWidget` 已支持可选 `CardView / HoverVisualRoot`、缺槽容错、运行时费用展示、hover 上浮/缩放和详情上报；`BattleHUD` 已优先加载 `WBP_HandPanel` 并管理手牌 hover 详情；`Docs/UI_Battle_WBP_Binding.md` 已记录 WBP 绑定清单与 PIE 检查项 | 后续在编辑器中调整正式战斗手牌 WBP；选中突出、扇形布局、拖拽出牌和详情样式美术化另行推进 |
| 背包 UI Presenter 收口 | `UWacomBackpackScreenPresenter` 已抽出标题文本、投影来源、详情数据和悬浮定位等纯展示逻辑；`SpecialZoneWidget` 标题/已出战可见性也已改走 Presenter；`BackpackScreen` 继续负责 Widget 编排和命令提交 | 后续如 `BackpackScreen` 继续膨胀，可再抽列表 section view data 或命令协调对象；当前不把拖拽/确认框/RunSession 命令搬进 Presenter |

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
| `Magnitude.Source.TargetStatusStacks` 借用 `FCardEffect::TargetZone` 传 Status Tag | 字段复用：TargetZone 字段对非 Shuffle 效果无其他用途，所以借来传状态 Tag | 给 `FCardEffect` / `FEffectContext` 加专用 `FilterTag` 字段 |
| `Effect.GainKeyword` / `Effect.RemoveStatus` 借用 `FEffectContext::MetaTag` 传 Keyword/Status Tag | 同上字段复用 | 同上 `FilterTag` 字段 |

### UI 层

| 项 | 临时做法 | 正式方案 |
|---|---|---|
| 战斗 UI 全量刷新 | 每次命令后 BattleHUD 从 Snapshot 全量刷新子 widget | 加动画时动画系统自己做 diff，数据刷新仍保持 Snapshot 源 |
| C++ 硬编码默认布局 | Widget 类 `Blueprintable` 非 Abstract，带 C++ 默认布局 | 后续美术只改 WBP |
| HP 条瞬间跳变 | `SetPercent` 直接设值 | 加 `SetTargetPercent` + Tick Lerp 插值 |
| 背包 UI C++ 默认布局 | 已切到拖拽模型，但视觉仍由 C++ 构造默认布局承载 | 美术阶段用 WBP 子类替代布局，C++ 保留协议和 fallback |
| 背包 UI 全量 RebuildAll | 每次 ViewModel 刷新后清空 WrapBox 重建所有子控件 | 增量 diff 或迁 ListView/TileView |
| 探索 HUD 时段总节点数 | 只显示"剩余节点"，没有"本时段总节点数"快照 | FRunState 加 `TotalNodeCountForPhase` 字段或 HUD 在时段切换时记录初始值 |
| ViewModel FieldNotify 暂未被 WBP 消费 | C++ 父类用粗粒度 OnRunViewModelRefreshedNative + 手动 SetText | 美术阶段切 WBP 时启用 ViewBinding 直接绑字段，删除粗粒度 multicast |

### 架构层

| 项 | 临时做法 | 正式方案 |
|---|---|---|
| BattleState 非反射 | 裸 struct + pImpl，GC 引用靠 Session 的 `ReferencedAssets` | 若需存档/网络，升级为 USTRUCT 或 UObject |
| MSVC 工具链 14.38 | UE 5.7 警告 "not preferred"，不影响功能 | 升级到 14.44+ |
| `RunSession.cpp::ApplySaveGameToRunState` 内嵌套 lambda 触发 MSVC C1001 ICE | 把 `RestoreCardInstanceList` 提取为 anonymous-namespace file-scope free function | MSVC 14.44+ 升级 / 切 Clang 后视情况合回 lambda |
| `OnRunStateChangedNative` 多次广播 | 一次玩家操作可能链式触发多次 Broadcast（如 AddCardToBackpack → RecomputeBurden → SetPressure 各一次）| ViewModel 端 UE_MVVM_SET_PROPERTY_VALUE 已 dedupe，FieldNotify 不会重复触发；粗粒度多播订阅方应保证刷新幂等 |

---

## §3 临时决定（未正式化的）

### UI 相关

- **[P1] 战斗 UI 全量刷新策略**：每次命令后从 Snapshot 刷新 BattleHUD 子 widget，不做增量 diff。
  → 后续加动画时，动画系统自己做 diff，数据刷新仍然全量。

- **[P1] 战斗 UI 不做 ViewModel 层**：BattleHUD 持有 `UBattleSession*`，子 widget 读 `FBattleSnapshot` 刷新。
  → 后续 UI 复杂度上升时抽 `UBattleViewModel`。

- **[P1] 手牌用 HorizontalBox 线性排列**：不做扇形、不做拖拽。
  → 后续美术阶段替换为自定义 `UHandLayoutPanel`。

- **[P1] 目标选择用"点击 EnemyPartWidget"实现**：不做拖拽到目标、不做射线检测；目标可选性已通过 `FBattleTargetSelectionView` 从 BattleHUD 输出。
  → 后续第一人称 HD-2D 表现时，目标选择可能改为"鼠标悬停 3D 部位 → 高亮 → 点击"，但仍消费同一份 TargetSelectionView。

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

- **[P3.4] OnCompanionCount 触发时超手牌上限**：触发时随机插入当前 Hand，然后立即执行普通卡上限 10，超限卡进弃牌区。
  → 后续若有"手牌满时不触发"的规则变更，改 `RunOnCompanionCountPassives` 里改为拒绝回手。

- **[P5] 暮蛉 OnTwilightTriggered 需要暮气"生效"**：第一阶段暮气只记录层数不生效。
  → 需要先定义"暮气生效"的触发点，然后才能触发暮蛉被动。规则未决项。

### 架构相关

- **[P2.3] 锚点左右归属用"遍历顺序"启发式**：`FHandCardSnapshot` 没告诉 UI 某张锚点卡是左手还是右手。
  → 正式方案：给 `FHandCardSnapshot` 加 `EHandAnchorRole AnchorRole`（None/Left/Right）。

---

## §4 已正式化（参考用）

以下条目曾是临时决定，现已写入正式规则文档，代码实现与文档一致：

- 中毒穿透护盾 → `WacomBattle.md §6`
- 中毒触发时机（打牌后 + 行动后）→ `WacomBattle.md §6`
- 晕厥层数模型（每次行动消耗 1 层）→ `WacomBattle.md §11`
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
13. 是否需要正式 `DrawToZone` 效果：从 Draw/Discard/Exhaust 入手后直接放入指定 `HandZone.*`，还是保持 `Effect.Draw` 随机插入 + `Effect.Shuffle.*` 只腾挪现有手牌。
14. 如果未来 `DrawToZone` 指向不存在的区域（如 `HandZone.Both` 但双手区不存在，或左右手都离手），效果应失败、随机插入、还是降级到可用区域。
15. 如果未来 `DrawToZone` 在普通手牌满时触发，上限处理应先弃牌再放入以保证新卡进手，还是先放入再按普通上限规则弃掉超限卡。
