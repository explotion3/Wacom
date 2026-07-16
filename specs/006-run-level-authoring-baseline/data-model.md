# Data Model: Run 正式关卡制作基线收口

## 1. Run Floor Scene Descriptor

**Owner**: WacomApp / World

**Form**: 一个无 Tick、无 Collision、运行时隐藏的可放置 Actor。

| Field | Type | Authoring | Invariant |
|---|---|---|---|
| `FloorDefinition` | `UWacomFloorMapDefinition*` | `EditAnywhere, BlueprintReadOnly` | 非空；一个 World 只能有一个 Descriptor；`FloorId` 必须与当前 Run Snapshot 一致 |

**Does not contain**:

- Nodes、Edges、规则门槛或活动定义副本。
- Journey、目标 World 或下一层信息。
- 运行时 setter、Blueprint rule function、Tick 或可交互 Collision。

**Lifecycle**:

1. World 加载完成。
2. PlayerController 需要刷新 Run scene binding。
3. Resolver 枚举 Descriptor 并验证唯一、非空、FloorId 一致。
4. working registry 枚举 Anchor/Path/Branch/host。
5. 全部成功后一次性提交 registry 和 presentation binding。

## 2. Authoring Baseline Journey/Floor

**Owner**: WacomData assets，人工维护。

| Asset | Stable-for-this-slice identity | Purpose |
|---|---|---|
| `/Game/Wacom/Data/Map/Authoring/DA_Journey_LevelAuthoring` | `Journey.Authoring` | `GM_Wacom` 当前使用的过渡 Journey |
| `/Game/Wacom/Data/Map/Authoring/DA_Floor_LevelAuthoring_01` | `Floor.Authoring.01` | `L_Exploration` 当前关联的过渡 Floor 图 |

**Invariants**:

- 从当前已验证图复制 Node/Edge 内容，以保持现有 PIE 路径可玩。
- 不由 Debug builder 修改。
- 不承诺最终 Floor 1 节点数量、NodeId、布局或 SaveGame 兼容。
- `L_Exploration` Descriptor 与 `GM_Wacom` 默认 Journey 必须最终解析到同一 FloorId。

## 3. Debug Run Fixture

**Owner**: WacomEditor Debug builder。

| Asset | Identity / role | Builder access |
|---|---|---|
| `/Game/Wacom/Data/Map/DA_Journey_Debug` | Debug Journey | Read/write |
| `/Game/Wacom/Data/Map/DA_Floor_Debug_01` | Debug Floor graph | Read/write |
| `/Game/Wacom/Debug/GameModes/GM_WacomRunDebug` | DefaultJourneyDefinition 指向 Debug Journey | Read/write |
| `/Game/Wacom/Maps/Debug/L_RunExploration_Debug` | Debug actor fixture | Read/write |

**Read-only dependencies**:

- `/Game/Wacom/Core/Player/BP_WacomPlayerCharacter`
- `/Game/Wacom/Run/Path/Blueprints/BP_WacomRunPathSegmentActor`
- `/Game/Wacom/Run/Path/Blueprints/BP_WacomRunPathBranchTargetActor`
- `/Game/Wacom/Run/Path/Blueprints/BP_WacomRunMapNodeAnchorActor`
- 正式交互宿主 Blueprint（若 Debug map 需要放置实例）

依赖缺失或父类不符合合同即失败；builder 不创建替代共享资产。

## 4. Runtime Descriptor Resolution

**Form**: App-private 普通 C++ result。

| Field | Meaning |
|---|---|
| `Status` | 成功或稳定拒绝状态 |
| `Descriptor` | 唯一 Scene Descriptor weak pointer |
| `FloorDefinition` | Descriptor 引用的 Floor definition |
| `FloorId` | 已验证身份 |
| `World` / diagnostic context | 只用于诊断，不持久化 |

**Failure categories**:

- Missing descriptor
- Duplicate descriptor
- Null Floor definition
- Empty FloorId
- Snapshot Floor mismatch
- Invalid World / invalid Snapshot

失败不修改 `URunSession`、registry、coordinator、camera、HUD 或 first-person card layer。

## 5. Scene Binding Diagnostic

**Owner**: WacomEditor validation contract。

| Field | Type | Meaning |
|---|---|---|
| `Severity` | Info / Warning / Error | 稳定严重度 |
| `Code` | plain C++ enum | 可被自动化断言的稳定原因类别 |
| `ObjectPath` | string/soft path | 可定位 Descriptor、Actor 或 asset |
| `Message` | localized text | 给制作人员阅读的说明 |

**Core diagnostic code families**:

- `World.*`: invalid world, unsupported world type.
- `Descriptor.*`: missing, duplicate, null floor, invalid floor identity.
- `Node.*`: missing, duplicate, unexpected, invalid host/payload.
- `Edge.*`: missing, duplicate, unexpected, source/target mismatch.
- `Branch.*`: missing, duplicate, unexpected, illegal choice target.
- `Spline.*`: point count, zero length, non-finite transform, reversed direction, endpoint warning/error.

**Report invariants**:

- 诊断排序稳定：Severity → Code → ObjectPath。
- `IsValid()` 仅在无 Error 时为 true；Warning 不改变 commandlet 成功退出码。
- Validator 不调用 `Modify`、`MarkPackageDirty`、`SavePackage` 或 Actor setter。

## 6. Identities and Save Impact

- Floor/Node/Edge/Activity 身份仍由现有 WacomData 类型定义。
- Scene Descriptor 不生成新身份，只选择一个 Floor definition。
- Authoring identities 属于 pre-save 制作基线；未来正式 Journey/Floor 设计可替换。
- SaveGame schema、版本和迁移逻辑无变化。
