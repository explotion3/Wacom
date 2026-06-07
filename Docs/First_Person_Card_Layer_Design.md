---
type: presentation-contract
scope: wacom-first-person-card-layer
status: active
updated: 2026-06-06
tags:
  - wacom/ui
  - wacom/cards
  - wacom/first-person
  - wacom/battle
  - wacom/run
---

# First-person Card Layer 文档

> [!info] 本文职责
> 本文记录第一人称卡牌层的当前制作与运行时合同：正式布局默认、Battle / Run 数据源、hover / inspect / drag、target / drop 边界、legacy / prototype 隔离和 `WBP_FirstPersonCardView` 制作要求。

> [!warning] 边界
> 第一人称卡牌层只做 UI 表现和玩家意图桥接。Battle 规则见 [WacomBattle.md](./WacomBattle.md)，Run 规则见 [WacomRun.md](./WacomRun.md)，世界 target / drop 路由见 [WacomWorldInteraction.md](./WacomWorldInteraction.md)，BattleHUD 命令出口见 [WacomBattleUI.md](./WacomBattleUI.md)。

## §1 正式方向

正式主手牌方向是 HUD-rendered first-person card layer：

```text
Battle / Run snapshot
-> card presentation data
-> HUD / UMG card widgets
-> first-person anchor projects hand center
-> card layer applies authored 2D layout and slot motion
```

卡牌仍由 UMG 渲染，因此保留 HUD 级别清晰度、材质动画和 WBP 可迭代性。第一人称感来自 anchor 投影、身体 / tunnel / battle base rotation、soft clamp、hand center smoothing 和 slot motion，而不是把常驻手牌做成 `WidgetComponent`。

## §2 核心对象

| 类型 | 职责 | 不负责 |
|---|---|---|
| `UWacomFirstPersonCardAnchorComponent` | 计算第一人称手牌 anchor、layout config、projection / clamp、slot config 和 runtime source ownership | 不持有卡牌 Widget，不提交 Battle / Run 命令 |
| `UWacomFirstPersonCardLayerWidget` | 按 entries reconcile slot widget，维护 active / outgoing slot，绘制 drag arrow 和 layer-level feedback | 不读取 Battle / Run 规则状态 |
| `UWacomFirstPersonCardLayerSlotWidget` | 持有单卡 `UWacomCardView`，处理 hover / press / inspect / drag gesture 和 visual slot motion | 不直接调用 BattleSession 或 RunSession |
| `UWacomRunFirstPersonCardSourceComponent` | 探索期把 Run BattleDeck / menu lease 写入 anchor runtime source | 不提交 Run 规则 |
| `FWacomBattleHUDFirstPersonHandBridge` | BattleHUD 内部同步 battle hand、transition hints、drag preview / release | 不暴露 Blueprint API |
| `FWacomBattleHUDCardDetailController` | first-person viewport 详情数据、motion、定位和 teardown | 不改变卡牌规则 |

## §3 Authoring 默认

默认 first-person hand authoring surface 是：

```text
ProjectionMode = BodyLocked
Layout = fixed Authored2D
ViewportClampMode = SoftClampToViewport
```

`BodyLocked` 使用 Battle base rotation 或 Run Tunnel spline base 作为稳定身体基准，不让 cursor look 重新计算卡牌世界槽位。投影仍使用当前真实相机，因此鼠标移动镜头时仍有合理第一人称空间变化。

`Look Responsive Projected` 是保留的次级投影风格。它会把 `UWacomCursorLookDriverComponent` 的当前鼠标镜头偏移按 `LookInfluenceYaw / LookInfluencePitch` 混入手牌锚点，再使用当前真实相机投影。这样 look 同时影响 anchor 计算和相机投影，适合需要更强跟随感、空间漂移或视差感的手牌表现；代价是稳定性和可读性弱于 `BodyLocked`，需要按具体场景单独调参。C++ 枚举值仍叫 `LegacyWorldProjected`，只是为了蓝图 / 资产序列化兼容。

第一人称手牌运行时固定使用 `Authored2D`：只投影整副手牌中心点。每张卡的位置、下坠、扇形角度、层级和主体底部可读保护由 2D 参数计算，避免每张卡分别世界投影导致尺寸和扇形拉扯。旧的 `LegacyProjectedFan2D` 每卡 3D 槽位分别投影路径已清理，不再作为 PIE / debug comparison 入口。

`SoftClampToViewport` 允许手牌中心部分离开视口，超过 soft allowance 后再柔性拉回，保留空间感。`HardClampToViewport` 用于复现旧的始终屏内行为，`AllowOffscreen` 用于验证最接近空间物体的表现。

`UWacomFirstPersonCardLayoutPreset` 是 first-person hand 表现调参 DataAsset，位于 `WacomApp`，不是 `WacomData` 规则数据。Preset 运行时生成 resolved config，不把值写回组件 UPROPERTY，也不覆盖 `FirstPersonCardViewClass`、prototype preview、debug 开关或 viewport ZOrder。

`UWacomFirstPersonCardAnchorComponent.FirstPersonCardLayoutPreset` 是组件默认 / fallback preset。Battle 和 Run 可以在各自 source 激活时声明 source-owned runtime preset override：`UBattleHUD.BattleFirstPersonCardLayoutPreset` 用于战斗手牌，`UWacomRunFirstPersonCardSourceComponent.RunFirstPersonCardLayoutPreset` 用于探索默认源和菜单租约。Runtime override 只在 `RuntimeCardLayerSourceId` 与 override `SourceId` 相同时参与 resolved config；例如 BattleHand 清理晚于 Run source 重启时，Battle 残留 preset 不会继续驱动 Run hand，也不会误清 Run 的 preset。

`UWacomFirstPersonCardLayoutPreset` 已接入 Editor DataValidation。会导致布局不可见、数学异常或范围非法的值会报 Invalid，例如非正缩放、非法 viewport anchor、像素吸附网格小于等于 0、edge drop 数量范围反向。能运行但调参风险较高的值只报 Warning，例如 BodyLocked 下配置了 LookInfluence、Look Responsive influence 偏高、scale / edge drop / fan angle 过大，或启用 smoothing / slot motion 但速度为 0。

`LookInfluenceYaw / LookInfluencePitch` 也属于 first-person hand 表现参数，可由 `UWacomFirstPersonCardLayoutPreset` 覆盖。推荐制作起点：战斗默认使用 `BodyLocked`；探索、特殊检查或希望手牌有更强空间跟随感的场景可使用 `Look Responsive Projected`。`LookInfluenceYaw` 建议先在 `0.05-0.35` 内调，`LookInfluencePitch` 建议先在 `0.03-0.20` 内调；如果手牌在移动鼠标时过度漂移、读牌不稳或与镜头响应产生二次晃动，优先降低这两个值，再调整 clamp / smoothing。

Anchor debug view 会同时报告 `ResolvedLayoutPresetName`、`bUsingRuntimeLayoutPresetOverride`、`RuntimeLayoutPresetOverrideSourceId`、`RawCursorLookOffset`、`AppliedAnchorLookOffset`、`LookInfluenceYaw`、`LookInfluencePitch` 和 `bLookResponsiveProjection`。排查时可以用它区分“鼠标确实产生了 look offset”与“该 offset 是否被当前 ProjectionMode / Preset 应用到 hand anchor”，以及当前 resolved preset 来自组件默认值还是当前 runtime source 的 override。Automation view 仍会暴露当前存储的 override owner / asset；它可能是等待原 owner 清理的残留值，不代表 resolved config 正在使用它。

边缘下坠是纯表现参数，不影响战斗规则、手牌数量或卡牌状态。`StaticCardEdgeDropPixels` 表示大手牌时最外侧卡牌的最大下坠；默认开启 `bScaleEdgeDropByHandCount` 后，5 张及以下使用 `ShortHandEdgeDropPixels`，12 张及以上使用 `StaticCardEdgeDropPixels`，中间数量用 SmoothStep 平滑过渡。左右手锚点牌在规则上承担手牌区域切分语义，但在 first-person hand 表现层仍按普通卡牌参与下坠、缩放、扇形角度和层级计算。推荐起点是 `ShortHandEdgeDropPixels = 64`、`StaticCardEdgeDropPixels = 110`、`EdgeDropScaleMinCardCount = 5`、`EdgeDropScaleMaxCardCount = 12`。

Hover 命中滞后是纯输入表现参数，不影响战斗规则、手牌数量或卡牌状态。`HoverHitHysteresisPixels` 默认 `16`，建议调参范围 `0-64` UMG 布局像素；它只用于卡牌基础命中分界线附近的 hover 稳定性，避免鼠标贴着重叠边界移动时频繁抖动切卡。

## §4 Runtime Source

Runtime source 优先级：

| Source | 写入者 | 行为 |
|---|---|---|
| Battle runtime hand | `UBattleHUD` first-person hand bridge | 使用 `FBattleSnapshot.Hand.Cards`，启用 battle hand hover / click / hold / drag |
| Run default source | `UWacomRunFirstPersonCardSourceComponent` | 探索期显示 Run BattleDeck 物理卡和可选投影卡，只读展示 |
| Run menu lease | `UWacomMenuWidgetBase` / owning menu | GameMenu 内临时显示候选持有卡，可启用 hold / drag 到 menu Zone |
| Static preview | Anchor prototype preview | PIE / 开发验证，不是 Battle / Run runtime source |

进入战斗时，GameMode / PlayerController 会清理探索期 Run source 和 active menu lease。退出战斗回到 Exploration 后，PlayerController 重新激活 Run first-person source 并刷新当前 BattleDeck 展示。

BattleHUD 的 first-person hand bridge 只拥有 `BattleHand` runtime source。清理或 `NativeDestruct` 可能晚于 Run source 重新激活，因此 BattleHUD 解绑自身 delegate 时必须检查 Anchor 当前 `RuntimeCardLayerSourceId`：只有仍为 `BattleHand` 时才关闭 first-person card interaction、取消拖拽和清 runtime data；如果已经被 `RunFirstPersonBattleDeck` 或 menu lease 接管，只能解绑 BattleHUD delegate 和清战斗 world preview，不得改写 Run source 的交互状态。

Layout preset ownership 与 runtime card data ownership 使用同一套 source 语义，但互相独立：BattleHand 同步时可设置 `BattleFirstPersonCardLayoutPreset`，Run default source / Run menu lease 同步时可设置 `RunFirstPersonCardLayoutPreset`，GameMenu suppression 空 source 不占用 preset。清理时只有 matching `SourceId` 能清自己的 preset override，避免 Battle / Run 生命周期交错时出现 preset 被旧 source 误清。

推荐配置：

- Battle hand preset：稳定 `BodyLocked`，优先保证读牌、拖拽目标和战斗反馈稳定。
- Run preset：可选 `Look Responsive Projected`，使用较低 `LookInfluenceYaw / LookInfluencePitch` 做探索期视差。
- Run menu lease：当前跟随 Run preset；如果后续菜单需要更平面的读卡风格，再单独引入 menu lease preset 字段。

BattleHUD runtime 战斗手牌不再有 legacy 2D hand 可见性恢复路径。退出战斗后的手牌恢复只依赖 Run source ownership 交接，不能通过旧 2D hand 兜底。

打开 Backpack / Pause / Shop / RunEvent 等 GameMenu 时，默认压制 Run default source，避免卡层遮挡菜单。菜单需要卡牌交互时，应显式申请 owned menu lease。

## §5 Slot Motion 与 Transition

Layer 使用稳定 motion key 复用 slot widget：

- Battle runtime hand 优先使用 `CardInstanceId`。
- Static preview 或 placeholder 使用 `StaticIndex:{Index}`。
- 同一 key 重新进入时复用 active 或回收 outgoing widget，避免幽灵 widget。

Anchor 每帧计算目标 slot，Layer 只在输入 slot、transition hint、配置或生命周期状态实际变化时完整 reconcile。Slot motion 独立 tick，把 visual position、angle、scale 和 opacity 追向 target slot。

默认表现：

- 新卡按 enter offset 和 enter opacity 淡入。
- 移除或投影失败的可见卡进入 outgoing，短暂淡出 / 下滑后移除。
- 目标大跳变超过 reset distance 时直接贴合。
- Drawn / Gained / Played / Discarded transition hint 只改变表现来源或离场方向，不改变 snapshot 真相、命令路径或 slot key。

Layer debug view 记录 active / outgoing / RootCanvas child / ticking slot 和本次刷新创建、复用、移除、异常修复数量。诊断日志默认关闭，只在手动排查时开启。

## §6 Battle 交互

第一人称战斗手牌不恢复旧 2D hand 的拖拽语义，也不直接提交 `UBattleSession`。

当前交互：

| 手势 | 行为 |
|---|---|
| hover idle card | 可查看详情；可打卡获得 lift / scale / ZOrder，不可打卡只允许读牌 |
| click playable card | 转发 `CardInstanceId` 到 `BattleHUD->OnCardClickedByUser()` |
| click unplayable card | 播放 deny feedback，不广播 click intent |
| hold | 超过读牌延迟后进入 inspect，源卡移动到读牌位置并显示 first-person detail，松开不提交 |
| drag no-target card | 向上超过 commit distance 后 armed，release 才提交无目标出牌 |
| drag targeted card | 源卡保持 selected-source，C++ arrow 指向鼠标；release 到合法 target 后提交 |

需要敌方部位目标的卡 release 到合法 world enemy part 后，BattleHUD 调用现有 play-card world target 路径。需要手牌目标的卡 release 到合法 first-person card target 后，BattleHUD 提交 hand-card target。UI 手势层只提交 target identity，不判断加费、减费、弃置或消耗规则。

悬浮和拖拽期间，卡牌层都会记录 DPI-aware widget-space 指针和归一化视口坐标。普通 hover 的 mouse move 被 UMG 处理后，`UWacomFirstPersonCardLayerWidget` 会广播 `FWacomFirstPersonCardPointerView`，由 BattleHUD 写入 Battle camera look 临时 override，探索 / Run 则由 `AWacomPlayerController` 写入 Run Tunnel cursor look override。因此鼠标在悬浮放大的卡牌上移动时，镜头仍会跟随当前指针，不会等离开卡牌后突然跳到新位置。

拖拽过程中仍保留 UMG mouse capture，并继续通过 `FWacomFirstPersonCardDragView` 传递拖拽指针。拖拽镜头旧参数 `bAllowCameraLookDuringCardDrag`、`CardDragCameraLookScale`、`CardDragCameraLookInterpSpeedOverride` 继续只控制 drag override，保持既有资产兼容；hover / pointer 通用路径使用 `bAllowCameraLookDuringCardPointer`、`CardPointerCameraLookScale`、`CardPointerCameraLookInterpSpeedOverride`，也可由 `UWacomFirstPersonCardLayoutPreset` 覆盖。拖拽 active 时会清空并压制普通 pointer view，避免 hover override 与 drag override 抢同一个 camera look 状态。

Hover 输入命中与 hover 视觉几何分离。Anchor 构建 slot view 时会先写入稳定输入几何：`InputHitCenter`、`InputHitScale`、`InputHitAngleDegrees`、`InputHitOrder`，再应用 hover / pending 的视觉 lift、scale 和 ZOrder。悬浮卡仍然可以放大、抬升并绘制在上层，但鼠标命中哪张牌由 `UWacomFirstPersonCardLayerWidget` 使用基础几何统一解析，不再由被 Slate 命中的单个 slot 自行决定。

稳定命中 resolver 只考虑 projected、非 exiting、有效 `CardInstanceId` 且可交互的 active slot。透明 bleed 不参与命中，命中范围仍只使用 `UWacomCardView.FixedCardBodyHitSize` 对应的 card body bounds。多张基础 body 重叠时，不按 hover 后 ZOrder 抢输入，而是按手牌左右顺序的相邻中心线分界选择；当前 hover 卡在分界线附近享有 `HoverHitHysteresisPixels` 滞后区，越过滞后区后才切到相邻卡。

Hover、click / press 起手、drag 起手和拖拽中的 card target probe 复用同一个稳定命中 resolver。这样即使视觉上放大的悬浮卡覆盖了右侧卡，鼠标移动到右侧卡的基础区域时也能自然切到右侧卡；拖拽探测手牌目标时会排除源卡，并继续保留现有 CardTarget probe / valid / invalid feedback 语义。

拖拽到手牌目标时，稳定命中 resolver 只负责解析目标 `CardInstanceId` 和目标 slot identity，不负责判断目标是否合法。BattleHUD 继续通过 `UBattleSession::ValidateTargetWithCard()` 校验，并把 `CardProbe`、`ValidCardTarget` 或 `InvalidCardTarget` 下发回 first-person card layer。同一拖拽目标上的 `ValidCardTarget / InvalidCardTarget` 会持续保留；只有指针切到另一张目标卡、离开 card body，或当前状态还没有 HUD 校验结果时，layer 才回到本地 `CardProbe` 等待下一次校验。

Card target 选中反馈拆成候选 affordance 和当前 focus 两层，不复用普通 hover。Affordance 表示一张手牌是否可作为当前源卡的目标，只负责绿色 / 红色 overlay，可多张同时存在；focus 表示拖拽指针当前命中的唯一目标卡，才应用 focus lift、scale 和 ZOrder boost。`CardProbe / ValidCardTarget / InvalidCardTarget` 都可作为 focus 状态，并继续用 overlay 颜色区分 probe、valid、invalid；该状态不调用普通 hover，不广播 hover / detail delegate，不写 `HoveredCardInstanceId`，也不影响 hover 稳定命中。像“加费测试”这类允许普通手牌和左右手锚点的 HandCard 目标卡，会显示全手牌合法候选全绿，但只有指针压中的那张卡获得唯一 focus。拖拽进入 active 状态后会清空并压制普通 hover，pointer enter / move 只更新拖拽手势和 CardTarget probe；release / cancel 当帧不会立即恢复 ordinary hover，避免目标卡 focus 与 hover lerp 叠加造成双重放大或双重缩小。`bEnableDragTargetFeedback` 只控制 probe / focus / overlay 的显示；关闭后拖拽目标 identity 仍可被解析并交给 HUD / Battle 规则处理。

Drag-target focus 默认参数为 `DragCardTargetFocusLiftPixels = 18`、`DragCardTargetFocusScale = 1.045`、`DragCardTargetFocusZOrderBoost = 650`。推荐调参范围：lift `0-48` UMG 布局像素、scale `1.0-1.12`、ZOrder boost `0-1400`。这些都是表现层参数，不改变手牌顺序、牌堆、出牌结算或 HandCard target validation。

## §7 Target / Drop 边界

First-person drag target 来源：

| Target | 来源 | 提交方 |
|---|---|---|
| Battle world enemy part | PlayerController 按 drag widget position 做 world probe，命中当前 SceneEnemyHost registry 的 part handle | BattleHUD command flow |
| Battle hand card | First-person card layer 的 card target bridge | BattleHUD command flow |
| Run world object | PlayerController probe `Interaction.Target.Run.Object` | Run world card drop receiver + RunSession |
| Run menu zone | Registered `UWacomRunMenuDropTargetWidget` | Owning menu submit policy |

Battle enemy world target 只来自当前 SceneEnemyHost registry 中的 PartActor / WorldTargetBridge。旧敌方 2D fallback 已删除，不再作为 first-person drag/drop target 或 TargetCue 注册入口。

Battle 目标合法性由 `UBattleSession::ValidateTargetWithCard()` 和 PlayCard resolver 判定。Run world drop 合法性由 receiver 和 `URunSession::ValidateRunWorldCardInteraction()` 判定。Run menu Zone drop 默认 probe-only，只有 owning menu 明确接管时才提交。

## §8 Projection Styles / Prototype Boundaries

| 入口 | 当前口径 |
|---|---|
| `BodyLocked` | 稳定默认投影风格：cursor look 不参与 hand anchor 计算，只通过当前真实相机影响最终投影 |
| `Look Responsive Projected` | 保留的视差投影风格：cursor look 先参与 hand anchor 计算，再通过当前真实相机投影；内部兼容枚举名仍为 `LegacyWorldProjected` |
| `LookInfluenceYaw / LookInfluencePitch` | 只服务 `Look Responsive Projected`，控制 cursor look 对 hand anchor 的影响比例 |
| Static preview layer | Prototype preview，只用于 PIE / 开发验证，不是 Battle / Run runtime source |
| Battle hand interaction | 使用 `bEnableBattleHandInteraction`、`SetBattleHandInteractionEnabled()`、`IsBattleHandInteractionEnabled()` |

`LegacyWorldProjected` 的枚举名因蓝图 / 资产序列化兼容暂时保留，但制作语义不是待删除 fallback。后续如果要进一步整理，可以单独做一刀把编辑器文案、文档和资产制作指南继续朝 `LookResponsiveProjected` 命名靠拢；是否真正重命名 C++ 枚举值需要先评估资产迁移成本。

不要把 Static preview layer 当作 Battle / Run runtime source，也不要恢复旧 `LegacyProjectedFan2D` 每卡 3D 槽位分别投影路径。当前两个投影风格都共享 `Authored2D` 手牌布局，区别只在整副手牌中心 anchor 是否吃 cursor look。

## §9 `WBP_FirstPersonCardView` 制作合同

推荐资产路径：`/Game/Wacom/UI/Card/WBP_FirstPersonCardView`

父类：`UWacomCardView`

使用入口：

- `BP_WacomPlayerCharacter -> FirstPersonCardAnchorComponent -> FirstPersonCardViewClass`
- 同一个入口服务 static preview 和 BattleHUD runtime battle hand。
- C++ 不硬编码该 WBP 路径；为空时使用 `UWacomCardView` 测试 fallback。

制作要求：

- 以 `WBP_CardView` 为基础复制，不改 `UWacomCardView::SetCardViewData()` 语义。
- 外层可以是大于主体的透明 bleed 画布，例如 392 x 422 或 392 x 516，用于完整渲染主体外装饰。
- 内部必须提供 `CardSizeBox` 主体 `SizeBox`，默认 296 x 420，并在 bleed 画布中尽量居中。
- 命中范围使用 `UWacomCardView.FixedCardBodyHitSize`，默认 296 x 420；不会因 bleed 画布、RetainerBox 或布局压缩而变小。
- RetainerBox 内部可轻微缩放，给旋转采样留下透明边缘。
- 费用图标使用固定 `CostDigitImage : Image` 绑定；多位数、缺图标或未绑定时不显示文字费用 fallback。
- 材质流光和 disabled overlay 继续走 `UWacomCardView` 现有绑定，不在 slot widget 内新增数据绑定。
- WBP 只负责卡面显示质量，不提交战斗命令，不读取 `UBattleSession`。

验收口径：

- 大角度扇形排布下没有明显锯齿、像素断裂或黑边。
- 卡面材质动画在 HUD first-person layer 中正常刷新。
- Hover、pending、disabled、pressed、confirm、deny 由 slot transform、opacity 和 feedback overlay 表现，WBP 不重复实现状态机。
- 透明 bleed 区只用于渲染，不扩大 hover、click、drag 起手或 card target probe 范围。

## §10 测试入口

推荐自动化前缀：

- `Wacom.UI.FirstPersonCardLayer`
- `Wacom.UI.RunFirstPersonCardLayer`
- `Wacom.UI.Battle`

测试访问通过 production 非反射 automation test view 和 `WacomTests/Private/UI/FirstPersonCardLayerTestAccess.h` 读取 / 驱动。CardView / EffectBadge 计数通过 `FWacomCardViewTestAccess`。PlayerController Run / Battle target probe 通过对应 `WacomTests/Private` access wrapper，不直接依赖散落 probe forwarding 方法或测试字段。
