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
| `WBP_WacomSpecialZoneWidget` | `UWacomSpecialZoneWidget`；只供 fallback / 旧 Host 的只读兼容表现 |
| `DA_BackpackWorkspaceStyle` | `UWacomBackpackWorkspaceStyle`；工作台布局、牌堆、颜色和动效参数 |
| `WBP_WacomDeckCardWidget` | 背包卡牌外壳；`CardFaceScaleBox=0.75`，内部直接承载现有 `WBP_FPCardView` |

`UWacomBackpackZonePileWidget` 保留行为等价的 C++ 被动 fallback，但正式制作必须使用 `WBP_BackpackZonePile`。旧 `UWacomBackpackPilePreviewWidget`、Preview ViewData、`PilePreviewWidgetClass` 和 `PreviewHost` 已删除；不得重新创建缩略卡链路。

## `WBP_BackpackScreen` 绑定

| 控件名 | 类型 | 运行时职责 |
|---|---|---|
| `TitleText` | `TextBlock` | 背包标题 |
| `GoldText` | `TextBlock` | 当前金币 |
| `WorkspaceHost` | `PanelWidget` | 唯一 Workspace 宿主，必须水平和垂直 Fill |
| `DeleteTargetHost` | `PanelWidget` | 工作台右下固定批量销毁目标 |
| `DeleteConfirmHost` | `PanelWidget` | 销毁确认覆盖层，初始 `Collapsed` |
| `CardDetailLayer` | `CanvasPanel` | 卡牌详情覆盖层 |
| `ArrangeAllButton` | `Button` | “整理通量卡牌”；只清通量自由布局 |
| `ResetPilePositionsButton` | `Button` | “重置牌堆位置”；恢复默认牌堆位置并收起展开项 |
| `CloseButton` | `Button` | 关闭背包 |

正式 Screen 不得包含 `ZoneRackHost`、`BattleDeckZoneHost`、`SpecialZonesHost` 或其它并行可操作 Zone 宿主。`DeleteTargetHost` 是 Workspace 上的覆盖层，不另占右侧栏。

## `WBP_BackpackWorkspace` 绑定

| 控件名 | 类型 | 运行时职责 |
|---|---|---|
| `WorkspaceCanvas` | `CanvasPanel` | 统一场景根；卡牌、牌堆、框选都放在同一坐标空间 |
| `PileFrameLayer` | `CanvasPanel` | 牌堆框与标题层，不持有真实卡牌 |
| `StaticCardLayer` | `CanvasPanel` | 通量与所有折叠/展开牌堆的常驻真实卡牌 |
| `MarqueeLayer` | `CanvasPanel` | 框选反馈层 |
| `CarryRoot` | `CanvasPanel` | 高 ZOrder 携带根；鼠标热路径只更新此节点的 RenderTranslation |
| `CarryCache` | `InvalidationBox` | `CarryRoot` 下的静止缓存分支；不承担鼠标位移 |
| `CarryLayer` | `CanvasPanel` | `CarryCache` 的唯一内容；持有除当前牌外的静态携带扇形 |
| `CarryActiveLayer` | `CanvasPanel` | `CarryRoot` 下、位于缓存分支上方的实时分支；只持有当前最前卡 |
| `SelectionMarquee` | `Border` | 框选矩形，初始 `Collapsed`，ZOrder 高于普通卡牌 |
| `EmptyStateText` | `TextBlock` | “通量区暂无卡牌”；不代表其它牌堆为空 |

`WorkspaceCanvas` 必须保持 `Visible`，不能改成 `SelfHitTestInvisible`，因为空白区域需要接收通量框选和释放。Screen 动态插入 Workspace 时必须按实际 Slot 类型设置 Fill，不能依赖 `UPanelWidget::AddChild` 默认槽位参数。

## `WBP_BackpackZonePile` 绑定

| 控件名 | 类型 | 运行时职责 |
|---|---|---|
| `FrameBorder` | `Border` | 随布局求解后的完整视觉边界扩大；自身不抢卡牌输入 |
| `DragHandle` | `Border` | 标题拖柄；只有从这里按下并越过阈值才移动整堆 |
| `TitleText` | `TextBlock` | 区域名称 |
| `CountText` | `TextBlock` | 实体数量、容量和可选投影摘要 |
| `StatusText` | `TextBlock` | 展开/收起提示或负重警告 |
| `DropFeedback` | `Border` | 合法/拒绝投放反馈，初始 `Collapsed` |

ZonePile 不包含 CardHost、PreviewHost 或规则按钮。点击标题或折叠牌堆主体请求展开；真实卡牌始终属于 Workspace 的 `StaticCardLayer` 或 `CarryRoot` 下两个携带分支之一。

`PileFrameLayer` 的实际子控件树是牌堆视觉所有权真相，`PileWidgets` 只作为瞬态索引。Reconcile 必须从 Canvas 现有 `UWacomBackpackZonePileWidget` 子控件复用或移除实例，并在 `NativeDestruct` 主动解除委托和删除动态牌堆子控件；不得因 Slate/UMG Destruct→Construct 边界只清空索引而把旧牌堆遗留成不可交互的孤儿视觉。

## 内嵌牌堆合同

- 备战区、特殊区是可移动普通牌堆；负重区是右上固定警告牌匣，销毁区固定在右下。
- 折叠与展开复用同一批完整 `UWacomDeckCardWidget`；不得在展开时复制、替换或临时创建另一套卡牌。
- 折叠牌堆显示全部真实卡面：固定 `220×320` 逻辑尺寸、`0.75` 卡面缩放、零旋转、水平露出默认 `16px`（可在 `10–24px` 内适配），最前卡完整显示。
- 特殊区第一张是真实主卡身份预览，之后是全部内容卡；主卡不可操作但保持正常不透明度并显示“主卡”标识。负重区显示全部负重卡并保持锁定。
- 备战实体卡和投影卡都常驻；投影卡保持只读、`0.72` 透明度和来源角标。
- 备战投影卡在展开区保持只读、半透明并显示来源角标，不参与选择、框选、携带或批量移动。
- 只有标题拖柄可移动整堆；内容区域继续属于卡牌输入。标题点击切换展开，打开新堆时旧堆先收拢。
- 同时只展开一个牌堆。展开卡保持 `220×320` 逻辑尺寸与卡面 `0.75` 固定缩放，使用 `32–72px` 自适应露出和轻微扇转，不使用滚动。
- Hover 只上抬并置顶，不缩放、不改卡面透明度。
- 牌堆释放吸附到默认 `16px` 网格或邻近边缘；主体允许部分重叠，但标题拖柄不能相互覆盖，且始终夹紧在 Workspace 内。
- 普通动效只插值位置和角度；展开/收起默认 `0.18s`，吸附默认 `0.12s`。禁止卡面淡入和动态缩放。
- 展开/收起过渡使用一次捕获的固定起点和最终目标；同一 Snapshot 或稳定几何刷新重复提交相同目标时必须保留当前 elapsed transition，不得取消过渡并把整组卡牌瞬移到扇形终点。收起时每张卡的目标必须直接来自最终折叠布局，禁止先聚到标题中心或其它共享中间点再二次排布。
- `Simplified` UI Motion 下展开、收起和吸附直接到达最终状态。

## 输入与事务合同

Workspace 交互模式互斥：`Idle / CardPress / Marquee / Carry / PileMove / Suspended`。

- 选择只属于一个来源区。通量背景框选只命中通量卡；展开牌堆背景只命中该牌堆卡。切换来源会清除旧选择。
- 当前框选仍使用“卡牌中心进入框选矩形”作为命中规则；接触即选属于后续体验优化。
- 空白 Workspace 是通量投放目标：通量卡只更新自由布局，其它实体卡通过现有原子移动进入通量区并保存释放位置。
- 放回来源牌堆等价于收拢；放到其它牌堆走 `MoveInstancesAtomic`；无效目标保持携带并显示拒绝反馈，不允许部分提交。
- 成功释放后，仍位于携带视觉分支的卡牌必须进入显式的 pending visual handoff，并由目标 Scene 布局直接消费、一次重挂载到 `StaticCardLayer`；Interaction Model 已移除卡牌但目标 Snapshot 尚未 reconcile 的窗口内，任何 Presentation 刷新、ActiveTimer 或 `SyncCarryLayer()` 都不得恢复来源 A 的基础布局。跨物理区提交虽然会改变正式 ViewKey 的 `PhysicalZone`，但 Reconciler 必须按 `InstanceId` 迁移同一个受保护实体 Widget，再原位写入目标 ViewData，不能销毁 A 实例并创建 B 实例。目标布局消费 pending handoff 时直接落在 B，禁止生成 A→B 过渡；部分释放只交接已提交卡，其余卡继续留在对应携带分支。
- 携带在合法折叠牌堆上停留约 `0.35s` 后自动展开；离开、取消、目标非法或来源 revision 漂移不得继续该自动展开请求。
- Escape 依次取消确认或瞬态交互、收起展开牌堆，再交给 CommonUI 关闭背包；B 始终直接关闭并清理瞬态状态。

## Retainer 与卡面

完整工作台卡直接复用 Battle 的 `WBP_FPCardView`，因此沿用同一张卡面的边框、费用、稀有度、Fake3D、视差、接触阴影和表面材质；不复用 Battle Slot、发牌、出牌、目标、溶解或 transition hint 状态机。卡面保持固定 `220×320` Widget 逻辑尺寸、`CardFaceScaleBox=0.75`、单位 Render Scale 和完全不透明根节点；不能根据 Workspace 高度连续改写缩放。

背包私有表现控制器限制任意时刻最多一张卡实时重绘：展开 Hover 卡优先；携带时改由当前最前卡占用预算。其余折叠卡、静置卡和非当前携带卡把 `WBP_FPCardView` 切为内容/状态变化时按需补绘。CommonUI Layer 过渡期间 Screen 暂停 retained caching 并直绘，过渡结束后恢复并请求最终补绘；不要用固定延迟或 Tick 轮询替代 PrimaryLayout transition delegate。

Carry 视觉使用单一鼠标锚点：开始携带时重挂载一次并计算局部扇形，普通鼠标移动只更新外层 `CarryRoot` 的 RenderTranslation，不重算扇形、不遍历静态卡、不请求 Snapshot/Scene reconcile。`CarryCache` 本身必须保持零位移并只缓存 `CarryLayer` 中的非当前卡；唯一允许持续动态重绘的当前最前卡放在其兄弟 `CarryActiveLayer`，避免实时 Retainer 子树使整组扇形缓存反复失效。Presentation 更新必须用统一的“任一携带视觉分支”判断，不能只识别 `CarryLayer` 而把 `CarryActiveLayer` 的当前牌按旧静态基础布局重写。这样 1 张与 21 张卡的指针热路径都只移动一个 Slate 根；滚轮换牌、数量变化和样式/几何变化才重算局部扇形并使缓存失效。当前卡使用与指针位置无关的中性倾斜，因此鼠标移动不得重复写 Retainer phase、卡面深度材质或 `RequestRender`；这些表现只在活动卡或携带状态发生结构变化时更新。

投影卡的 `0.72` 透明度是明确只读语义；普通卡、Hover、选中、展开和首次显示都必须保持 `1.0` 卡面透明度。

## Builder

正式资产由 `WacomBuildBackpackUI` 幂等生成：

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' '<Project>\Wacom.uproject' -run=WacomBuildBackpackUI -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache
```

Builder 必须让 `WBP_WacomDeckCardWidget` 承载 `WBP_FPCardView`，并编译保存 ZonePile、Workspace、DeleteConfirm、兼容 SpecialZone、Screen 和 Style；不再生成 `WBP_BackpackCardView`、Preview、Rack 或 Rack Entry。连续运行不得恢复已删除链路，也不得改变正式资产的绑定结构。

## Fallback 与迁移兼容

C++ fallback 保留 `DeleteZoneHost`、`BattleDeckZoneHost`、`FluxContentDropTargetHost`、`SpecialZonesHost`、`BurdenZoneHost` 等只读迁移 Host，用于缺失正式资产时观察 Snapshot；它们不拥有拖拽、移动或销毁输入，也不属于正式 WBP 制作合同。`CardDetailLayer` 仍可由 fallback 动态填充。

局部 `WBP_BackpackBattleDeckZone` 等旧 Zone 外壳可以继续作为只读 fallback 资源，但不得重新接入正式 Screen 或复制 Workspace 输入状态机。

## 验证

自动化至少覆盖：正式绑定无 Rack/Preview、单 `WBP_FPCardView` Retainer、特殊区主卡身份、投影只读、0/1/3/15/21 张折叠与展开布局、1/7/15/21 张 CarryLayer 单锚点、动态卡面数量上限、牌堆状态同 Run 保留/新 Run 清空、单来源框选、标题拖动、Escape/B 清理和完整投放矩阵。

PIE 至少检查：

- 1280×720、1920×1080、16:10 和超宽下默认牌堆位置、边界夹紧与重置。
- 21 张备战卡展开、左右自适应、Hover 上抬、框选和详情。
- 整堆拖动、网格/边缘吸附、标题不重叠和主体部分重叠。
- 携带悬停自动展开、跨区移动、原子拒绝、批量销毁和剩余携带。
- 同 Run 关闭重开保留牌堆布局；新 Run 清空；Full/Simplified 动效均无淡入、动态缩放或首帧半透明。
