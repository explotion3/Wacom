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
- 抽牌从 `DrawPileMotionAnchor`（或 `DrawPileView` 中心）进入；弃牌飞向 `DiscardPileMotionAnchor`（或 `DiscardPileView` 中心）。配置有效 Card Use Surface Effect 时，无目标牌与目标牌都停在提交位置播放当前 Style（默认像素翻面收牌，旧菱形波可切回）；`PlayTargetMotionAnchor` 和真实目标坐标仍会采集，但只供效果失效时的旧空间离场 fallback 与未来目标命中反馈使用。
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
└─ FPCardViewSizeBox / SizeBox [456 x 520]
   └─ RootOverlay / Overlay
      └─ Fake3DSurfaceRetainer : RetainerBox
         └─ SurfaceCaptureOverlay / Overlay [Clipping = Clip To Bounds - Without Intersecting]
            └─ CardContentSizeBox / SizeBox [360 x 424, Center]
               └─ SurfaceOverlay / Overlay
                  ├─ CardView : UWacomCardView
         │  └─ BleedCanvas / SizeBox
         │     └─ CardSizeBox / SizeBox
         │        └─ CardOverlay / Overlay
         │           ├─ CardSurfaceImage / Image（Optional；缺失时 C++ 在底层创建）
         │           ├─ 文本、费用与类型等稳定 UMG 内容
         │           └─ AttachmentParallaxHost（Optional；出血装饰容器）
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
| `SurfaceCaptureOverlay` | `Overlay` | Retainer direct content by convention | Retainer 完整离屏捕获根；覆盖 `456 x 520` 透明范围并重置继承自视口的普通 culling rect |
| `CardContentSizeBox` | `SizeBox` | Authored by convention | 保持原 `360 x 424` 卡面与反馈层尺寸并居中，避免扩大捕获面时拉伸内容 |
| `SurfaceOverlay` | `Overlay` | `CardContentSizeBox` content | 原有 CardView、FeedbackOverlay 与 InteractionFeedbackImage 的共同内容根 |
| `CardSizeBox` | `SizeBox` | `CardView` 内 Required by convention | 296 x 420 主体显示和交互参考范围 |
| `CostDigitImage` | `Image` | `CardView` 内 Optional | 一位费用数字 PaperSprite brush；真实改费时 C++ 临时绑定双 Atlas UI MID，结束后恢复权威新 Sprite |
| `EffectBadgeSlot1..4` | `PanelWidget` | `CardView` 内 Optional | 保持最多四个可见 EffectBadge 的 authored 位置；稳定身份来自数据 Key，不来自槽位索引 |
| `BadgeFrameImage` | `Image` | `WBP_CardEffectBadge` 内 Optional | Badge 框体 PaperSprite；数值重写期间保持显示，不承担数字 MID |
| `DigitHost` | `PanelWidget` | `WBP_CardEffectBadge` 内 Optional | 多位 PaperSprite 数字 Image 宿主；Preview/Rewrite 只给实际变化的数字临时绑定局部 MID |
| `DurabilityBackIcon` | `Image` | `CardView` 内 Optional | 耐久实体底板；first-person 视差会仅按该 Brush 生成运行时局部硬阴影，不复制耐久数字 |
| `SurfaceFoilOverlay` | `Widget` | `CardView` 内 Optional | 复用 `UWacomCardView` 弱流光 / 表面装饰；未绑定时不会自动创建覆盖层 |
| `CardOverlay` | `PanelWidget` | `CardView` 内 Optional BindWidget | 核心表面 Image 的运行时插入点；现有 WBP 已使用该名称时无需手动新增 Image |
| `CardSurfaceImage` | `Image` | `CardView` 内 Optional BindWidget | 单次绘制底色、插画、实体卡框和稀有度饰条；缺失时可由 C++ 插入到 CardOverlay 索引 0 |
| `AttachmentParallaxHost` | `Widget` | `CardView` 内 Optional BindWidget | 统一承载 `EffectBadgeSlot1..4 / DurabilityHost` 的前景视差；缺失时 C++ 逐控件 fallback |

WBP 合同：

- `WBP_FPCardView` 外层只负责包装和反馈层；通用卡面内容应放在 `CardView` 子控件里。
- `FeedbackOverlay` 和 `InteractionFeedbackImage` 都由 WBP 控制尺寸、锚点和层级；C++ 只写颜色、透明度和材质参数。
- `WBP_FPCardView` 一张卡只允许一个 Retainer。需要把旧 `CardView` 内 Retainer 移除，并让 `CardView / FeedbackOverlay / InteractionFeedbackImage` 一起成为 `Fake3DSurfaceRetainer` 内容；嵌套 Retainer 会增加离屏渲染成本并造成刷新时序不稳定。
- `SetEffectMaterial()` 只表示 Retainer 已收到源材质，不代表 Slate 运行时 MID 已建立或新参数已经绘制。C++ 会为 Surface、`CostDigitImage` 和 Badge Digit 分别提交进度 0 Generation，并在 `WBP_FPCardView` 完成一次真实 Paint 后才启动对应 Playback；不要在 WBP Timeline、Construct 或 Binding 中抢先重置这些 Brush/Effect Material。该准备帧不会改变 WBP 层级和 authored 动画时长。
- `SurfaceCaptureOverlay` 必须是 `Fake3DSurfaceRetainer` 的直接内容根，尺寸覆盖完整 `456 x 520` Retainer，Clipping 使用 `Clip To Bounds - Without Intersecting (Advanced)`。`CardContentSizeBox` 保持 `360 x 424` 并居中，四边各留 `48 px` 纯渲染空间。`UWacomFirstPersonCardViewWidget` 会在 Rebuild / Construct 时自动保证直接内容根的 clipping 值，以免 Slate 使用视口 culling rect 提前整批剔除靠近屏幕底边的内容。该结构只改变 Retainer 捕获边界，不移动卡牌、不改变扇形、角度或命中。
- `Fake3DSurfaceRetainer` Effect Material 使用 `/Game/DreamMaterials/Card/M_FirstPersonCard_Fake3D`，Retainer texture parameter 填 `Texture` 并启用效果；该材质来自 `DShader/Material/Card/M_FirstPersonCard_Fake3D.dsm`，Blend Mode 必须是 `AlphaComposite`（DreamShader 源写作 `PremultipliedAlpha`），以符合 Retainer 的预乘 Alpha 合成。C++ Card Depth 参数名固定为 `TiltX`、`TiltY`、`PerspectiveStrength`、`ContactShadowEnabled`、`ContactShadowLift`、`ContactShadowTiltOffsetXUV`、`ContactShadowTiltOffsetYUV`、`ContactShadowOpacityMultiplier`。倾斜 Offset 让阴影沿卡面倾斜反方向移动，来自 Anchor 的 UMG px 参数并按 Retainer 当前尺寸转换为 UV；Opacity Multiplier 默认 `1.5`，可在 Anchor `11 Card Depth` 调整。基础与临时 Surface 材质必须全部保留该合同。当前生产材质不包含 Selection 参数、Noise 采样、轮廓、扫光或 Glint；实体出血卡框只参与实时 `Texture.A` 接触阴影。没有材质或缺少可选绑定时安全退化，不取消 Hover / Drag 或抽弃牌动画。
- `WBP_FPCardView` 的 `CardView` 子控件实际使用 `WBP_FirstPersonCardView`；它与通用 `WBP_CardView` 的核心表面材质都使用 `/Game/DreamMaterials/Card/MI_WacomCardSurfaceComposite_Default`。DreamShader 真源是 `M_WacomCardSurfaceComposite.dsm + WacomCardSurfaceParallax.dsh`，`Scripts/SetupCardSurfacePerspectiveAssets.py` 会幂等同步两个 CardView 资产。C++ 按卡牌实例写入 `ArtTexture / ArtDepthTexture / ArtDepthEnabled`、`RarityTexture / RarityUVScaleBias` 与过滤后的 `TiltX / TiltY / ParallaxStrength`。`BackColorScale=0.96` 并固定在 Frame 平面；插画默认凹入 `-2.5` 源像素。无深度图时，插画实时 Alpha 会在固定 BackColor 上生成硬像素接触投影，默认 `ArtCastShadowEnabled=1 / ArtCastShadowOpacity=0.18 / ArtCastShadowStaticOffset=(0.75,1.0) / ArtCastShadowTiltOffsetPixels=2.680516`；投影只落在 BackColor 安全范围，不扩大卡牌 Alpha。`MaxArtParallaxPixels=4` 和半像素 Clamp 负责阻止极端视差采到边缘。Frame Alpha 另行生成约 `2px` 的静态/方向内沿阴影。可选 `CardIllustrationDepthMap` 使用黑深白浅的五级灰度，导入推荐 `Masks / sRGB=false / Nearest / NoMipmaps / UI`；为空时整层凹入仍正常工作。`RarityBorder` 继续使用 `PaperSprite` Atlas，但位置直接复用 Frame UV，不再具有独立深度；它可通过 `RarityReflectionEnabled` 使用 bevel、foil、虹彩和 glow，但正式默认关闭，内容可按稀有度主题显式开启。`ArtReflectionEnabled / FrameReflectionEnabled / RarityReflectionEnabled` 默认值为 `0 / 1 / 0`。Anchor `11 Card Depth` 的 `CardDragSurfaceParallaxStrengthMultiplier` 默认 `0.75`，通过现有响应滤波同步降低 Drag 的内层 UV 和出血装饰位移；不改变外层 Fake-3D。复合路径有效时旧 `BackColor / CardArt / Frame / RarityBorder / SurfaceFoilOverlay` 自动折叠；材质、插画或插入点无效时恢复旧路径。
- `AttachmentParallaxHost` 是推荐但非必需的制作整理。若后续在 WBP 中加入，应保持完整 bleed 尺寸、`HitTestInvisible`，并在不改变 Canvas/Overlay slot 参数的前提下把四个 `EffectBadgeSlot` 与 `DurabilityHost` 移入其中。当前没有该 Host 时运行时会逐个移动上述控件；无须为了启用第一版效果立即重排 WBP。
- 出血装饰局部阴影不需要 authored WBP 节点。C++ 会在 `BadgeFrameImage` 与 `DurabilityBackIcon` 各自所属的 `Overlay` 中，运行时插入 `BadgeFrameShadowImage_Runtime / DurabilityShadowImage_Runtime`，位置紧邻实体底板之前，并复制源 Brush、ImageSize 与 Overlay Slot 对齐。若源控件不在 Overlay、Brush 无效或装饰隐藏，则对应阴影静默折叠；数字、文字、Glow、Preview/Rewrite MID 不会进入阴影。Anchor `11 Card Depth | Attachment Shadow` 控制颜色、`0.17` 默认透明度、`(2.0, 2.5)` 静态偏移、`0.80` 反向运动比例和 `6px` 上限。默认值通过增加分离距离提升可见度，同时保持低于外部 caster threshold `0.18`；不要在 WBP 中另建同名 Image。
- Card Glyph Transfer 不要求增加 WBP 控件：first-person card layer 会在 RootCanvas 内运行时创建一个 `HitTestInvisible` 的高层级批量 Slate renderer。弃牌堆洗回优先使用 `DiscardPileMotionAnchor / DrawPileMotionAnchor`，普通弃牌的每张起点来自对应 outgoing Slot 的实际中心，目标使用 `DiscardPileMotionAnchor → DiscardPileView`。renderer 会把坐标和内收弧线约束到逻辑 Viewport 安全区（默认边距 36px）；安全区只影响特效路径，不修改 WBP 控件。主体、窄像素拖尾、确定性飘散像素和接收 Impact 合并为一次 `MakeCustomVerts`；默认牌印为 42×66px、拖尾宽度为 10.5→3px、粒子初始边长为 6–13.5px，不生成完整卡背轮廓牌影。普通弃牌先用约 0.11 秒把卡面原地收束成交叉显现的牌印，再以单枚约 0.28 秒、0.055 秒错峰飞入弃牌堆；逐枚抵达会更新数量并播放约 0.12 秒像素方印。洗牌继续使用单枚 0.36 秒、0.045 秒间隔及其可选三段音效；每枚真实发射会让 `DiscardPileView` 减一并反向后坐，每枚真实抵达会让 `DrawPileView` 加一、接收回弹并播放约 0.10 秒小方印，最后一枚方印约 1.18 倍。普通弃牌不复用洗牌音效。默认 Style 为 `/Game/Wacom/UI/Card/SurfaceEffects/DA_FPCardPileTransferStyle_PixelGlyph`；主体、拖尾/粒子/Impact 颜色在 `/Game/DreamMaterials/Card/MI_FirstPersonCard_PileTransferGlyph_Default` 调整，几何、时序和预算在 Style 调整。Anchor 制作入口位于 `14 Card Pile Transfer`，普通弃牌和洗牌分别有启用开关。必要锚点或 Style 无效时普通弃牌回退旧空间 Exit，洗牌 phase 立即完成，均不阻塞后续规则表现。
- Surface 效果不增加第二个 Retainer、Image 或 WBP 动画轨道。普通 Played 默认使用 `/Game/Wacom/UI/Card/SurfaceEffects/DA_FPCardUseEffect_EdgeFlip`，其像素闪边、侧边辉光和短余像参数位于 `/Game/DreamMaterials/Card/MI_FirstPersonCard_SurfaceEffects_EdgeFlipUse_Default`；成功使用后仍在手牌的同一 Card ID 按 `0.22s` 翻出、`0.06s` 隐藏换位、`0.18s` 翻入、`0.04s` 落定，且无手牌锚点时在原位翻回。旧 `/Game/Wacom/UI/Card/SurfaceEffects/DA_FPCardUseEffect_DiamondWave` 与对应 MI 完整保留，可直接替换 Anchor Style 回退。实际 Exhausted 使用 `/Game/Wacom/UI/Card/SurfaceEffects/DA_FPCardPlayedDissolveStyle_OrderedDither`，旧 PixelAsh 方案继续可切换。C++ 只在活动期间切换现有 Retainer，结束后恢复 WBP 创作源材质并重新取得实际 MID；所有材质保持 `UI Domain + AlphaComposite + Texture` 合同。Card Use Material Instance 无效时，离手牌回退旧 Played 空间离场，留在手牌则直接回到最终布局；Exhaust Style / 材质 / 噪声无效时回退 Discarded 空间离场。
- Hand Target Impact 同样不增加 WBP 控件或第二个 Retainer。有效手牌目标预演与成功压印使用 `/Game/Wacom/UI/Card/SurfaceEffects/DA_FPCardHandTargetImpactStyle_PixelStamp` 和 `/Game/DreamMaterials/Card/MI_FirstPersonCard_SurfaceEffects_HandTargetImpact_Default`；参数入口位于 Anchor `15 Card Hand Target Impact`。目标被弃牌或消耗时，既有离场会等到约 `0.11s` 刻印峰值再接管唯一 Retainer；Style / MI 无效时无等待地回退原行为。WBP 只需保持现有 `Fake3DSurfaceRetainer`、`SurfaceOverlay` 与 `Texture` 参数合同，不需要 Animation、Image、Overlay 或额外绑定。
- Card Data Rewrite 与目标费用 Preview 都不增加 WBP 层，二者复用 `CostDigitImage` 的同一个临时 UI MID。Preview 模式只绘制预测的一位数 PaperSprite，正式 `Cost` 仍保持旧值；降低费用使用冰蓝/暖金呼吸，提高使用紫红/暗蓝呼吸，取消或提交时恢复正式旧 Sprite。Outcome 阶段中，C++ 在 ViewData 更新前从语义 Hint 的权威 `CostBefore / CostAfter` 解析旧/新 PaperSprite，并把两者的 Atlas Texture 与 UV Rect 写入 Rewrite 模式；不能从当前 Brush 猜旧值。动画结束后恢复权威新 Sprite、ImageSize、RenderTransform 与 Pivot。默认 Style / MI 为 `/Game/Wacom/UI/Card/SurfaceEffects/DA_FPCardDataRewriteStyle_Pixel` 与 `/Game/DreamMaterials/Card/MI_WacomCard_CostDigitRewrite_Default`，制作入口位于 Anchor `16 Card Data Rewrite`。MI 调节 Neutral / Beneficial / Detrimental 色板、约 14 列硬像素网格、边缘宽度与亮度；Style 管理 Preview 呼吸、`0.34s` 消散/停顿/中心重组/回弹时序、错峰与可选声音。该 MID 不替换唯一 `Fake3DSurfaceRetainer`，所以 HandTarget 刻印、Fake3D、视差、反光和接触阴影可并行。不要增加 TextBlock 替身、Overlay、Animation、第二个 Image 或第二个 Retainer。
- EffectBadge Preview/Rewrite 复用现有 `WBP_CardEffectBadge`、`BadgeFrameImage` 和 `DigitHost`，不新增 WBP 层。框体始终保留；只有预测或正式变化的数字 Image 使用 `/Game/DreamMaterials/Card/MI_WacomCard_EffectBadgeFeedback_Default` 临时 MID，结束后恢复普通 PaperSprite Brush。卡面按 Badge Kind 聚合同类型效果，稳定身份为 `Badge.<Kind>`：基础与条件伤害共用一枚 Damage Badge，中毒层数使用独立 Poison Badge；整个语义组都 skipped 时只降低亮度，不绘制额外叉号。默认 Style 是 `/Game/Wacom/UI/Card/SurfaceEffects/DA_FPCardEffectBadgeFeedbackStyle_Pixel`，Anchor 入口为 `21 Card Effect Badge Feedback`。四个 `EffectBadgeSlot` 仍是最终布局槽位，Preview 不覆盖权威 `Value`；正式 Added/Removed 目前没有规则事务来源，WBP 不应自行增删控件来触发动画。
- `T_FirstPersonCard_PlayedDissolveNoise` 导入设置为 `Compression=Masks`、`sRGB=false`、`Filter=Nearest`、`Mip Gen=NoMipmaps`、`LOD Group=UI`。DreamShader 中 `PlayedDissolveNoiseTexture` 的默认 Path 也必须指向这张 Masks 资产，不能使用 Engine `DefaultTexture`，否则 SM6 会报告 `Sampler type is Masks, should be Color` 并导致 Surface 材质错误回退。它只供活动 Surface-Effect MID 使用；原始源图和确定性五级灰度结果保存在 `DShader/Texture/Card/`。
- 正式 Drag 拾牌反馈不新增 WBP 控件，也不使用 `FeedbackOverlay / InteractionFeedbackImage`。它由 Slot 的 Motion Mixer 对整个 wrapper 叠加约 `0.14s` 的局部上提/缩放，并从 Anchor `10 Interaction Feedback | Drag Pickup` 读取声音；因此 `WBP_FPCardView` 无需增加动画轨道、Image 或第二个 Retainer。首次快捷键让无目标卡长距离飞向鼠标时，声音立即播放，局部上提/缩放等卡牌追到 pointer 后再播放；卡牌已在 pointer 附近时仍立即播放。
- 像素棱镜 Style 与 `T_FirstPersonCard_SelectionPixelClusters` 暂作为未来 `CardDataChanged / Upgrade` 效果源保留。纹理导入设置仍为 `Compression=Masks`、`sRGB=false`、`Filter=Nearest`、`Mip Gen=NoMipmaps`、`LOD Group=UI`，但当前 Fake-3D 材质不引用它；复用算法位于 `DShader/Shared/WacomCardPixelPrism.dsh`，不得为了拖拽效果重新接回生产材质。
- Card Depth 材质由 DreamShader 1.4.1 生成，`.dsm` 是长期真源。若 Content 资产缺失，使用 DreamShader commandlet 对上述源文件执行 `compile -Force`；不要在 Unreal 材质图里做无法回写到 `.dsm` 的平行修改。
- 材质图人工复查时，Fake3D 主卡面应有 `RG` 投影 UV、`B` inside mask、`RGB` 卡面颜色、`A` 卡面透明度；接触阴影还应有 9 个 `A` 采样 mask 与 `ContactShadowColor RGB`。若单通道节点仍显示 `RGB` 或 `RGA`，说明资产没有使用带 ComponentMask 默认通道修复的 DreamShader 版本重新生成。
- `WBP_FPCardView` 已删除历史 `ShadowHost / CardShadowImage`；生产链只允许 `Fake3DSurfaceRetainer` 内的实时 Alpha 接触阴影，禁止重新加入与 Card Depth、消散 caster 不同步的宽泛矩形阴影。
- `InteractionFeedbackImage` 优先使用 Anchor 的 `InteractionFeedbackMaterial`；该材质为空时，会复用 WBP Image brush 上预设的材质。推荐制作流程是：常规风格直接把材质放到 `InteractionFeedbackImage` 的 brush 上；需要角色 / 场景级替换时再在 Anchor 上填 override。若没有材质，pressed / confirm / commit 仍可退化为普通 tint，deny 只保留 shake，不退回整卡红色 overlay。
- 交互反馈材质需要支持 C++ 写入参数：`FeedbackColor`、`EdgeWidth`、`EdgeSoftness`、`VignetteStrength`、`VignetteRadius`、`VignetteSoftness`、`Opacity`、`Pulse`。
- 不再支持旧 `DenyFeedbackEdgeImage` fallback；源卡交互反馈统一绑定到 `InteractionFeedbackImage`。
- RootOverlay 使用 `Inherit`；`SurfaceCaptureOverlay` 使用 `Clip To Bounds - Without Intersecting (Advanced)` 并覆盖 `456 x 520` 透明捕获面，`CardContentSizeBox` 维持 `360 x 424` 居中。四边 `48 px` 余量用于倾斜边角、实时阴影和出血装饰，不扩大命中区域。
- `SurfaceFoilOverlay` 只作为旧卡面 fallback 的显式 opt-in 装饰层；核心表面复合材质活动时会折叠它，箔片 / 虹彩应在默认 MI 中调节，避免双重高光。
- `CardView.CardSizeBox` 默认保持 296 x 420，并居中放在 bleed 画布中；缺失时运行时回退旧主体尺寸。
- 透明 bleed 只负责渲染，不扩大 hover、click、drag 起手或 Card target probe 范围。
- 不绑定按钮，不在 WBP 图里实现 hover / pending / disabled 状态机。
- 材质流光和表面装饰继续走内层 `UWacomCardView` 路径；first-person 反馈走 wrapper 绑定控件，不在 slot widget 内新增 Image / 材质刷新逻辑。

最小 PIE 验收：

- Battle 中 first-person hand 使用该卡面，旋转时边缘没有明显黑边、断线或主体裁切。
- 带实体出血卡框装饰的卡牌，其紧贴接触阴影跟随真实装饰轮廓；Hover / Drag 时接触阴影平滑变软、变淡，并沿卡面倾斜反方向产生可读位移，且没有明显双黑边。
- EffectBadge 框体和耐久底板在 Hover / Drag 时应能看到独立的硬像素局部接触影；数字本身无影。局部影只在卡面上表达附件高度，不应形成第二个宽泛的整卡场景阴影。
- Hover 卡面随卡内 pointer 克制倾斜，按下后倾角减弱；Drag 改由 pointer velocity 产生惯性，pointer 停止后倾角回正但抬升阴影保持到 release。
- 抽牌 / 出牌 / 弃牌 semantic transition 期间卡面逐渐压平；目标候选卡不倾斜，只有 Drag source 消费 fake-3D。
- Battle 真实抽牌在 stagger 等待期应显示牌背，飞行中横向翻成正面并在手牌位置轻压落定；这由 Anchor `18 Card Draw Reveal`、`DA_FPCardDrawRevealStyle_PixelBack` 和单个 `Fake3DSurfaceRetainer` 完成，不需要 WBP 增加 Image、Retainer 或 Animation。牌背图案只映射居中的 `CardContentSizeBox`，费用、耐久、EffectBadge 等出血轮廓继续使用实时 CardView Alpha 的统一牌背边缘色。替换正式牌背时优先改默认或主题 MI 的 `CardBackTexture`，不要改 WBP 层级。
- 回合结束保留牌使用 Anchor `20 Card Retain Seal`、`DA_FPCardRetainSealStyle_Pixel` 与同一个 `Fake3DSurfaceRetainer`。Sealing 后 Held 只保留低强度四角/外缘刻印并轻微抬升，抽牌与左右手生成完成后再由 `RetainedRelease` 解除；左右手 Anchor 本身不播放。WBP 不增加 Image、Overlay、Animation 或第二个 Retainer，`Texture`、Fake3D 与实时 Alpha 接触阴影合同保持不变。
- 普通 Played 成功后卡牌停在提交位置，Commit 脉冲后短促上提并出现一次暖象牙金 / 蓝色像素闪边，随后横向压缩、角度回正并成为发光侧边，默认约 `0.28s`。成功使用后仍留在 Hand 的牌先在提交位置翻到侧边，完全隐藏时切换到最终槽位，再从侧边反向展开；换位帧必须处于 `OpacityMultiplier=0`，不能在屏幕上滑动。横向压缩只作用于最终 RenderTransform，基础 Slot scale 与命中区域保持不变。实际 Exhausted 才播放左下向右上的 OrderedDither 棋盘与短程原色残片。效果完成、强制完成、Slot 复用或 teardown 后必须恢复完整横向比例和基础 Fake3D MID。无目标、世界目标和手牌目标均使用普通 Card Use 行为；可在 Anchor 替换 DiamondWave Style，或分别禁用 Card Use / Exhausted 效果检查 fallback。
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
| `HpBar` | `UWacomProgressBar` | Required | 玩家 HP |
| `ShieldText` | `TextBlock` | Optional | 护盾文本，0 时可隐藏 |
| `StatusList` | `UWacomBattleStatusIconListWidget` | Optional | 玩家 runtime 状态图标行；为空状态时自动隐藏 |
| `DamagePulseSurface` | `Border` | Required for impact feedback | HP 下降时的红色被动覆盖面；必须不可命中 |
| `ShieldPulseSurface` | `Border` | Required for impact feedback | 护盾下降时的蓝色被动覆盖面；必须不可命中 |

正式 WBP 还必须提供 `DamagePulseAnimation` 与 `ShieldPulseAnimation`。两个动画只改变对应 Surface 的表现属性，不能提交命令、插值规则数值或改变输入模式；Root 与全部子控件保持 `HitTestInvisible`。

制作提示：

- 推荐把状态列表实例直接命名为 `StatusList`。C++ 会在 `StatusList` 未绑定时回退查找唯一一个 `UWacomBattleStatusIconListWidget` 子控件，但存在多个状态列表时不会猜测。
- `StatusList` 只在 Snapshot 里有非 `Status.Shield` 状态时显示；单纯配置图标 Brush 不会让 PIE 自动出现状态。
- Action Preview 不需要新增必绑控件。预览激活时，C++ 会用 Battle 规则层产出的 projected player state 覆盖 `HpBar / ShieldText / StatusList` 当前显示，并用可调透明度提示这是预览态；清理后恢复最近一次真实 Snapshot。

WBP 不应做：不提交玩家命令，不修改 BattleSession。

编辑器关闭时可运行 `-run=WacomBuildPlayerStatusUI -BuildImpactFeedback`，为已识别的正式布局幂等补齐两个 Surface 与动画；再次执行不得产生语义修改。`-run=WacomBuildPlayerStatusUI -InspectOnly` 只读检查父类、基础状态栏布局、bindings、动画与命中策略。若同名控件/动画已被人工用于不兼容结构，工具停止并拒绝覆盖。

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
| `PileFeedbackRoot` | `Widget` | Optional | 推荐绑定；统一承载发牌与接收时图标+数字的整体弹性 RenderTransform |
| `ReceiveFeedbackRoot` | `Widget` | Optional | 旧接收反馈绑定；缺少 `PileFeedbackRoot` 时继续作为兼容 fallback |

WBP 推荐结构：用 `PileFeedbackRoot` 包住牌堆 Image 与 `CountText`，并把该节点的 Render Pivot 设为中心 `(0.5, 0.5)`；这样 DrawPile 发牌后坐和 DiscardPile 接收回弹都只修改视觉根节点，不影响 Widget 的 authored Canvas Slot、尺寸或布局。未补新绑定时，C++ 按 `PileFeedbackRoot → ReceiveFeedbackRoot → 整个 UPileCountView` 回退，现有 WBP 仍可工作。自行添加一个 Image 表达抽牌堆 / 弃牌堆 / 消耗牌堆类型，再把 `CountText` 叠到图标合适位置。`LabelText` 和 `FrameBorder` 已不再是制作合同，不要新建或绑定。

WBP 不应做：不修改牌堆或规则状态，不自行计算数量，也不为收发反馈建立会被连续触发反复重启的 UMG Animation。`UPileCountView` 会把 Receive / Send 短脉冲合成到同一个 playback，并在结束、Reset 或 Destruct 时精确恢复 authored RenderTransform；Reduced Motion 只更新数量，不修改 Transform。Send 参数直接在 `WBP_PileCountView` 的 `Wacom|Common UI|Pile Count|Send Feedback` 调整：默认先沿发出方向压缩 `1.5px`，再沿反方向后坐 `3px` 并单次回弹。它既可由真实 Battle `Drawn` Enter Started 驱动 DrawPile 发牌，也可由洗牌牌印真实 Launch 驱动 DiscardPile 离堆；Receive 同理服务普通弃牌抵达 DiscardPile 与洗牌抵达 DrawPile。`SetCount()`、Run 手牌进入或普通 Snapshot refresh 不会自行触发这些反馈。

## Enemy Panel WBP

### BP_WacomBattleEnemyPanelWidget

父类：`UWacomBattleEnemyPanelWidget`

资产路径：`/Game/Wacom/UI/Enemy/BP_WacomBattleEnemyPanelWidget`

用途：敌人 Host 头顶的 screen-space 被动面板。一个实例只消费一个 `FWacomBattleEnemyPanelViewData`；Snapshot 到 ViewData 的转换属于 App-private HUD coordinator，WBP 不读取 `UBattleSession` 或 `FBattleSnapshot`。

必需绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `EnemyNameText` | `TextBlock` | 敌人名称 |
| `EnemyInitiativeText` | `TextBlock` | 当前部位先机合计 |
| `PartList` | `VerticalBox` | 按 Definition / ViewData 顺序承载稳定部位条目 |
| `PanelContextHighlight` | `Widget` | hover 或 Action Preview 上下文强调 |

类默认值：

| 属性 | 正式值 | 运行时职责 |
|---|---|---|
| `PartEntryWidgetClass` | `BP_WacomBattleEnemyPartEntryWidget` | 创建、排序和复用部位条目；为空属于合同错误 |

Panel 按稳定 `EnemySlotId + PartSlotId` 复用条目，只有部位真正移除、战斗清空或 Widget 析构时才销毁缓存。新条目使用 `0.045s` stagger 触发 Intro；Timer 使用弱对象 delegate，并在 clear / destruct 时清理。普通面板正式支持最多 4 个部位；更多部位可运行，但 Validator 会提示改用未来的 Boss 专用 WidgetClass。

WBP 不应做：不读取或修改 `UBattleSession`，不构建 ViewData，不提交命令，也不在图中创建/销毁部位条目。

### BP_WacomBattleEnemyPartEntryWidget

父类：`UWacomBattleEnemyPartEntryWidget`

资产路径：`/Game/Wacom/UI/Enemy/BP_WacomBattleEnemyPartEntryWidget`

用途：展示单个部位的紧凑状态，并在 hover / Action Preview 时展开细节。布局、字体、颜色、九宫格和动画都属于 WBP；C++ 父类只应用 ViewData 和语义状态。

必需绑定：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `PartNameText` | `TextBlock` | 部位名 |
| `HpBar` / `HpText` | `ProgressBar` / `TextBlock` | HP 比例与数值 |
| `ShieldContainer` / `ShieldText` | `Widget` / `TextBlock` | 护盾区域与数值；零护盾折叠 |
| `InitiativeText` | `TextBlock` | 当前先机 |
| `IntentText` / `ResistanceText` | `TextBlock` | 当前意图与抵抗 |
| `DetailsContainer` | `Widget` | hover / preview 展开的细节区域 |
| `StatusList` | `UWacomBattleStatusIconListWidget` | 正式状态图标列表 |
| `ContextHighlight` / `ActionPreviewOverlay` | `Widget` / `Widget` | hover 与 projected 语义 |
| `DestroyedOverlay` | `Widget` | 部位破坏终态弱化层 |

必需动画：`IntroAnimation`、`DamagePulseAnimation`、`ShieldPulseAnimation`、`DestroyedPulseAnimation`、`ContextHighlightAnimation`。C++ 只按语义触发；时长、曲线、颜色和关键帧由 WBP 独占。

刷新语义：

- 紧凑态显示名称、HP、护盾、当前先机和意图；只展开匹配 hover 的 `PartSlotId` 或具有 projected preview 的条目。
- Action Preview 临时覆盖 HP、护盾、先机、状态与 Destroyed 显示；清理后恢复最近一次真实 ViewData。
- Preview 不触发 Damage / Shield / Destroyed pulse。只有真实 Snapshot 值发生变化时才播放；同次刷新命中多个变化时按 `Destroyed > Damage > Shield` 只播最高语义，避免终态叠闪。
- Destroyed 部位保留条目并显示终态覆盖，不改变 Host Downed 或 Trigger retirement 顺序。
- Root 和所有子控件必须不可命中，不能抢占 first-person 卡牌输入或场景目标射线。

WBP 不应做：不提交战斗命令，不反向写入 Snapshot，不读取 Actor 命中，也不自行推断 Preview 或真实伤害。

面板背景使用 `T_UI_PixelPanel_EnemyInfo_9Slice_512x160` 九宫格。Host WidgetComponent 默认启用 Desired Size、Pivot `(0.5, 1.0)`，让 1–4 个部位向上增长；只有显式关闭 Desired Size 时才使用 `EnemyPanelDrawSize` 兼容固定尺寸。Host 的 `EnemyPanelWidgetClass` 优先于 `UWacomUIDeveloperSettings.DefaultBattleEnemyPanelWidgetClass`；两者均无有效类时记录错误并隐藏面板，不创建 C++ 完整布局 fallback。

编辑器关闭时可执行 `-run=WacomBuildEnemyUI -MigrateLegacy`，仅把两个已识别的空壳旧路径迁成正式合同；遇到非空且合同不匹配的人工布局会停止并拒绝覆盖。`-run=WacomBuildEnemyUI -InspectOnly` 只读检查父类、绑定、动画、PartEntry class、命中策略和九宫格资源。该命令不接入 `WacomRegenerateContent`，重复迁移不得 dirty package。

### WBP_WacomBattleEnemySinglePartPanelWidget

父类仍为 `UWacomBattleEnemyPanelWidget`，资产路径为 `/Game/Wacom/UI/Enemy/WBP_WacomBattleEnemySinglePartPanelWidget`。它只适用于恰好一个有效 Definition PartSlot 的普通敌人，消费与多部位面板完全相同的 ViewData。`EnemyNameText / EnemyInitiativeText / PartList / PanelContextHighlight` 合同不变；类默认 `PartEntryWidgetClass=WBP_WacomBattleEnemySinglePartEntryWidget` 且开启 compact presentation。常态折叠名称与聚合 Initiative，hover 或 Preview 时展开名称。

### WBP_WacomBattleEnemySinglePartEntryWidget

父类仍为 `UWacomBattleEnemyPartEntryWidget`，资产路径为 `/Game/Wacom/UI/Enemy/WBP_WacomBattleEnemySinglePartEntryWidget`。除通用 Entry binding 外，正式紧凑布局要求：

| 控件名 | 推荐类型 | 运行时职责 |
|---|---|---|
| `InitiativeDiamond` / `InitiativeText` | `Widget` / `TextBlock` | 红色菱形与正向当前 Initiative 数字 |
| `IntentDiamond` / `IntentIcon` | `Widget` / `Image` | 黑色半透明菱形与准确 Intent 图标 |
| `HpBar` / `HpText` | `ProgressBar` / `TextBlock` | `CurrentHp / MaxHp` 比例；文字仅显示 `CurrentHp` |
| `ShieldContainer` / `ShieldBar` / `ShieldText` | `Widget` / `ProgressBar` / `TextBlock` | `Shield / MaxHp` 比例、准确护盾数字；零时折叠 |
| `DestroyedMark` | `Widget` | Destroyed 时显示 `X`，优先于 Initiative / Intent change pulse |

通用的 `StatusList` 常态可见（无状态时自身折叠），`DetailsContainer` 只在 hover / Preview 展开。新增可选动画 `InitiativePulseAnimation` 与 `IntentChangedAnimation`，只由真实 Snapshot 变化触发；Preview 不触发。所有根节点和子控件必须保持 `HitTestInvisible`，不得获取焦点。

Intent 图标 Style 位于 `/Game/Wacom/UI/Enemy/Intent/DA_EnemyIntentPresentation_Default`。Style 只接受准确、唯一、非空的 `IntentId -> IconBrush`；TrainingWarrior 的 Attack / Guard / Cleave 分别映射剑击、盾牌和横斩像素图标，未知 Intent 使用白色四角星 fallback。不要按 Intent 显示名、Actor 名或 effect 猜图标。

新资产使用隔离的 Editor 命令构建，不会调用或覆盖旧面板迁移：

```powershell
-run=WacomBuildEnemyUI -BuildSinglePartCompact
-run=WacomBuildEnemyUI -InspectSinglePartCompact
```

Builder 只管理带自身合同标记的新 WBP、Style 与四张图标；遇到同路径未知人工内容会停止。重复 Build 必须无语义变更，Inspect 永远只读。

## PIE Smoke Checklist

- `WBP_BattleHUD` 能显示玩家状态、CommandBar、牌堆数量、CombatLogFeed 和 PresentationStack；敌人聚合面板不挂在 HUD Canvas，而挂在 `AWacomBattleEnemyActor` 头顶。
- CommandBar 里的 Wait / EndTurn 可点击并由 HUD runtime view data 控制可用性。
- `WBP_FPCardView` 的 `CardSizeBox` 主体命中范围正确，bleed 画布不扩大交互范围。
- Combat Log 连续追加后可滚动，Presentation Stack 小卡不挡输入。
- 有 `SceneEnemyHostSlots` 的战斗中，每个 `AWacomBattleEnemyActor` 头顶的 EnemyPanel 能按敌人聚合展示所有部位状态；PartActor 只显示 target、drag preview、prediction 等场景反馈，普通部位 hover 使用所属敌人的聚合面板响应。
- TrainingWarrior 自动使用单部位紧凑面板：Attack / Guard / Cleave 图标不同，HP 文本只显示当前值，Shield 为零时收起，hover / 拖卡时展开详情，Destroyed 显示 `X`；多部位 Snake 仍使用原面板。
- `EncounterDefinition` 正式入口必须配置 `SceneEnemyHostSlots`；推荐先执行 `SyncSceneEnemyHostSlotsFromEncounter()` 生成 slots，再逐项填写 Host。缺 Host、漏映射或多余 EnemySlotId 是摆放错误。
- 旧 `WBP_CardWidget / WBP_HandPanel / WBP_EnemyInfoBar / WBP_EnemyPartWidget / EventLogPanel / EventToast / WBP_BattleEnemyPartStatusBadgeWidget` 已删除，不再作为 BattleHUD 制作入口。
