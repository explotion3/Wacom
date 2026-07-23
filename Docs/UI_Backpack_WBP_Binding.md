# Backpack WBP 制作与绑定合同

> 本文记录背包正式 UI 的长期制作合同。背包容量、移动、投影、负重和销毁规则以 [WacomRun.md](./WacomRun.md) 为准；App 协调边界见 [WacomUI.md](./WacomUI.md)。

## 正式结构

背包正式结构是一个填满 `UI.Layer.GameMenu` 的 `UWacomBackpackScreen`，内部只有一个统一 `UWacomBackpackWorkspaceWidget` 负责卡牌、牌堆、框选、携带和整堆移动。通量区就是工作台空白区域；备战区、特殊区和非空负重区以工作台内嵌牌堆出现。右侧 `ZoneRack` 及其 Entry 已删除，不再是运行时或资产制作合同。

```text
Run Snapshot / revision
  -> UWacomBackpackScreen coordinator
  -> Workspace scene reconciler
  -> passive Workspace cards / ZonePile frame / DeleteConfirm
  -> Workspace intent
  -> Screen command flow
  -> URunSession atomic command
  -> one refresh -> reconcile
```

`ZonePileWidget` 只消费牌堆元数据和已求解边界，不访问 `URunSession`；全部真实卡牌由 Workspace 统一持有。规则校验、批量提交、Toast、销毁确认和失败恢复由 Screen 私有 flow 负责。

## 正式资产

| 资产 | 父类 / 用途 |
|---|---|
| `WBP_BackpackScreen` | `UWacomBackpackScreen`；全屏框架、Header、Workspace 和覆盖层 |
| `WBP_BackpackWorkspace` | `UWacomBackpackWorkspaceWidget`；统一 Canvas、框选和空状态 |
| `WBP_BackpackZonePile` | `UWacomBackpackZonePileWidget`；牌堆框、标题、状态、拖柄与投放反馈 |
| `WBP_BackpackDeleteConfirm` | `UWacomBackpackDeleteConfirmWidget`；批量销毁确认 |
| `WBP_BackpackControlsHelp` | `UWacomBackpackControlsHelpWidget`；被动操作说明层，打开时独占焦点，关闭后恢复原焦点 |
| `WBP_BackpackCardDetailPanel` | `UWacomCardDetailPanel`；背包专用中性检查栏外观，继续消费通用 `FWacomCardDetailViewData` |
| `WBP_BackpackScreen.WorkspaceStyle` | 指向 `DA_BackpackWorkspaceStyle` 的普通资产引用；禁止在 Widget Blueprint 根详情中内联 UObject |
| `DA_BackpackWorkspaceStyle` | `UWacomBackpackWorkspaceStyle`；工作台布局、牌堆、颜色和动效的正式运行时制作入口 |
| `WBP_WacomDeckCardWidget` | 背包卡牌外壳；`CardFaceScaleBox` 制作基线为 `1.0`，运行时按 Style 的统一显示缩放承载现有 `WBP_FPCardView` |
| `Icons/T_BackpackState_Focus` | 64×64 透明像素图标；虚拟焦点，形状必须能脱离颜色识别 |
| `Icons/T_BackpackState_Selected` | 64×64 透明像素图标；已选状态 |
| `Icons/T_BackpackState_ValidDrop` | 64×64 透明像素图标；合法投放状态 |
| `Icons/T_BackpackState_RejectedDrop` | 64×64 透明像素图标；拒绝投放状态 |

`UWacomBackpackZonePileWidget` 保留行为等价的 C++ 被动 fallback，但正式制作必须使用 `WBP_BackpackZonePile`。旧 `UWacomBackpackPilePreviewWidget`、Preview ViewData、`PilePreviewWidgetClass` 和 `PreviewHost` 已删除；不得重新创建缩略卡链路。

## `WBP_BackpackScreen` 绑定

| 控件名 | 类型 | 运行时职责 |
|---|---|---|
| `TitleText` | `TextBlock` | 背包标题 |
| `GoldText` | `TextBlock` | 当前金币 |
| `WorkspaceHost` | `PanelWidget` | 唯一 Workspace 宿主，必须水平和垂直 Fill |
| `DeleteTargetHost` | `PanelWidget` | 工作台右下固定批量销毁目标 |
| `DeleteTargetBackground` | `Border` | 销毁区低对比底板；携带进入时转为危险投放面 |
| `DeleteTargetOutline` | `Border` | 可着色危险九宫格轮廓；始终 `HitTestInvisible` |
| `DeleteTargetIcon` | `Image` | 可着色的破损卡牌图标，始终不参与命中 |
| `DeleteTargetFocusIcon` | `Image` | 独立虚拟焦点图标；不得替代危险语义图标 |
| `DeleteTargetLabel` | `TextBlock` | 销毁区名称或释放提示 |
| `DeleteTargetCountText` | `TextBlock` | 携带数量预览，无携带时隐藏 |
| `DeleteConfirmHost` | `PanelWidget` | 销毁确认覆盖层，初始 `Collapsed` |
| `CardDetailLayer` | `CanvasPanel` | 窄屏卡牌详情避让浮层 |
| `CardDetailDockSize` | `SizeBox` | 宽屏固定详情栏宽度；低于断点时 `Collapsed` |
| `CardDetailDockHost` | `PanelWidget` | 宽屏详情面板宿主，复用同一详情 Widget |
| `CardDetailEmptyText` | `TextBlock` | 宽屏无浏览焦点时的轻量提示 |
| `ArrangeAllButton` | `Button` | “整理通量卡牌”；只清通量自由布局 |
| `ResetPilePositionsButton` | `Button` | “重置牌堆位置”；恢复默认牌堆位置并收起展开项 |
| `InteractionHintText` | `TextBlock` | 独立情境提示条；按 CommonInput 当前输入类型及 Workspace 模式刷新 |
| `ControlsHelpButton` | `Button` | 可聚焦“操作说明”入口；与 F1 打开同一个帮助层 |
| `ControlsHelpHost` | `PanelWidget` | 被动模态帮助宿主；初始 `Collapsed`，不访问 Run |
| `CloseButton` | `Button` | 关闭背包 |

正式 Screen 不得包含 `ZoneRackHost`、`BattleDeckZoneHost`、`SpecialZonesHost` 或其它并行可操作 Zone 宿主。`DeleteTargetHost` 是 Workspace 上的覆盖层，不另占右侧栏。

## `WBP_BackpackWorkspace` 绑定

| 控件名 | 类型 | 运行时职责 |
|---|---|---|
| `WorkspaceCanvas` | `CanvasPanel` | 统一场景根；卡牌、牌堆、框选都放在同一坐标空间 |
| `PileFrameLayer` | `CanvasPanel` | 牌堆框与标题层，不持有真实卡牌 |
| `StaticCardLayer` | `CanvasPanel` | 通量与所有折叠/展开牌堆的常驻真实卡牌 |
| `SettlementLayer` | `CanvasPanel` | 成功释放和 Escape 返回的高层收落；只暂存原 Widget，不复制卡牌 |
| `MarqueeLayer` | `CanvasPanel` | 框选反馈的兼容制作层；运行时最终矩形由 Workspace 原生绘制层负责 |
| `CarryRoot` | `CanvasPanel` | 高 ZOrder 携带根；只应用受最大落后约束的视觉锚点，不承担规则命中 |
| `CarryCache` | `InvalidationBox` | `CarryRoot` 下的静止缓存分支；不承担鼠标位移 |
| `CarryLayer` | `CanvasPanel` | `CarryCache` 的唯一内容；持有除当前牌外的静态携带水平牌列 |
| `CarryActiveLayer` | `CanvasPanel` | `CarryRoot` 下、位于缓存分支上方的实时分支；只持有当前最前卡 |
| `SelectionMarquee` | `Border` | 框选兼容绑定，始终 `Collapsed`；不得与 Workspace 原生最终绘制重复显示 |
| `EmptyStateText` | `TextBlock` | “通量区暂无卡牌”；不代表其它牌堆为空 |

`WorkspaceCanvas` 必须保持 `Visible`，不能改成 `SelfHitTestInvisible`，因为空白区域需要接收通量框选和释放。Screen 动态插入 Workspace 时必须按实际 Slot 类型设置 Fill，不能依赖 `UPanelWidget::AddChild` 默认槽位参数。

## `WBP_BackpackZonePile` 绑定

| 控件名 | 类型 | 运行时职责 |
|---|---|---|
| `FrameBorder` | `Border` | 随布局求解后的完整视觉边界扩大；自身不抢卡牌输入 |
| `DragHandle` | `Border` | 标题拖柄；只有从这里按下并越过阈值才移动整堆 |
| `AccentStrip` | `Border` | 区域语义色带；始终 `HitTestInvisible` |
| `ZoneIcon` | `Image` | 区域单色图标；Brush 由 Style 提供并乘以语义色 |
| `NavigationFocusIcon` | `Image` | 独立虚拟焦点图标；牌堆投放反馈仍由 `DropFeedback` 表达 |
| `TitleText` | `TextBlock` | 区域名称 |
| `CountBadge` | `Border` | 独立数量/容量标签底板 |
| `CountText` | `TextBlock` | 实体数量、容量和可选投影摘要 |
| `StatusText` | `TextBlock` | 仅显示空状态或负重警告；不得常驻原型操作说明 |
| `DropFeedback` | `Border` | 完整牌堆投放遮罩，初始 `Collapsed` 且 `HitTestInvisible` |
| `DropFeedbackText` | `TextBlock` | 目标名称、确认或拒绝原因 |
| `DropFeedbackCountText` | `TextBlock` | 容量预览，例如 `7 + 3 / 18` |

ZonePile 不包含 CardHost、PreviewHost 或规则按钮。短点标题或折叠牌堆主体请求展开；从牌堆内容区域拖过框选阈值则进入该牌堆的 `Marquee`，不会误触展开。真实卡牌常驻 `StaticCardLayer`，携带时进入 `CarryRoot` 下两个携带分支，短时收落时进入 `SettlementLayer`。

`WBP_WacomDeckCardWidget` 另须保留两个互不替代的兼容 `Image` 绑定：`WorkspaceFocusIcon` 对应虚拟焦点，`WorkspaceStateIcon` 对应 `Selected / ValidDrop / RejectedDrop`。运行时两者保持 `Collapsed`，DeckCard 只缓存按 `InstanceId` 归约后的权威焦点/语义状态与 Style Brush；实际 32×32 标记由 Workspace 在全部卡牌、兄弟 `SObjectWidget` 与 `WBP_FPCardView / Retainer` 子树完成之后集中原生绘制，并使用每张卡的最终几何显式跟随 `CardMotionRoot` 位移和旋转。不得改回每卡 `NativePaint + 1`：不同兄弟卡及 Retainer 缓存返回的局部 LayerId 不可比较，后绘制兄弟仍可能覆盖先前角标。焦点和语义标记允许同时可见，语义归约优先级固定为 `RejectedDrop > ValidDrop > Selected`。携带卡从 Screen 已完成的目标校验结果读取 Valid/Rejected，不得在卡牌 Widget 内重新猜测 Run 规则。

Workspace 的统一 post-child overlay 顺序固定为：完整 UMG/Slate/Retainer 子树 → 经过卡牌视觉栈遮挡裁决的焦点/语义角标 → 活跃框选填充 → 2px 框选边框。角标仍高于所属卡自身的 Retainer，但其 32×32 有向矩形若与更高层卡身相交则整枚隐藏；视觉栈按 `Static < Settlement < CarryCache < CarryActive`、Canvas ZOrder 与同层子顺序比较，禁止下层携带卡角标穿透当前卡牌。框选矩形复用 `SelectionColor`，填充透明度复用 `CardStateOverlayOpacity`。`SelectionMarquee` 和 `MarqueeLayer` 只保留资产结构兼容与审计价值，不再拥有运行时绘制顺序。活跃框选的指针移动只能更新 Interaction Model 坐标并触发 Paint invalidation，不得调用完整 Presentation 刷新、Scene reconcile 或 Carry Strip rebuild。

`PileFrameLayer` 与四个卡牌层的实际子控件树是视觉所有权真相。App-private `FWacomBackpackWorkspaceVisualRegistry` 每次 Scene reconcile 都从 Canvas 子控件线性重建 `ViewKey -> Card`、物理 `InstanceId -> Card` 和 `ZoneKey -> Pile` 索引，并持有 Scene 顺序的唯一弱引用卡牌名册；Screen 与 Workspace 不得复制第二份长期数组。瞬态层中的权威卡优先于静态副本，跨区继续复用同一物理 Widget，重复静态副本和幽灵牌框当次移除。基础布局、过渡、选择冻结、Settlement 与释放交接统一由 `FWacomBackpackWorkspaceRuntime` 内的 App-private Visual State 持有，Scene reconcile 只通过它清理不可见身份和连续重定向活动过渡。`NativeDestruct` 必须解除卡牌/牌堆委托、删除动态牌堆子控件，并一次清空 Registry、Visual State 与 Motion Coordinator。

## 内嵌牌堆合同

- 备战区、特殊区是可移动普通牌堆；负重区是右上固定警告牌匣，销毁区固定在右下。
- 折叠与展开复用同一批完整 `UWacomDeckCardWidget`；不得在展开时复制、替换或临时创建另一套卡牌。
- 折叠牌堆显示全部真实卡面：复用 `296×420` Battle 原生卡面，以 `CardDisplayScale=0.78` 的固定背包显示缩放得到约 `231×328` 的布局与命中尺寸；卡牌零旋转、水平露出默认 `16px`（可在 `10–24px` 内适配），最前卡完整显示。
- 特殊区第一张是真实主卡身份预览，之后是全部内容卡；主卡不可操作但保持正常不透明度并显示“主卡”标识。负重区显示全部负重卡并保持锁定。
- 备战实体卡和投影卡都常驻；投影卡保持只读、`0.72` 透明度和来源角标。
- 备战投影卡在展开区保持只读、半透明并显示来源角标，不参与选择、框选、携带或批量移动。
- 只有标题拖柄可移动整堆；标题或牌堆主体短点切换展开。卡牌、牌堆和内容区框选统一使用 Slate 的屏幕空间拖拽阈值（`FSlateApplication::HasTraveledFarEnoughToTriggerDrag`），由平台与 DPI 校准；不得再写本地坐标 `5px` 常量。打开新堆时旧堆先收拢。
- 同时只展开一个牌堆。展开卡继续使用统一的固定背包显示缩放和零旋转，不使用滚动、裁切或按牌堆高度动态缩放；布局由“左压缩堆 + 中央完整窗口 + 右压缩堆”组成，空间不足时减少完整窗口卡数并压缩外围露出。
- 展开的备战区、特殊区和负重区使用 `HandLensStrip`：空间足够时全部卡牌完整居中展示；空间不足时形成“左压缩堆 + 动态完整区 + 右压缩堆”。布局由鼠标在整条稳定走廊中的连续横向位置驱动，不再限制固定 `1–5` 张。完整卡默认间隔 `24px`，外围期望露出 `59px`、严格下限 `16px`；严格布局仍有空间时可在边界重叠不超过 `178px` 的前提下优先从右、再从左提升一张完整卡。折叠牌堆保持原固定布局。
- 编辑器调参入口为 `WBP_BackpackScreen → WorkspaceStyle → DA_BackpackWorkspaceStyle`。展开牌堆只消费 `HandLensFullGapPixels / HandLensCompressedExposurePixels / HandLensMinimumExposurePixels / HandLensPromotionOverlapTolerancePixels`；`FocusWindowMaximumCards / FocusWindowFullGapPixels / FocusWindowCompressedExposurePixels / FocusWindowMinimumExposurePixels` 保留给多卡携带，用户制作值互不覆盖。多卡携带的 `FocusWindowMaximumCards` 正式基线为 `1`，因此只完整展示当前滚轮卡，其余卡进入左右压缩段；仍可在 Style 中调高以同时完整展示相邻卡。Style 版本 4 在版本 3 的区域 Appearance、投放反馈和响应式详情之上增加四个可访问性图标 Brush；已有运动、Hand Lens 和用户微调值必须原样保留。Builder 对已有 Style 只复用、不写入、不保存；仅在资产缺失时按当前基线创建。本轮 v3 → v4 已按用户明确授权通过下文一次性定向 Commandlet 完成；未来资产迁移仍须重新取得精确范围授权，不得扩展该 Commandlet 清单。
- 牌堆身份使用颜色、单色图标和轮廓三重编码：备战冷蓝双线框、特殊紫色切角框、负重琥珀加重警示框、销毁红色破损卡牌与危险框。图标和九宫格纹理由 Builder 在缺失时确定性创建，Style Brush 保持可着色；不得在卡牌材质或 Run 规则中硬编码区域主题。
- 标题拖柄必须始终位于可见卡身上方。展开牌堆基础卡位需预留 `ExpandedCardHoverLiftPixels` 的垂直净空，使焦点卡上抬后仍不覆盖标题；不得仅靠提高整个牌框层级而截断卡牌输入。
- 投放反馈为 `None / Valid / Rejected / Destructive` 四态 ViewData。合法目标使用区域强调色和容量预览，拒绝显示规则层返回的原因，危险状态只用于销毁目标；所有反馈层均 `HitTestInvisible`，原子拒绝后保持携带。
- 详情响应式模式由 Screen 几何决定：逻辑宽度不低于 `DetailDockBreakpointPixels`（基线 1600）时预留 `DetailDockWidthPixels`（基线 360）的固定检查栏；窄屏把同一详情 Widget 重挂到 `CardDetailLayer`，使用 `DetailFloatingSize` 并在卡牌相反侧夹紧。模式切换不得重建卡牌或详情 Widget，也不得改变 Workspace Snapshot。
- 展开布局按当前卡数和工作台几何预先计算稳定 `FrameRect` / 透镜走廊；鼠标横坐标映射为 `0..CardCount-1` 的连续 `LensFocus`，只有左堆、完整区、右堆的目标身份发生变化时才重排，同一段内移动只更新 Fake3D。整个牌列从当前视觉姿态平滑滑向新目标，默认约 `0.32s` Ease-Out；透镜布局焦点与浏览卡身份必须分离。目标条带只服务布局诊断；运行时浏览、详情、Fake3D 和左键拾取必须统一按当前插值后的视觉卡身与 Canvas ZOrder 命中，重叠位置选择实际显示在最上层的卡，纵向严格等于当前卡身，完整卡之间的真实空隙不命中卡牌。动画期间即使鼠标静止，按需 Motion Timer 也要重新解析鼠标下的视觉卡，但不得重算透镜或刷新 Snapshot。标题拖柄始终拥有最高输入优先级；进入标题冻结最后透镜布局并清除浏览焦点。详情面板使用 Workspace 捕获的实际视觉矩形，经 Workspace → Absolute → `CardDetailLayer` 四角转换后定位。
- 展开牌堆可按住左 Shift 临时锁定当前 Hand Lens 三段布局。锁定只阻止 `LensFocus` 驱动的新布局求解，实际视觉卡身命中、详情、Fake3D 和普通左键拾取仍正常；松开左 Shift 后立即使用最新缓存鼠标位置恢复重排。开始携带、框选、CardPress、整堆移动、Suspended、收起/切换牌堆、失焦或关闭背包时必须清除该瞬态锁定。Ctrl 多选合同不变。
- 焦点卡上抬并置顶，不缩放、不改卡面透明度；邻居仅水平移动外层局部姿态。鼠标离开实际卡身后默认等待 `0.12s`，随后只清除上抬、详情和实时卡面，最后一次三段式窗口冻结到该牌堆收起或切换，不返回首次中央窗口。
- 投影卡、特殊区主卡和负重卡可以进入焦点牌列、启用详情浏览和上抬反馈，但继续保持各自透明度、角标和只读语义，不能选择、框选或携带。
- 牌堆释放吸附到默认 `16px` 网格或邻近边缘；主体允许部分重叠，但标题拖柄不能相互覆盖，且始终夹紧在 Workspace 内。
- 普通动效只插值位置和角度；展开/收起默认 `0.18s`，吸附默认 `0.12s`。禁止卡面淡入和动态缩放。
- Workspace 只能拥有一个按需帧 `ActiveTimer`。Carry、PileMove、携带目标悬停展开、焦点退出、局部姿态、Settlement 与展开/收起基础 Canvas 过渡全部并入该 Timer；每帧只更新指针锚点和活动 Motion Records，不能调用全量表现刷新、Snapshot 刷新或 Scene Reconcile。收起完成由实际布局过渡全部结束触发，不使用与 Style 时长平行的回调 Timer。几何稳定采样和下一帧 Retainer 补绘仍是独立一次性任务。这项为 C++ 性能合同，不需要新增 WBP 节点或改写 `DA_BackpackWorkspaceStyle`。
- 展开/收起过渡使用一次捕获的固定起点和最终目标；同一 Snapshot 或稳定几何刷新重复提交相同目标时必须保留当前 elapsed transition，不得取消过渡并把整组卡牌瞬移到水平终点。收起时每张需要移动的卡必须直接前往最终折叠位置，禁止先聚到标题中心或其它共享中间点再二次排布；已处于最终位置的锚定卡不创建无意义过渡。
- `Simplified` UI Motion 下展开、收起和吸附直接到达最终状态；通量 Hover、展开焦点和携带当前卡均不得产生空间上抬、角度补偿或视觉弹簧。运行中切换到 Simplified 时必须立即清除已有局部偏移并同步命中中心。

## 输入与事务合同

Workspace 交互模式互斥：`Idle / CardPress / Marquee / Carry / PileMove / Suspended`。

`UWacomBackpackWorkspaceWidget` 只作为 UMG/Slate 适配器保留绑定、生命周期、输入入口、Canvas 应用和单一 ActiveTimer。`FWacomBackpackWorkspaceRuntime` 私有拥有：`GestureController`（屏幕空间阈值、按压、捕获、起手释放保护及牌堆回滚）、`NavigationController`（`InstanceId / Zone` 稳定虚拟焦点、空间邻居和语义目标）及 `PresentationController`（Hand Lens、选择冻结、Carry 编排、Hover 展开/收起和几何稳定状态）。Slate 焦点始终停留在 Workspace 根；这些控制器不访问 `URunSession`。

无鼠标输入映射固定为：方向键/摇杆空间导航；Enter/A 拾取或向当前目标释放一张；Space/X 选择/取消；T/Y 空闲时切换特殊区卡牌状态、携带时释放全部；Q/E 与 LB/RB 切换当前携带卡；Esc/B 取消瞬态或返回；F1/界面按钮打开帮助层。Snapshot 等价刷新按 `InstanceId / Zone` 恢复虚拟焦点，身份消失时回退到最近可聚焦目标。无效牌堆仍可聚焦并显示 Rejected，但不得提交；规则原因由 Screen 的现有校验/Toast flow 提供。

释放意图必须显式标注 `Pointer / Flux / Pile / Delete` 和目标 Zone。Pointer 保留精确坐标；Flux 释放会清除该卡手工布局并交给确定性布局器；Pile/Delete 分别复用现有原子移动和删除确认。Passive Widget 只广播意图，Run 写入仍由 Screen command flow 提交。

- 选择只属于一个来源区。Interaction Model 只接收按物理 `InstanceId` 去重后的可移动实体卡；同一卡的投影、特殊区主卡预览和负重卡仅进入浏览表现，不得进入选择/携带命中表。通量背景框选只命中通量卡；折叠或展开牌堆的内容背景框选只命中该牌堆的可移动实体卡。折叠状态禁止卡牌本体直接点击，但不能因此从框选命中表移除实体卡；切换来源会清除旧选择。
- `CardPress`、携带、整堆移动和 Suspended 会锁定或清除展开牌堆浏览焦点。框选或 Ctrl 选择开始时捕获当前实际卡位、角度和 ZOrder，停止未完成的局部重排并让 Interaction Model 使用这份视觉快照；冻结保持到选择清空、开始携带、牌堆收起或来源区切换，退出时从当前视觉姿态连续返回最后窗口，不得瞬移。
- 普通左键按下任意可移动卡牌时，Interaction Model 必须在同一输入帧确定选择集合并进入 `Carry`；不得先清除浏览焦点后等待后续 `PointerMove`。对应第一次左键松开只消费起手释放保护并保持携带。Ctrl 点击继续作为多选编辑手势，不强制立即起手。
- 框选使用屏幕所见卡牌本体作为命中真相：开始框选时捕获可移动卡的实际中心、固定卡面尺寸和当前角度，框选矩形与该有向卡身矩形相交即选中，边缘接触也算命中；只读卡和非来源区卡仍被排除。
- 空白 Workspace 是通量投放目标：通量卡只更新自由布局，其它实体卡通过现有原子移动进入通量区并保存释放位置。
- 放回来源牌堆等价于收拢；放到其它牌堆走 `MoveInstancesAtomic`；无效目标保持携带并显示拒绝反馈，不允许部分提交。
- 成功释放后，仍位于携带视觉分支的卡牌必须进入显式 pending visual handoff。Interaction Model 已移除卡牌但目标 Snapshot 尚未 reconcile 的窗口内，任何 Presentation 刷新、ActiveTimer 或 `SyncCarryLayer()` 都不得恢复来源 A 的基础布局。跨物理区提交虽然会改变正式 ViewKey 的 `PhysicalZone`，但 Reconciler 必须按 `InstanceId` 迁移同一个受保护实体 Widget，不能销毁 A 实例并创建 B 实例。目标 Scene 到达后把同一 Widget 重挂到 `SettlementLayer`：外层直接采用目标 B 的 Canvas 布局，`CardMotionRoot` 从捕获的实际释放姿态形成局部偏移并在默认 `0.18s` 内归零，因此视觉路径是“释放点 → B”，绝不经过来源 A。所有 `Static / CarryCache / CarryActive / Settlement` 卡牌跨层迁移必须走 App-private 的 Slate 保活重挂载入口：只在一次 `RemoveFromParent → AddChild` 原子操作期间持有卡牌与目标 Panel 的 `TSharedRef<SWidget>`，使目标层接管同一个 `SObjectWidget`、`WBP_FPCardView`、`SRetainerWidget` 与 RenderTarget；禁止长期保存第二套 Slate 强引用。Carry/Settlement 权威实例在 Scene 重绑时必须保留现有局部姿态和 Retainer 捕获面；相同卡定义、相同移动开关或相同 retained-rendering 模式不得重复提交卡面数据或重建捕获状态，避免释放瞬间出现一帧空白。部分释放只收落已提交卡，其余卡继续携带并平滑重排；原子拒绝不启动收落。
- Scene Builder 一次消费 Snapshot、Workspace State、精简 Carry 摘要、Style 和几何，输出顺序对齐的 Card Entries / Layouts、Pile Entries、展开边界与空状态；不得在 Card apply 阶段再次扫描全部牌堆。Visual Registry 的现有实例搜索覆盖 `StaticCardLayer`、`CarryLayer`、`CarryActiveLayer` 和 `SettlementLayer`。完整 ViewKey 相同的卡只能保留一个 Widget；若旧刷新已同时留下静态副本和 Carry/Settlement 权威实例，优先保留瞬态层中的原 Widget并立即移除静态副本，避免再次框选时出现不可交互残影。
- 携带在合法折叠牌堆上停留约 `0.35s` 后自动展开；离开、取消、目标非法或来源 revision 漂移不得继续该自动展开请求。
- Escape 依次取消确认或瞬态交互、收起展开牌堆，再交给 CommonUI 关闭背包；取消携带时从当前视觉姿态收落回来源布局。整堆拖动开始时必须快照牌框 Canvas 位置与 ZOrder，取消时同时恢复牌框和所属卡牌，只有成功释放才清除快照并提交 Workspace State Store，保证 A/B 视觉与状态原子一致。B 始终直接关闭并立即清理所有瞬态动画、Timer、父级与实时 Retainer。

## Retainer 与卡面

完整工作台卡直接复用 Battle 的 `WBP_FPCardView`，因此沿用同一张卡面的边框、费用、稀有度、Fake3D、视差、接触阴影和表面材质；不复用 Battle Slot、发牌、出牌、目标、溶解或 transition hint 状态机。`WBP_WacomDeckCardWidget` 必须提供 `CardMotionRoot`，包装完整卡面、反馈和角标，用局部 Translation / Angle 表达 Hover、拾起、滚轮当前卡和 Settlement；外层 Canvas 始终只表达基础布局。卡面原生制作尺寸保持 `296×420`，`CardFaceScaleBox` 资产制作值保持 `1.0`，运行时统一应用 `CardDisplayScale`（基线 `0.78`）；布局、视觉卡身、命中、框选、Hand Lens 与 Carry 必须共同使用缩放后的尺寸。该缩放独立于 Battle/Run `PresentationScale`，也不能根据 Workspace 高度连续改写。滚轮切换时，新当前卡必须在 `CarryActiveLayer` 内获得独占最高 ZOrder；上一张卡可以暂留活动层完成回落，但不得遮挡新当前卡。

携带版与静态版不存在两套卡面 WBP：跨层前后始终是同一个 `WBP_WacomDeckCardWidget -> WBP_FPCardView` 实例。父级迁移本身不得调用卡面数据绑定、切换 Retainer 模式或请求补绘；只有 Hover/Carry 当前卡等语义状态确实变化时，Motion Coordinator 才能更新实时表现策略。

`WBP_BackpackScreen` 的 `WorkspaceStyle` 必须是指向 `DA_BackpackWorkspaceStyle` 的非 `Instanced` 资产引用。制作时从 Class Defaults 双击引用资产调整布局、牌列、颜色和动效参数。不得把 `UWacomBackpackWorkspaceStyle` 标记为 `EditInlineNew / DefaultToInstanced`，也不得在 Screen 属性上使用 `Instanced / ShowOnlyInnerProperties`；这些内联对象合同会让 UE 5.8 Widget Blueprint 的 PropertyEditor 在构建根节点详情时递归生成分类树并导致栈溢出。

背包私有表现控制器限制任意时刻最多一张卡实时重绘：展开 Hover 卡优先；携带时改由当前最前卡占用预算。其余折叠卡、静置卡和非当前携带卡把 `WBP_FPCardView` 切为内容/状态变化时按需补绘。CommonUI Layer 过渡期间 Screen 暂停 retained caching 并直绘，过渡结束后恢复并请求最终补绘；不要用固定延迟或 Tick 轮询替代 PrimaryLayout transition delegate。

Carry 同时维护两个坐标：Interaction Model 中的精确逻辑指针用于目标判定和 Run 原子命令；`CarryRoot` 使用独立视觉锚点，以默认 `34/s` 响应速度追赶且最大落后 `14px`，鼠标停止后在 `0.5px` 内归位。多卡携带使用以 `Carry.CurrentIndex` 为焦点的紧凑牌列：默认完整窗口只有当前滚轮卡一张，卡牌中心固定在鼠标锚点，其余卡按有效露出间距向左右压缩。只有开始携带、滚轮切换、部分释放、数量或几何变化会重算牌列，普通鼠标移动只平移 `CarryRoot`，不得重算牌列、遍历静态卡、请求 Snapshot/Scene reconcile 或逐卡重绘。`CarryCache` 本身必须保持零位移并缓存全部非当前卡及其局部让位运动；滚轮交接期间仅新旧当前卡可短时同处 `CarryActiveLayer`，运动结束后旧卡回到缓存分支。这样 `1/7/15/21` 张卡的鼠标热路径成本保持一致。

Hover 卡使用局部 `48px` 上抬，紧凑牌列邻居默认约 `0.18s` 完成水平让位；携带刚开始时默认最右释放卡保持平放，但只要发生过一次有效滚轮切换，之后滚轮选中的任何当前卡（包括重新切回最右卡）都约 `0.14s` 上抬 `56px` 并获得组内绝对最高 ZOrder。牌堆和携带基础姿态始终为零旋转，二者都不得改变卡面缩放或透明度。背包活动卡 DepthMotion 复用 Battle 的低层速度倾斜、Fake3D、表面视差和接触阴影能力，但由背包私有表现控制器拥有状态；让位邻居保持静态 Retainer，只有 Hover 卡或携带当前卡最多一张启用实时 Retainer。`Simplified Motion` 保留紧凑牌列的可读位置，但关闭离开延迟、空间上抬、视觉弹簧、速度倾斜和 Settlement，直接应用最终姿态。

投影卡的 `0.72` 透明度是明确只读语义；普通卡、Hover、选中、展开和首次显示都必须保持 `1.0` 卡面透明度。

## Builder

正式资产由 `WacomBuildBackpackUI` 幂等生成：

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' '<Project>\Wacom.uproject' -run=WacomBuildBackpackUI -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache
```

Builder 必须让 `WBP_WacomDeckCardWidget` 以 `CardMotionRoot -> 卡牌 Overlay -> CardFaceScaleBox -> WBP_FPCardView` 承载正式卡面，并让 Workspace 具备 `SettlementLayer`；同时编译保存 Workspace、ZonePile、DeleteConfirm、背包专用 Detail 和 Screen。中性区域图标/框体纹理只在缺失时创建，已有同路径资产不得被重画。不再生成 `WBP_BackpackCardView`、Preview、Rack、Rack Entry、`WBP_BackpackBattleDeckZone` 或 `WBP_WacomSpecialZoneWidget`。`DA_BackpackWorkspaceStyle` 是受版本控制的人工制作真相：Builder 只在资产缺失时用当前基线创建并保存，资产已存在时不得重写或重新保存任何 `EditAnywhere` 制作字段。新增字段使用 C++ 默认值；确需迁移既有资产时必须提交显式、带版本条件的定向迁移，不能把无条件赋值重新放回通用 Builder。禁止为 Style v4 运行会批量重存人工资产的通用 Builder。连续运行不得恢复已删除链路，也不得改变正式资产的绑定结构、制作参数或资产 Hash。

### 一次性 Style v4 定向 Commandlet

`WacomMigrateBackpackWorkspaceV4` 是用户为本轮明确授权的 MCP 外一次性迁移通道，不是新的通用 Builder。它先校验既有 WBP 父类、关键树形、可选槽位类型和 Style 版本，只允许保存以下九个固定 Package：

- `/Game/Wacom/UI/Backpack/Icons/T_BackpackState_Focus`
- `/Game/Wacom/UI/Backpack/Icons/T_BackpackState_Selected`
- `/Game/Wacom/UI/Backpack/Icons/T_BackpackState_ValidDrop`
- `/Game/Wacom/UI/Backpack/Icons/T_BackpackState_RejectedDrop`
- `/Game/Wacom/UI/Backpack/WBP_BackpackControlsHelp`
- `/Game/Wacom/UI/Backpack/WBP_BackpackScreen`
- `/Game/Wacom/UI/Card/WBP_WacomDeckCardWidget`
- `/Game/Wacom/UI/Backpack/WBP_BackpackZonePile`
- `/Game/Wacom/UI/Backpack/DA_BackpackWorkspaceStyle`

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' '<Project>\Wacom.uproject' -run=WacomMigrateBackpackWorkspaceV4 -InspectOnly -Unattended -NoPause -NoSplash -NullRHI -nop4
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' '<Project>\Wacom.uproject' -run=WacomMigrateBackpackWorkspaceV4 -Apply -Unattended -NoPause -NoSplash -NullRHI -nop4
```

`-InspectOnly` 全程不调用补丁函数、不保存资产；当前已满足时返回 `0`，合同有效但仍需迁移时返回 `2`。`-Apply` 只创建缺失的四张确定性 64×64 透明纹理和 Help WBP，只为三个既有 WBP 增加新绑定，并且只允许 Style 从 v3 补四个 Brush 后升到 v4；遇到未知 v4 绑定或清单外 Package 会拒绝。二次运行必须报告 `current=true / saved=0` 且九项 SHA-256 不变。该命令不得增加新 Package、迁移其它 UI 或替代 `Docs/UnrealMCPWorkflow.md` 的常规资产写入流程。

## C++ fallback

C++ fallback 与正式 WBP 使用完全相同的数据流：`Snapshot -> WorkspaceReconciler -> Workspace`。它只构建顶部栏、`WorkspaceHost`、详情层、销毁确认层和操作按钮，不创建隐藏的备战、通量、特殊区或负重区列表，也不保留旧 Zone Host。`OpenNativeFallbackPIEValidation` 应验证同一套 Workspace 输入、卡牌身份复用和命令提交路径，而不是迁移期只读界面。

## 验证

自动化至少覆盖：正式绑定无 Rack/Preview、单 `WBP_FPCardView` Retainer、特殊区主卡身份、投影只读浏览、0/1/2/3/5/7/15/21 张折叠/展开及数量自适应紧凑牌列、重排中当前视觉姿态与 ZOrder 命中、稳定牌框、1/7/15/21 张 CarryLayer 单锚点和滚轮让位、跨层前后顶层与嵌套卡面的 Slate 指针连续、纯父级迁移不重写卡面/Retainer 策略、Settlement 跨刷新唯一实例、重复 20 轮携带/释放无残留、动态卡面数量上限、牌堆状态同 Run 保留/新 Run 清空、折叠/展开牌堆单来源框选与只读排除、标题拖动、Escape/B 清理和完整投放矩阵。

PIE 至少检查：

- 1280×720、1920×1080、16:10 和超宽下默认牌堆位置、边界夹紧与重置。
- 21 张备战卡展开、左右自适应、Hover 上抬、框选和详情。
- 整堆拖动、网格/边缘吸附、标题不重叠和主体部分重叠。
- 携带悬停自动展开、跨区移动、原子拒绝、批量销毁和剩余携带。
- 同 Run 关闭重开保留牌堆布局；新 Run 清空；Full/Simplified 动效均无淡入、动态缩放或首帧半透明。
- 纯键盘和纯手柄完成浏览、选择、跨区移动、特殊牌切换、删除、整理和退出；帮助层关闭后焦点回到打开前控件，Deactivate 后不再响应 CommonInput 通知。
- 灰度/色觉模拟下 Focus、Selected、ValidDrop、RejectedDrop 仍能按图形区分；Focus 与语义图标可同时显示。
- 24 卡与 100 卡分别在 1280×720、1920×1080 预热 10 秒后采集 60 秒 Insights；空指针移动和活跃框选矩形更新均不得触发 Scene reconcile、完整 presentation refresh 或 carry-strip rebuild，实时 Retainer 不超过一个。16:10 与 2560×1080 仅作为布局门禁。
