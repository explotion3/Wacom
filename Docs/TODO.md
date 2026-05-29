---
type: task-index
scope: wacom-short-term
status: active
updated: 2026-05-29
tags:
  - wacom/todo
  - wacom/docs
---

# TODO 索引

> [!info] 本文职责
> 这里只放短期任务入口和跳转，不写规则真相。当前规则看领域文档；长期方向看 [[Roadmap]]；临时写法看 [[TechDebt]]；待确认问题看 [[Questions]]。

> [!warning] 执行约束
> 标记为 `Blocked` 的任务不要直接写死设计口径。先在 [[Questions]] 或对应领域文档中收口，再进入实现。

## 文档分工

| 文档 | 职责 | 维护规则 |
|---|---|---|
| [[Roadmap]] | 未实现功能、后续方向、可按阶段推进的内容扩展 | 新功能还没有进入短期实现前，先放这里 |
| [[TechDebt]] | 临时写法、兼容字段、临时决定、正式替代方案 | 代码里出现 `TODO(技术债)` 时同步到这里 |
| [[Questions]] | 会影响规则、策划口径或长期架构的待确认问题 | 不在代码里静默写死这些问题 |
| 本文 | 短期任务索引和跳转 | 不承载长说明，避免重新变成大杂烩 |

## 当前推进顺序

1. 先收口 P0 规则问题。
2. 再推进地图 / 节点服务。
3. 地图口径确定后，再接击倒分支、RunEvent / Shop 的节点生成与存档恢复。
4. UI WBP、表现 polish 和存档恢复按 Demo 范围穿插推进。

## P0 策划确认

- [ ] **确认减速、暮气、冻结等状态的数值公式和触发时机**
  - 状态：`Blocked: 策划确认`
  - 归属：战斗规则
  - 入口：[Questions: 状态与触发时机](./Questions.md#questions-status)

- [ ] **明确击倒事件 Aid / Withdraw / Destroy 的正式分支效果**
  - 状态：`Blocked: 策划确认`
  - 归属：战斗 / Run
  - 入口：[Questions: 击倒与战后结算](./Questions.md#questions-knockdown)

## P1 近期实现候选

- [ ] **接入击倒事件实际分支、奖励卡差异化和节点事件联动**
  - 状态：`Blocked: P0 击倒口径`
  - 归属：战斗 / RunEvent
  - 入口：[Roadmap: 击倒事件扩展](./Roadmap.md#roadmap-knockdown)

- [ ] **推进地图系统：节点、通道、迷雾、撤离回路、地图状态**
  - 状态：`Ready: 需先定模块边界`
  - 归属：Run / 地图
  - 入口：[Roadmap: 地图与探索](./Roadmap.md#roadmap-map)

- [ ] **推进 RunEvent：随机事件池、更多条件效果、地图节点生成、存档**
  - 状态：`Blocked: 地图节点口径`
  - 归属：Run / Data / App
  - 入口：[Roadmap: 探索事件](./Roadmap.md#roadmap-runevent)

- [ ] **商店正式化：随机商品池、价格公式、正式 WBP、存档接入**
  - 状态：`Blocked: 地图节点 / 存档口径`
  - 归属：Run / Data / App
  - 入口：[Roadmap: 商店](./Roadmap.md#roadmap-shop)

- [ ] **背包正式 WBP、拖拽 polish、必要时做增量刷新**
  - 状态：`Ready: 美术 / WBP 工作`
  - 归属：UI / Run
  - 入口：[Roadmap: 背包 UI](./Roadmap.md#roadmap-backpack-ui)

## P2 后续清理

- [x] **用编辑器迁移旧内容目录并清理 `_GAME` 临时蓝图目录**
  - 状态：`Done: Content Organization V1-A`
  - 归属：Content / App / Data
  - 入口：[Content Organization](./Content_Organization.md)

- [ ] **战斗手牌表现升级：扇形布局、拖拽出牌、3D 目标选择**
  - 状态：`Blocked: HD-2D 表现方案`
  - 归属：UI / 战斗表现
  - 入口：[Roadmap: 战斗 UI](./Roadmap.md#roadmap-battle-ui)

- [x] **First-person card layer polish：详情面板跟随卡牌与层级修正**
  - 状态：`Done: V0-H 已接入独立 viewport popup host；V0-I 已用 BattleHandPresentationMode 收口第一人称手牌配置入口；V0-V 已加入共享详情读牌 motion`
  - 归属：UI / 战斗表现
  - 说明：旧 `UHandPanel` 详情继续使用 BattleHUD 内部 `CardDetailLayer`；first-person hover 详情使用独立 viewport panel。两种战斗详情 host 共用 `bEnableCardDetailReadabilityPolish` 下的短 hover delay、淡入淡出、轻量 scale、位置平滑跟随和贴边 side hysteresis；TargetSelect、命令提交、BattleEnd、Session 切换和 runtime hand 清理仍强制隐藏详情。战斗手牌入口由 `LegacyHandPanel / FirstPersonHandWithLegacyFallback / FirstPersonHandOnly` 三种模式控制。

- [ ] **First-person card render quality：扇形布局微调与专用卡面规范**
  - 状态：`In Progress: V0-K projected basis 已回退；V0-L 默认改为 Authored2D hand layout solver；V0-M 默认允许手牌锚点 SoftClamp 离屏；V0-N 已加入 hand center screen smoothing；V0-O 已加入 card slot motion polish；V0-P 已加入 slot motion lifecycle diagnostics / self-repair；V0-Q 已加入 event-aware card transitions；V0-R 已加入 pending / TargetSelect focus polish；V0-S 已加入 playable / hover / press feedback polish；V0-T 已加入 hand layout preset / tuning profile；V0-U 已加入 transition origin / card movement readability；V0-V 已加入 hover detail / inspect readability polish；V0-W 已加入 play commit / target impact readability`
  - 归属：UI / 战斗表现
  - 说明：继续保留 `WBP_FirstPersonCardView` 的 RetainerBox、透明留白、内部缩放和贴图采样规范。V0-K 的 projected card basis / 动态 projected scale 在 PIE 中带来像素拉伸，已回退，不作为当前主线。V0-L 改为只投影手牌中心，再用 `AuthoredCardSpacingPixels / AuthoredMaxHandWidthPixels / StaticCardEdgeDropPixels / FanYawDegrees` 等 2D 参数排布卡牌。V0-M 增加 `ViewportClampMode`，默认 `SoftClampToViewport`，允许手牌中心部分离屏后柔性拉回；`HardClampToViewport / AllowOffscreen` 用于 PIE 对照。V0-N 给 anchor tick 增加 RunTunnel / BattleCamera prerequisite，并只对 `Authored2D` 的整副手牌中心做 screen smoothing，保留前进下降、后退上升的空间反馈，减少移动时高频上下抖动。V0-O 在 slot 层按 `CardInstanceId` / static index 复用 widget，平滑位置、角度、缩放和透明度，新卡淡入、移除或投影失败的可见卡短暂淡出，详情跟随动画中的 visual slot。V0-P 增加 `FWacomFirstPersonCardLayerMotionDebugView`，统计 active / outgoing / RootCanvas child / ticking slot，并修复 active/outgoing 重叠、重复 key、未追踪 child 和 outgoing 超限；反复获得毒牙等即时加手牌事件时应不再出现幽灵 slot 或 FPS 阶梯式下降。V0-Q 让 BattleEvent 只作为表现 hint：抽牌从下方进，击倒获得毒牙等 `CardGained` 从上方进，出牌向上离开，手牌上限弃置向下离开；snapshot diff 仍是手牌变化真相。V0-U 在此基础上增加 transition origin profile：Drawn / Gained 默认从 hand anchor 加偏移进入，Played / Discarded 默认保留 slot offset 离开，也可切到 DPI-aware widget-space 的 `ViewportAnchor` 来模拟屏幕边缘、牌堆或战斗空间方向；scale / angle accent 只影响 visual 起点或 outgoing 终点。V0-R 让 TargetSelect 中 pending 卡上浮 / 放大 / 提层 / 轻微归正，并让其他手牌轻微降透明；pending 卡 hover 不再额外叠加 lift / scale / ZOrder。V0-S 让可打 hover、按下、有效释放确认和不可打点击拒绝都有轻量 C++ 反馈，不要求改 WBP 卡面，也不改变 BattleHUD 命令路径。V0-T 增加 `UWacomFirstPersonCardLayoutPreset`，把 layout、projection/clamp、smoothing、slot motion、transition origin、event transition、hover/pending/disabled 和 interaction feedback 等表现数值收进 `WacomApp` DataAsset；组件参数继续作为 fallback，preset 不覆盖卡面 WBP、debug、ZOrder 或 BattleHUD 呈现模式。V0-W 新增成功出牌后的 first-person commit pulse，并在目标卡成功提交时发送 `TargetConfirmed` cue 给 2D/场景目标；确认 cue 只表示点击被接受，后续伤害 / 破坏仍由 `DamageDealt / EnemyPartHpEmptied` 表现队列负责。本轮新增 commit 数值暂不维护到 preset，直接在 `FirstPersonCardAnchorComponent` 上配置。后续只有在美术反馈需要时再微调扇形参数、下坠、层级、hover/pending 姿态、slot motion 速度、commit pulse 或角度 clamp；也可单独做多环境 preset / 战斗状态自动切 preset。不要为了抗锯齿牺牲当前手牌排布表现，也不要回退到 3D Presenter / RenderTarget 路线。

- [ ] **存档系统恢复：Bootstrap 读盘、PauseMenu Save、MainMenu Continue**
  - 状态：`Blocked: Demo 范围确认`
  - 归属：Run / App
  - 入口：[Roadmap: 存档恢复](./Roadmap.md#roadmap-save)

- [ ] **清理 UI MVVM 迁移尾项：WBP ViewBinding、逐步移除 C++ 手动 SetText fallback**
  - 状态：`Ready: WBP 化后清理`
  - 归属：UI 架构
  - 入口：[TechDebt: UI 架构债](./TechDebt.md#techdebt-ui-architecture)

## 维护规则

> [!tip] 新任务放哪
> - 1-2 轮内准备做：放本文。
> - 只是未来方向：放 [[Roadmap]]。
> - 当前实现里已经存在的临时方案：放 [[TechDebt]]。
> - 需要你或策划确认口径：放 [[Questions]]。

> [!note] 迁移说明
> 原 `TODO.md` 的未实现功能已迁入 [[Roadmap]]；临时写法、临时决定、兼容入口已迁入 [[TechDebt]]；待确认规则问题已迁入 [[Questions]]。已经正式化到 `WacomRun.md`、`WacomBattle.md`、`WacomApp.md`、`WacomUI.md`、`WacomData.md` 的实现，不再在这里重复追踪。
