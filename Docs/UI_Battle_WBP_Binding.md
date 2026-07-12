---
type: ui-binding-contract
scope: wacom-ui-battle
status: active
updated: 2026-07-11
tags:
  - wacom/ui
  - wacom/wbp
  - wacom/battle
  - wacom/contract
---

# 战斗 UI WBP 绑定清单

> [!info] 本文职责
> 本文只记录 Battle WBP 制作合约：父类、推荐资产路径、绑定槽位、WBP 不应承担的职责和最小 PIE smoke 检查。

> [!warning] 合同边界
> 本文不定义战斗规则、BattleHUD 数据流、world target 系统或 first-person hand 运行时行为。规则见 [WacomBattle.md](./WacomBattle.md)，Battle UI 行为见 [WacomBattleUI.md](./WacomBattleUI.md)，场景目标见 [WacomWorldInteraction.md](./WacomWorldInteraction.md)，第一人称卡牌层见 [First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)。

## 制作原则

- WBP 只做显示、布局、动画和轻量表现 hook；玩家命令只回传给 `UBattleHUD` 或对应父类入口。
- WBP 不直接调用或缓存 `UBattleSession`，不消费或修改 `BattleState`，不自行解析 `FBattleEvent` 作为规则状态；`UWacomBattleWidgetBase.SetSession / GetSession` 不再暴露给 Blueprint，只保留为旧 C++ 兼容 wrapper。正式 C++ owner 注入使用 `SetInjectedBattleSession / GetInjectedBattleSession`。
- `BindWidgetOptional` 缺失不会崩溃，但对应区域不会显示或刷新；required binding 缺失会导致父类构造失败或控件不可用。
- 正式 BattleHUD 新制作应使用 `CombatLogFeed + BattleCombatLogBlock`；旧 `EventLogPanel / EventToast` 已删除，不再作为主 HUD 绑定。
- Scene enemy authoring、PartActor debug summary 和 target handle 细节只在 [WacomWorldInteraction.md](./WacomWorldInteraction.md) 维护。

## WBP_BattleHUD

父类：`UBattleHUD`

推荐资产路径：按项目 UI Settings 或 BattleHUD class 配置；完整 WBP 应绑定下列主要槽位。

推荐绑定：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `PlayerStatusBar` | `UPlayerStatusBar` | Optional | 玩家 HP / Shield / 状态图标显示 |
| `CommandBar` | `UBattleCommandBarWidget` | Optional | Wait / EndTurn 命令按钮、等待值和 pending 文案 |
| `DrawPileView` | `UPileCountView` | Optional | 抽牌堆数量 |
| `DiscardPileView` | `UPileCountView` | Optional | 弃牌堆数量；当本回合使用牌堆非空时显示为 `弃牌堆数+本回合使用数`，例如 `2+3` |
| `ExhaustPileView` | `UPileCountView` | Optional | 消耗牌堆数量 |
| `DrawPileMotionAnchor` | `UWidget`，推荐 `SizeBox` | Optional | `Drawn` 卡牌的完整逻辑起点；缺失或几何无效时回退 `DrawPileView` 中心 |
| `DiscardPileMotionAnchor` | `UWidget`，推荐 `SizeBox` | Optional | `Discarded` 卡牌的完整逻辑终点；缺失或几何无效时回退 `DiscardPileView` 中心 |
| `PlayTargetMotionAnchor` | `UWidget`，推荐 `SizeBox` | Optional | 无真实目标 Played 的完整逻辑终点；真实目标坐标优先，缺失时回退旧 Played origin |
| `CombatLogFeed` | `UBattleCombatLogFeedWidget` | Optional | 正式常驻玩家战斗记录 |
| `BattlePresentationStack` | `UBattlePresentationStackWidget` | Optional | 已提交卡牌的只读表现 backlog |

WBP 不应做：

- 不绑定或调用旧 `EventLogPanel / EventToast`；这些旧类已删除。
- 不绑定旧 `HandPanel` 或 `CardDetailLayer` 作为 BattleHUD runtime 路径。
- 不直接 Push 击倒弹窗、直接消费 `FBattleEvent`、提交 Battle 规则命令或维护表现队列。
- 不把 `BattlePresentationStack` 做成可点击、可拖拽或规则栈。
- 不绑定敌方 2D fallback widget。正式场景敌人走 `SceneEnemyHostSlots + AWacomBattleEnemyActor` prefab，并由 Host 蓝图/子 Actor 明确声明 PartActor。配置 `EncounterDefinition` 的 Trigger 必须执行 `SyncSceneEnemyHostSlotsFromEncounter()` 并用 `SceneEnemyHostSlots` 覆盖每个 EnemySlotId。
- 不读取或假设 `Snapshot.Enemy`。敌人快照只在 `FBattleSnapshot.Enemies` 中，BattleHUD C++ 会把目标选择、日志和场景 bridge 同步到所有 enemy slot；WBP 不应自行维护第一敌人的兼容显示。

最小 PIE 验收：

- 玩家状态、牌堆数量、CommandBar 和 CombatLogFeed 在 Snapshot 刷新后显示。
- 拖牌指向合法敌人部位 / 手牌目标，或无目标卡已经达到 armed commit 可释放状态时，玩家状态条和敌人部位面板可以直接显示 Action Preview projected value；单纯拖出手牌区但未 armed、未指向有效目标或目标无效时不显示玩家侧收益预览。
- `CombatLogFeed` 可滚动，连续出牌后能查看最近命令块。
- `BattlePresentationStack` 只显示小卡表现，不响应输入。
- 抽牌从 `DrawPileMotionAnchor`（或 `DrawPileView` 中心）进入；弃牌飞向 `DiscardPileMotionAnchor`（或 `DiscardPileView` 中心）。配置有效 Played Dissolve 时，无目标牌与目标牌都停在提交位置原地消散；`PlayTargetMotionAnchor` 和真实目标坐标仍会采集，但只供消散失效时的旧空间离场 fallback 与未来目标命中反馈使用。
- 有 `SceneEnemyHostSlots` 的战斗通过 Host prefab 扫描到的 PartActor Status Badge 阅读敌方状态；缺 Host 时没有 2D 敌方 fallback，且 `EncounterDefinition` 正式入口会被编辑器验证判为 invalid。

当前 `FBattleSnapshot.PileCounts` 额外公开 `PlayedCount`（本回合使用牌堆数量）。本轮 WBP 合同不要求新增 `PlayedPileView`，正式 HUD 仍只绑定并显示抽牌堆、弃牌堆和消耗牌堆三项；`UBattleHUD` 会把 `DiscardCount` 与 `PlayedCount` 合并显示在 `DiscardPileView` 上，`PlayedCount > 0` 时显示为类似 `2+3` 的复合数量。

BattleHUD 战斗手牌由 first-person card layer 提供，不再通过 WBP_BattleHUD 绑定 `UHandPanel`。战斗卡牌详情由 BattleHUD 创建 viewport-level `UWacomCardDetailPanel`，不再需要 BattleHUD WBP 提供 `CardDetailLayer`。详情面板与背包共用 `FWacomCardDetailViewData.Sections` 和 `WBP_CardDetailPanel / WBP_CardDetailSection` 制作合同：Builder 生成语义 `Blocks / Runs`，Panel 动态创建 Section，Section 通过 `UWacomCardDetailRichTextBlock` 渲染正文。WBP 不应从原始 `Description`、`Passive.DisplayText` 或 token kind 自行推断分区；`UWacomCardDetailPanel` 不再提供 `GetDescriptionText()`。

三个 MotionAnchor 推荐使用约 `8 x 8` 的无绘制 `SizeBox`，Visibility 设为 `HitTestInvisible`，不能设为 `Collapsed`。运行时读取缓存几何中心并转换成 DPI-aware 逻辑 viewport 坐标；MotionAnchor 不接收输入、不保存规则状态，也不需要 Blueprint Tick。移动 MotionAnchor 就是在调整完整起点 / 终点，C++ 不再在其上叠加旧位置 offset。当前 `BP_BattleHUD` 资产需要由 UI 制作者按上述准确命名添加可选控件；未添加期间 Draw / Discard 已通过现有 pile view 自动回退，PlayTarget 继续使用旧 Played origin。

## WBP_FPCardView

父类：`UWacomFirstPersonCardViewWidget`

推荐资产路径：`/Game/Wacom/UI/Card/WBP_FPCardView`

配置入口：`BP_WacomPlayerCharacter -> FirstPersonCardAnchorComponent -> FirstPersonCardViewClass`

用途：first-person card layer 的卡面 wrapper，服务静态预览和 BattleHUD runtime battle hand。它组合通用 `UWacomCardView` 与 first-person 专属反馈层，只显示 `FWacomCardViewData` 和交互反馈，不承接点击、hover 命令、目标选择或战斗规则。

推荐结构：

```text
WBP_FPCardView
└─ RootOverlay / Overlay
   └─ Fake3DSurfaceRetainer : RetainerBox
      └─ SurfaceOverlay / Overlay [Clipping = Clip To Bounds - Without Intersecting]
         ├─ CardView : UWacomCardView
         │  └─ BleedCanvas / SizeBox
         │     └─ CardSizeBox / SizeBox
         │        └─ 原 WBP_CardView 卡面内容
         ├─ FeedbackOverlay : Image
         └─ InteractionFeedbackImage : Image
```

关键绑定 / 命名：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `CardView` | `UWacomCardView` | Optional BindWidget | 通用卡面显示、`FWacomCardViewData` 刷新、主体命中几何来源 |
| `FeedbackOverlay` | `Image` | Optional BindWidget | playable hover / drag target / card target affordance 的 full-card overlay |
| `InteractionFeedbackImage` | `Image` | Optional BindWidget | pressed / confirm / commit / deny 的第一人称源卡交互反馈层；尺寸、层级和默认材质由 WBP 控制 |
| `Fake3DSurfaceRetainer` | `RetainerBox` | Optional BindWidget | 卡面唯一 Retainer；常态承载 fake-3D 与实时 Alpha 接触阴影，Played 消散活动期间由 C++ 临时切换专用 Surface-Effect MID |
| `SurfaceOverlay` | `Overlay` | Retainer direct content by convention | Retainer 完整离屏捕获根；覆盖完整 bleed 范围并重置继承自视口的普通 culling rect |
| `CardSizeBox` | `SizeBox` | `CardView` 内 Required by convention | 296 x 420 主体显示和交互参考范围 |
| `CostDigitImage` | `Image` | `CardView` 内 Optional | 单位费用数字图标 brush |
| `SurfaceFoilOverlay` | `Widget` | `CardView` 内 Optional | 复用 `UWacomCardView` 弱流光 / 表面装饰；未绑定时不会自动创建覆盖层 |

WBP 合同：

- `WBP_FPCardView` 外层只负责包装和反馈层；通用卡面内容应放在 `CardView` 子控件里。
- `FeedbackOverlay` 和 `InteractionFeedbackImage` 都由 WBP 控制尺寸、锚点和层级；C++ 只写颜色、透明度和材质参数。
- `WBP_FPCardView` 一张卡只允许一个 Retainer。需要把旧 `CardView` 内 Retainer 移除，并让 `CardView / FeedbackOverlay / InteractionFeedbackImage` 一起成为 `Fake3DSurfaceRetainer` 内容；嵌套 Retainer 会增加离屏渲染成本并造成刷新时序不稳定。
- `SurfaceOverlay` 必须是 `Fake3DSurfaceRetainer` 的直接内容根，尺寸覆盖完整 Retainer / bleed 区域，Clipping 使用 `Clip To Bounds - Without Intersecting (Advanced)`。`UWacomFirstPersonCardViewWidget` 会在 Rebuild / Construct 时自动保证该值，以免 Slate 使用视口 culling rect 提前整批剔除靠近屏幕底边的 `TypeText`。该修复只改变 Retainer 内部绘制边界，不移动卡牌、不改变扇形、角度或命中。
- `Fake3DSurfaceRetainer` Effect Material 使用 `/Game/DreamMaterials/Card/M_FirstPersonCard_Fake3D`，Retainer texture parameter 填 `Texture` 并启用效果；该材质来自 `DShader/Material/Card/M_FirstPersonCard_Fake3D.dsm`，Blend Mode 必须是 `AlphaComposite`（DreamShader 源写作 `PremultipliedAlpha`），以符合 Retainer 的预乘 Alpha 合成。C++ Card Depth 参数名固定为 `TiltX`、`TiltY`、`PerspectiveStrength`、`ContactShadowEnabled`、`ContactShadowLift`。当前生产材质不包含 Selection 参数、Noise 采样、轮廓、扫光或 Glint；实体出血卡框只参与实时 `Texture.A` 接触阴影。没有材质或缺少可选绑定时安全退化，不取消 Hover / Drag 或抽弃牌动画。
- Played 消散不增加第二个 Retainer、Image 或 WBP 动画轨道。默认 Anchor Style 为 `/Game/Wacom/UI/Card/SurfaceEffects/DA_FPCardPlayedDissolveStyle_OrderedDither`，使用 `/Game/DreamMaterials/Card/M_FirstPersonCard_SurfaceEffects_OrderedDither`；旧 `DA_FPCardPlayedDissolveStyle_PixelAsh` 与 `M_FirstPersonCard_SurfaceEffects` 保留为可切换灰烬方案。C++ 只在活动期间切换现有 Retainer，结束后恢复 WBP 创作源材质并重新取得实际 MID；所有材质保持 `UI Domain + AlphaComposite + Texture` 合同。Style、材质或噪声无效时自动使用旧 Played 空间离场。
- `T_FirstPersonCard_PlayedDissolveNoise` 导入设置为 `Compression=Masks`、`sRGB=false`、`Filter=Nearest`、`Mip Gen=NoMipmaps`、`LOD Group=UI`。DreamShader 中 `PlayedDissolveNoiseTexture` 的默认 Path 也必须指向这张 Masks 资产，不能使用 Engine `DefaultTexture`，否则 SM6 会报告 `Sampler type is Masks, should be Color` 并导致 Surface 材质错误回退。它只供活动 Surface-Effect MID 使用；原始源图和确定性五级灰度结果保存在 `DShader/Texture/Card/`。
- 正式 Drag 拾牌反馈不新增 WBP 控件，也不使用 `FeedbackOverlay / InteractionFeedbackImage`。它由 Slot 的 Motion Mixer 对整个 wrapper 叠加约 `0.14s` 的局部上提/缩放，并从 Anchor `10 Interaction Feedback | Drag Pickup` 读取声音；因此 `WBP_FPCardView` 无需增加动画轨道、Image 或第二个 Retainer。首次快捷键让无目标卡长距离飞向鼠标时，声音立即播放，局部上提/缩放等卡牌追到 pointer 后再播放；卡牌已在 pointer 附近时仍立即播放。
- 像素棱镜 Style 与 `T_FirstPersonCard_SelectionPixelClusters` 暂作为未来 `CardDataChanged / Upgrade` 效果源保留。纹理导入设置仍为 `Compression=Masks`、`sRGB=false`、`Filter=Nearest`、`Mip Gen=NoMipmaps`、`LOD Group=UI`，但当前 Fake-3D 材质不引用它；复用算法位于 `DShader/Shared/WacomCardPixelPrism.dsh`，不得为了拖拽效果重新接回生产材质。
- Card Depth 材质由 DreamShader 1.4.1 生成，`.dsm` 是长期真源。若 Content 资产缺失，使用 DreamShader commandlet 对上述源文件执行 `compile -Force`；不要在 Unreal 材质图里做无法回写到 `.dsm` 的平行修改。
- 材质图人工复查时，Fake3D 主卡面应有 `RG` 投影 UV、`B` inside mask、`RGB` 卡面颜色、`A` 卡面透明度；接触阴影还应有 9 个 `A` 采样 mask 与 `ContactShadowColor RGB`。若单通道节点仍显示 `RGB` 或 `RGA`，说明资产没有使用带 ComponentMask 默认通道修复的 DreamShader 版本重新生成。
- `WBP_FPCardView` 已删除历史 `ShadowHost / CardShadowImage`；生产链只允许 `Fake3DSurfaceRetainer` 内的实时 Alpha 接触阴影，禁止重新加入与 Card Depth、消散 caster 不同步的宽泛矩形阴影。
- `InteractionFeedbackImage` 优先使用 Anchor 的 `InteractionFeedbackMaterial`；该材质为空时，会复用 WBP Image brush 上预设的材质。推荐制作流程是：常规风格直接把材质放到 `InteractionFeedbackImage` 的 brush 上；需要角色 / 场景级替换时再在 Anchor 上填 override。若没有材质，pressed / confirm / commit 仍可退化为普通 tint，deny 只保留 shake，不退回整卡红色 overlay。
- 交互反馈材质需要支持 C++ 写入参数：`FeedbackColor`、`EdgeWidth`、`EdgeSoftness`、`VignetteStrength`、`VignetteRadius`、`VignetteSoftness`、`Opacity`、`Pulse`。
- 不再支持旧 `DenyFeedbackEdgeImage` fallback；源卡交互反馈统一绑定到 `InteractionFeedbackImage`。
- RootOverlay 使用 `Inherit`；SurfaceOverlay 使用 `Clip To Bounds - Without Intersecting (Advanced)`，并覆盖完整透明 bleed 画布。建议主体外每边预留约 `32-40 px`，保证透视角完整，同时避免 Retainer 内小型文字在视口边缘整批瞬隐。
- `SurfaceFoilOverlay` 是显式 opt-in 装饰层；需要卡面流光时由 WBP 自己添加并绑定该 Image。
- `CardView.CardSizeBox` 默认保持 296 x 420，并居中放在 bleed 画布中；缺失时运行时回退旧主体尺寸。
- 透明 bleed 只负责渲染，不扩大 hover、click、drag 起手或 Card target probe 范围。
- 不绑定按钮，不在 WBP 图里实现 hover / pending / disabled 状态机。
- 材质流光和表面装饰继续走内层 `UWacomCardView` 路径；first-person 反馈走 wrapper 绑定控件，不在 slot widget 内新增 Image / 材质刷新逻辑。

最小 PIE 验收：

- Battle 中 first-person hand 使用该卡面，旋转时边缘没有明显黑边、断线或主体裁切。
- 带实体出血卡框装饰的卡牌，其紧贴接触阴影跟随真实装饰轮廓；Hover / Drag 时接触阴影平滑变软、变淡，且没有明显双黑边。
- Hover 卡面随卡内 pointer 克制倾斜，按下后倾角减弱；Drag 改由 pointer velocity 产生惯性，pointer 停止后倾角回正但抬升阴影保持到 release。
- 抽牌 / 出牌 / 弃牌 semantic transition 期间卡面逐渐压平；目标候选卡不倾斜，只有 Drag source 消费 fake-3D。
- Played 成功后卡牌停在提交位置，Commit 脉冲后从左下向右上形成 Bayer 透明棋盘前沿；默认 `0.28` 密度的原色网点在约 `0.14s` 内飘散，其中约 `75%` 沿消散方向移动约 `34px`，其余向四周做约 `0.55` 倍距离的短散射。接触阴影只跟随尚未消失的卡面 caster，并在前约 `0.10s` 结束；完成前 outgoing 卡槽仍存在，完成后无残留材质或像素。无目标、世界目标和手牌目标均使用同一行为；禁用 Style 或移除噪声后应安全回退旧飞向目标的离场。
- Battle 无目标 / 世界目标 / 手牌目标和 Run 正式拖拽都只在首次进入正式 Drag 时播放一次短促拾牌上提/缩放；随后只保留现有 Fake-3D 与接触阴影。鼠标按下超过拖拽阈值后，Pressed 压缩必须先结束，再播放完整拾牌脉冲。首次远距离快捷键拿起无目标卡时，卡牌抵达 pointer 后必须仍能看到这次脉冲；连续按同一牌位快捷键时保持立即重播。Hover、Inspect、Pending 本身和 Drag 内部状态切换不触发；退出、取消、换牌后立即清理，Reduced Motion 取消额外上提/缩放但仍允许拾牌音。
- 鼠标在主体范围外、bleed 范围内不触发 hover 或拖拽起手。
- 费用图标、卡名、类型、效果徽章和耐久显示仍跟普通 CardView 数据一致。

## Combat Log WBP

### WBP_BattleCombatLogFeed

父类：`UBattleCombatLogFeedWidget`

推荐资产路径：`/Game/Wacom/UI/Battle/WBP_BattleCombatLogFeed`

推荐绑定：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `BlocksScrollBox` | `ScrollBox` | Optional | 常驻记录滚动区域 |
| `BlocksBox` | `PanelWidget` | Optional | C++ 动态填充命令块 |
| `TitleText` | `TextBlock` | Optional | 标题 |

配置项：

| 属性 | 用途 |
|---|---|
| `MaxVisibleBlocks` | 最多保留命令块数量 |
| `bAutoScrollToLatest` | 追加后是否滚动到最新 |
| `BlockWidgetClass` | 单个命令块 Widget 类 |

WBP 不应做：

- 不自行消费 raw `FBattleEvent`。
- 不提交战斗命令。
- 不替代 `UWacomBattleCombatLogBuilder` 的 ViewData 构造职责。

最小 PIE 验收：

- 快速连续出牌后仍可滚动查看最近命令块。
- 追加新命令块时可按配置滚动到最新。

### WBP_BattleCombatLogBlock

父类：`UBattleCombatLogBlockWidget`

推荐资产路径：`/Game/Wacom/UI/Battle/WBP_BattleCombatLogBlock`

推荐绑定：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `HeaderText` | `TextBlock` | Optional | 命令块标题 |
| `DetailsBox` | `PanelWidget` | Optional | C++ 动态填充 detail line |

WBP 合同：

- `SetCombatLogBlockData()` 保存完整 `FWacomBattleCombatLogBlockView` 并触发 `BP_OnCombatLogBlockUpdated`。
- WBP 可读取 `VisualTone / IconKey` 调整样式。
- 命令块只是显示组件，不提交战斗命令。

## Presentation Stack WBP

### WBP_BattlePresentationStack

父类：`UBattlePresentationStackWidget`

推荐资产路径：`/Game/Wacom/UI/Battle/WBP_BattlePresentationStack`

推荐绑定：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `StackCanvas` | `CanvasPanel` | Optional | C++ 动态摆放只读小卡 entry |

配置项：

| 属性 | 用途 |
|---|---|
| `MaxVisibleEntries` | 最多显示的小卡数量 |
| `MiniCardViewClass` | 小卡使用的只读 CardView 类 |
| `MiniCardSize` | 小卡宽高，Slate 像素 |
| `EntryOffset` | 小卡错位距离，Slate 像素 |

WBP 不应做：

- 不额外显示卡名、目标、数量或规则解释文案。
- 不使用 `WBP_FPCardView` 作为小卡类。
- 不把 stack entry 做成可点击、可拖拽或规则命令入口。

最小 PIE 验收：

- 出牌后出现缩小完整卡面。
- 小卡保持 `HitTestInvisible`，不挡手牌、按钮或目标选择。

## Shared Battle Widgets

### WBP_PlayerStatusBar

父类：`UPlayerStatusBar`

推荐绑定：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `HpBar` | `UWacomProgressBar` | Optional | 玩家 HP |
| `ShieldText` | `TextBlock` | Optional | 护盾文本，0 时可隐藏 |
| `StatusList` | `UWacomBattleStatusIconListWidget` | Optional | 玩家 runtime 状态图标行；为空状态时自动隐藏 |

制作提示：

- 推荐把状态列表实例直接命名为 `StatusList`。C++ 会在 `StatusList` 未绑定时回退查找唯一一个 `UWacomBattleStatusIconListWidget` 子控件，但存在多个状态列表时不会猜测。
- `StatusList` 只在 Snapshot 里有非 `Status.Shield` 状态时显示；单纯配置图标 Brush 不会让 PIE 自动出现状态。
- Action Preview 不需要新增必绑控件。预览激活时，C++ 会用 Battle 规则层产出的 projected player state 覆盖 `HpBar / ShieldText / StatusList` 当前显示，并用可调透明度提示这是预览态；清理后恢复最近一次真实 Snapshot。

WBP 不应做：不提交玩家命令，不修改 BattleSession。

### WBP_BattleStatusIconList

父类：`UWacomBattleStatusIconListWidget`

用途：把 `Statuses / StatusStacks` 显示为水平状态图标行。玩家状态条本轮正式使用；敌人部位条目可选接入同一控件。

推荐绑定：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `StatusContainer` | `HorizontalBox` 或其他 `PanelWidget` | Optional | C++ 动态填充状态图标；缺省时 fallback 创建水平列表 |

推荐 WBP 变量：

| 属性 | 用途 |
|---|---|
| `StatusIconWidgetClass` | 每个状态图标使用的 Widget 类，推荐 `WBP_BattleStatusIcon` |
| `PoisonIconBrush / SlowIconBrush / FreezeIconBrush / TwilightIconBrush / StunnedIconBrush` | 各正式状态的图标 Brush |
| `FallbackStatusIconBrush` | 未知状态或未配置专用 Brush 时的图标 |
| `bShowDesignTimePreview` | 设计器预览开关，默认开启；只影响 UMG 视口 |
| `PreviewStatuses / PreviewStatusStacks` | 设计器预览用状态和层数，默认示例为 Poison / Slow / Freeze |

刷新语义：

- 固定顺序为 Poison、Slow、Freeze、Twilight、Stunned；未知状态按 tag 名排序。
- `Status.Shield` 不显示在状态列表里，护盾仍由 HP / Shield UI 单独显示。
- 空状态时列表整体折叠。
- 运行时 Brush 会自动补一个默认图标尺寸，避免 Texture 已配置但 `ImageSize` 为空时在列表里按 0 尺寸排布。

### WBP_BattleStatusIcon

父类：`UWacomBattleStatusIconWidget`

推荐绑定：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `IconImage` | `Image` | Required | 状态图标 |
| `StackText` | `TextBlock` | Optional | 角落层数数字；层数最小显示为 1 |
| `StackBadge` | `Widget` | Optional | 角落层数底板；跟随状态显示/隐藏 |

推荐 WBP 变量：

| 属性 | 用途 |
|---|---|
| `bShowDesignTimePreview` | 单独打开 Icon WBP 时显示预览内容，默认开启 |
| `PreviewStatusTag / PreviewDisplayName / PreviewStackCount` | 单个 Icon 的设计器预览状态、名称和层数 |
| `PreviewIconBrush` | 单个 Icon 的设计器预览图标；为空时优先使用 `IconImage` 在 WBP 中配置的 Brush |

WBP 不应做：不从规则层查询状态，不自行改状态层数，不把 `Status.Shield` 混入状态图标。

### WBP_BattleCommandBar

父类：`UBattleCommandBarWidget`

推荐绑定：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `WaitButton` | `UWacomBattleCommandButtonWidget` | Optional | 直接放在 CommandBar 中的等待按钮；绑定后由 WBP 控制位置 |
| `EndTurnButton` | `UWacomBattleCommandButtonWidget` | Optional | 直接放在 CommandBar 中的结束回合按钮；绑定后由 WBP 控制位置 |
| `CommandButtonContainer` | `PanelWidget` | Optional fallback | 未绑定 `WaitButton / EndTurnButton` 时，承载 runtime 动态生成的命令按钮 |
| `WaitValueText` | `TextBlock` | Optional | 当前等待值，例如 `Wait Value: 2` |
| `PendingText` | `TextBlock` | Optional | pending turn-boundary 文案，例如“等待排队中” |

重要变量：

| 变量 | 类型 | 用途 |
|---|---|---|
| `CommandButtonWidgetClass` | `TSubclassOf<UWacomBattleCommandButtonWidget>` | fallback 动态生成 Wait / EndTurn 按钮使用的 WBP 类；直接绑定按钮时可不配置 |
| `WaitIconBrush` | `FSlateBrush` | 等待按钮图标；配置后写入 `WaitButton.IconImage` |
| `EndTurnIconBrush` | `FSlateBrush` | 结束回合按钮图标；配置后写入 `EndTurnButton.IconImage` |

当前推荐做法：在 `WBP_BattleCommandBar` 里直接放两个 `WBP_BattleCommandButton` 实例，分别命名为 `WaitButton` 和 `EndTurnButton`，位置、间距、锚点完全由 CommandBar 资产控制。C++ 会把 Wait / EndTurn 的 view data 写入这两个按钮，并接收它们的点击事件；此模式下不会从 `CommandButtonContainer` 动态生成按钮。按钮图标也在 `WBP_BattleCommandBar` 上配置：设置 `WaitIconBrush / EndTurnIconBrush` 后，子按钮里的 `IconImage` 会随 view data 显示；Brush 留空时图标自动隐藏。

WBP 不应做：不直接提交 `FBattleCommand`，不自行判断按钮可用性，不把 pending 文案混入 `WaitValueText`。按钮可见性、可用性和 pending 状态由 BattleHUD runtime presenter 推送的 `FWacomBattleCommandBarViewData` 决定；本轮正式可见命令只有 Wait / EndTurn。

CommandBar 的轻量协议定义在 `BattleCommandBarTypes.h`：`EWacomBattleCommandId`、`FWacomBattleCommandButtonView` 和 `FWacomBattleCommandBarViewData` 是 HUD / runtime presenter / tests 共用的 interface；`BattleCommandBarWidget.h` 只承载 UMG Widget 类、WBP 绑定和按钮生成实现。

### WBP_BattleCommandButton

父类：`UWacomBattleCommandButtonWidget`

推荐资产路径：`/Game/Wacom/UI/Battle/Action/WBP_BattleCommandButton`

样式资产：

| 资产 | 推荐路径 | 用途 |
|---|---|---|
| ButtonStyle | `/Game/Wacom/UI/Style/Button/tiny_menu_Button` | 命令按钮的 CommonButtonStyle |
| TextStyle | `/Game/Wacom/UI/Style/Text/MyCommonTextStyle` | 命令按钮文字 CommonTextStyle |

`WBP_BattleCommandButton` 是 Battle CommandBar 的业务按钮 Widget；`tiny_menu_Button`、`MyCommonTextStyle` 等通用样式资产不放在 `/Game/Wacom/UI/Battle/Action/`，避免 Battle 目录继续承载跨界面的 Style contract。

推荐绑定：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `ButtonText` | `CommonTextBlock` | Optional | 命令显示名 |
| `IconImage` | `Image` | Optional | 命令图标 |
| `InputHintText` | `TextBlock` | Optional | 快捷键 / 手柄提示 |
| `PendingIndicator` | `Widget` | Optional | 当前命令 pending 时显示 |

WBP 不应做：不在按钮图里调用 HUD / Session；点击只通过父类广播 `EWacomBattleCommandId`，由 `UBattleHUD` 分发到 `OnWaitRequested / OnEndTurnRequested`。

### WBP_PileCountView

父类：`UPileCountView`

用途：抽牌堆 / 弃牌堆 / 消耗牌堆数量显示。牌堆类型由 WBP 中的 Image 素材表达，C++ 不再绑定文字标签或底板。

C++ 绑定：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `CountText` | `TextBlock` | Optional | 数量；允许 HUD 写入类似 `2+3` 的复合数量文本 |

WBP 推荐结构：自行添加一个 Image 放抽牌堆 / 弃牌堆 / 消耗牌堆图标，再把 `CountText` 叠到图标合适位置。`LabelText` 和 `FrameBorder` 已不再是制作合同，不要新建或绑定。

WBP 不应做：不修改牌堆或规则状态，不自行计算数量。

## Enemy Panel WBP

### WBP_BattleEnemyPanelWidget

父类：`UWacomBattleEnemyPanelWidget`

用途：敌人 Host 头顶的世界空间聚合面板。`AWacomBattleEnemyActor.EnemyPanelWidgetComponent` 承载该 WBP，HUD 只从 `FBattleSnapshot.Enemies` 派发只读 view data，按敌人聚合展示所有部位。

推荐绑定：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `EnemyListBox` | `VerticalBox` | Optional | 每个敌人一组；缺省时 C++ fallback 自动创建 |

推荐类默认值：

| 属性 | 推荐值 | 运行时职责 |
|---|---|---|
| `PartEntryWidgetClass` | `BP_WacomBattleEnemyPartEntryWidget` | 面板内每个部位条目的正式 WBP 类；为空时使用 C++ fallback |

WBP 不应做：不直接读取或修改 `UBattleSession`，不在部位 Actor 上创建常驻状态 UI。

### WBP_BattleEnemyPartEntryWidget

父类：`UWacomBattleEnemyPartEntryWidget`

用途：敌人面板内的通用部位条目，展示单个部位的名称、HP/MaxHP、护盾、先机、意图和状态。当前可继续使用状态文本，也可选接共享状态图标列表。

推荐绑定：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `PartNameText` | `TextBlock` | Optional | 部位名 |
| `HpText` | `TextBlock` | Optional | HP / MaxHP |
| `ShieldText` | `TextBlock` | Optional | 护盾；无护盾时可为空或隐藏 |
| `InitiativeText` | `TextBlock` | Optional | 当前先机 |
| `StatsText` | `TextBlock` | Optional | HP / 护盾 / 先机的汇总兜底文本；正式 WBP 拆出上面三个字段后可以不放 |
| `IntentText` | `TextBlock` | Optional | 当前意图 |
| `StatusList` | `UWacomBattleStatusIconListWidget` | Optional | 状态图标行；绑定后优先使用图标列表 |
| `StatusText` | `TextBlock` | Optional | 状态标签汇总；未绑定 `StatusList` 时的回退 |
| `DestroyedOverlay` | `Widget` | Optional | 部位破坏时的弱化/覆盖层 |

刷新语义：

- Panel 按 `EnemySlotId` 复用敌人组，按 `EnemySlotId + PartSlotId` 复用部位条目；同一部位只更新 view data，不重建条目 Widget。
- `Shield == 0` 时 `ShieldText` 会清空并折叠；如果 WBP 只绑定 `StatsText` 而未绑定 `HpText / ShieldText / InitiativeText`，汇总文本仍会显示。
- `StatusList` 绑定时，C++ 使用共享图标列表并隐藏 `StatusText`；未绑定时保留旧状态文本格式。
- `bDestroyed` 时 `DestroyedOverlay` 显示，条目整体透明度降低。
- Action Preview 不需要新增必绑控件。预览激活时，C++ 会用 Battle 规则层产出的 projected part view 覆盖 `HpText / ShieldText / InitiativeText / StatusList / DestroyedOverlay` 当前显示；如果部位会因本次打牌立即行动，`InitiativeText` 显示 `0`。同一次出牌触发多个敌人部位行动时，敌人聚合面板会显示所有 projected 部位的净结果，不限于当前鼠标指向部位。预览刷新不触发真实 HP / Shield pulse，清理后恢复基础 ViewData。
- C++ fallback 使用暗色紧凑面板和水平部位条目：部位名、HP、护盾、先机、意图同排展示，状态和破坏标记作为次级信息显示。
- C++ fallback 自带轻量表现动效：新增条目错峰淡入/轻微下移归位，HP、护盾和破坏状态变化时短促 pulse。正式 WBP 可以用 UMG Animation 覆盖更完整的动效表现。

WBP 不应做：不提交战斗命令，不反向写入 Snapshot，不承担 world target/hover/drag preview 反馈。
## PIE Smoke Checklist

- `WBP_BattleHUD` 能显示玩家状态、CommandBar、牌堆数量、CombatLogFeed 和 PresentationStack；敌人聚合面板不挂在 HUD Canvas，而挂在 `AWacomBattleEnemyActor` 头顶。
- CommandBar 里的 Wait / EndTurn 可点击并由 HUD runtime view data 控制可用性。
- `WBP_FPCardView` 的 `CardSizeBox` 主体命中范围正确，bleed 画布不扩大交互范围。
- Combat Log 连续追加后可滚动，Presentation Stack 小卡不挡输入。
- 有 `SceneEnemyHostSlots` 的战斗中，每个 `AWacomBattleEnemyActor` 头顶的 EnemyPanel 能按敌人聚合展示所有部位状态；PartActor 只显示 target、drag preview、prediction 等场景反馈，普通部位 hover 使用所属敌人的聚合面板响应。
- `EncounterDefinition` 正式入口必须配置 `SceneEnemyHostSlots`；推荐先执行 `SyncSceneEnemyHostSlotsFromEncounter()` 生成 slots，再逐项填写 Host。缺 Host、漏映射或多余 EnemySlotId 是摆放错误。
- 旧 `WBP_CardWidget / WBP_HandPanel / WBP_EnemyInfoBar / WBP_EnemyPartWidget / EventLogPanel / EventToast / WBP_BattleEnemyPartStatusBadgeWidget` 已删除，不再作为 BattleHUD 制作入口。
