---
type: archived-background
scope: wacom-run-tunnel-spike
status: archived
updated: 2026-06-05
tags:
  - wacom/run
  - wacom/app
  - wacom/presentation
  - archive
---

# Run Tunnel Exploration Spike V0

> [!warning] Archived
> 本文已归档，只记录 Run Tunnel Spike V0 的历史背景和 PIE authoring 参考。它不是当前 Run Tunnel、Run 规则、输入或 UI 制作合同。

> [!info] 当前事实入口
> Run Tunnel movement、输入协调、战斗 suspend / resume 见 [WacomApp.md](./WacomApp.md)。Run 规则边界见 [WacomRun.md](./WacomRun.md)。世界交互和 target routing 见 [WacomWorldInteraction.md](./WacomWorldInteraction.md)。first-person hand 表现见 [First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)。

> [!note] 历史结论
> 本 Spike 的核心操作模型已被吸收到正式探索路径：`UWacomRunTunnelMovementComponent` 是探索期默认移动组件，普通隐藏鼠标 FPS FreeLook 不再是正式玩家路径。鼠标位置到 yaw / pitch offset 的算法已抽到共享 `UWacomCursorLookDriverComponent`，供 Run Tunnel 和 Battle camera look 共同使用。

## 1. Historical Goal

Spike V0 用来验证 Wacom 是否适合采用类似 Shroom and Gloom 的 Run 表现方式：

```text
受限第一人称视角
  + W/S 沿 Spline 推进
  + 多层手绘镂空贴图形成纸片隧道
  + 鼠标小范围观察和点击场景目标
  + 岔路选择后无转场沿新 Spline 继续推进
```

当时关注点是 PIE 中能否直观看到纸片隧道的空间沉浸感、镜头限制、W/S 推进手感、无转场分支切换，以及后续 HUD 卡牌交互的空间落点。

## 2. Archived Scope

Spike 只验证表现和输入手感，不定义正式 Run 规则。

当时明确不处理：

- PCG 生成或正式 Run 地图生成。
- Run 卡牌拖拽与场景物体交互。
- 背包、地图、商店、RunEvent、战斗入口规则接入。
- 存档、Run 节点消耗、事件结算。
- 正式美术资产规范。
- 多人、网络同步或运行时地图随机。

这些边界仍然成立：Run Tunnel 是 App / 表现 / 输入层能力，不写入 `FRunState`，不替代 `URunSession` 事务。

## 3. Module Boundary Notes

| 模块 | Spike 中的角色 | 当前事实入口 |
|---|---|---|
| `WacomApp` | Spline 推进、相机限制、点击岔路 | [WacomApp.md](./WacomApp.md) |
| `WacomRun` | 不依赖纸片隧道表现 | [WacomRun.md](./WacomRun.md) |
| `WacomUI` | HUD / first-person hand 交互专题承接 | [WacomUI.md](./WacomUI.md)、[First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md) |
| World Interaction | 场景 target、hover、click、drop routing | [WacomWorldInteraction.md](./WacomWorldInteraction.md) |

历史原则：

- 场景表现只提交输入意图，不直接修改 RunState。
- 纸片 Layer 只负责视觉，不承担点击和规则。
- 岔路点击只切换 active Spline，不结算 Run 事件。

## 4. Historical Class Sketch

### `AWacomRunTunnelSegmentActor`

历史用途：一段纸片隧道通道。

它持有 `USplineComponent` 作为摄像机 / Pawn 轨道，可作为美术手摆 Layer 的父 Actor 或组织容器，并提供 Spline 长度、指定距离 transform 等查询。

它不负责玩家输入、Run 规则、分支选择或自动生成 Layer。

### `AWacomRunTunnelBranchTargetActor`

历史用途：岔路点击目标。

它暴露 `TargetSegment / TargetStartDistance`，拥有显式点击碰撞，被点击后请求当前 tunnel driver 切换 active segment。

它不判断 Run 节点是否可进入，不消耗节点，不打开事件 / 商店 / 战斗，也不直接操作 `URunSession`。

### `UWacomRunTunnelMovementComponent`

当前正式探索移动组件，挂在 `AWacomPlayerCharacter` 上。

它记录 `ActiveSegment / DistanceAlongSpline`，处理 W/S 推进，通过 `UWacomCursorLookDriverComponent` 根据鼠标屏幕位置计算 yaw / pitch offset，并把 Pawn / Camera 对齐到 Spline transform。

正式输入协调和战斗进出暂停 / 恢复顺序见 [WacomApp.md](./WacomApp.md)。

## 5. Historical Input Model

| 输入 | 历史行为 |
|---|---|
| W | 沿 active Spline 正向推进 |
| S | 沿 active Spline 反向后退或减速后退 |
| 鼠标移动 | 显示鼠标；屏幕中心为正前方，屏幕边缘对应 yaw / pitch clamp 极值 |
| 左键 | 点击 BranchTarget 切换目标 Segment |
| A / D | 不做自由横移 |

历史建议参数：

| 参数 | 初始值 |
|---|---:|
| MoveSpeed | 220 cm/s |
| YawClamp | 12 deg |
| PitchClamp | 8 deg |
| LookInterpSpeed | 12 /s |
| FOV | 60 到 75 |

## 6. PIE Authoring Reference

历史测试关卡建议：

```text
Segment_Start
  -> Y Junction
    -> Segment_Left
    -> Segment_Right
```

每段至少 3 到 5 层占位 layer。岔路口需要左 / 右分支 target、足够前景遮挡，并确保最大 yaw / pitch 下不明显露边。

历史启用步骤：

1. 在关卡中放置 `AWacomRunTunnelSegmentActor` 或蓝图子类作为起始通道。
2. 编辑 `PathSpline`，让它表示第一人称摄像机路径。
3. 起始 Segment 可开启 `bAutoActivateOnBeginPlay` 并设置 `AutoActivateStartDistance`。
4. 手摆纸片 Layer；Layer 只负责视觉，建议禁用 Visibility 碰撞。
5. 为岔路出口放置 `AWacomRunTunnelBranchTargetActor` 或蓝图子类。
6. 在 BranchTarget 上填写 `TargetSegment` 和 `TargetStartDistance`。
7. PIE 后起始 Segment 寻找本地 `AWacomPlayerCharacter`，激活其 Run Tunnel movement component。

这些步骤保留为排查旧 PIE 场景的参考，不是新增内容的制作合同。

## 7. Current Implementation Notes

- `AWacomPlayerCharacter` 默认带 `UWacomRunTunnelMovementComponent`，组件默认 inactive，等待起始 Segment 或正式 Run flow 激活。
- `AWacomPlayerCharacter` 同时带 `UWacomCursorLookDriverComponent`；Run Tunnel 和 Battle camera look 共享 cursor offset 状态。
- Tunnel active 时，Character 普通自由移动不执行；`A/D` 输入被忽略。
- 鼠标 look 使用屏幕绝对位置，不使用隐藏鼠标的增量输入。
- Movement component 会禁用 `CharacterMovement`；退出或无 active Segment 时不恢复普通 FPS fallback。
- 进入战斗或探索输入被禁用时，Tunnel movement `Suspend`：暂停 Tick、停止消费 W/S 和分支点击，但保留 active Segment 和距离。
- 战斗结束或探索输入恢复时，Tunnel movement `Resume`：Coordinator 回到 `Exploration`，并恢复进入战斗前的 Segment / Distance。
- 左键点击由 PlayerController Release 路由处理；Battle scene target click 先执行，未消费时才尝试 RunTunnel branch click。
- Segment 切换 blend 不在本文定义；若后续需要，应进入当前事实文档和任务计划。

## 8. Historical Risks

| 风险 | 历史处理建议 |
|---|---|
| 镜头一转就露边 | 缩小 yaw / pitch，增大 layer overscan，调整 FOV |
| 纸片感太强，没有空间感 | 增加层数、强化前景遮挡、调整层间距离 |
| 岔路直接切换突兀 | 增加短 transform blend 或弯曲过渡 Spline |
| 点击命中背景 layer | Layer 禁用碰撞，BranchTarget 单独阻挡 Visibility |
| UI 吞左键 | 使用 PlayerController 输入路径兜底 |
| 早期代码污染正式探索 | 保持 Run 规则和表现输入分层 |

## 9. Historical Next Ideas

这些是历史方向记录，不是当前待执行清单：

- Run HUD 卡牌常驻展示。
- Run 卡牌拖拽到场景目标。
- Map / Backpack / Interact 等 Run 工具卡。
- 岔路与 RunSession 节点、事件、战斗入口的 intent 接入。
- Editor-time layer anchor 生成工具。
- PCG 生成 RunTunnelGraph 和普通 layer。

当前短期任务和长期方向以 [TODO.md](./TODO.md) 与 [Roadmap.md](./Roadmap.md) 为准。
