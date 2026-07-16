# Data Model: 背包真实卡牌牌堆与即时携带

## Card View Key

用于在多个视觉层和只读投影之间稳定复用 Widget。

| Field | Meaning |
|---|---|
| `InstanceId` | 卡牌实例身份 |
| `OwnerInstanceId` | 特殊区内容/主卡关联身份；普通卡为空 |
| `PhysicalZone` | 权威物理来源区 |
| `ReuseRole` | `PhysicalList`、`BattleDeckProjected`、`SpecialOwner`、`SpecialContent` |

相等性和 Hash 必须包含全部字段。父层、折叠状态和当前布局不属于身份。

## Workspace Card Scene Item

| Field | Meaning |
|---|---|
| `ViewKey` | 稳定视图身份 |
| `CardViewData` | 被动卡面内容 |
| `SourceZone` | 选择、携带与投放来源语义 |
| `PileZone` | 当前显示所属牌堆；通量卡为空 |
| `InteractionMode` | `Interactive` / `PileCollapsed` / `ReadOnly` |
| `ReadOnlyKind` | `None` / `BattleProjection` / `SpecialOwner` / `BurdenLocked` |
| `PresentationMode` | `Static` / `RealtimeHover` / `RealtimeCarryCurrent` |
| `BaseLayout` | StaticCardLayer 中的最终位置、角度、尺寸和 ZOrder |

### Invariants

- `PileCollapsed` 只禁止输入，不改变实体卡正常透明度。
- `ReadOnly` 永远不进入框选、携带或批量移动。
- 同一时刻最多一个 Scene Item 处于实时表现模式。

## Pile Scene View

| Field | Meaning |
|---|---|
| `ZoneKey` | 牌堆稳定身份 |
| `Title` / `Count` / `Capacity` | 标题与状态文本 |
| `Anchor` | 持久化标题锚点 |
| `ZOrder` | 牌堆层级 |
| `bExpanded` | 是否当前展开 |
| `ExpansionDirection` | `Left` / `Right` |
| `HeaderRect` | 拖动与防覆盖矩形 |
| `FrameRect` | 当前完整视觉框体 |
| `CardKeys` | 有序真实卡牌 ViewKey 列表 |
| `WarningState` | 投影、主卡、负重或容量状态摘要 |

## Pile Card Layout

| Field | Meaning |
|---|---|
| `Position` | 相对 Workspace 的卡牌左上角 |
| `Size` | 固定 `220×320` |
| `AngleDegrees` | 折叠为 0，展开为轻微扇转 |
| `ZOrder` | 最前卡最高 |
| `bInteractive` | 展开且非只读时为 true |

Solver 同时返回完整 `FrameRect`，确保标题、卡面和反馈区域都能被命中、夹紧和避让。

## Carry Presentation State

| Field | Meaning |
|---|---|
| `CardKeys` | 有序携带卡牌身份快照 |
| `CurrentIndex` | 当前可释放、位于最前的卡 |
| `AnchorLocal` | 最新 Workspace 局部鼠标锚点 |
| `LocalFanLayouts` | 各卡相对锚点的位置/角度/ZOrder |
| `LayoutRevision` | 仅结构变化时递增 |
| `PointerRevision` | 每次有效锚点变化递增 |

### Invariants

- 鼠标移动只更新 `AnchorLocal` 与 `PointerRevision`。
- `LocalFanLayouts` 只在开始、当前索引/数量、样式或几何变化时重算。
- 卡牌在 CarryLayer 中仍由原 `ViewKey` 标识，不能创建复制 Widget。

## Card Rendering Policy

| Mode | Retainer behavior | Dynamic effects |
|---|---|---|
| `Static` | 内容/状态变化时请求重绘 | 关闭 |
| `RealtimeHover` | 持续更新 | Fake3D/视差/阴影/表面 |
| `RealtimeCarryCurrent` | 持续更新 | Fake3D/视差/阴影/表面 |

退出实时模式时必须清空局部指针、旋转、材质参数和反馈残留，并请求一次最终静态重绘。
