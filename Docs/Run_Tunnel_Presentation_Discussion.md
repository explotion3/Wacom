---
type: discussion-draft
scope: wacom-run-presentation
status: draft
updated: 2026-05-27
tags:
  - wacom/run
  - wacom/presentation
  - wacom/exploration
  - draft
---

# Run 纸片隧道表现讨论稿

> [!info] 本文职责
> 本文只记录 Run 阶段第一人称纸片隧道表现的讨论和假设，不是正式实现规格。正式落地前，需要再拆成 `WacomApp` / `WacomUI` / `WacomRun` 的具体文档和任务。

## 1. 参考方向

当前参考对象是 Shroom and Gloom 一类“第一人称卡牌地牢”表现：

- 玩家主要通过 W / S 沿通道前后推进。
- 鼠标负责小范围观察、卡牌操作和场景物体交互。
- 画面不是完整自由 3D 关卡，而是多层手绘镂空贴图沿前方叠加，形成隧道深度。
- 摄像机可看的角度被限制在较小范围内，避免看到贴图侧边、层间空隙和未绘制区域。
- Run 过程大部分时间也有卡牌在 HUD 中，卡牌可与场景物体交互。

Wacom 可以学习的是交互语法和镜头约束，而不是完全复制内容机制。

## 2. 基础表现模型

把 Run 场景理解成“受限第一人称纸片隧道”：

```text
Camera / Pawn
  -> Layer 0: 近景镂空手绘贴图
    -> Layer 1: 中景镂空手绘贴图
      -> Layer 2: 远景镂空手绘贴图
        -> Interaction targets / branch / event / enemy
```

技术侧核心不是让玩家自由 FPS 移动，而是让摄像机沿美术可控路径移动。

建议初始口径：

- W / S：沿当前通道 Spline 前进或后退。
- 鼠标：小范围 yaw / pitch 观察，同时用于 UI 卡牌和场景交互。
- A / D：暂不做自由横移；如需要，只做很小的镜头摆动或以后再设计侧身机制。
- 摄像机 yaw / pitch：相对当前 Spline 朝向做 clamp。
- FOV：避免过广，优先保证贴图层不露边。

示例限制值仅作讨论：

| 项 | 初始建议 |
|---|---:|
| Yaw clamp | -12° 到 +12° |
| Pitch clamp | -8° 到 +8° |
| FOV | 60 到 75 |
| 横向位移 | V0 不做自由横移 |

## 3. 岔路：不转场的连续 Spline Graph

用户提出的重点是：Shroom and Gloom 的岔路不是“镜头转场 / 切下一个 tunnel rig”，而是玩家选择岔路后，Pawn 或 Camera 直接沿新的 Spline 推进。

这个方向值得学习。

原因：

- 沉浸感更强：玩家感觉自己真的在同一个空间内走向分岔，而不是从一个房间切到另一个房间。
- 更适合纸片隧道：岔路本来就可以靠前景遮挡、Y 形洞口和受限视角隐藏空间不连续。
- 更适合 Run 卡牌交互：地图、门、障碍、事件对象可以自然摆在岔路口，而不是弹出传统选择菜单。
- 更像“手工摆死的地牢舞台”：每条路径都是设计好的镜头轨道，美术可以精确控制每个角度看到什么。

推荐模型不是“多个独立 rig 之间切换”，而是一个连接图：

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

运行时：

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

如果需要更平滑，可以在切换 edge 时做短时间 tangent blend，但不要 fade / loading / 镜头黑屏。

## 4. 岔路美术遮掩规则

连续岔路是否不露馅，核心看美术和镜头约束是否一起设计。

建议规则：

- 岔路口要有专门的 junction layer，不要只把两条普通通道硬拼。
- Y 形或 T 形分叉处用前景遮挡、黑洞、门框、岩壁、树根等挡住层间接缝。
- 每条分支的入口要在最大 yaw / pitch 下仍有 overscan。
- 玩家未选择的分支可以保持暗、远、遮挡重，减少需要真实建模的区域。
- 分支选择后，摄像机沿新 Spline 的朝向自然转过去，贴图层也按新方向铺开。
- 对急转弯，优先用弯曲 Spline + 近景遮挡，而不是瞬间旋转 90°。

这意味着美术资产不是单张“通用隧道片”能解决全部问题；至少需要：

- 直道层组。
- 岔路口层组。
- 转弯层组。
- 门 / 障碍 / 事件物件层。
- 远景遮挡层。

## 5. UE 实现草案

V0 可以先放在 `WacomApp` 表现层，不让 `WacomRun` 依赖场景表现。

候选 Actor 形态：

| 类型 | 职责 |
|---|---|
| `AWacomRunTunnelGraphActor` | 持有或索引当前关卡的 tunnel nodes / segments |
| `AWacomRunTunnelSegmentActor` | 一条可移动通道，包含 `USplineComponent` 和若干贴图 layer |
| `AWacomRunTunnelJunctionActor` | 岔路口，声明可选出口、交互 target 和选择条件 |
| `AWacomRunRailPawn` 或现有 Character 扩展 | 沿 active Spline 移动摄像机 |

摄像机计算：

```text
BaseTransform = ActiveSpline.GetTransformAtDistance(Distance)
LookOffset = Clamp(MouseLookYawPitch)
CameraTransform = BaseTransform + LookOffset
```

贴图层建议：

- 用 plane mesh 或 custom quad。
- 材质优先 `Masked + Unlit`，减少透明排序问题。
- 每层挂在 segment actor 下，由美术手摆或按 layer distance 生成。
- V0 不急着做复杂 streaming，先验证一个手工 tunnel graph。

分支选择：

- 到达 junction 范围时暂停或减速推进。
- 场景分支目标使用现有 cursor trace / target component 思路。
- 玩家点击某个分支目标后，设置下一条 active segment。
- RunSession 是否消耗节点、触发事件、进入战斗，由 Run/App 的正式交互入口决定，tunnel graph 只负责表现移动。

## 6. 与 Wacom 当前系统的关系

推荐边界：

- `WacomRun`：保存 Run 规则状态、节点消耗、地图/事件结果，不知道 tunnel layer。
- `WacomApp`：负责 Pawn、Camera、Spline graph、世界交互、UI Router。
- `WacomUI`：负责 HUD 卡牌、背包、地图、提示、卡牌拖拽交互。
- 场景 Actor：只表达“这里有一个可交互目标 / 分支 / 入口”，不直接修改 RunState。

这和当前项目已有原则一致：

```text
场景表现 / 输入
  -> PlayerController / Router
    -> RunSession command
      -> Snapshot / UI refresh / scene feedback
```

## 7. 推荐结论

值得学习 S&G 的“连续岔路 Spline 推进”。

但建议不要把它理解成普通第一人称关卡，而是理解成：

```text
手工设计的镜头轨道图 + 受限第一人称视角 + 分层手绘贴图 + 卡牌化场景交互
```

相比“到岔路后转场切 rig”，连续 Spline graph 更适合 Wacom 的沉浸感目标。它的代价是美术和关卡需要按镜头限制共同制作，岔路口要专门设计遮挡和分支入口。

V0 最小验证建议：

1. 一个手工关卡里放 1 条直道 + 1 个 Y 形岔路 + 2 条短分支。
2. 摄像机沿 Spline 移动，yaw / pitch clamp。
3. 每条路径用 3 到 5 层 masked 手绘占位贴图。
4. 岔路点击目标后不转场，直接切到目标 Spline 继续推进。
5. 验证最大看角下是否露边、点击是否稳定、移动是否有沉浸感。

## 8. PCG 生成可能性

纸片隧道可以使用 PCG，但建议把 PCG 用在“结构和摆放”上，而不是一开始就尝试生成最终构图。

适合 PCG 的部分：

- RunTunnelGraph：节点、边、分支、深度和 seed。
- Spline 路径：直道、弯道、分支边的空间轨道。
- 普通 layer anchor：沿 Spline 按距离生成贴图层摆放点。
- 主题组合：按 theme / biome / phase 选择不同 layer set。
- 交互对象点位：门、宝箱、事件、敌人、商店、休息点等。
- 非交互装饰：蘑菇、藤蔓、碎石、暗角遮挡等。

不适合 V0 完全交给 PCG 的部分：

- 每张手绘镂空贴图本身。
- 岔路口是否不露馅的最终画面构图。
- 特殊事件场景、关键镜头和强叙事画面。
- 需要美术精确控制的前景遮挡和洞口形状。

推荐混合方案：

```text
Run 结构        -> PCG 生成
通道 Spline     -> PCG 生成或模板拼接
普通 layer      -> PCG 沿 Spline 铺放
岔路口 / 特殊房间 -> 手工 prefab / template
交互对象        -> PCG 放点，但交互仍走 App / Run 正式入口
```

岔路口尤其建议使用模板，而不是完全随机拼：

```text
JunctionTemplate_Y_LeftRight
JunctionTemplate_T_StraightLeft
JunctionTemplate_DoorChoice_2
JunctionTemplate_EventFork
```

PCG 只负责选择模板、放到节点位置、连接入口和出口 Spline。模板内部的遮挡层、洞口、前景框和远景黑场由美术手工保证。

阶段建议：

| 阶段 | 目标 |
|---|---|
| V0 | 手工放一条直道、一个岔路、两条分支，验证镜头 clamp 和不露馅 |
| V1 | 用 editor-time / construction-time 工具生成 Spline、layer anchors 和普通层 |
| V2 | 引入 seed、theme、junction templates，生成一小张可玩 Run 图 |
| V3 | 评估 runtime PCG；只有在加载、存档和性能边界清楚后再做 |

优先 editor-time 生成，而不是一开始 runtime 生成。原因是纸片隧道强依赖构图和遮挡，早期让美术可调比完全随机更重要。

## 9. 其他建议

### 先定镜头合同

纸片隧道能否成立，第一优先级是镜头合同，而不是 PCG。

需要尽早锁定：

- 最大 yaw / pitch。
- FOV 范围。
- W / S 推进速度。
- 是否允许后退。
- 是否允许停在岔路口自由观察。
- 鼠标用于看向四周时，是否同时影响 UI 卡牌拖拽。

这些值会直接决定美术需要画多宽、洞口要留多大、岔路能不能藏住拼接。

### 做一套 layer 制作规范

建议给美术一份最小 layer 规范：

- 画布比例和世界尺寸换算。
- 安全可视区和 overscan 区。
- 镂空洞口最小边距。
- 近景 / 中景 / 远景层的推荐距离。
- alpha 使用规则：V0 优先 masked，半透明留到后续验证排序。
- 命名和 theme tag 规则。

否则技术即使用 PCG 摆对位置，也很难保证不同贴图层组合后不露馅。

### Run 卡牌交互要和隧道一起验证

不要只验证“能在纸片隧道里走”。Wacom 的核心卖点应该是“在纸片隧道里用卡牌操作世界”。

V0 场景最好同时包含：

- 一个岔路选择目标。
- 一个可被卡牌作用的场景物体。
- 一个打开背包或地图的 Run 卡。
- 一个进入战斗或事件的远景目标。

这样能尽早发现鼠标看向、HUD 卡牌拖拽、场景 trace、UI focus 之间是否打架。

### 不要太早承诺完整随机地图

S&G 看起来更像强手工控制的地图。Wacom 即使未来用 PCG，也应该先追求“可控的半程序化”：

```text
设计师定义节奏和模板
PCG 填充路径、层组和普通交互点
关键节点保持手工调校
```

这比完全随机更符合当前项目阶段，也更容易做出稳定美术质量。

### 保留传统 UI fallback

即使 Run 交互逐步卡牌化，也建议保留调试和可访问性 fallback：

- 背包仍可用快捷键打开。
- 地图可有菜单入口。
- 关键交互失败时能显示明确提示。
- RunSession 入口不依赖某个特定场景 Actor 才能调用。

卡牌化是主要体验，不应该让调试和验证变得困难。
