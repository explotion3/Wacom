# Data Model: Run 探索规则核心重构

## Static authoring entities (`WacomData`)

### JourneyDefinition

| Field | Meaning | Validation |
|---|---|---|
| JourneyId | 稳定旅程身份 | 非空、项目内唯一 |
| SupportedCharacters | 可用于验证入口可满足性的角色集合 | 至少一项，引用有效 |
| Floors | 有序 Floor 列表 | 非空、FloorId 唯一、引用有效 |
| PhaseBudgets | Morning/Day/Dusk/Night/Sunrise 默认预算 | 非负，默认 2/6/2/2/1 |
| BaseDecayCurve | 按 JourneyDay 读取 | 首版整数日均返回 5 |
| OverstayDecayCurve | 按 FloorDay 读取 | 1–3=0、4=2、5=5、6=9、7+=12 |

### FloorMapDefinition

| Field | Meaning | Validation |
|---|---|---|
| FloorId | 稳定 Floor 身份 | Journey 内唯一 |
| EntryNodeId | 首次进入节点 | 必须存在 |
| Nodes | 节点定义有序数组 | NodeId 唯一 |
| Edges | 有向边有序数组 | EdgeId 唯一，端点存在 |

### MapNodeDefinition

| Field | Meaning | Validation |
|---|---|---|
| NodeId | Floor 内稳定身份 | 非空、唯一 |
| NodeType | Navigation/Encounter/RunEvent/Shop/Treasure/FloorEntrance | 必须有对应 payload |
| MapPosition | 仅地图 UI 排版 | 不参与规则距离或可达性 |
| bAllowsCamp | 是否允许作为 Camp 地点 | 不改变节点类型 |
| LandmarkVisibility | 普通、远景入口或 Boss 轮廓 | 只影响 Snapshot 展示事实 |
| Content | 固定 tagged payload | 非匹配字段必须为空 |

内容 payload：

- `Navigation`: 无内容引用，进入即可 Resolved。
- `Encounter`: EncounterDefinition + `bBoss`。
- `RunEvent`: RunEventDefinition。
- `Shop`: ShopDefinition，首次安全进入即可 Resolved，但商店可重入。
- `Treasure`: PickupDefinition 或 WorldCardInteractionDefinition 二选一。
- `FloorEntrance`: TargetFloorId + `OwnedCardRequirements[]`。

### MapEdgeDefinition

| Field | Meaning | Validation |
|---|---|---|
| EdgeId | Floor 内可诊断的稳定身份 | 非空、Floor 内唯一 |
| FromNodeId | 起点 | 必须存在 |
| ToNodeId | 终点 | 必须存在且不能等于起点 |

反向移动必须另配 Edge；局部沿 Spline 后退不会创建反向逻辑 Edge。

跨 Floor、历史摘要和 App/Run 公共结果统一使用 `MapEdgeHandle = FloorId + EdgeId`；场景 Registry 只在当前活动 Floor 范围内使用单独 `EdgeId`，不得把它当 Journey 全局身份。

### OwnedCardRequirement

每条 requirement 必须由一张真实持有卡满足：Definition/CardId 白名单为 OR，RequiredKeywords 为 AND，BlockedKeywords 为附加排除。多个 requirements 之间为 AND。检索范围包括 Backpack、BattleDeck、BurdenZone 和全部 SpecialZones，不消费实例。

## Runtime entities (`WacomRun`)

### RunTimeState

| Field | Initial/Rule |
|---|---|
| CurrentDayNumber | 1 |
| CurrentTimePhase | Morning |
| RemainingActionPoints | Morning budget 后自动扣除 Planning 的 1 点 |
| PhaseBudgets | 从 JourneyDefinition 复制到 working state，允许未来正式规则修改 |
| NightGate | Night 时为 AwaitingChoice；选择 Explore 后 Open，Camp 后进入 Camp transaction |

### RunExplorationState

| Field | Meaning |
|---|---|
| JourneyDefinition | 当前静态规则来源 |
| CurrentFloorId | 当前 Floor |
| CurrentNodeId | 已提交的逻辑位置；通道途中仍为源节点 |
| FloorProgress[] | 每层有序 NodeProgress 与只读历史摘要 |
| UnlockedEntranceIds | 首次成功后永久解锁的入口身份 |
| FloorEnteredDayNumber | 当前 Floor 开始占用的 JourneyDay |
| ExplorationStateVersion | 每次成功探索提交 +1，初始化成功为 1 |

### MapNodeProgress

```text
Hidden -> Revealed -> Visited -> Resolved
```

- 不允许逆向或跳过必要前置；Entry 可在初始化中从 Hidden 依次推进。
- 当前节点与出边目标揭示；旧 Floor 不再成为 travel target。
- Floor Entrance 与 Boss 可以输出 `LandmarkVisible` 远景展示事实，但不会因此从 Hidden 推进为 Revealed。
- Navigation、Shop 等安全可重入节点在首次安全到达后 Resolved。
- Encounter/RunEvent/Treasure 由成功内容结算推进 Resolved。

### TraversalTicket (Session-private)

| Field | Meaning |
|---|---|
| Token | 唯一且不可猜测的单次身份 |
| VersionBefore | Begin 时探索版本 |
| FloorId/EdgeId | 已验证的边 |
| SourceNodeId/TargetNodeId | 不从 App 反向提供 |
| State | Pending/Completed/Cancelled |

同一 Session 至多一个 pending traversal。Begin 不改变 CurrentNode；Complete 原子切换；Cancel 无地图副作用。

### NodeActivityToken (Session-private)

| Field | Meaning |
|---|---|
| Token | 单次活动身份 |
| NodeHandle | 当前内容节点 |
| Kind | Encounter/RunEvent/Shop/Treasure；Camp 使用独立 `FRunCampTicket` |
| ReservedActionPoints | 固定成本活动的预留点数 |

Shop first-purchase、RunEvent visit 等本次访问事实保存在各自 Session-private transaction state，不塞入通用 ticket。

Traversal、node activity、Camp、Floor transition confirmation 互斥。Token 不进入 SaveGame。

### FloorTransitionConfirmation

Request 阶段返回当前版本、目标 Floor、已知未完成数量、未知区域提示、压力和 requirement facts。Confirm 必须携带同一 token 与版本；成功后保留时间/AP/压力，初始化目标 Floor，并失效所有旧 Floor travel target。

### Camp transaction

Begin 仅在 Night、空闲且至少 1 AP 时成功；通过 directed BFS 选当前或最近 `Resolved + bAllowsCamp` 节点，同距离按 NodeId。完成一个 typed handler 后提交预留、清空剩余 Night AP、跳过 Sunrise、进入下一 Morning。Cancel 释放预留；已发生的免费 relocation 不回滚。

## Explicit result entities

### RunInitializationResult

- Status
- Initial ordered events
- PostSnapshot
- 成功版本 1；失败事件为空且旧 Session 状态不变

### RunExplorationResolution

- Status
- VersionBefore/VersionAfter
- Ordered domain events（只携带稳定身份/语义，不携带 Actor）
- PostSnapshot
- 可选 traversal/camp/floor-confirmation token facts

成功结果要求 `VersionAfter = VersionBefore + 1`；失败版本相等、事件为空。

### RunExplorationSnapshot

包含未来 Map UI 和 App 所需的只读 facts：当前 Journey/Floor/Node、时间/AP、Floor Exposure、压力摘要、各节点 lifecycle/可传送性、出边可用性、active transaction kind、Night gate 和 Floor transition preview。它不包含 Spline、世界坐标、Widget 或 Actor。

## Persistence boundary

本轮不向 `UWacomSaveGame` 增加上述字段，不升级 schema。Node/Edge/Floor 身份从第一版起保持稳定，为未来 SaveGame 设计提供输入，但不能据此宣称当前地图可恢复。
