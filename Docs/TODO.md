---
type: task-index
scope: wacom-short-term
status: active
updated: 2026-07-18
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

1. 先收口剩余 P0 规则问题；击倒卡牌奖励粒度、运行时合同、Floor 1 八张奖励卡内容及其 Production 资产已经完成，只剩其它非卡牌分支效果。
2. 再推进地图 / 节点服务。
3. 地图口径确定后，再接击倒分支、RunEvent / Shop 的节点生成与存档恢复。
4. UI WBP、表现 polish 和存档恢复按 Demo 范围穿插推进。

文档重构进度：V0-FP 完成剩余文档漂移清理：Battle WBP 装备占位、Data Durability、GDD 自引用和索引文档 wikilink 已收口。

## P0 策划确认

- [ ] **明确击倒事件 Aid / Withdraw / Destroy 的其它正式分支效果**
  - 状态：`Blocked: 非卡牌效果需策划确认`
  - 归属：战斗 / Run
  - 入口：[Questions: 击倒与战后结算](./Questions.md#questions-knockdown)

- [x] **击倒 Aid / Destroy 分支奖励合同基线**
  - 状态：`Done: 分支字段、legacy fallback、原子结算、被动预览与 Production 门禁已落地`
  - 归属：Data / Battle / App / Editor / Tests
  - 入口：[WacomData.md](./WacomData.md) / [WacomBattle.md](./WacomBattle.md) / `specs/012-knockdown-branch-reward-baseline/`
  - 说明：Aid/Destroy 显式字段使用统一查询，旧资产可 legacy-only 读取，混填为制作错误；FormalProduction 要求双分支显式配置。UI 只显示 Battle 奖励摘要。Floor 1 按四个敌人各一对的 8 个 CardId 已由 Spec 013 冻结并由 Spec 014 创建，11 个正式 Part 已使用显式双分支引用且 legacy 为空。

## P0.5 近期架构规划

- [x] **WacomMap 总体规划：新建地图 / 节点 / 通道规则文档入口**
	- 状态：`Done: 规则核心、Run Path、当前 Floor Map Screen、正式制作基线与隔离 Debug builder 已落地`
  - 归属：Map / Run / App
  - 入口：[WacomMap.md](./WacomMap.md) / [Roadmap: 地图与探索](./Roadmap.md#roadmap-map) / [Questions: Run、探索与地图](./Questions.md#questions-run-map)
	- 说明：Logical Map Graph、Action Point、节点生命周期与类型、同层 Map Travel、不可逆 Floor Transition、Floor Exposure、Camp Action / Camp Activity、节点内容原子结算、原子初始化、Run Path Traversal、当前 Floor Map Screen、Descriptor-first 原子场景绑定、只读 Validator、Authoring baseline、Debug-only builder 与 Journey 成功结算/主菜单交接已经落地；正式三层各 20 节点/21 边图、`26–28 / 44–46 AP` 总节奏、蛇印/蜕印稳定身份和独立持久 Credential 均已冻结，Floor 1 的 46 个 Production DataAsset 与独立 Floor/map/Host 灰盒已创建并审计。每日预算保持 `2 / 6 / 2 / 2 / 1`；Dusk 保留 Picnic，Night 选择 Camp / Night Exploration。下一步是完整 Production Journey、Floor 2/3 内容与场景、跨层 world transition、失败总结交接和 Camp handler/UI，不恢复已删除的原型轨道路径。
	- 2026-07-15 地图闭环：单合法出口 W 自动前进，多合法出口使用道路入口选择，Anchored 无需点击 Actor 即可持续 cursor look；M / 手柄 View 打开当前 Floor 地图，死胡同默认聚焦最近的已完成节点，并可免费同层返回。

- [ ] **Run Map 后续页面与交互扩展**
	- 状态：`Backlog`
	- 归属：Map / UI
	- 说明：旧 Floor 只读历史页签、地图缩放/拖拽、多 Floor Transition 页面和更复杂的迷雾表现后续独立规划；首版当前 Floor Screen 不预留隐藏按钮或半实现状态。

- [x] **正式 Floor 1 内容设计与稳定身份冻结**
  - 状态：`Done: 三层身份、20 Node/21 Edge、内容槽、AP 与生产门禁已冻结`
  - 归属：Map / Run / Data / App
  - 入口：[WacomMap.md](./WacomMap.md) / `specs/007-formal-floor1-content-freeze/`
  - 说明：正式身份为 `Journey.Main.01` 与 `Floor.Main.01/02/03`；Floor 1 冻结 20 Node/21 Edge、`4/6/4/4/1/1` 类型配比、15 个 Production 内容槽、四处 Camp 落点、`8–9 AP` 最短推进和 `14–15 AP` 完整探索。Authoring/Debug 资产保持独立；46 个静态 Production 内容 DataAsset 与独立 `DA_Floor_Main_01 / L_Run_Floor_Main_01` 灰盒已创建，Production Journey 与 Floor 2/3 资产尚未创建。蛇印软锁已由独立 Credential 解除；Floor 2/3 图后续由 Spec 009 完成。

- [x] **Floor 1 Production 内容合同冻结**
  - 状态：`Done: 15 个节点 Definition + 23 个支持资产合同已冻结`
  - 归属：Data / Battle / Run / Map
  - 入口：[WacomData.md](./WacomData.md) / [WacomDataAuthoring.md](./WacomDataAuthoring.md) / `specs/011-formal-floor1-production-content-freeze/`
  - 说明：已冻结并播种 4 Enemy、11 Part、4 Behavior、24 Intent、6 Encounter、4 核心 Card、4 Pickup、1 Shop 和 4 Event/13 Choice；战斗梯度为 `16 → 26–32 → 44 → 52 HP`，两条前半路线各有从 0 Gold 获得一次购买力的选择，Floor AP 保持 `8–9 / 14–15`。Spec 012/013 的八张击倒奖励卡和十一 Part 映射也已随 Spec 014 落地；世界权威、美术/平衡与正式 PIE 继续独立处理。

## P1 近期实现候选

- [x] **蛇印任务凭证保留 / 恢复正式规则**
  - 状态：`Done: 独立持久 Credential、原子 Pickup、入口已落地；Credential 自 v4 起持久化，当前 schema 为 v5`
  - 归属：Run / Data / Tests
  - 入口：[WacomRun.md](./WacomRun.md) / [WacomMap.md](./WacomMap.md) / `specs/008-run-credential/`
  - 说明：正式资格为 `Credential.Run.SerpentSigil`；蛇印 Pickup 与表现卡在一个 working-state 事务中授予资格，删牌不影响 Credential，FloorEntrance 非消耗检查 Credential。Credential 在 SaveGame v4 引入确定排序持久化，当前 v5 保留该合同；v3 明确迁移为空且不从卡牌反推。实现为通用数据合同，没有 Floor 1 运行时特例。

- [x] **正式 Floor 2/3 图与 Journey 总时长冻结**
  - 状态：`Done: 两层 20 Node/21 Edge、蜕印门槛、终局图与 5–6 天节奏已冻结`
  - 归属：Map / Run / Data
  - 入口：[WacomMap.md](./WacomMap.md) / `specs/009-formal-floor23-journey-pacing-freeze/`
  - 说明：Floor 2/3 各冻结 20 Node/21 Edge 双分岔图；Floor 2 使用 `Card.Run.MoltSeal` + `Credential.Run.MoltSeal` 进入 Floor 3，Floor 3 无 Shop/Entrance 并以无出边 Guardian 作为 Journey terminal design node。三层最短/完整为 `26–28 / 44–46 AP`，正常目标约 5–6 天。未创建 Production 资产。

- [x] **通用 Journey 成功结算与终局交接**
  - 状态：`Done: terminal Outcome/summary/event、Save v5、被动总结页与主菜单交接已落地`
  - 归属：Run / App / Save / UI / Tests
  - 入口：[WacomRun.md](./WacomRun.md) / [WacomMap.md](./WacomMap.md) / [WacomApp.md](./WacomApp.md) / `specs/010-journey-success-settlement-baseline/`
  - 说明：Production Journey 必须声明合法 `SuccessTerminalNode`；terminal 非撤离 Victory 原子提交 `Outcome=Succeeded`、成功摘要和末尾 `JourneySucceeded` event。SaveGame v5 可持久化 Outcome/摘要但只允许 InProgress 恢复；GameMode 在 Return-to-Run staging 后显示原生 fallback 总结页，确认/Back 或 push failure 都 teardown 后次帧返回 `L_MainMenu`。没有复用 Defeat、Actor label、EncounterId、伪 FloorEntrance 或 Level Blueprint 特例。

- [ ] **Defeat / 压力满 / 手指耗尽的统一总结交接**
  - 状态：`Ready for design`
  - 归属：Run / App / Save / UI / Tests
  - 入口：[WacomRun.md](./WacomRun.md) / [WacomApp.md](./WacomApp.md)
  - 说明：复用 JourneySummary 的 passive Screen/handoff 方向，另案冻结失败原因、摘要字段、事件时点、是否保留战斗结果细节和返回主菜单口径。不得把成功事件或成功文案反向复用于失败。

- [ ] **Floor 2/3 Production 内容 Definition 制作**
  - 状态：`Ready for design: 31 个内容槽与命名已冻结`
  - 归属：Data / Editor / Tests
  - 入口：[WacomDataAuthoring.md](./WacomDataAuthoring.md) / `specs/009-formal-floor23-journey-pacing-freeze/data-model.md`
  - 说明：制作 `MoltCavern` 的 15 个与 `VenomCore` 的 16 个非 Debug typed definitions，独立冻结敌人槽、事件选项、Shop 库存和奖励数值并补 Data Validation。蜕印 Pickup 必须同时授予 `Card.Run.MoltSeal` 与 `Credential.Run.MoltSeal`。

- [x] **Floor 1 Production 38 core + 8 branch reward DataAsset 实现与校验**
  - 状态：`Done: 46 个 package 已 seed-only 创建并完成真实资产、AssetRegistry、哈希与幂等审计`
  - 归属：Data / Editor / Tests
  - 入口：[WacomDataAuthoring.md](./WacomDataAuthoring.md) / `specs/014-formal-floor1-production-assets/`
  - 说明：Spec 014 以 exact manifest 创建 12 Card、4 Behavior、11 Part、4 Enemy、6 Encounter、4 Event、4 Pickup 与 1 Shop；三张 Starter 卡和现有 `PoisonFang` 保持只读且哈希不变。46/46 真实加载、strict/structural validation、AssetRegistry class/count、forbidden reference、双跑幂等与 LFS 审计通过；没有修改地图、材质、卡牌表现、背包或其它 Agent 资产。

- [x] **Floor 1 Production Floor / Map / Host 灰盒基线**
  - 状态：`Done: 新建独立 Floor/map、四个敌人 Host、16 个内容 Host 与 Exit marker，并完成真实资产/场景/幂等审计`
  - 归属：Map / Data / App / Editor / Tests
  - 入口：[WacomMap.md](./WacomMap.md) / [WacomDataAuthoring.md](./WacomDataAuthoring.md) / `specs/015-formal-floor1-production-scene-baseline/`
  - 说明：已选择新建 `/Game/Wacom/Maps/Run/L_Run_Floor_Main_01`，未迁移或修改 `L_Exploration`。Exact 7-package seeder 创建 `DA_Floor_Main_01`、4 个可复用 SerpentWood 灰盒 Enemy Host、非交互 Exit marker 和 20/21 场景；已有资产只读，第二次运行 `0 created / 0 saved`。完整 Journey、Floor 2/3、跨层 travel、正式美术和 Golden Path PIE 仍是后续任务。

- [x] **Floor 1 直接关卡 Preview 启动层**
  - 状态：`Done: PIE-only transient Journey、双 Package bootstrap、自动化与用户 Golden Path PIE 已通过`
  - 归属：App / Editor / Tests / Map
  - 入口：[WacomApp.md](./WacomApp.md) / [WacomDataAuthoring.md](./WacomDataAuthoring.md) / `specs/016-formal-floor1-preview-bootstrap/`
  - 说明：只支持在 Editor 中直接打开 `L_Run_Floor_Main_01` PIE。基础 GameMode Journey 解析语义保持不变；Preview GameMode 从唯一 Descriptor 构造 transient 单层 Journey，双 Package bootstrap 只创建 Preview BP 并设置 map override/Entry PlayerStart。用户已验收 Entry、镜头/W/S、HUD/手牌、M/ESC 点击、路径、战斗与内容交互返回、Exit dead end；主菜单仍进入 `L_Exploration`。Preview 不提供跨层、成功总结、存档或发行启动，完整 Production Journey 落地后必须移除。

- [ ] **Production Journey 与 Floor 2/3 场景/跨层交接**
  - 状态：`Blocked: Floor 2/3 definitions/maps and FloorId-to-world handoff are absent`
  - 归属：Map / Data / App / Editor / Tests
  - 入口：[WacomMap.md](./WacomMap.md) / `specs/009-formal-floor23-journey-pacing-freeze/`
  - 说明：创建 `DA_Journey_Main_01`、Floor 2/3 Production DataAsset 与 world，配置 SuccessTerminalNode，并实现由 App flow 消费 Floor transition result 的 FloorId-to-world handoff。不得在 Floor 1 Exit marker 或 Level Blueprint 中硬编码 travel。

- [ ] **正式资产依赖审计与 `/Game/Wacom` 迁移**
  - 状态：`In Progress: JSON v2 + Placeholder 发布门槛已完成，外部依赖仍待分批迁移`
  - 归属：Content / Editor / Build
  - 入口：[Content Dependency Audit](./Content_Dependency_Audit.md) / [Worktree Development](./Worktree_Development.md) / [Content Organization](./Content_Organization.md)
  - 说明：`WacomAuditContentDependencies` 已形成稳定 JSON v2 contract，显式列出 `/Game/Wacom/Art/Placeholders` 并提供发布用 `-FailOnPlaceholder`。2026-07-16 外部依赖基线仍集中在 `/Game/Art`、`/Game/Asset` 和 `/Game/DreamMaterials`；Snake 的 9 个 Slime Placeholder 已进入受控 `/Game/Wacom` 路径，但只获开发占位授权且会阻止发布。下一步先确认 `Card_Luo`、`Plane`、`Chong_CardAsset` 及音频/字体/UI/Boar 样例的来源与授权，再按 A–F 批次迁移；正式 Snake 美术到位后切换 Host、删除 Placeholder 并让 release gate 通过。

- [x] **背包视觉制作与四分辨率适配**
  - 状态：`Done: formal/fallback assets, responsive matrix and final PIE complete`
  - 归属：App / UI / Editor / Tests
  - 入口：`specs/005-backpack-visual-production/`
  - 说明：该轮完成全局 `1920×1080` DPI、Root/Main/Body/Workspace/Overlay Fill、Header Auto、固定卡面缩放和静态 Retainer，并通过四分辨率、16:10、超宽及高卡量验收。2026-07-16 后续内嵌牌堆改造已删除旧右侧区域栏，但继续沿用本轮的分辨率、卡面与 Retainer 合同。

- [ ] **背包真实卡牌牌堆与携带性能改造**
  - 状态：`Implementation and automated/Builder verification complete; final PIE/Insights pending`
  - 归属：App / UI / Editor / Tests
  - 入口：[Backpack WBP 制作与绑定合同](./UI_Backpack_WBP_Binding.md) / [Wacom UI](./WacomUI.md)
	- 说明：通量区保持统一自由工作台；备战、特殊和负重牌堆已迁移为折叠/展开共用的全部真实卡面。正式链路删除 Preview class/ViewData/Builder 依赖，新增 `WBP_BackpackZonePile` 与 Workspace 分层，DeckCard 改承载 `WBP_FPCardView`。携带路径使用单一 `CarryRoot` 锚点：静止 `CarryCache` 缓存非当前卡，独立 `CarryActiveLayer` 只承载当前实时卡；鼠标移动不重算牌列、不全量刷新静态卡。跨区提交按 `InstanceId` 原位迁移同一实体 Widget，不再因 `PhysicalZone` 变化创建替代实例。2026-07-17 又完成内部收口：`FWacomBackpackWorkspaceSceneBuilder` 一次生成顺序对齐的卡牌、牌堆、布局与命中合同；`FWacomBackpackWorkspaceVisualRegistry` 以四个实际 Canvas 层为所有权真相并线性重建身份索引，统一清除重复卡与幽灵牌框；`FWacomBackpackWorkspaceRuntime` 作为单一 App-private 所有权根持有 Registry 和 Motion Coordinator。Carry、PileMove、携带目标悬停展开、局部姿态、Settlement、焦点退出和牌堆基础布局过渡共用一个按需帧 `ActiveTimer`，收拢完成由实际过渡结束触发。1945 行 Screen Spec 已拆为 CardView、EffectBadge、CardDetail 与 Screen Refresh/Projection 四个专题，FocusWindowStrip、Scene Builder、Registry 与 Motion Coordinator 契约由小型测试覆盖。自动化 379 项通过，Builder 双跑及六个正式资产 Hash 稳定；用户此前连续 10 次 PIE 验证通过，完成本轮最终 1/21 张携带 / Insights 观察后更新为 Done。对应阶段工件见 `specs/006-backpack-real-card-piles/`。

- [x] **背包 Workspace 重构：正式实现、旧路径迁移与统一 PIE 验收完成**
  - 状态：`Done: C++ + formal WBP + DreamShader + legacy cleanup + final PIE complete`
  - 归属：Run / App / UI / Tests
  - 入口：`specs/003-backpack-workspace-refactor/`
  - 说明：`specs/003-backpack-workspace-refactor/` 的 T001–T076 已全部完成，建立了 Workspace、同 Run 瞬态布局、框选与持续扇形携带、分层 Back、原子批量移动/销毁、确认恢复、正式 WBP/Style、静态 Retainer 卡面与 Wacom-native DreamShader 反馈。该轮的“单活动区 + 常驻右侧区域栏”表现结构已在 2026-07-16 内嵌牌堆改造中替换；规则事务与输入合同继续复用。
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

- [x] **Scene Enemy UI：等宽分段生命条与双侧详情**
  - 状态：`Done: C++ contract / WBP / lifecycle / automation`
  - 归属：App / UI / Editor
  - 入口：[WacomBattleUI.md](./WacomBattleUI.md#6-scene-enemy-ui) / [UI_Battle_WBP_Binding.md](./UI_Battle_WBP_Binding.md#enemy-panel-wbp)
  - 说明：单/多部位敌人共用 Definition 顺序的 HP segment、Shield overlay 和三枚 Buff +N；详情面板复用同一 ViewData，并由稳定 Part identity、Idle input gate 与 BattleHUD 私有 coordinator 管理。后续只保留 Boss 专用面板、Intent effects 公开说明和视觉 polish，不建立第二套敌人状态数据链。

- [ ] **战斗表现队列：快速连出后的视觉追赶 polish**
  - 状态：`Ready: 体验 polish`
  - 归属：App / UI
  - 入口：[WacomBattleUI.md](./WacomBattleUI.md) / [Roadmap: 战斗 UI](./Roadmap.md#roadmap-battle-ui)
  - 说明：Queue Advance、Presentation Plan Poll 与 Stack Entry Exit 的 TimerHandle / teardown 所有权已收敛到 App-private keyed timer owner，raw delegate 与无 World 析构访问已清除；后续只追踪表现体验，如 cue 合并、速度压缩、正式动画、stack 入场 polish、动画回放或规则层 command batch id。

- [ ] **First-person hand 动效第二阶段：世界目标命中反馈与整体 PIE polish**
  - 状态：`In Progress: 手牌目标像素刻印已接线，待世界目标与 PIE polish`
  - 归属：App / First-person Card Layer
  - 入口：[First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md) / [WacomBattleUI.md](./WacomBattleUI.md)
  - 说明：基础合同已完成显式空帧替换、Motion Mixer / 互斥 Transition Playback、真实 draw/discard/play anchors、Card Depth、单 Retainer 实时 Alpha 接触阴影、Drag Pickup、普通 Played 像素翻面收牌、Exhausted 消耗消散、成功 HandCard 目标刻印，以及 EndTurn `Sealing → Held → Releasing` 保留牌封存。所有 Battle PlayCard 已收口为 `SourceOut → PrimaryTarget → Outcome → SourceReturn` 命令编排；费用与 EffectBadge Preview/Rewrite 分别直接作用于现有局部数字 Image，命令 Outcome 可并行等待两者完成。EffectBadge 已按 Damage/Poison/Heal/Shield 等语义类型聚合稳定 Key，具备可逆 Preview、整组 skipped 低亮和 ValueChanged/Added/Removed Playback；当前正式触发只连接明确许可事件下的可见 ValueChanged。Battle/Run 来源级 Soft Sprite 与 Enter Sound 异步预热、局部 MID 提前建立和真实 Paint Ready Gate 已完成；下一步继续做整体 PIE 调参、Win64 Shipping PSO 冷启动、reduced-motion / 全局速度策略，以及等正式动态 Effect 事务落地后接入 Added/Removed。耐久、卡名、插画、稀有度与 Run 升级仍需各自权威变化事实。不复制第三方代码、shader 或资产。

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
  - 状态：`In Progress: Simple / Multi-Part 通用行动层已完成，剩余正式美术与高级表现`
  - 归属：App / UI / Battle World Target
  - 入口：[WacomWorldInteraction.md](./WacomWorldInteraction.md) / [WacomBattleUI.md](./WacomBattleUI.md)
  - 说明：TrainingWarrior 已验证正式 Simple Host 与语义动画；通用 Multi-Part Part Action 已按完整稳定身份、精确 Layer、完成 barrier 和 watchdog 接线。`BP_EnemyHost_Snake` 与开发敌人 `BP_EnemyHost_SlimeTrio` 当前都使用会阻止发布的独立 Slime Placeholder，因缺少正式行动素材而故意不配置 Style。后续优先制作正式 Snake 和 SlimeTrio 分段 Idle / Action 素材，分别迁到 `/Game/Wacom/Art/Enemies/<Pack>` 并补显式 Intent Style；材质描边、贴近部位 tooltip、风险动效、PaperZD/Animator 和 Status Badge 美术替换继续独立切片。

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

- [x] **Multi-Part Enemy Part 语义行动动画层**
  - 状态：`Done: 完整 Part key 路由 + 原地 Flipbook barrier`
  - 归属：App / Battle World Presentation
  - 入口：[WacomBattleUI.md](./WacomBattleUI.md) / [WacomWorldInteraction.md](./WacomWorldInteraction.md) / [WacomDataAuthoring.md](./WacomDataAuthoring.md)
  - 说明：`EnemyPartActed Count > 0` 现在从 queue 到 scene coordinator 保留完整 Part key；Simple Host 继续播放 Host Style，Multi-Part 精确播放匹配 PartActor 的一个目标 Flipbook Layer。Host / Part 已共用 App-private playback 生命周期，统一弱 timer、serial、Impact / Complete exactly-once、watchdog 与 cancel 语义；Destroyed 抢占、BattleEnd retiring Part 与重新入战恢复已覆盖。正式 Snake Style 等待正式行动素材后再制作。

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

- [ ] **接入击倒事件其它分支效果与节点事件联动**
  - 状态：`Blocked: 非卡牌效果与触发条件仍需 P0 口径`
  - 归属：战斗 / RunEvent
  - 入口：[Roadmap: 击倒事件扩展](./Roadmap.md#roadmap-knockdown)
  - 说明：Aid/Destroy 差异化奖励字段、原子授予和简单预览已完成；本项只追踪左手 buff、永久强化/破坏、特殊节点或地图回路等未冻结能力。

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
  - 说明：该轮完成正式 Screen、Workspace、DeleteConfirm、Style、反馈材质和旧单卡 UMG drag/drop owner 清理；当时的右侧区域控件已由 2026-07-16 内嵌牌堆改造正式删除。20–100 卡 PIE 未观察到明显空闲 Tick 或逐帧 Snapshot 重建，因此仍不提前实施虚拟列表。

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
