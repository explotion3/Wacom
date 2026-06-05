---
type: archived-background
scope: wacom-run-presentation
status: archived
updated: 2026-06-05
tags:
  - wacom/run
  - wacom/presentation
  - wacom/exploration
  - archive
---

# Run 纸片隧道表现讨论稿

> [!warning] Archived
> 本文已归档，只记录 Run 纸片隧道表现的历史讨论和假设。它不是当前 Run Tunnel、Run 规则、输入或 UI 制作合同。

> [!info] 当前事实入口
> Run Tunnel movement、输入协调和战斗 suspend / resume 见 [WacomApp.md](./WacomApp.md)。Run 规则边界见 [WacomRun.md](./WacomRun.md)。世界交互和 target routing 见 [WacomWorldInteraction.md](./WacomWorldInteraction.md)。first-person hand 表现见 [First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)。

## 1. Reference Direction

历史参考对象是 Shroom and Gloom 一类“第一人称卡牌地牢”表现：

- 玩家主要通过 W / S 沿通道前后推进。
- 鼠标负责小范围观察、卡牌操作和场景物体交互。
- 画面不是完整自由 3D 关卡，而是多层手绘镂空贴图沿前方叠加，形成隧道深度。
- 摄像机可看的角度被限制在较小范围内，避免看到贴图侧边、层间空隙和未绘制区域。
- Run 过程大部分时间有卡牌在 HUD 中，卡牌可与场景物体交互。

Wacom 可借鉴的是交互语法和镜头约束，不是完整复制内容机制。

## 2. Presentation Model

历史表现模型：

```text
Camera / Pawn
  -> Layer 0: 近景镂空手绘贴图
    -> Layer 1: 中景镂空手绘贴图
      -> Layer 2: 远景镂空手绘贴图
        -> Interaction targets / branch / event / enemy
```

技术核心不是自由 FPS 移动，而是摄像机沿美术可控路径移动。

历史建议：

- W / S：沿当前通道 Spline 前进或后退。
- 鼠标：小范围 yaw / pitch 观察，同时用于 UI 卡牌和场景交互。
- A / D：不做自由横移；如需要，只做很小镜头摆动或后续侧身机制。
- 摄像机 yaw / pitch：相对当前 Spline 朝向做 clamp。
- FOV：避免过广，优先保证贴图层不露边。

历史讨论值：

| 项 | 建议 |
|---|---:|
| Yaw clamp | -12° 到 +12° |
| Pitch clamp | -8° 到 +8° |
| FOV | 60 到 75 |
| 横向位移 | 不做自由横移 |

## 3. Continuous Spline Branching

历史讨论的重点是：岔路不是“镜头转场 / 切下一个 tunnel rig”，而是玩家选择岔路后，Pawn 或 Camera 沿新的 Spline 推进。

这个方向的价值：

- 玩家感觉自己在同一个空间里走向分岔。
- 纸片隧道可以用前景遮挡、Y 形洞口和受限视角隐藏空间不连续。
- 地图、门、障碍和事件对象可以自然摆在岔路口。
- 每条路径都是设计好的镜头轨道，美术可以精确控制每个角度看到什么。

历史模型：

```text
RunTunnelGraph
  Node A
    Edge A->B: Spline + layer stack
    Edge A->C: Spline + layer stack
  Node B
    Edge B->D
  Node C
    Edge C->E
```

运行时概念：

```text
ActiveSegment = 当前 Spline edge
DistanceAlongSegment = 当前距离
CameraTransform = Spline transform + clamped look offset

到达 Junction Window:
  显示可选岔路 / 场景目标
  玩家用鼠标或卡牌选择目标分支
  ActiveSegment 切到目标 edge
  Camera 沿新 edge 的 tangent 继续推进
```

如果需要更平滑，可在切换 edge 时做短 tangent blend；新的实现合同应写入当前事实文档。

## 4. Art Masking Notes

连续岔路是否不露馅，核心看美术和镜头约束是否一起设计。

历史建议：

- 岔路口要有专门的 junction layer，不要只把两条普通通道硬拼。
- Y 形或 T 形分叉处用前景遮挡、黑洞、门框、岩壁、树根等挡住层间接缝。
- 每条分支入口要在最大 yaw / pitch 下仍有 overscan。
- 未选择的分支可以保持暗、远、遮挡重，减少需要真实建模的区域。
- 急转弯优先用弯曲 Spline + 近景遮挡，而不是瞬间旋转 90°。

这意味着美术资产不是单张“通用隧道片”能解决全部问题；至少需要直道、岔路口、转弯、门 / 障碍 / 事件物件和远景遮挡层。

## 5. UE Implementation Sketch

历史草案建议把 V0 放在 `WacomApp` 表现层，不让 `WacomRun` 依赖场景表现。

候选 Actor 形态：

| 类型 | 历史职责 |
|---|---|
| `AWacomRunTunnelGraphActor` | 持有或索引 tunnel nodes / segments |
| `AWacomRunTunnelSegmentActor` | 一条可移动通道，包含 `USplineComponent` 和若干贴图 layer |
| `AWacomRunTunnelJunctionActor` | 岔路口，声明可选出口、交互 target 和选择条件 |
| `AWacomRunRailPawn` 或现有 Character 扩展 | 沿 active Spline 移动摄像机 |

摄像机概念：

```text
BaseTransform = ActiveSpline.GetTransformAtDistance(Distance)
LookOffset = Clamp(MouseLookYawPitch)
CameraTransform = BaseTransform + LookOffset
```

贴图层历史建议：

- 用 plane mesh 或 custom quad。
- 材质优先 `Masked + Unlit`。
- 每层挂在 segment actor 下，由美术手摆或按 layer distance 生成。
- 早期先验证手工 tunnel graph，不急着做复杂 streaming。

分支选择历史建议：

- 到达 junction 范围时暂停或减速推进。
- 场景分支目标使用 cursor trace / target component 思路。
- 玩家点击分支目标后设置下一条 active segment。
- `URunSession` 是否消耗节点、触发事件或进入战斗，由 App / Run 正式入口决定；tunnel graph 只负责表现移动。

## 6. Relation To Wacom Architecture

历史边界与当前架构原则一致：

```text
场景表现 / 输入
  -> PlayerController / Router
    -> RunSession command
      -> Snapshot / UI refresh / scene feedback
```

模块分工：

- `WacomRun` 保存 Run 规则状态、节点消耗、地图 / 事件结果，不知道 tunnel layer。
- `WacomApp` 负责 Pawn、Camera、Spline graph、世界交互和 UI Router。
- `WacomUI` 负责 HUD 卡牌、背包、地图、提示和卡牌拖拽交互。
- 场景 Actor 只表达“这里有一个可交互目标 / 分支 / 入口”，不直接修改 RunState。

当前事实仍以 [WacomApp.md](./WacomApp.md)、[WacomRun.md](./WacomRun.md)、[WacomWorldInteraction.md](./WacomWorldInteraction.md) 和 UI 专题为准。

## 7. Archived Conclusion

历史结论：连续 Spline graph 比“到岔路后转场切 rig”更适合 Wacom 的沉浸感目标。

推荐理解方式：

```text
手工设计的镜头轨道图
  + 受限第一人称视角
  + 分层手绘贴图
  + 卡牌化场景交互
```

代价是美术和关卡需要按镜头限制共同制作，岔路口要专门设计遮挡和分支入口。

## 8. PCG Notes

纸片隧道可以使用 PCG，但历史建议把 PCG 用在结构和摆放上，而不是一开始生成最终构图。

适合 PCG 的部分：

- RunTunnelGraph：节点、边、分支、深度和 seed。
- Spline 路径：直道、弯道、分支边的空间轨道。
- 普通 layer anchor：沿 Spline 按距离生成贴图层摆放点。
- 主题组合：按 theme / biome / phase 选择不同 layer set。
- 交互对象点位：门、宝箱、事件、敌人、商店、休息点等。
- 非交互装饰：蘑菇、藤蔓、碎石、暗角遮挡等。

不适合早期完全交给 PCG 的部分：

- 每张手绘镂空贴图本身。
- 岔路口是否不露馅的最终画面构图。
- 特殊事件场景、关键镜头和强叙事画面。
- 需要美术精确控制的前景遮挡和洞口形状。

历史混合方案：

```text
Run 结构        -> PCG 生成
通道 Spline     -> PCG 生成或模板拼接
普通 layer      -> PCG 沿 Spline 铺放
岔路口 / 特殊房间 -> 手工 prefab / template
交互对象        -> PCG 放点，但交互仍走 App / Run 正式入口
```

历史阶段设想：

| 阶段 | 目标 |
|---|---|
| V0 | 手工放一条直道、一个岔路、两条分支，验证镜头 clamp 和不露馅 |
| V1 | 用 editor-time / construction-time 工具生成 Spline、layer anchors 和普通层 |
| V2 | 引入 seed、theme、junction templates，生成一小张可玩 Run 图 |
| V3 | 评估 runtime PCG；只有加载、存档和性能边界清楚后再做 |

## 9. Historical Notes

镜头合同优先于 PCG。Yaw / pitch、FOV、推进速度、是否允许后退、岔路观察方式和鼠标是否同时影响 UI 卡牌拖拽，会直接决定美术安全区和洞口尺寸。

Layer 制作规范需要单独成文时，应从当前美术需求出发，而不是复活本文作为正式规格。历史建议包括画布比例、世界尺寸换算、安全可视区、overscan、镂空洞口边距、layer 距离、alpha 规则、命名和 theme tag。

Run 卡牌交互需要和隧道一起验证。核心体验不是“能在纸片隧道里走”，而是“在纸片隧道里用卡牌操作世界”。当前 world interaction 事实见 [WacomWorldInteraction.md](./WacomWorldInteraction.md)。

即使 Run 交互逐步卡牌化，也应保留调试和可访问性 fallback：背包快捷键、地图菜单入口、失败提示和不依赖特定场景 Actor 的 RunSession 入口。
