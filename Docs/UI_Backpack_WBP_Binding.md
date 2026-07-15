# Backpack WBP 制作与绑定合同

> 本文记录背包正式 UI 的长期制作合同。背包容量、移动、投影、负重和销毁规则以 [WacomRun.md](./WacomRun.md) 为准；App 协调边界见 [WacomUI.md](./WacomUI.md)。

## 正式结构

背包正式结构是一个填满 `UI.Layer.GameMenu` 的 `UWacomBackpackScreen`，内部只有一个统一 `UWacomBackpackWorkspaceWidget` 负责卡牌、牌堆、框选、携带和整堆移动。通量区就是工作台空白区域；备战区、特殊区和非空负重区以工作台内嵌牌堆出现。右侧 `ZoneRack` 及其 Entry 已删除，不再是运行时或资产制作合同。

```text
Run Snapshot / revision
  -> UWacomBackpackScreen coordinator
  -> Workspace scene reconciler
  -> passive Workspace cards / ZonePile / PilePreview / DeleteConfirm
  -> Workspace intent
  -> Screen command flow
  -> URunSession atomic command
  -> one refresh -> reconcile
```

`ZonePileWidget` 和简化缩略卡只消费 ViewData，不访问 `URunSession`。规则校验、批量提交、Toast、销毁确认和失败恢复由 Screen 私有 flow 负责。

## 正式资产

| 资产 | 父类 / 用途 |
|---|---|
| `WBP_BackpackScreen` | `UWacomBackpackScreen`；全屏框架、Header、Workspace 和覆盖层 |
| `WBP_BackpackWorkspace` | `UWacomBackpackWorkspaceWidget`；统一 Canvas、框选和空状态 |
| `WBP_BackpackDeleteConfirm` | `UWacomBackpackDeleteConfirmWidget`；批量销毁确认 |
| `WBP_WacomSpecialZoneWidget` | `UWacomSpecialZoneWidget`；只供 fallback / 旧 Host 的只读兼容表现 |
| `DA_BackpackWorkspaceStyle` | `UWacomBackpackWorkspaceStyle`；工作台布局、牌堆、颜色和动效参数 |
| `WBP_BackpackCardView` | 静态 Retainer 卡面；工作台实体卡与投影卡的完整卡面 |

`UWacomBackpackZonePileWidget` 与 `UWacomBackpackPilePreviewWidget` 当前使用 C++ 被动 fallback 构造。缩略预览是独立固定尺寸组件，不缩放完整 `WBP_BackpackCardView`，不使用 Retainer、动态 Surface Foil 或卡牌输入。

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
| `SelectionMarquee` | `Border` | 框选矩形，初始 `Collapsed`，ZOrder 高于普通卡牌 |
| `EmptyStateText` | `TextBlock` | “通量区暂无卡牌”；不代表其它牌堆为空 |

`WorkspaceCanvas` 必须保持 `Visible`，不能改成 `SelfHitTestInvisible`，因为空白区域需要接收通量框选和释放。Screen 动态插入 Workspace 时必须按实际 Slot 类型设置 Fill，不能依赖 `UPanelWidget::AddChild` 默认槽位参数。

## 内嵌牌堆合同

- 备战区、特殊区是可移动普通牌堆；负重区是右上固定警告牌匣，销毁区固定在右下。
- 折叠普通牌堆最多显示三张简化缩略卡、标题、实体数量、容量及可选投影数量。
- 特殊区缩略预览第一张是不可操作的主卡身份封面，随后最多两张内容卡；展开后只生成内容卡实体。
- 备战投影卡在展开区保持只读、半透明并显示来源角标，不参与选择、框选、携带或批量移动。
- 只有标题拖柄可移动整堆；内容区域继续属于卡牌输入。标题点击切换展开，打开新堆时旧堆先收拢。
- 同时只展开一个牌堆。展开卡保持 `220×320` 逻辑尺寸与卡面 `0.75` 固定缩放，使用 `32–72px` 自适应露出和轻微扇转，不使用滚动。
- Hover 只上抬并置顶，不缩放、不改卡面透明度。
- 牌堆释放吸附到默认 `16px` 网格或邻近边缘；主体允许部分重叠，但标题拖柄不能相互覆盖，且始终夹紧在 Workspace 内。
- 普通动效只插值位置和角度；展开/收起默认 `0.18s`，吸附默认 `0.12s`。禁止卡面淡入和动态缩放。
- `Simplified` UI Motion 下展开、收起和吸附直接到达最终状态。

## 输入与事务合同

Workspace 交互模式互斥：`Idle / CardPress / Marquee / Carry / PileMove / Suspended`。

- 选择只属于一个来源区。通量背景框选只命中通量卡；展开牌堆背景只命中该牌堆卡。切换来源会清除旧选择。
- 当前框选仍使用“卡牌中心进入框选矩形”作为命中规则；接触即选属于后续体验优化。
- 空白 Workspace 是通量投放目标：通量卡只更新自由布局，其它实体卡通过现有原子移动进入通量区并保存释放位置。
- 放回来源牌堆等价于收拢；放到其它牌堆走 `MoveInstancesAtomic`；无效目标保持携带并显示拒绝反馈，不允许部分提交。
- 携带在合法折叠牌堆上停留约 `0.35s` 后自动展开；离开、取消、目标非法或来源 revision 漂移不得继续该自动展开请求。
- Escape 依次取消确认或瞬态交互、收起展开牌堆，再交给 CommonUI 关闭背包；B 始终直接关闭并清理瞬态状态。

## Retainer 与卡面

完整工作台卡使用静态 retained `WBP_BackpackCardView`。卡面保持固定 `220×320` Widget 逻辑尺寸、`CardFaceScaleBox=0.75`、单位 Render Scale 和完全不透明根节点；不能根据 Workspace 高度连续改写缩放。

背包 wrapper 默认关闭共享 first-person 卡面的动态 `SurfaceFoilOverlay`，避免 Retainer 缓存冻结动态材质帧。CommonUI Layer 过渡期间 Screen 暂停 retained caching 并直绘，过渡结束后恢复并请求最终补绘；不要用固定延迟或 Tick 轮询替代 PrimaryLayout transition delegate。

投影卡的 `0.72` 透明度是明确只读语义；普通卡、Hover、选中、展开和首次显示都必须保持 `1.0` 卡面透明度。

## Builder

正式资产由 `WacomBuildBackpackUI` 幂等生成：

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' '<Project>\Wacom.uproject' -run=WacomBuildBackpackUI -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache
```

Builder 必须编译并保存 CardView、Workspace、DeleteConfirm、兼容 SpecialZone、Screen 和 Style；不再创建 Rack 或 Rack Entry。连续运行不得恢复已删除 Rack 资产，也不得改变正式资产的绑定结构。

## Fallback 与迁移兼容

C++ fallback 保留 `DeleteZoneHost`、`BattleDeckZoneHost`、`FluxContentDropTargetHost`、`SpecialZonesHost`、`BurdenZoneHost` 等只读迁移 Host，用于缺失正式资产时观察 Snapshot；它们不拥有拖拽、移动或销毁输入，也不属于正式 WBP 制作合同。`CardDetailLayer` 仍可由 fallback 动态填充。

局部 `WBP_BackpackBattleDeckZone` 等旧 Zone 外壳可以继续作为只读 fallback 资源，但不得重新接入正式 Screen 或复制 Workspace 输入状态机。

## 验证

自动化至少覆盖：正式绑定无 Rack、静态 Retainer、特殊区主卡身份预览、投影只读、0/1/3/21 张手风琴、牌堆状态同 Run 保留/新 Run 清空、单来源框选、标题拖动、Escape/B 清理和完整投放矩阵。

PIE 至少检查：

- 1280×720、1920×1080、16:10 和超宽下默认牌堆位置、边界夹紧与重置。
- 21 张备战卡展开、左右自适应、Hover 上抬、框选和详情。
- 整堆拖动、网格/边缘吸附、标题不重叠和主体部分重叠。
- 携带悬停自动展开、跨区移动、原子拒绝、批量销毁和剩余携带。
- 同 Run 关闭重开保留牌堆布局；新 Run 清空；Full/Simplified 动效均无淡入、动态缩放或首帧半透明。
