---
type: prototype-plan
scope: wacom-run-tunnel-spike
status: draft
updated: 2026-05-27
tags:
  - wacom/run
  - wacom/app
  - wacom/presentation
  - prototype
---

# Run Tunnel Exploration Spike V0

> [!info] 本文职责
> 本文记录纸片隧道探索 Spike V0 的实施边界。它不是正式 Run 规则文档，也不替代 `WacomRun.md`、`WacomApp.md` 或 `WacomUI.md`。

> [!note] 关联讨论
> 背景讨论见 `Run_Tunnel_Presentation_Discussion.md`。本 Spike 只验证最小表现和操作手感。

## 1. 目标

验证 Wacom 是否适合采用类似 Shroom and Gloom 的 Run 表现方式：

```text
受限第一人称视角
  + W/S 沿 Spline 推进
  + 多层手绘镂空贴图形成纸片隧道
  + 鼠标小范围观察和点击场景目标
  + 岔路选择后无转场沿新 Spline 继续推进
```

本轮成功标准不是“做完正式 Run 系统”，而是让策划、美术和程序能在 PIE 中直观看到：

- 纸片隧道是否有足够空间沉浸感。
- 镜头限制是否能遮住层间破绽。
- W/S 推进是否舒服。
- 岔路不转场切换 Spline 是否成立。
- 后续 HUD 卡牌交互是否有落点。

## 2. 非目标

V0 不做以下内容：

- PCG 生成。
- 正式 Run 地图生成。
- Run 卡牌拖拽与场景物体交互。
- 背包、地图、商店、RunEvent、战斗入口的正式规则接入。
- 存档、Run 节点消耗、事件结算。
- 正式美术资产规范。
- 多人、网络同步或运行时地图随机。
- 替换现有探索移动系统。

本轮只做可控原型，不把所有 Run 体验一次性定死。

## 3. 模块边界

| 模块 | 本轮职责 |
|---|---|
| `WacomApp` | 新增原型 Actor / Component，处理 Spline 推进、相机限制、点击岔路 |
| `WacomRun` | 不修改；Run 规则不依赖纸片隧道表现 |
| `WacomUI` | 不修改；HUD 卡牌交互后续单独接入 |
| `WacomData` | 不修改；不新增正式数据资产 |
| `WacomTests` | 可选补最小 C++ 自动化测试；PIE 视觉验证更关键 |

原则：

- 场景原型只提交表现和输入意图，不直接修改 RunState。
- 纸片 Layer 只是视觉，不承担点击和规则。
- 岔路点击只切换 active Spline，不结算 Run 事件。

## 4. 最小类设计

### 4.1 `AWacomRunTunnelSegmentActor`

一段纸片隧道通道。

职责：

- 持有 `USplineComponent` 作为摄像机 / Pawn 轨道。
- 作为美术手摆 Layer 的父 Actor 或组织容器。
- 提供 Spline 长度、指定距离 Transform 等查询。
- 可选记录默认进入距离和调试名称。

不负责：

- 玩家输入。
- Run 规则。
- 分支选择。
- 自动生成 Layer。

蓝图建议：

```text
BP_RunTunnelSegment
  - PathSpline
  - LayerRoot
    - Layer_00_Plane
    - Layer_01_Plane
    - Layer_02_Plane
    - Layer_03_Plane
  - OptionalMarkers
```

Layer 在 V0 中允许手工摆放。后续若方向成立，再考虑 editor-time 工具或 PCG 沿 Spline 生成 layer anchors。

### 4.2 `AWacomRunTunnelBranchTargetActor`

岔路点击目标。

职责：

- 暴露 `TargetSegment`。
- 暴露 `TargetStartDistance`。
- 拥有显式点击碰撞。
- 被点击后请求当前 tunnel driver 切换 active segment。

不负责：

- 判断 Run 节点是否可进入。
- 消耗节点。
- 打开事件、商店或战斗。
- 直接操作 `URunSession`。

V0 可以用 Visibility trace 或 Actor click 作为输入路径。若和 UI 输入冲突，优先采用 PlayerController 显式 cursor trace。

### 4.3 `UWacomRunTunnelPrototypeComponent`

原型驱动组件，建议挂在 `AWacomPlayerCharacter` 或 `AWacomPlayerController` 可访问对象上。

职责：

- 记录当前 `ActiveSegment`。
- 记录 `DistanceAlongSpline`。
- 处理 W/S 输入推进。
- 根据鼠标在屏幕内的绝对位置计算 yaw / pitch 目标值，并插值平滑到目标。
- 每帧把 Pawn / Camera 对齐到 Spline transform。
- 处理 branch target 切换。

核心状态：

```text
ActiveSegment
DistanceAlongSpline
MoveSpeed
LookYawOffset
LookPitchOffset
TargetLookYawOffset
TargetLookPitchOffset
YawClamp
PitchClamp
bTunnelPrototypeEnabled
```

相机计算：

```text
BaseTransform = ActiveSegment.Spline.GetTransformAtDistance(DistanceAlongSpline)
CameraRotation = BaseTransform.Rotation + ClampedLookOffset
CameraLocation = BaseTransform.Location
```

V0 可先 Tick 驱动。正式方案若成立，再考虑更干净的 movement mode 或 pawn class。

## 5. 输入口径

V0 输入：

| 输入 | 行为 |
|---|---|
| W | 沿 active Spline 正向推进 |
| S | 沿 active Spline 反向后退或减速后退 |
| 鼠标移动 | 显示鼠标；屏幕中心为正前方，屏幕边缘对应 yaw / pitch clamp 极值 |
| 左键 | 点击 BranchTarget 切换目标 Segment |
| A / D | V0 不做自由横移 |

初始建议参数：

| 参数 | 初始值 |
|---|---:|
| MoveSpeed | 220 cm/s |
| YawClamp | 12 deg |
| PitchClamp | 8 deg |
| LookInterpSpeed | 12 /s |
| FOV | 60 到 75，先沿用现有相机或手动调 |

点击路径建议：

```text
LeftMouseButton
  -> PlayerController GetHitResultUnderCursor(ECC_Visibility)
    -> 找到 AWacomRunTunnelBranchTargetActor
      -> PrototypeComponent.SwitchToSegment(TargetSegment, TargetStartDistance)
```

这和此前 Battle scene target click router 的经验一致：显式 cursor trace 比完全依赖 `Primitive.OnClicked` 更容易调试。

## 6. 视觉 Layer 口径

V0 Layer 由蓝图或关卡手摆，不由 C++ 强制管理。

推荐：

- 使用 Plane / Quad StaticMesh。
- 材质优先 `Masked + Unlit`。
- 贴图需要中间镂空，边缘有足够 overscan。
- Layer 不开交互碰撞。
- 可点击目标单独放 collision actor / component。

初始摆放建议：

| Layer | 距离示例 |
|---|---:|
| 近景 | 120 到 180 cm |
| 中近景 | 280 到 360 cm |
| 中景 | 480 到 600 cm |
| 远景 | 720 到 900 cm |
| 出口遮挡 | 视岔路口构图手调 |

实际效果必须以 PIE 玩家摄像机为准，蓝图视口只作为摆放工具。

## 7. 岔路切换口径

V0 岔路不做黑屏、不 fade、不打开传统选择 UI。

最小流程：

```text
玩家沿 Segment_A 前进
  -> 到达岔路口
    -> 左右分支目标可被点击
      -> 点击左侧
        -> ActiveSegment = Segment_Left
        -> DistanceAlongSpline = TargetStartDistance
        -> 继续沿 Segment_Left 推进
```

可选增强：

- 切换时做 0.2 到 0.4 秒 transform blend。
- 到达岔路窗口时自动减速。
- 未选择前禁止继续超过岔路终点。

V0 先做直接切换；如果观感突兀，再加 blend。

## 8. PIE 验证场景

建议创建或使用一个临时测试关卡，手工摆：

```text
Segment_Start
  -> Y Junction
    -> Segment_Left
    -> Segment_Right
```

每段至少有 3 到 5 层占位 layer。

岔路口需要：

- 左分支 target。
- 右分支 target。
- 足够前景遮挡。
- 最大 yaw / pitch 下不明显露边。

验证清单：

- 按 W 能平稳前进。
- 按 S 能后退或至少停住并反向移动。
- 鼠标移动到屏幕边缘时，镜头刚好触到 clamp 极值，不能看到 layer 侧边。
- 走到岔路口能点击左 / 右 target。
- 点击后无转场进入目标分支。
- BranchTarget 点击不被背景 layer collision 干扰。
- HUD / CommonUI 没有明显吞掉左键 trace。
- 退出 PIE 无残留 Actor、timer 或输入状态。

### V0 实际启用步骤

1. 在关卡中放置一个 `AWacomRunTunnelSegmentActor` 或它的蓝图子类，作为起始通道。
2. 编辑 `PathSpline`，让它表示玩家第一人称摄像机要经过的路径，而不是纸片 Layer 的中心线。
3. 在起始 Segment 上开启 `bAutoActivateOnBeginPlay`，并按需要设置 `AutoActivateStartDistance`。
4. 在 Segment 蓝图视口或关卡中手摆纸片 Layer；Layer 只负责视觉，建议禁用 Visibility 碰撞。
5. 为每个岔路出口放置 `AWacomRunTunnelBranchTargetActor` 或蓝图子类。
6. 在 BranchTarget 上填写 `TargetSegment` 和 `TargetStartDistance`；它自带 `ClickBounds`，默认 QueryOnly 并阻挡 `ECC_Visibility`。
7. PIE 后，起始 Segment 会在下一帧寻找本地 `AWacomPlayerCharacter`，激活它身上的 `UWacomRunTunnelPrototypeComponent`。
8. Tunnel 激活后会通过 `UWacomInputContextCoordinatorSubsystem` 切到 `Exploration + RunTunnel`：鼠标可见、CommonUI `All + NoCapture`、探索 IMC 保持启用；左键 Release 仍由 PlayerController 显式 trace 路由。
9. `W/S` 沿 active Segment 推进，鼠标位置驱动小范围看向四周；左键 Release 命中 BranchTarget 时直接切到目标 Segment。

### 当前实现备注

- `AWacomPlayerCharacter` 默认带 `UWacomRunTunnelPrototypeComponent`，但组件默认 inactive。
- Tunnel active 时，Character 的普通自由移动不执行；`A/D` 输入被忽略。
- 鼠标 look 不再使用隐藏鼠标的增量输入。屏幕中心表示正前方；鼠标移到左 / 右边缘时，对应 `YawClampDegrees` 的负 / 正极值；鼠标移到上 / 下边缘时，对应抬头 / 低头极值。
- `LookInterpSpeed` 控制镜头向鼠标目标角度的插值速度；设为 0 时立即贴合鼠标位置。
- Prototype component 会禁用 `CharacterMovement`，退出原型时恢复 `MOVE_Walking`。
- 进入战斗或探索输入被禁用时，Tunnel 原型只会 `Suspend`：暂停 Tick、停止消费 W/S 和分支点击，但保留 active Segment、距离和 `RunTunnel` 探索子模式。
- 战斗结束或探索输入恢复时，Tunnel 原型会 `Resume`：Coordinator 回到 `Exploration + RunTunnel`，并恢复进入战斗前的 Segment / Distance，继续纸片隧道控制。
- Spline transform 表示相机路径；组件会扣除第一人称 Camera 的相对位置，使摄像机落在 Spline 上。
- 左键点击由 `AWacomPlayerController::InputKey` 的 Release 路由处理；Battle scene target click 仍先执行，未消费时才尝试 RunTunnel branch click。
- V0 不做 Segment 切换 blend；如果 PIE 中突兀，再单独加 V0.1。

## 9. 风险与处理

| 风险 | 处理 |
|---|---|
| 镜头一转就露边 | 缩小 yaw / pitch，增大 layer overscan，调整 FOV |
| 纸片感太强，没有空间感 | 增加层数、强化前景遮挡、调整层间距离 |
| 岔路直接切换突兀 | 增加短 transform blend 或弯曲过渡 Spline |
| 点击命中背景 layer | Layer 禁用碰撞，BranchTarget 单独阻挡 Visibility |
| UI 吞左键 | 使用 PlayerController 输入路径兜底，或只在 prototype mode 下 route |
| 早期代码污染正式探索 | 加显式 prototype 开关，默认关闭，不替换现有探索系统 |

## 10. 完成标准

本 Spike 完成时应满足：

- 编译通过。
- PIE 中可以沿至少 3 条 Segment 移动。
- 受限视角能基本隐藏纸片层破绽。
- 左右岔路可点击切换，且没有转场。
- 代码不修改 `WacomRun` 规则，不改变现有战斗和 UI 主流程。
- 文档记录实际验证结果和后续建议。

## 11. 后续方向

如果 Spike 成立，再考虑下一步：

1. Run HUD 卡牌常驻展示。
2. Run 卡牌拖拽到场景目标。
3. Map / Backpack / Interact 等 Run 工具卡。
4. 岔路与 RunSession 节点、事件、战斗入口的正式 intent 接入。
5. Editor-time layer anchor 生成工具。
6. PCG 生成 RunTunnelGraph 和普通 layer。
