---
type: ui-binding-contract
scope: wacom-ui-battle
status: active
updated: 2026-07-18
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
- 正式 BattleHUD 常驻区域使用 `CombatLogFeed + BattleCombatActivityRow`；完整 `BattleCombatLogBlock` 只供后续详细日志页面复用。旧 `EventLogPanel / EventToast` 已删除。
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
| `CombatLogFeed` | `UBattleCombatLogFeedWidget` | Optional | 三行短时活动播报与“最后行动 + 当前回合” Footer |
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
- `CombatLogFeed` 最多显示三行短时活动；播放结束后只保留最后根行动按钮和当前回合。除最后行动按钮外不遮挡 HUD、手牌或世界目标。
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

用途：first-person card layer 的卡面 wrapper，服务静态预览和 BattleHUD runtime battle hand。它组合通用 `UWacomCardView` 与唯一 Surface Retainer；短时交互反馈由 Slot Motion、Card Depth 与 CardView Slate Paint 驱动，不承接点击命令、目标选择或战斗规则。

推荐结构：

```text
WBP_FPCardView
└─ FPCardViewSizeBox / SizeBox [456 x 520]
   └─ RootOverlay / Overlay
      └─ Fake3DSurfaceRetainer : RetainerBox
         └─ SurfaceCaptureOverlay / Overlay [Clipping = Clip To Bounds - Without Intersecting]
            └─ CardContentSizeBox / SizeBox [360 x 424, Center]
               └─ SurfaceOverlay / Overlay
                  └─ CardView : UWacomCardView
         │  └─ BleedCanvas / SizeBox
         │     └─ CardSizeBox / SizeBox
         │        └─ CardOverlay / Overlay
         │           ├─ CardSurfaceImage / Image（Optional；缺失时 C++ 在底层创建）
         │           ├─ 文本、费用与类型等稳定 UMG 内容
         │           └─ AttachmentParallaxHost（Optional；出血装饰容器）
```

关键绑定 / 命名：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `CardView` | `UWacomCardView` | Optional BindWidget | 通用卡面显示、`FWacomCardViewData` 刷新、主体命中几何来源 |
| `Fake3DSurfaceRetainer` | `RetainerBox` | Optional BindWidget | 卡面唯一 Retainer；常态承载 fake-3D 与实时 Alpha 接触阴影，Played 消散活动期间由 C++ 临时切换专用 Surface-Effect MID |
| `SurfaceCaptureOverlay` | `Overlay` | Retainer direct content by convention | Retainer 完整离屏捕获根；覆盖 `456 x 520` 透明范围并重置继承自视口的普通 culling rect |
| `CardContentSizeBox` | `SizeBox` | Authored by convention | 保持原 `360 x 424` 卡面与反馈层尺寸并居中，避免扩大捕获面时拉伸内容 |
| `SurfaceOverlay` | `Overlay` | `CardContentSizeBox` content | `CardView` 的内容根；不再放置旧交互 Overlay Image |
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

- `WBP_FPCardView` 外层只负责 first-person 包装；通用卡面内容应放在 `CardView` 子控件里。
- `FeedbackOverlay / InteractionFeedbackImage` 已删除，不应重新加入。Pressed 使用 wrapper RenderTransform 与 Card Depth；权威 Commit 只保留运动脉冲。无效目标源卡括角与 formal-release Deny 裂痕当前暂时关闭；App-private Painter 保留为未来重做入口，不要求 WBP 增加节点。
- `WBP_FPCardView` 一张卡只允许一个 Retainer。`CardView` 是 `Fake3DSurfaceRetainer` 内唯一的 authored 卡面内容；嵌套 Retainer 会增加离屏渲染成本并造成刷新时序不稳定。
- `SetEffectMaterial()` 只表示 Retainer 已收到源材质，不代表 Slate 运行时 MID 已建立或新参数已经绘制。C++ 会为 Surface、`CostDigitImage` 和 Badge Digit 分别提交进度 0 Generation，并在 `WBP_FPCardView` 完成一次真实 Paint 后才启动对应 Playback；不要在 WBP Timeline、Construct 或 Binding 中抢先重置这些 Brush/Effect Material。该准备帧不会改变 WBP 层级和 authored 动画时长。
- 三路准备状态由每个 Slot 的 `FWacomFirstPersonCardPresentationReadinessCoordinator` 统一管理；单 Retainer 所有权由 `FWacomFirstPersonCardSurfaceEffectArbiter` 决定。WBP 不负责材质优先级，也不能用 Binding 根据当前效果重复调用 `SetEffectMaterial()`。
- Battle 入场还存在独立的来源级异步预热门。它从 `FirstPersonCardViewClass` 的 Widget Tree Archetype 读取 CardView / Badge 的 Soft Sprite 路径，不实例化 WBP；WBP 无需增加 Preload 节点、Timeline 或 Loading Overlay。相机 staging 与资源预热并行，最多等待 `1.5s`，随后由 HUD 统一解除 hand suppression。
- `SurfaceCaptureOverlay` 必须是 `Fake3DSurfaceRetainer` 的直接内容根，尺寸覆盖完整 `456 x 520` Retainer，Clipping 使用 `Clip To Bounds - Without Intersecting (Advanced)`。`CardContentSizeBox` 保持 `360 x 424` 并居中，四边各留 `48 px` 纯渲染空间。`UWacomFirstPersonCardViewWidget` 会在 Rebuild / Construct 时自动保证直接内容根的 clipping 值，以免 Slate 使用视口 culling rect 提前整批剔除靠近屏幕底边的内容。该结构只改变 Retainer 捕获边界，不移动卡牌、不改变扇形、角度或命中。
- `Fake3DSurfaceRetainer` Effect Material 使用 `/Game/DreamMaterials/Card/M_FirstPersonCard_Fake3D`，Retainer texture parameter 填 `Texture` 并启用效果；该材质来自 `DShader/Material/Card/M_FirstPersonCard_Fake3D.dsm`，Blend Mode 必须是 `AlphaComposite`（DreamShader 源写作 `PremultipliedAlpha`），以符合 Retainer 的预乘 Alpha 合成。C++ Card Depth 参数名固定为 `TiltX`、`TiltY`、`PerspectiveStrength`、`ContactShadowEnabled`、`ContactShadowLift`、`ContactShadowTiltOffsetXUV`、`ContactShadowTiltOffsetYUV`、`ContactShadowOpacityMultiplier`。倾斜 Offset 让阴影沿卡面倾斜反方向移动，来自 Anchor 的 UMG px 参数并按 Retainer 当前尺寸转换为 UV；Opacity Multiplier 默认 `1.5`，可在 Anchor `11 Card Depth` 调整。基础与临时 Surface 材质必须全部保留该合同。当前生产材质不包含 Selection 参数、Noise 采样、轮廓、扫光或 Glint；实体出血卡框只参与实时 `Texture.A` 接触阴影。没有材质或缺少可选绑定时安全退化，不取消 Hover / Drag 或抽弃牌动画。
- `WBP_FPCardView` 的 `CardView` 子控件实际使用 `WBP_FirstPersonCardView`；它与通用 `WBP_CardView` 的核心表面材质都使用 `/Game/DreamMaterials/Card/MI_WacomCardSurfaceComposite_Default`。DreamShader 真源是 `M_WacomCardSurfaceComposite.dsm + WacomCardSurfaceParallax.dsh`，`Scripts/SetupCardSurfacePerspectiveAssets.py` 会幂等同步两个 CardView 资产。C++ 按卡牌实例写入 `ArtTexture / ArtDepthTexture / ArtDepthEnabled`、`RarityTexture / RarityUVScaleBias` 与过滤后的 `TiltX / TiltY / ParallaxStrength`。`BackColorScale=0.96` 并固定在 Frame 平面；插画默认凹入 `-2.5` 源像素。无深度图时，插画实时 Alpha 会在固定 BackColor 上生成硬像素接触投影，默认 `ArtCastShadowEnabled=1 / ArtCastShadowOpacity=0.18 / ArtCastShadowStaticOffset=(0.75,1.0) / ArtCastShadowTiltOffsetPixels=2.680516`；投影只落在 BackColor 安全范围，不扩大卡牌 Alpha。`MaxArtParallaxPixels=4` 和半像素 Clamp 负责阻止极端视差采到边缘。Frame Alpha 另行生成约 `2px` 的静态/方向内沿阴影。可选 `CardIllustrationDepthMap` 使用黑深白浅的五级灰度，导入推荐 `Masks / sRGB=false / Nearest / NoMipmaps / UI`；为空时整层凹入仍正常工作。`RarityBorder` 继续使用 `PaperSprite` Atlas，但位置直接复用 Frame UV，不再具有独立深度；它可通过 `RarityReflectionEnabled` 使用 bevel、foil、虹彩和 glow，但正式默认关闭，内容可按稀有度主题显式开启。`ArtReflectionEnabled / FrameReflectionEnabled / RarityReflectionEnabled` 默认值为 `0 / 1 / 0`。Anchor `11 Card Depth` 的 `CardDragSurfaceParallaxStrengthMultiplier` 默认 `0.75`，通过现有响应滤波同步降低 Drag 的内层 UV 和出血装饰位移；不改变外层 Fake-3D。复合路径有效时旧 `BackColor / CardArt / Frame / RarityBorder / SurfaceFoilOverlay` 自动折叠；材质、插画或插入点无效时恢复旧路径。
- `AttachmentParallaxHost` 是推荐但非必需的制作整理。若后续在 WBP 中加入，应保持完整 bleed 尺寸、`HitTestInvisible`，并在不改变 Canvas/Overlay slot 参数的前提下把四个 `EffectBadgeSlot` 与 `DurabilityHost` 移入其中。当前没有该 Host 时运行时会逐个移动上述控件；无须为了启用第一版效果立即重排 WBP。
- 出血装饰局部阴影不需要 authored WBP 节点。C++ 会在 `BadgeFrameImage` 与 `DurabilityBackIcon` 各自所属的 `Overlay` 中，运行时插入 `BadgeFrameShadowImage_Runtime / DurabilityShadowImage_Runtime`，位置紧邻实体底板之前，并复制源 Brush、ImageSize 与 Overlay Slot 对齐。若源控件不在 Overlay、Brush 无效或装饰隐藏，则对应阴影静默折叠；数字、文字、Glow、Preview/Rewrite MID 不会进入阴影。Anchor `11 Card Depth | Attachment Shadow` 控制颜色、`0.17` 默认透明度、`(2.0, 2.5)` 静态偏移、`0.80` 反向运动比例和 `6px` 上限。默认值通过增加分离距离提升可见度，同时保持低于外部 caster threshold `0.18`；不要在 WBP 中另建同名 Image。
- Card Glyph Transfer 不要求增加 WBP 控件：first-person card layer 会在 RootCanvas 内运行时创建一个 `HitTestInvisible` 的高层级批量 Slate renderer。弃牌堆洗回优先使用 `DiscardPileMotionAnchor / DrawPileMotionAnchor`，普通弃牌的每张起点来自对应 outgoing Slot 的实际中心，目标使用 `DiscardPileMotionAnchor → DiscardPileView`。renderer 会把坐标和内收弧线约束到逻辑 Viewport 安全区（默认边距 36px）；安全区只影响特效路径，不修改 WBP 控件。主体、窄像素拖尾、确定性飘散像素和接收 Impact 合并为一次 `MakeCustomVerts`；默认牌印为 42×66px、拖尾宽度为 10.5→3px、粒子初始边长为 6–13.5px，不生成完整卡背轮廓牌影。普通弃牌先用约 0.11 秒把卡面原地收束成交叉显现的牌印，再以单枚约 0.28 秒、0.055 秒错峰飞入弃牌堆；逐枚抵达会更新数量并播放约 0.12 秒像素方印。洗牌继续使用单枚 0.36 秒、0.045 秒间隔及其可选三段音效；每枚真实发射会让 `DiscardPileView` 减一并反向后坐，每枚真实抵达会让 `DrawPileView` 加一、接收回弹并播放约 0.10 秒小方印，最后一枚方印约 1.18 倍。普通弃牌不复用洗牌音效。默认 Style 为 `/Game/Wacom/UI/Card/SurfaceEffects/DA_FPCardPileTransferStyle_PixelGlyph`；主体、拖尾/粒子/Impact 颜色在 `/Game/DreamMaterials/Card/MI_FirstPersonCard_PileTransferGlyph_Default` 调整，几何、时序和预算在 Style 调整。Anchor 制作入口位于 `14 Card Pile Transfer`，普通弃牌和洗牌分别有启用开关。必要锚点或 Style 无效时普通弃牌回退旧空间 Exit，洗牌 phase 立即完成，均不阻塞后续规则表现。
- Surface 效果不增加第二个 Retainer、Image 或 WBP 动画轨道。普通 Played 默认使用 `/Game/Wacom/UI/Card/SurfaceEffects/DA_FPCardUseEffect_EdgeFlip`，其像素闪边、侧边辉光和短余像参数位于 `/Game/DreamMaterials/Card/MI_FirstPersonCard_SurfaceEffects_EdgeFlipUse_Default`；成功使用后仍在手牌的同一 Card ID 按 `0.22s` 翻出、`0.06s` 隐藏换位、`0.18s` 翻入、`0.04s` 落定，且无手牌锚点时在原位翻回。旧 `/Game/Wacom/UI/Card/SurfaceEffects/DA_FPCardUseEffect_DiamondWave` 与对应 MI 完整保留，可直接替换 Anchor Style 回退。实际 Exhausted 使用 `/Game/Wacom/UI/Card/SurfaceEffects/DA_FPCardPlayedDissolveStyle_OrderedDither`，旧 PixelAsh 方案继续可切换。C++ 只在活动期间切换现有 Retainer，结束后恢复 WBP 创作源材质并重新取得实际 MID；所有材质保持 `UI Domain + AlphaComposite + Texture` 合同。Card Use Material Instance 无效时，离手牌回退旧 Played 空间离场，留在手牌则直接回到最终布局；Exhaust Style / 材质 / 噪声无效时回退 Discarded 空间离场。
- Hand Target Impact 同样不增加 WBP 控件或第二个 Retainer。有效手牌目标预演与成功压印使用 `/Game/Wacom/UI/Card/SurfaceEffects/DA_FPCardHandTargetImpactStyle_PixelStamp` 和 `/Game/DreamMaterials/Card/MI_FirstPersonCard_SurfaceEffects_HandTargetImpact_Default`；参数入口位于 Anchor `15 Card Hand Target Impact`。`CardContentSizeBox` 必须在 `Fake3DSurfaceRetainer` 内保持居中：C++ 只使用 Retainer 与 CardContent 的本地布局尺寸解析 UV Rect，外层手牌位置、扇形角度和 RenderTransform 不得改变该 Rect。材质先使用 Fake3D `ProjectedUV` 采样卡面，再映射为 `CardLocalUV` 绘制 Preview / Commit；刻印只落在卡体与卡面 Alpha 内，不覆盖 Retainer bleed 和出血装饰。Commit 扫光持续越过卡体四边后淡出；不要用扩大 Retainer、Absolute Geometry 或硬编码全屏 UV 半径修正中心和终点。目标被弃牌或消耗时，既有离场会等到约 `0.11s` 刻印峰值再接管唯一 Retainer；Style / MI 无效时无等待地回退原行为。WBP 只需保持现有 `Fake3DSurfaceRetainer`、`CardContentSizeBox`、`SurfaceOverlay` 与 `Texture` 参数合同，不需要 Animation、Image、Overlay 或额外绑定。
- Card Data Rewrite 与目标费用 Preview 都不增加 WBP 层，二者复用 `CostDigitImage` 的同一个临时 UI MID。Preview 模式只绘制预测的一位数 PaperSprite，正式 `Cost` 仍保持旧值；降低费用使用冰蓝/暖金呼吸，提高使用紫红/暗蓝呼吸，取消或提交时恢复正式旧 Sprite。Outcome 阶段中，C++ 在 ViewData 更新前从语义 Hint 的权威 `CostBefore / CostAfter` 解析旧/新 PaperSprite，并把两者的 Atlas Texture 与 UV Rect 写入 Rewrite 模式；不能从当前 Brush 猜旧值。该合同同样适用于左右手 Anchor 的 `CostDigitImage`，Anchor 只加入费用重写，不会被当作普通抽牌、保留牌或 EffectBadge 变化目标。动画结束后恢复权威新 Sprite、ImageSize、RenderTransform 与 Pivot。默认 Style / MI 为 `/Game/Wacom/UI/Card/SurfaceEffects/DA_FPCardDataRewriteStyle_Pixel` 与 `/Game/DreamMaterials/Card/MI_WacomCard_CostDigitRewrite_Default`，制作入口位于 Anchor `16 Card Data Rewrite`。MI 调节 Neutral / Beneficial / Detrimental 色板、约 14 列硬像素网格、边缘宽度与亮度；Style 管理 Preview 呼吸、`0.34s` 消散/停顿/中心重组/回弹时序、错峰与可选声音。该 MID 不替换唯一 `Fake3DSurfaceRetainer`，所以 HandTarget 刻印、Fake3D、视差、反光和接触阴影可并行。不要增加 TextBlock 替身、Overlay、Animation、第二个 Image 或第二个 Retainer。
- EffectBadge Preview/Rewrite 复用现有 `WBP_CardEffectBadge`、`BadgeFrameImage` 和 `DigitHost`，不新增 WBP 层。框体始终保留；只有预测或正式变化的数字 Image 使用 `/Game/DreamMaterials/Card/MI_WacomCard_EffectBadgeFeedback_Default` 临时 MID，结束后恢复普通 PaperSprite Brush。卡面按 Badge Kind 聚合同类型效果，稳定身份为 `Badge.<Kind>`：基础与条件伤害共用一枚 Damage Badge，中毒层数使用独立 Poison Badge；整个语义组都 skipped 时只降低亮度，不绘制额外叉号。默认 Style 是 `/Game/Wacom/UI/Card/SurfaceEffects/DA_FPCardEffectBadgeFeedbackStyle_Pixel`，Anchor 入口为 `21 Card Effect Badge Feedback`。四个 `EffectBadgeSlot` 仍是最终布局槽位，Preview 不覆盖权威 `Value`；正式 Added/Removed 目前没有规则事务来源，WBP 不应自行增删控件来触发动画。
- `T_FirstPersonCard_PlayedDissolveNoise` 导入设置为 `Compression=Masks`、`sRGB=false`、`Filter=Nearest`、`Mip Gen=NoMipmaps`、`LOD Group=UI`。DreamShader 中 `PlayedDissolveNoiseTexture` 的默认 Path 也必须指向这张 Masks 资产，不能使用 Engine `DefaultTexture`，否则 SM6 会报告 `Sampler type is Masks, should be Color` 并导致 Surface 材质错误回退。它只供活动 Surface-Effect MID 使用；原始源图和确定性五级灰度结果保存在 `DShader/Texture/Card/`。
- 正式 Drag 拾牌反馈不新增 WBP 控件，也不使用 `FeedbackOverlay / InteractionFeedbackImage`。它由 Slot 的 Motion Mixer 对整个 wrapper 叠加约 `0.14s` 的局部上提/缩放，并从 Anchor `10 Interaction Feedback | Drag Pickup` 读取声音；因此 `WBP_FPCardView` 无需增加动画轨道、Image 或第二个 Retainer。首次快捷键让无目标卡长距离飞向鼠标时，声音立即播放，局部上提/缩放等卡牌追到 pointer 后再播放；卡牌已在 pointer 附近时仍立即播放。
- 像素棱镜 Style 与 `T_FirstPersonCard_SelectionPixelClusters` 暂作为未来 `CardDataChanged / Upgrade` 效果源保留。纹理导入设置仍为 `Compression=Masks`、`sRGB=false`、`Filter=Nearest`、`Mip Gen=NoMipmaps`、`LOD Group=UI`，但当前 Fake-3D 材质不引用它；复用算法位于 `DShader/Shared/WacomCardPixelPrism.dsh`，不得为了拖拽效果重新接回生产材质。
- Card Depth 材质由 DreamShader 1.4.1 生成，`.dsm` 是长期真源。若 Content 资产缺失，使用 DreamShader commandlet 对上述源文件执行 `compile -Force`；不要在 Unreal 材质图里做无法回写到 `.dsm` 的平行修改。
- 材质图人工复查时，Fake3D 主卡面应有 `RG` 投影 UV、`B` inside mask、`RGB` 卡面颜色、`A` 卡面透明度；接触阴影还应有 9 个 `A` 采样 mask 与 `ContactShadowColor RGB`。若单通道节点仍显示 `RGB` 或 `RGA`，说明资产没有使用带 ComponentMask 默认通道修复的 DreamShader 版本重新生成。
- `WBP_FPCardView` 已删除历史 `ShadowHost / CardShadowImage`；生产链只允许 `Fake3DSurfaceRetainer` 内的实时 Alpha 接触阴影，禁止重新加入与 Card Depth、消散 caster 不同步的宽泛矩形阴影。
- Anchor `10 Interaction Feedback` 直接管理 Pressed 建立/退出时长、Scale、向下位移、接触阴影倍率，以及 Deny shake/四角刻线的颜色、透明度、Inset、Length 与 Thickness。没有交互反馈材质或 WBP Brush override。
- Hover 和无效目标 Probe 不画整卡 tint。只有带有效 Target Handle 的正式无效 release 才触发 Deny 返回运动与可选声音；源卡括角和裂痕当前不绘制。Simplified Motion 继续关闭 Deny 实体运动与 Pressed 运动。
- RootOverlay 使用 `Inherit`；`SurfaceCaptureOverlay` 使用 `Clip To Bounds - Without Intersecting (Advanced)` 并覆盖 `456 x 520` 透明捕获面，`CardContentSizeBox` 维持 `360 x 424` 居中。四边 `48 px` 余量用于倾斜边角、实时阴影和出血装饰，不扩大命中区域。
- `SurfaceFoilOverlay` 只作为旧卡面 fallback 的显式 opt-in 装饰层；核心表面复合材质活动时会折叠它，箔片 / 虹彩应在默认 MI 中调节，避免双重高光。
- `CardView.CardSizeBox` 默认保持 296 x 420，并居中放在 bleed 画布中；缺失时运行时回退旧主体尺寸。
- 透明 bleed 只负责渲染，不扩大 hover、click、drag 起手或 Card target probe 范围。
- 不绑定按钮，不在 WBP 图里实现 hover / pending / disabled 状态机。
- 材质流光和表面装饰继续走内层 `UWacomCardView` 路径；first-person 微反馈由 Slot/Depth/CardView Paint 合成，不新增 wrapper Image 或反馈材质。

最小 PIE 验收：

- Battle 中 first-person hand 使用该卡面，旋转时边缘没有明显黑边、断线或主体裁切。
- 带实体出血卡框装饰的卡牌，其紧贴接触阴影跟随真实装饰轮廓；Hover / Drag 时接触阴影平滑变软、变淡，并沿卡面倾斜反方向产生可读位移，且没有明显双黑边。
- EffectBadge 框体和耐久底板在 Hover / Drag 时应能看到独立的硬像素局部接触影；数字本身无影。局部影只在卡面上表达附件高度，不应形成第二个宽泛的整卡场景阴影。
- Hover 卡面随卡内 pointer 克制倾斜，按下后倾角减弱；Drag 改由 pointer velocity 产生惯性，pointer 停止后倾角回正但抬升阴影保持到 release。
- Pressed 平滑缩到约 `0.985x` 并下沉约 `2px`，松开后平滑归位；成功提交前不出现 optimistic Confirm，权威 Commit 只保留运动脉冲。
- 无效目标 Hover 不出现拒绝提示；正式无效 release 才播放水平阻尼 shake 与四角红色硬像素 L 刻线。Simplified Motion 下不 shake，但刻线仍短促显示。
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

推荐资产路径：`/Game/Wacom/UI/Battle/CombatLog/WBP_BattleCombatLogFeed`

推荐绑定：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `ActivityRowsBox` | `PanelWidget` | Optional | C++ 维护最多三个复用 Activity Row；必须 `HitTestInvisible` |
| `LastActionButton` | `Button` | Optional | Footer 中唯一可点击入口；只广播详细日志打开意图 |
| `LastActionIcon` | `Image` | Optional | 最后一个玩家/敌人根行动图标 |
| `TurnRoot` | `Widget` | Optional | 沙漏与回合数的稳定布局根；始终保留 |
| `TurnIcon` | `Image` | Optional | 默认 Style 提供的中性像素沙漏图标 |
| `TurnText` | `TextBlock` | Optional | 当前表现已推进到的回合数 |

正式资产不再适配旧 `BlocksScrollBox / BlocksBox / TitleText`。`BP_BattleHUD.CombatLogFeed` 必须实际嵌入 `WBP_BattleCombatLogFeed` 生成类，不能只放一个原生 `UBattleCombatLogFeedWidget`，否则不会继承默认 Style、Row Class 和 Footer 图标。编辑器关闭时运行 `-run=WacomBuildCombatActivityUI -Build`，Builder 会把已识别旧 Feed 一次迁移为正式 WBP、三行活动区与 Footer，同时保留有效的人工 Canvas 位置；资产失效时原生 C++ 只提供行为等价的测试/降级布局，不在 PIE 中动态拼接正式 WBP。

配置项：

| 属性 | 用途 |
|---|---|
| `ActivityStyle` | 玩家头像、Intent Style、Tag 图标、fallback 图标与播放时序 |
| `ActivityRowWidgetClass` | 单行 Widget 类，推荐 `WBP_BattleCombatActivityRow` |

WBP 不应做：

- 不自行消费 raw `FBattleEvent`。
- 不提交战斗命令。
- 不替代 `UWacomBattleCombatLogBuilder` 的 ViewData 构造职责。
- 不在按钮点击时直接 Push CommonUI Screen；只把意图交给 `UBattleHUD`。

最小 PIE 验收：

- 玩家出牌显示头像与卡名，敌人行动显示 Intent 图标与名称，多目标结果逐条进入。
- 同时最多三行；队列结束后临时行消失，Footer 的最后行动图标和沙漏回合数保留。
- Root 与行不拦截 Wait、EndTurn、手牌和世界目标；只有最后行动按钮可点击。

### WBP_BattleCombatActivityRow

父类：`UBattleCombatActivityRowWidget`

推荐资产路径：`/Game/Wacom/UI/Battle/CombatLog/WBP_BattleCombatActivityRow`

推荐绑定：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `RowRoot` | `Border` | Optional | 根行动与结果行的暗色像素底板 |
| `ActivityIcon` | `Image` | Optional | 已由 Style 解析完成的图标 |
| `IndentSpacer` | `SizeBox` | Optional | 根行动宽度为 0；结果行由 C++ 设为约 22px 缩进 |
| `ActivityText` | `TextBlock` | Optional | 玩家可读行动或结果文案 |

Row 必须 `HitTestInvisible`，不提供按钮、拖拽或 Battle 命令入口。

### Combat Activity Builder

`WacomBuildCombatActivityUI` 管理正式 Feed、Row、详细 Screen、回合 Divider、默认 Style 和中性有限色像素图标图集。推荐顺序：

1. `-Build`：把已识别的原生 Feed 定向替换为正式 `WBP_BattleCombatLogFeed`、创建缺失资产，并首次把 `BP_BattleHUD.CombatLogFeed` 放到 `(28,122)`、`420×190`。
2. `-InspectOnly`：只读验证父类、绑定、Class Defaults、图标、命中和 HUD 位置合同。
3. 再次 `-Build`：不得产生新的资产差异。

Feed/Row/Details/Divider WidgetTree 属于 Builder 管理。默认 Style 已存在时只补缺失引用，不覆盖人工时序或图标调参。首次 HUD 位置合同建立后，后续构建保留有效的人工 Canvas Offset，只审计尺寸和类型；未知人工旧布局会失败并停止，而不是覆盖。

### WBP_BattleCombatLogDetailsScreen

父类：`UWacomBattleCombatLogDetailsScreen`

推荐资产路径：`/Game/Wacom/UI/Battle/CombatLog/WBP_BattleCombatLogDetailsScreen`

推荐绑定：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `BackdropButton` | `Button` | Optional | 全屏暗色背景；点击面板外关闭，必须不可聚焦 |
| `PanelRoot` | `Widget` | Optional | 左侧 680px 面板命中根，内部空白点击不得关闭 |
| `CloseButton` | `Button` | Optional | 显式关闭入口，必须不可聚焦 |
| `DetailsToggle` | `CheckBox` | Optional | 简略/详细切换；偏好由 Coordinator 在本次战斗内记忆 |
| `HistoryScrollBox` | `ScrollBox` | Optional | 打开后滚动到最新行动 |
| `HistoryList` | `VerticalBox` | Optional | 动态排列回合 Divider、根行动与结果行 |
| `EmptyText` | `TextBlock` | Optional | 空历史显示“暂无战斗记录” |

Screen Push 到 `UI.Layer.GameMenu`，但不继承普通 Menu/UIOnly 基类；它请求 `All + NoCapture`，保持镜头和后台 Battle Presentation。Battle 命令门控属于 HUD Coordinator，不得在 WBP 直接暂停 Session。Esc、右键、Gamepad B、Backdrop 和关闭按钮都走同一幂等关闭路径。

### WBP_BattleCombatLogTurnDivider

父类：`UBattleCombatLogTurnDividerWidget`

推荐资产路径：`/Game/Wacom/UI/Battle/CombatLog/WBP_BattleCombatLogTurnDivider`

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `TurnIcon` | `Image` | Optional | 复用活动 Style 的沙漏图标 |
| `TurnText` | `TextBlock` | Optional | “第 N 回合开始/结束” |
| `DividerLine` | `Image` | Optional | 开始/结束色的硬像素分割线 |

Divider 与 Activity Row 都是只读展示，不提交命令，也不自行访问 Battle Session。

### WBP_BattleCombatLogBlock（旧完整文本兼容）

父类：`UBattleCombatLogBlockWidget`

推荐资产路径：`/Game/Wacom/UI/Battle/CombatLog/WBP_BattleCombatLogBlock`

推荐绑定：

| 控件名 | 推荐类型 | 绑定形状 | 运行时职责 |
|---|---|---|---|
| `HeaderText` | `TextBlock` | Optional | 命令块标题 |
| `DetailsBox` | `PanelWidget` | Optional | C++ 动态填充 detail line |

WBP 合同：

- `SetCombatLogBlockData()` 保存完整 `FWacomBattleCombatLogBlockView` 并触发 `BP_OnCombatLogBlockUpdated`。
- WBP 可读取 `VisualTone / IconKey` 调整样式。
- 命令块只是显示组件，不提交战斗命令。新详细页面使用回合分区与 Activity Row，不再复制这条旧文本式图标解析路径。

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
| `VitalsTrackImage` | `Image` | Optional（正式 V2 必须） | 直接 UI 材质绘制权威 HP、延迟伤害、行动预测、低血和护盾外框 |
| `HpValueText` | `TextBlock` | Optional（正式 V2 必须） | 居中显示 `当前 HP / 最大 HP`；Preview 显示 projected HP |
| `ShieldValueRoot` | `SizeBox` 或其它 Widget | Optional（正式 V2 必须） | 右侧固定宽度的护盾数值根；护盾为零时 Hidden 而非 Collapsed |
| `ShieldText` | `TextBlock` | Optional | 护盾绝对数值，不显示护盾进度 |
| `StatusList` | `UWacomBattleStatusIconListWidget` | Optional | 玩家 runtime 状态图标行；为空状态时自动隐藏 |
| `HpBar` | `UWacomProgressBar` | Optional legacy fallback | 缺少 V2 材质时保证 HP 仍可读；正式 V2 WBP 不依赖它 |

正式 V2 WBP 不再提供 `DamagePulseSurface / ShieldPulseSurface` 或对应 UMG Animation。`UPlayerStatusBar` 的私有 Playback 只在 HP 延迟条、护盾反馈活动时推进，`VitalsTrackImage` 的材质参数负责局部表现；Root 与全部子控件保持 `HitTestInvisible`。

制作提示：

- 推荐把状态列表实例直接命名为 `StatusList`。C++ 会在 `StatusList` 未绑定时回退查找唯一一个 `UWacomBattleStatusIconListWidget` 子控件，但存在多个状态列表时不会猜测。
- `StatusList` 只在 Snapshot 里有非 `Status.Shield` 状态时显示；单纯配置图标 Brush 不会让 PIE 自动出现状态。
- Action Preview 不降低整个状态栏透明度。材质同时显示权威 HP 与 projected 增减区段，中央文本和护盾数值显示 projected 值，`StatusList` 显示 projected statuses；清理后恢复最近一次真实 Snapshot，且不播放真实受击动画或音效。

WBP 不应做：不提交玩家命令，不修改 BattleSession。

编辑器关闭时可运行 `-run=WacomBuildPlayerStatusUI -BuildVitalsV2`，幂等重建已识别的状态栏、删除旧双脉冲动画、把 `BP_BattleHUD.PlayerStatusBar` 放到左上角 `(28,24)` 并把状态图标调整为 `32×32`。`-InspectOnly` 只读检查父类、bindings、材质、HUD Canvas 位置、图标尺寸和命中策略。旧 `-BuildImpactFeedback` 只作为命令行兼容别名保留，实际执行同一 V2 构建。

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

### 通用 Panel：单段与多段

`BP_WacomBattleEnemyPanelWidget` 和 `WBP_WacomBattleEnemySinglePartPanelWidget` 都继承 `UWacomBattleEnemyPanelWidget`，消费同一份 `FWacomBattleEnemyPanelViewData`。单部位默认类仍由 Host 自动选择；显式 Host override 优先。两者都必须提供：

| 控件名 | 类型 | 运行时职责 |
|---|---|---|
| `EnemyNameText` | `TextBlock` | 兼容绑定，紧凑条中常态折叠 |
| `EnemyInitiativeText` | `TextBlock` | 兼容绑定，紧凑条中常态折叠 |
| `PartList` | `HorizontalBox` | 按 Definition / ViewData 顺序承载等宽部位段 |
| `PanelContextHighlight` | `Widget` | hover / projected context 的非交互强调 |

`PartList` 每个 child slot 由 C++ 设置 `Fill + zero padding`；HP 较少的部位也获得完整段宽。Panel 按稳定 `EnemySlotId + PartSlotId` 复用条目，只有部位真正移除、Battle clear 或 destruct 才移除。多部位普通面板支持 2–4 段；更多部位使用未来 Boss WidgetClass。Root 必须是 `SelfHitTestInvisible`，Panel 自身不建立拦截层；`PartList` 及其直到 Root 的所有祖先都必须是 `Visible` 或 `SelfHitTestInvisible`，因为动态加入的 Part Entry 需要穿过这条 Slate 命中路径。

### 通用 Part Entry：分段生命条

`BP_WacomBattleEnemyPartEntryWidget` 和 `WBP_WacomBattleEnemySinglePartEntryWidget` 都继承 `UWacomBattleEnemyPartEntryWidget`。单部位约 `250 × 84`，多部位只是横向等宽复用同一个语义条目。

| 控件名 | 类型 | 运行时职责 |
|---|---|---|
| `InitiativeDiamond` / `InitiativeText` | `Widget` / `TextBlock` | 段上方当前 Initiative；旧多部位资产也应提供 |
| `IntentDiamond` / `IntentIcon` | `Widget` / `Image` | 段上方准确 Intent 图标；未知映射显示 Style fallback |
| `HpBar` / `HpText` | `ProgressBar` / `TextBlock` | 本段 `CurrentHp / MaxHp`；文字只显示当前 HP |
| `ShieldContainer` | `Overlay` 或等价 Widget | 不参与段宽；Shield 为 0 时整体折叠 |
| `ShieldFrame` | `Image/Border` | 引用 `T_UI_EnemyShieldFrame_9Slice`，DrawAs Box 的蓝色外框 |
| `ShieldBadge` / `ShieldText` | `Widget` / `TextBlock` | 引用 `T_UI_EnemyShieldBadge` 的盾牌徽章与准确护盾数字 |
| `StatusList` / `StatusOverflowText` | `UWacomBattleStatusIconListWidget` / `TextBlock` | 段下方最多 3 枚；独立文本显示 `+N`，不要求修改共享 StatusList WBP |
| `InspectHitTarget` | `Button` | 唯一命中热区；只上报完整 `FBattlePartSlotIdentity` |
| `PartNameText` / `IntentText` / `ResistanceText` / `DetailsContainer` | 对应旧类型 | 兼容必绑但在紧凑条中折叠；完整文字由详情面板显示 |
| `ContextHighlight` / `ActionPreviewOverlay` / `DestroyedOverlay` / `DestroyedMark` | `Widget` | hover、Preview 与终态覆盖；Destroyed 不移除段 |

不再制作或使用独立 `ShieldBar`。Shield Frame / Badge 由 projected ViewData 同步显隐，但 Preview 不触发真实 Shield pulse。必需动画仍为 `IntroAnimation`、`DamagePulseAnimation`、`ShieldPulseAnimation`、`DestroyedPulseAnimation`、`ContextHighlightAnimation`；单部位资产继续提供 `InitiativePulseAnimation` 与 `IntentChangedAnimation`。Entry Root 是 `SelfHitTestInvisible`，仅 `InspectHitTarget` 可命中；Runtime 只在 Idle、无拖卡、无 Preview、无表现结算时启用按钮。`InspectHitTarget` 到 Entry Root 的每一级容器都必须允许子控件命中，装饰面可以保持 `HitTestInvisible`，但不能作为热区祖先。WBP 不自行设置 viewport ZOrder；Host 的 `WidgetComponent` 统一使用专用 `WacomBattleEnemyPanelScreenLayer@8000`，详情 WBP 位于 `8500`。禁用检查时 Entry 与热区必须回到 `HitTestInvisible`，否则会阻断 BattleHUD 的世界目标点击。

Intent 图标 Style 仍位于 `/Game/Wacom/UI/Enemy/Intent/DA_EnemyIntentPresentation_Default`，只接受准确 `IntentId -> IconBrush`，不得按显示名或 effects 猜图标。

### WBP_WacomBattleEnemyInspectionWidget

父类：`UWacomBattleEnemyInspectionWidget`。资产路径：`/Game/Wacom/UI/Enemy/WBP_WacomBattleEnemyInspectionWidget`。它是 BattleHUD 唯一的非模态 viewport 详情实例，不读取 Session，也不向规则层提交命令。

| 控件名 | 类型 | 运行时职责 |
|---|---|---|
| `LeftPanel` / `RightPanel` | `Widget` | 双侧可动画容器；中央区域保持空且不可命中 |
| `EnemyNameText` / `EnemyStateText` | `TextBlock` | 敌人名与剩余/已击破整体状态 |
| `PartNavigator` | `PanelWidget` | Definition 顺序的稳定部位导航 Row |
| `SelectedPartNameText` | `TextBlock` | 当前部位名 |
| `HpBar` / `HpText` | `ProgressBar` / `TextBlock` | 当前/最大 HP 详情 |
| `ShieldContainer` / `ShieldText` | `Widget` / `TextBlock` | 准确 Shield；零值折叠 |
| `InitiativeText` | `TextBlock` | 当前 Initiative |
| `IntentText` / `ResistanceText` | `TextBlock` | Intent 名称，以及 Intent Initiative / Resistance |
| `StatusList` | `UWacomBattleStatusIconListWidget` | 完整 Buff 与层数，不限制 3 枚 |
| `DestroyedOverlay` | `Widget` | 当前部位终态 |
| `CloseButton` | `Button` | 被动 Close 请求 |

必需动画：`OpenLeftAnimation`、`OpenRightAnimation`、`CloseAnimation`，都必须有真实 widget binding。Root 为 `SelfHitTestInvisible`；只有 `CloseButton` 和部位 Row 按钮可命中。`PartNavigator`、`CloseButton` 及其祖先链必须允许子控件命中。开始拖卡、进入 TargetSelect / Resolving、BattleEnd、Host/Part 移除和 HUD destruct 会由 coordinator 关闭或清理。

### WBP_WacomBattleEnemyInspectionPartRowWidget

父类：`UWacomBattleEnemyInspectionPartRowWidget`。必需绑定为 `PartSelectButton`、`PartNameText`、`HpText`、`ShieldContainer`、`ShieldText`、`InitiativeText`、`SelectionHighlight`、`DestroyedOverlay`。Root 为 `SelfHitTestInvisible`，仅 `PartSelectButton` 可命中；按钮到 Root 的祖先链必须允许子控件命中，点击只广播稳定 Part identity。

资产修改只通过受控 Editor / MCP 写入。只读合同检查使用：

```powershell
-run=WacomBuildEnemyUI -InspectSegmentedVitals
```

该模式验证四个紧凑 WBP、详情 WBP / Row、Shield Frame / Badge、父类、required bindings、动画、HorizontalBox 和命中策略；永远不修改资产。旧 `-MigrateLegacy / -BuildSinglePartCompact` 只服务 v1 生成资产，不得覆盖标记为 segmented v2 的人工正式布局。

单部位几何由 Panel 独占：`SinglePartPanelRoot` 固定宽度 `250`，Entry 不得再声明宽度或最小宽度，`CompactSize` 只保留 `84` 的行高。这样 `WidgetComponent.DrawAtDesiredSize` 与运行时 `HorizontalBox Fill` 只有一个水平尺寸真相，避免固定宽度、最小宽度和 Fill 叠加导致拉伸或空白。`-InspectSegmentedVitals` 会验证这一合同。

## PIE Smoke Checklist

- `WBP_BattleHUD` 能显示玩家状态、CommandBar、牌堆数量、三行 Combat Activity Feed 和 PresentationStack；敌人聚合面板不挂在 HUD Canvas，而挂在 `AWacomBattleEnemyActor` 头顶。
- CommandBar 里的 Wait / EndTurn 可点击并由 HUD runtime view data 控制可用性。
- `WBP_FPCardView` 的 `CardSizeBox` 主体命中范围正确，bleed 画布不扩大交互范围。
- Combat Activity 连续追加时最多三行并按顺序顶出；只有 Footer 最后行动按钮可命中，Presentation Stack 小卡不挡输入。
- 有 `SceneEnemyHostSlots` 的战斗中，每个 `AWacomBattleEnemyActor` 头顶的 EnemyPanel 能按敌人聚合展示所有部位状态；PartActor 只显示 target、drag preview、prediction 等场景反馈，普通部位 hover 使用所属敌人的聚合面板响应。
- TrainingWarrior 自动使用单部位紧凑面板：Attack / Guard / Cleave 图标不同，HP 文本只显示当前值，Shield 为零时收起，hover / 拖卡时展开详情，Destroyed 显示 `X`；多部位 Snake 仍使用原面板。
- `EncounterDefinition` 正式入口必须配置 `SceneEnemyHostSlots`；推荐先执行 `SyncSceneEnemyHostSlotsFromEncounter()` 生成 slots，再逐项填写 Host。缺 Host、漏映射或多余 EnemySlotId 是摆放错误。
- 旧 `WBP_CardWidget / WBP_HandPanel / WBP_EnemyInfoBar / WBP_EnemyPartWidget / EventLogPanel / EventToast / WBP_BattleEnemyPartStatusBadgeWidget` 已删除，不再作为 BattleHUD 制作入口。
