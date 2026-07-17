---
type: task-index
scope: wacom-short-term
status: active
updated: 2026-07-16
tags:
  - wacom/todo
  - wacom/docs
---

# TODO 索引

> [!info] 本文职责
> 本文只放短期任务入口、优先级、归属和跳转，不写规则真相或实现史。当前规则看领域文档；长期方向看 [`Roadmap.md`](Roadmap.md)；临时写法看 [`TechDebt.md`](TechDebt.md)；待确认问题看 [`Questions.md`](Questions.md)。

> [!warning] 执行约束
> 标记为 `Blocked` 的任务不要直接写死设计口径。先在 [`Questions.md`](Questions.md) 或对应领域文档中收口，再进入实现。

## 文档分工

| 文档 | 职责 | 维护规则 |
|---|---|---|
| [`Roadmap.md`](Roadmap.md) | 未实现功能、长期方向、可分阶段推进的内容扩展 | 新功能还没有进入短期实现前，先放这里 |
| [`TechDebt.md`](TechDebt.md) | 临时写法、兼容字段、临时决定、正式替代方案 | 代码里出现 `TODO(技术债)` 时同步到这里 |
| [`Questions.md`](Questions.md) | 会影响规则、策划口径或长期架构的待确认问题 | 不在代码里静默写死这些问题 |
| 本文 | 1-2 轮内可能推进的短期任务索引 | 不承载长说明，不复制领域规则 |

## 当前推进顺序

1. 先收口 P0 规则问题。
2. 再推进地图 / 节点服务。
3. 地图口径确定后，再接击倒分支、RunEvent / Shop 的节点生成与存档恢复。
4. UI WBP、表现 polish 和存档恢复按 Demo 范围穿插推进。

文档重构进度：V0-FP 完成剩余文档漂移清理：Battle WBP 装备占位、Data Durability、GDD 自引用和索引文档 wikilink 已收口。

## P0 策划确认

- [ ] **明确击倒事件 Aid / Withdraw / Destroy 的正式分支效果**
  - 状态：`Blocked: 策划确认`
  - 归属：战斗 / Run
  - 入口：[Questions: 击倒与战后结算](./Questions.md#questions-knockdown)

## P0.5 近期架构规划

- [x] **WacomMap 总体规划：新建地图 / 节点 / 通道规则文档入口**
	- 状态：`Done: 规则核心、Run Path、当前 Floor Map Screen、正式制作基线与隔离 Debug builder 已落地`
  - 归属：Map / Run / App
  - 入口：[WacomMap.md](./WacomMap.md) / [Roadmap: 地图与探索](./Roadmap.md#roadmap-map) / [Questions: Run、探索与地图](./Questions.md#questions-run-map)
	- 说明：Logical Map Graph、Action Point、节点生命周期与类型、同层 Map Travel、不可逆 Floor Transition、Floor Exposure、Camp Action / Camp Activity、节点内容原子结算、原子初始化、Run Path Traversal、当前 Floor Map Screen、Descriptor-first 原子场景绑定、只读 Validator、Authoring baseline 与 Debug-only builder 已经落地。每日预算保持 `2 / 6 / 2 / 2 / 1`；Dusk 保留 Picnic，Night 选择 Camp / Night Exploration。下一步是正式 Floor 1、正式 Camp handler/UI、多 Floor 内容和旅程规模填充，不恢复已删除的原型轨道路径。
	- 2026-07-15 地图闭环：单合法出口 W 自动前进，多合法出口使用道路入口选择，Anchored 无需点击 Actor 即可持续 cursor look；M / 手柄 View 打开当前 Floor 地图，死胡同默认聚焦最近的已完成节点，并可免费同层返回。

- [ ] **Run Map 后续页面与交互扩展**
	- 状态：`Backlog`
	- 归属：Map / UI
	- 说明：旧 Floor 只读历史页签、地图缩放/拖拽、多 Floor Transition 页面和更复杂的迷雾表现后续独立规划；首版当前 Floor Screen 不预留隐藏按钮或半实现状态。

- [ ] **正式 Floor 1 内容设计与稳定身份冻结**
  - 状态：`Blocked: 策划确认`
  - 归属：Map / Run / Data / App
  - 入口：[WacomMap.md](./WacomMap.md) / [Questions: Run、探索与地图](./Questions.md#questions-run-map)
  - 说明：当前 `Floor.Authoring.01` 的 8 节点图只是制作基线。后续需独立确认正式 Floor 1 节点图与稳定 NodeId、内容密度、跨层入口、正式 Camp 内容，以及哪些身份进入未来 SaveGame schema；不得直接把 Authoring 或 Debug 身份冻结为正式内容。

## P1 近期实现候选

- [ ] **正式资产依赖审计与 `/Game/Wacom` 迁移**
  - 状态：`In Progress: Phase A 只读基线完成，等待资产所有权确认后分批迁移`
  - 归属：Content / Editor / Build
  - 入口：[Content Dependency Audit](./Content_Dependency_Audit.md) / [Worktree Development](./Worktree_Development.md) / [Content Organization](./Content_Organization.md)
  - 说明：`WacomAuditContentDependencies` 已形成稳定 JSON contract。2026-07-16 基线为 163 个 Wacom 起始 package、277 个遍历 package、114 个外部依赖（92 直接、22 传递），集中在 `/Game/Art` 7、`/Game/Asset` 82、`/Game/DreamMaterials` 25；`Content/DreamMaterials` 现有 61 个 `.uasset` 已整体由 Git LFS 管理，只剩 `/Game/DreamMaterials/M_Card_Step2_Inst` 在依赖图中存在但本地缺失。下一步先确认 `Card_Luo`、`Plane`、`Chong_CardAsset` 及音频/字体/UI/Boar 样例的来源与授权，再按 A–F 批次用 Content Browser 迁移或建立版本化依赖 manifest。

- [x] **背包视觉制作与四分辨率适配**
  - 状态：`Done: formal/fallback assets, responsive matrix and final PIE complete`
  - 归属：App / UI / Editor / Tests
  - 入口：`specs/005-backpack-visual-production/`
  - 说明：该轮完成全局 `1920×1080` DPI、Root/Main/Body/Workspace/Overlay Fill、Header Auto、固定卡面缩放和静态 Retainer，并通过四分辨率、16:10、超宽及高卡量验收。2026-07-16 后续内嵌牌堆改造已删除右侧 Rack，但继续沿用本轮的分辨率、卡面与 Retainer 合同。

- [ ] **背包真实卡牌牌堆与携带性能改造**
  - 状态：`Implementation and automated/Builder verification complete; final PIE/Insights pending`
  - 归属：App / UI / Editor / Tests
  - 入口：[Backpack WBP 制作与绑定合同](./UI_Backpack_WBP_Binding.md) / [Wacom UI](./WacomUI.md)
  - 说明：通量区保持统一自由工作台；备战、特殊和负重牌堆已迁移为折叠/展开共用的全部真实卡面。正式链路删除 Preview class/ViewData/Builder 依赖，新增 `WBP_BackpackZonePile` 与 Workspace 分层，DeckCard 改承载 `WBP_FPCardView`。携带路径使用单一 `CarryRoot` 锚点：静止 `CarryCache` 缓存非当前扇形卡，独立 `CarryActiveLayer` 只承载当前实时卡；鼠标移动不插值、不重算扇形、不全量刷新静态卡。跨区提交按 `InstanceId` 原位迁移同一实体 Widget，不再因 `PhysicalZone` 变化创建替代实例。牌堆 Reconcile 以实际 Canvas 子控件为视觉所有权真相并在 Destruct 主动清理，已消除反复展开/收起后累积不可交互牌堆框的问题；用户连续 10 次 PIE 验证通过。`WacomEditor -DisableUnity`、Builder 双跑 Hash、`Wacom.UI.Backpack` 71/71、`Wacom.Run.Backpack` 2/2 已通过；完成剩余 1/21 张携带手感与 Insights 观察后更新为 Done。对应阶段工件见 `specs/006-backpack-real-card-piles/`。

- [x] **背包 Workspace 重构：正式实现、旧路径迁移与统一 PIE 验收完成**
  - 状态：`Done: C++ + formal WBP + DreamShader + legacy cleanup + final PIE complete`
  - 归属：Run / App / UI / Tests
  - 入口：`specs/003-backpack-workspace-refactor/`
  - 说明：`specs/003-backpack-workspace-refactor/` 的 T001–T076 已全部完成，建立了 Workspace、同 Run 瞬态布局、框选与持续扇形携带、分层 Back、原子批量移动/销毁、确认恢复、正式 WBP/Style、静态 Retainer 卡面与 Wacom-native DreamShader 反馈。该轮的“单活动区 + 常驻 ZoneRack”表现结构已在 2026-07-16 内嵌牌堆改造中替换；规则事务与输入合同继续复用。
  - 2026-07-14 Surface Foil 跟进（历史）：当时的 `WBP_BackpackCardView` 静态 wrapper 合同已由 2026-07-16 的 `specs/006-backpack-real-card-piles/` 取代；新正式路径直接复用 `WBP_FPCardView` 并通过单动态卡预算控制实时材质，不再扩展旧 wrapper。
  - 2026-07-15 T070 布局与卡面第一轮：24 张实体牌、4 个牌匣基线下，中央单活动区和高亮牌匣识别体感无延迟；用户明确豁免秒表记录并接受该定性结果，文档不虚构具体秒数。用户已确认三张卡的手动位置、角度和 ZOrder 在切区及同 Run 关闭/重开后保持，新 PIE Run 不继承旧布局；越界释放仍保留约 30% 卡牌主体；卡面静置保持完全不透明，费用/名称/耐久/出血徽章稳定，无冻结 Surface Foil 或采样清晰度变化。本轮无代码缺陷需要修复。
  - 2026-07-15 T070 事务、生命周期与表现轮：用户已确认活动区牌匣收拢、单张/整组跨区移动、容量与 stale 原子拒绝、批量销毁确认/取消/成功、切区取消、Deactivate/Reactivate、详情与确认焦点、键盘导航、反馈区分、命中几何及空闲表现均正常。携带中按 B 可关闭且重开后无捕获、扇形、旧选择或携带残留，鼠标/框选/卡牌点击立即可用。Escape 已定稿并完成分层 Back PIE：携带/框选/待决按压时先取消指针事务，下一次空闲 Escape 交给 CommonUI 关闭，B 始终直接关闭；实现和 `ScreenComposition` 自动化合同已补，`WacomEditor`、`Wacom.UI.Backpack` 63/63、`Wacom.Run.Backpack` 2/2 通过。T070 所有 PIE 项已完成，秒表项按用户明确豁免以“体感无延迟”收口。
  - 2026-07-14 Workspace Feedback SM6 修复：`UMaterialExpressionVertexColor` 默认输出 0 实际为 RGB `float3`，Alpha 是独立输出 4；隐式 `.w` 与对输出 0 使用 A `ComponentMask` 都会报 `Not enough components`。`.dsm` 现直接消费 OutputIndex 0/4，并通过 DreamShader `-Force` 重建正式材质；`WorkspaceFeedbackCompiles` 红→绿测试会实际重编译材质、穿透 Named Reroute 核对输出索引并拒绝非法 A mask，真实 D3D12 `PCD3D_SM6` 定向测试及完整 `Wacom.UI.Backpack` 61/61 均通过。
  - 全量回归证据：`Automation RunTests Wacom` 找到 1302 项后，在背包范围外失败并中止：`Wacom.UI.Battle.BattleHUD.HandPresentation.ShortcutStartsDragByHandIndex`、`Wacom.UI.Battle.FirstPersonTargetPreview.NoTargetCommitShowsPlayerActionPreview` 失败，随后 `BattlePresentationQueueSpec.cpp:259` 发生 `Array index out of bounds: 1 into an array of size 1`。该组属于暂停的 Battle/first-person 卡牌表现线程，本切片未越界修改。

- [x] **敌人系统重构：稳定身份、行为数据化、场景表现拆分**
  - 状态：`Done: 主链路已落地`
  - 归属：Data / Battle / Run / App / Editor
  - 入口：`specs/002-enemy-system-refactor/`
  - 说明：稳定 enemy key、Run 撤离重入进度、command / snapshot / event / result packet、行为数据化、Encounter 接线、SceneEnemyHost registry、Host 整体视觉、PartActor hit-only / VisualLayers 和 BattleHUD 场景目标主链路已落地。长期事实已回写到 `Docs/WacomBattle.md`、`Docs/WacomData.md`、`Docs/WacomRun.md`、`Docs/WacomWorldInteraction.md`、`Docs/WacomBattleUI.md` 和 `Docs/WacomDataAuthoring.md`；`specs/002-enemy-system-refactor/` 只保留为阶段性规划和最终搜索 / 验证清单。

- [x] **UI Ownership / BattleHUD 私有 coordinator 拆分**
  - 状态：`Done`
  - 归属：UI 架构 / App
  - 入口：[WacomUI: UI Ownership Map](./WacomUI.md#wacomui-ui-ownership-map) / [TechDebt: UI 架构债](./TechDebt.md#techdebt-ui-architecture)

- [x] **交互目标系统：统一 target handle、Provider 接口、World 命中**
  - 状态：`Done`
  - 归属：Core / App
  - 入口：[WacomWorldInteraction.md](./WacomWorldInteraction.md)

- [x] **交互目标系统：Battle scene enemy current Host registry**
  - 状态：`Done`
  - 归属：App / Battle World Target
  - 入口：[WacomWorldInteraction.md](./WacomWorldInteraction.md) / [WacomBattleUI.md](./WacomBattleUI.md)

- [ ] **战斗表现队列：快速连出后的视觉追赶 polish**
  - 状态：`Ready: 体验 polish`
  - 归属：App / UI
  - 入口：[WacomBattleUI.md](./WacomBattleUI.md) / [Roadmap: 战斗 UI](./Roadmap.md#roadmap-battle-ui)
  - 说明：后续只追踪表现体验，如 cue 合并、速度压缩、正式动画、stack 入场 polish、动画回放或规则层 command batch id。

- [ ] **First-person hand 动效第二阶段：世界目标命中反馈与整体 PIE polish**
  - 状态：`In Progress: 手牌目标像素刻印已接线，待世界目标与 PIE polish`
  - 归属：App / First-person Card Layer
  - 入口：[First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md) / [WacomBattleUI.md](./WacomBattleUI.md)
  - 说明：基础合同已完成显式空帧替换、Motion Mixer / 互斥 Transition Playback、真实 draw/discard/play anchors、Card Depth、单 Retainer 实时 Alpha 接触阴影、Drag Pickup、普通 Played 像素翻面收牌、Exhausted 消耗消散、成功 HandCard 目标刻印，以及 EndTurn `Sealing → Held → Releasing` 保留牌封存。所有 Battle PlayCard 已收口为 `SourceOut → PrimaryTarget → Outcome → SourceReturn` 命令编排；费用与 EffectBadge Preview/Rewrite 分别直接作用于现有局部数字 Image，命令 Outcome 可并行等待两者完成。EffectBadge 已按 Damage/Poison/Heal/Shield 等语义类型聚合稳定 Key，具备可逆 Preview、整组 skipped 低亮和 ValueChanged/Added/Removed Playback；当前正式触发只连接明确许可事件下的可见 ValueChanged。下一步继续做整体 PIE 调参、音效预热、reduced-motion / 全局速度策略，以及等正式动态 Effect 事务落地后接入 Added/Removed；耐久、卡名、插画、稀有度与 Run 升级仍需各自权威变化事实。不复制第三方代码、shader 或资产。

- [ ] **战斗规则内容化：按 authoring matrix 扩展正式卡牌 / 敌人内容**
  - 状态：`In Progress: 内容扩展`
  - 归属：Battle / Data / Editor
  - 入口：[WacomDataAuthoring.md](./WacomDataAuthoring.md) / [WacomBattle.md](./WacomBattle.md)
  - 说明：继续做正式掉落 / 奖励池、更多敌人包、卡牌平衡和正式卡组入口；新增规则能力前先接 resolver、合同、文档和验证。

- [x] **Battle Card Zone Transition 后续迁移**
  - 状态：`Done: CardZoneAggregate + 全路径 typed transition 已完成`
  - 归属：Battle 规则内核
  - 入口：[WacomBattle §5](./WacomBattle.md) / [TechDebt: 规则层技术债](./TechDebt.md)
  - 说明：卡牌注册、六个定位容器、Runtime Location 与顺序已收口到 `CardZoneAggregate`；Effect、EndTurn、HandLimit、奖励和 Companion 统一由 `BattleCardZoneTransition` 消费 typed facts，旧 post-move event API 已删除。

- [ ] **战斗场景敌人表现 polish：正式美术、描边、tooltip 和高级动画状态机**
  - 状态：`In Progress: TrainingWarrior 正式内容包已完成，剩余表现 polish`
  - 归属：App / UI / Battle World Target
  - 入口：[WacomWorldInteraction.md](./WacomWorldInteraction.md) / [WacomBattleUI.md](./WacomBattleUI.md)
  - 说明：主链路已经能支撑普通小怪 Host 整体图 + hit-only 部位，以及精英 / Boss PartActor VisualLayers。TrainingWarrior 已作为首个正式包落地：BattleWarrior 的 36 个受控依赖晋升到 `/Game/Wacom` 并由 Git LFS 管理，规则数据、奖励卡、语义 Style、Host prefab 和单敌人 Encounter 均可重复生成。PartActor 通用 Destroyed 粒子 / 可选逐层破损终态也已完成；TrainingWarrior 按内容合同只使用粒子 + Host Downed，不伪造 Body 破损图。`SetupBattleEnemyHostAnimationPIEAssets.py` 继续只服务本地 Snake 调试。后续追踪材质描边、贴近部位 tooltip、风险动效、更多敌人包、首个正式 Multi-Part 破损资源、MultiPart/PaperZD 高级状态机和 Status Badge 美术替换。

- [x] **Battle 世界目标 TargetConfirmed + Damage：消费 Enemy Part Cue Playback + ImpactAnchor**
  - 状态：`Done: Niagara 像素确认 / 伤害反馈已接线`
  - 归属：App / Battle World Presentation
  - 入口：[WacomBattleUI.md](./WacomBattleUI.md) / [WacomWorldInteraction.md](./WacomWorldInteraction.md)
  - 说明：`TargetConfirmed / Damage / Destroyed` 已接入共用 Niagara 像素反馈，读取统一 Playback、ImpactAnchor、稳定 Seed、Reduced Motion 与 semantic / decorative flash 分类；不恢复旧缩放脉冲。Destroyed 的逐层终态见下一项。

- [x] **Battle 世界目标 Destroyed 正式破坏反馈**
  - 状态：`Done: 两段像素崩裂 + 35% 原地破损换图`
  - 归属：App / Battle World Presentation
  - 入口：[WacomBattleUI.md](./WacomBattleUI.md) / [WacomWorldInteraction.md](./WacomWorldInteraction.md)
  - 说明：消费既有最高优先级 `Destroyed` Cue 与 ImpactAnchor，先播放语义裂印和可关闭位移的大碎块，再在默认 35% 处原地替换配置了 `DestroyedSprite / DestroyedFlipbook` 的 VisualLayer；缺失资源保持原图，HitOnly 只播放粒子并继续由 Host Downed 承担整体终态。组件、registry topology 和规则事件不变；攻击方向、局部材质闪白和镜头震动留给独立切片。

- [x] **EncounterDefinition 运行时接线：BattleTrigger 引用 Encounter 并构造 Battle EnemySlots**
	- 状态：`Done: Trigger 已接线`
	- 归属：Data / Run / App / Battle
	- 入口：[WacomData.md](./WacomData.md#wacomdata-encounter-definition) / [WacomRun.md](./WacomRun.md#wacomrun-battle-settlement)
	- 说明：`ABattleTriggerActor.EncounterDefinition` 已转换为 `FBattleInitParams.EnemySlots`；运行态 `EncounterId` 仍用 Trigger `PersistentId`。旧单敌人 Trigger 入口已删除。

- [x] **BattleTrigger 多 SceneEnemyHost 映射：EnemySlotId -> Host registry**
	- 状态：`Done: Trigger 已接线`
	- 归属：App / Battle World Target
	- 入口：[WacomWorldInteraction.md](./WacomWorldInteraction.md#5-battle-scene-target) / [WacomBattleUI.md](./WacomBattleUI.md#6-scene-enemy-ui)
	- 说明：`ABattleTriggerActor.SceneEnemyHostSlots` 已按 `EnemySlotId` 绑定多 Host；旧单 `SceneEnemyHost` Trigger 入口已删除。后续敌人方向集中到正式蛇 Host 蓝图 prefab 资产、正式美术样式和制作工具。

- [x] **Resolved Encounter Scene Host 退役生命周期**
	- 状态：`Done: Downed -> 返回探索 barrier -> Trigger/Host/Part 退役`
	- 归属：Run / App / Battle World Target
	- 入口：[WacomRun.md](./WacomRun.md) / [WacomWorldInteraction.md](./WacomWorldInteraction.md#5-battle-scene-target) / [WacomBattleUI.md](./WacomBattleUI.md#6-scene-enemy-ui)
	- 说明：非撤离 Victory 仅在 Encounter ticket 成功提交后退役；Trigger 先禁用交互，Host 完整保留 Destroyed/Downed 终态，返回镜头和 ExitBattle 后置工作都完成后才隐藏并禁用 Encounter 内 Host/Part。Withdraw、Defeat、Undetermined 和结算失败保留；SaveGame v3 继续用 `DestroyedTriggerIds` 作为 Map Node Resolved 的兼容投影。

- [ ] **交互目标系统：Card-World / Card-Card drag resolver / Zone 命中来源接入**
  - 状态：`Ready: 剩余正式内容`
  - 归属：App / Battle / Run
  - 入口：[WacomWorldInteraction.md](./WacomWorldInteraction.md) / [First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)
  - 说明：基础链路已进入当前事实文档；TODO 只追踪正式内容、动画、SaveGame、复杂筛选、掉落表和更多奖励类型。

- [ ] **交互目标系统：Run / Zone 域规则求值接入**
  - 状态：`Ready: Run / Zone 正式规则接入时`
  - 归属：Run
  - 入口：[WacomBattle.md](./WacomBattle.md) / [WacomRun.md](./WacomRun.md)
  - 说明：Battle 已由 `PlayCard Evaluation` 权威处理 Card-World / Card-Card；本项只追踪 Run 与 Zone 后续各自的领域求值，避免重新引入跨域通用 Resolver。

- [ ] **接入击倒事件实际分支、奖励卡差异化和节点事件联动**
  - 状态：`Blocked: P0 击倒口径`
  - 归属：战斗 / RunEvent
  - 入口：[Roadmap: 击倒事件扩展](./Roadmap.md#roadmap-knockdown)

- [ ] **推进 RunEvent：随机事件池、更多条件效果、地图节点生成、存档**
  - 状态：`Blocked: 地图节点口径`
  - 归属：Run / Data / App
  - 入口：[Roadmap: 探索事件](./Roadmap.md#roadmap-runevent)

- [ ] **商店正式化：随机商品池、价格公式、正式 WBP、存档接入**
  - 状态：`Blocked: 地图节点 / 存档口径`
  - 归属：Run / Data / App
  - 入口：[Roadmap: 商店](./Roadmap.md#roadmap-shop)

- [x] **背包正式 WBP 与交互 polish**
  - 状态：`Done: Workspace/材质/旧路径清理与统一 PIE polish 完成`
  - 归属：UI / Run
  - 入口：[Roadmap: 背包 UI](./Roadmap.md#roadmap-backpack-ui)
  - 说明：该轮完成正式 Screen、Workspace、DeleteConfirm、Style、反馈材质和旧单卡 UMG drag/drop owner 清理；当时的 Rack/Entry 已由 2026-07-16 内嵌牌堆改造正式删除。20–100 卡 PIE 未观察到明显空闲 Tick 或逐帧 Snapshot 重建，因此仍不提前实施虚拟列表。

- [ ] **卡牌详情 token：ConditionTokenBuilder**
  - 状态：`Ready: 详情表现继续收口`
  - 归属：UI / Data presentation
  - 入口：[WacomUI.md](./WacomUI.md#wacomui-ui-ownership-map) / [WacomData.md](./WacomData.md#3-card-definition)
  - 说明：当前详情 token 已覆盖主动效果和被动触发 / 效果；`FEffectCondition` 只显示“有条件”弱提示。后续需要把 `Condition.Self.InZone`、`Condition.Target.HasStatus` 等条件生成结构化 token。

- [ ] **卡牌详情描述字段收口：Description vs FlavorText**
  - 状态：`Ready: 内容清理前置规划`
  - 归属：Data / UI presentation
  - 入口：[WacomData.md](./WacomData.md#3-card-definition) / [WacomUI.md](./WacomUI.md)
  - 说明：规则说明应逐步从 `Description` 迁到结构化 token；`Description` 是否保留为补充说明或拆出 `FlavorText` 需要单独规划，避免直接删除导致现有卡牌可读性下降。

- [ ] **RunEvent 正式 WBP 外观接入**
  - 状态：`Ready: 美术 / WBP 工作`
  - 归属：UI / RunEvent
  - 入口：[RunEvent WBP 绑定清单](./UI_RunEvent_WBP_Binding.md)
  - 说明：只替换外观、布局和 preview 表现；C++ fallback 继续负责动态选项、ZoneId、menu lease、drop intent 和 RunEvent 事务。

## P2 后续清理

- [ ] **Draw Reveal 正式主题牌背与多主题 MI**
  - 状态：`Ready: 临时像素牌背已接入，等待美术替换`
  - 归属：UI / 卡牌表现 / 美术
  - 入口：[First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md) / [DreamShader_Production_Guide.md](./DreamShader_Production_Guide.md)
  - 说明：Battle 真实 Drawn 的等待、翻面、落定和 Reduced Motion 合同已完成；当前 `T_FPCardDrawBack_Temporary` 是无文字、无角色、主题中性的临时像素牌背。正式制作只需在默认或主题 `MI_FirstPersonCard_SurfaceEffects_DrawReveal_*` 替换 `CardBackTexture`，不得新增第二个 Retainer 或把图案扩展到实体出血轮廓。

- [ ] **场景敌人像素命中 Niagara PIE 调参与正式音效**
  - 状态：`Ready: Graph/Style 已完成，等待手感验收`
  - 归属：App / Battle 表现 / VFX
  - 入口：[WacomBattleUI.md](./WacomBattleUI.md) / [WacomWorldInteraction.md](./WacomWorldInteraction.md)
  - 说明：`NS_WacomBattleEnemyPartImpact_Pixel` 的六个 CPU Burst Emitter、四通道 Dynamic Material Parameter、默认 MI/Style 与 Debug Snake Host 绑定均已由 WacomEditor 生成流程完成；尺寸读取单个部位 `HitBounds` 的摄像机平面投影，Destroyed 默认约 `1.35x` 覆盖。剩余工作是 PIE 调整确认、伤害、崩裂的手感与遮挡，并在 Style 中指定正式 TargetConfirmed/Damage/Destroyed 音效。攻击方向、局部材质闪白和镜头震动留给后续切片。

- [x] **用编辑器迁移旧内容目录并清理 `_GAME` 临时蓝图目录**
  - 状态：`Done: Content Organization V1-A`
  - 归属：Content / App / Data
  - 入口：[Content Organization](./Content_Organization.md)

- [ ] **战斗手牌表现升级：扇形布局、拖拽出牌、3D 目标选择**
  - 状态：`Blocked: HD-2D 表现方案`
  - 归属：UI / 战斗表现
  - 入口：[Roadmap: 战斗 UI](./Roadmap.md#roadmap-battle-ui)

- [x] **First-person card layer polish：详情面板跟随卡牌与层级修正**
  - 状态：`Done`
  - 归属：UI / 战斗表现
  - 入口：[First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)

- [ ] **First-person card render quality：扇形布局微调与专用卡面规范**
  - 状态：`In Progress: 美术与渲染 polish`
  - 归属：UI / 战斗表现
  - 入口：[First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md) / [WacomBattleUI.md](./WacomBattleUI.md)
  - 说明：后续按美术反馈微调扇形参数、卡面采样、Retainer 外元素、slot motion、commit pulse 和 drag / aim 读牌姿态。

- [x] **牌堆牌印表现第二切片：接收脉冲与弃牌化牌印**
  - 状态：`Completed: 普通弃牌与弃牌堆接收反馈已落地`
  - 归属：UI / 战斗表现
  - 入口：[First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)
  - 说明：2026-07-13 已完成普通 `CardDiscarded` 的实体卡原地收束、逐卡真实起点、牌印飞向弃牌堆、逐枚计数、像素 Impact 与真实 `DiscardPileView` 接收回弹；弃牌堆洗回继续复用同一批量 Slate renderer。两种传输按 Kind+Sequence 去重并 FIFO 播放，普通弃牌不复用洗牌音效，失效配置回退旧 Exit，Reduced Motion 不跨屏飞行或修改 PileView Transform。接收反馈由通用 `UPileCountView` 管理可叠加脉冲并精确恢复 authored Transform，当前只连接 `DiscardToPile`；未来可按同一接口扩展 DrawPileView / ExhaustPileView。

- [ ] **单一玩家档案与旅程存档：活动旅程、滚动备份、历史摘要**
  - 状态：`Deferred: 产品口径已确认，等待首版旅程闭环与核心内容稳定`
  - 归属：Run / App
  - 入口：[Roadmap: 存档恢复](./Roadmap.md#roadmap-save)
  - 说明：采用一个玩家档案、一个活动旅程、若干不可见滚动备份；结束旅程生成只读历史摘要。当前不实施，避免在 Run 节点、角色、奖励、事件、商店和首版旅程闭环仍持续变化时过早固化 schema。待首版旅程的最小可玩闭环、必须持久化的 RunState 字段和恢复入口稳定后，再独立规划 schema / migration / atomic write / rolling backup / recovery Bootstrap / PauseMenu Save / MainMenu Continue；slot 语义不得写回 MainMenu Screen。

- [ ] **MainMenu 正式表现：实时场景与剩余子页面**
  - 状态：`In Progress: Title / MainMenu 两层栈与正式 WBP 已完成，等待 PIE 视觉验收和场景切片`
  - 归属：App / UI
  - 入口：[Roadmap: App 壳层与菜单](./Roadmap.md#roadmap-app-shell-and-menus)
  - 说明：`WBP_TitleScreen + WBP_MainMenuScreen + WBP_MainMenuNavButton` 已建立第一版正式壳层；每次进入 `L_MainMenu` 先显示 Press Any Key，主菜单 ESC / B 返回稳定 Title 根且不会弹空 UI 栈。Settings Screen 与主菜单 / Pause 双入口也已完成。剩余工作是 `L_MainMenu` 实时场景、固定镜头、音频 / 字体 / 像素纹理美术验收，以及 Journey History / Credits 子页面。

- [x] **Settings Screen：WBP、视频确认 Modal 与双入口**
  - 状态：`Done: 五分类页面、token 事务、双入口和 Run Path CameraShake 已落地`
  - 归属：App / UI
  - 入口：[WacomApp.md](./WacomApp.md) / [WacomUIFoundation.md](./WacomUIFoundation.md)
  - 说明：`WBP_SettingsScreen / OptionRow / Button / ConfirmationDialog` 与 C++ fallback 已完成；主菜单和暂停菜单统一走 `FWacomSettingsScreenFlow`。Apply 后留在页面，视频模式使用 15 秒确认，脏返回确认放弃，外部 teardown 安全 Cancel / Revert。项目平衡档统一首次启动与“恢复默认”，恢复按钮只装入可撤销 Draft，仍需 Apply。分辨率已收敛为最低 `1280 × 720` 的常用档位与模式专属平台过滤；全局 UI 使用 `1920 × 1080` 设计基准的封顶 DPI 规则，只在较小视口缩小，高分辨率不放大固定 HUD 元素。`BP_WacomPlayerCharacter` 正式启用 CameraShake 并关闭 WalkBob。

- [ ] **低于 1280 × 720 的紧凑 UI 重排**
  - 状态：`Deferred: 当前桌面最低支持分辨率不需要该能力`
  - 归属：App / UI
  - 入口：[WacomUIFoundation.md](./WacomUIFoundation.md)
  - 说明：只有未来降低最低分辨率、支持更窄设备或横向空间不足的输入场景时，才为 Settings、主菜单与共享 Footer 设计正式断点和重排；当前不以额外 ScaleBox 或隐藏功能绕过布局合同。

- [ ] **清理 UI MVVM 迁移尾项：WBP ViewBinding、逐步移除 C++ 手动 SetText fallback**
  - 状态：`Ready: WBP 化后清理`
  - 归属：UI 架构
  - 入口：[TechDebt: UI 架构债](./TechDebt.md#techdebt-ui-architecture)

- [ ] **WacomApp Public UI API surface 收口切片**
  - 状态：`Ready: 低风险公开面继续收口`
  - 归属：UI 架构 / App
  - 入口：[TechDebt: WacomApp Public UI API surface](./TechDebt.md#techdebt-wacomapp-public-ui-api-surface)
  - 说明：后续优先处理 prototype / test-only surface。真正删除、迁到测试模块、迁到开发插件、移除控制台入口或重命名旧反射字段，都另起资产影响明确的切片；Blueprint-visible 制作面默认保守保留。

- [ ] **重点卡牌插画五级深度图制作**
  - 状态：`Ready: 可按卡逐张制作`
  - 归属：Card Art / UI Presentation
  - 入口：[WacomData.md](./WacomData.md) / [DreamShader_Production_Guide.md](./DreamShader_Production_Guide.md)
  - 说明：核心卡面已经支持可选 `CardIllustrationDepthMap`；普通卡继续使用统一凹入层。后续为高稀有度或重点卡牌使用原始透明插画生成 Image2 深度初稿，保持同构图并量化为五级灰度，人工确认后再写入 CardDefinition，不批量伪造所有卡牌深度。

## 维护规则

> [!tip] 新任务放哪
> - 1-2 轮内准备做：放本文。
> - 只是未来方向：放 [`Roadmap.md`](Roadmap.md)。
> - 当前实现里已经存在的临时方案：放 [`TechDebt.md`](TechDebt.md)。
> - 需要你或策划确认口径：放 [`Questions.md`](Questions.md)。

> [!note] 迁移说明
> 原 `TODO.md` 的未实现功能已迁入 [`Roadmap.md`](Roadmap.md)；临时写法、临时决定、兼容入口已迁入 [`TechDebt.md`](TechDebt.md)；待确认规则问题已迁入 [`Questions.md`](Questions.md)。已经正式化到领域文档、UI 专题文档和 Data 专题文档的实现，不再在这里重复追踪。
