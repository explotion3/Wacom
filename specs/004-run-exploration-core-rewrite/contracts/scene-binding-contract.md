# Contract: Run Scene Binding and Path Presentation

## Reflected authoring types

- `UWacomRunPathTraversalComponent`: 局部 Spline 跟随和 View Source。
- `AWacomRunPathSegmentActor`: 一个 EdgeId 对应的 PathSpline。
- `AWacomRunPathBranchTargetActor`: 只保存/上报 EdgeId，不保存 TargetSegment 规则连接。
- `AWacomRunMapNodeAnchorActor`: NodeId 到世界落点/View pose 的映射。
- `UWacomRunMapNodeBindingComponent`: Battle/Shop/Event/Treasure Host 的 NodeId 映射。

Actor 不保存 Floor 图、目标 NodeId、行动成本、内容完成状态或地图 UI 坐标。

## Traversal component state

```text
Inactive | Anchored | Traversing | Suspended
```

- Anchored 消费越过阈值的一次性导航意图：W 请求自动前进或提示选择，A/D / 左摇杆请求左右切换；按住期间不重复，释放后才能再次触发。
- 只有 Traversing 把 W/S 轴解释为连续移动并改变 Spline distance。Anchored Tick 只更新 Cursor Look / View Transform，不修改路径距离或启动移动镜头反馈。
- 到达 start/end 使用 hysteresis/one-shot latch，每次 traversal 每个边界最多广播一次。
- Suspended 保留 path/distance，停止移动和 CameraShake；Resume 不重建规则 ticket。
- 组件提供当前 View Transform 与 ViewStage return request，不拥有 RunSession 或 Map legality。
- Cursor look 继续委托现有 CursorLookDriver；Camera motion 继续消费 Local Settings delegate，无新增 Tick 轮询设置。

## Route choice presentation

- `OutgoingEdges` 为空为 `DeadEnd`；有结构出口但没有 `bCanTraverse=true` 为 `Unavailable`；1 条合法 Edge 为 `Automatic`；2 条以上为 `ChoiceRequired`。
- `Automatic` 在首次 W 时复用正式 Begin traversal；`ChoiceRequired` 的 W 没有规则副作用，只提示并脉冲合法入口。
- BranchTarget 只为静态多出口节点制作。运行时只有当前 Snapshot 合法 Edge 的目标进入 Available/Focused；离开 Anchored、Suspend、开始移动或普通 Session 重绑时隐藏。
- BranchTarget Blueprint 只能消费 Hidden/Available/Focused 视觉事件，不能决定合法性或直接调用 RunSession。

## Scene binding registry

Coordinator 在场景就绪时建立 Registry：

- `EdgeId -> PathSegment`
- `NodeId -> NodeAnchor`
- `NodeId + expected content kind -> presentation host`

重复身份、缺失绑定或类型不匹配使相关命令 preflight 失败。Registry 是 App runtime cache，不反向生成/修改 Floor DataAsset。

## Result application order

Begin traversal：

1. Registry 验证 Edge path、source anchor、target anchor 与目标节点预期 content host，并缓存 target transform。
2. Session 返回 Begin resolution/ticket。
3. Coordinator 记录 applied version。
4. Traversal component 进入 Traversing。

End traversal：

1. Component 广播 reached end。
2. Coordinator 在提交规则前重新验证 target anchor 与预期 content host；绑定已失效时提交 Cancel(ticket) 并回到 source anchor。
3. 绑定仍有效时提交 Complete(ticket)。
4. 规则失败时关闭输入并回到 source anchor，记录可诊断错误。
5. 规则成功后应用 PostSnapshot、定位 target anchor、触发 content host；成功提交后禁止回到 source anchor。若成功结果应用期间 Actor 意外失效，使用 Begin 时缓存的 target transform 保持逻辑/画面同侧、关闭后续输入并报告可诊断错误。

Back to start：提交 Cancel(ticket)，成功后回到 Anchored source。

## Existing peripheral reuse

必须保持默认行为：

- Cursor Look 强度与 invert Y。
- CameraShake 强度 0/0.5/1 与 stop grace。
- ViewStage return blend。
- first-person Anchor tick prerequisite、View Transform 和 Card Layer transition。
- Battle/Shop/Event Screen 的 CommonUI/input/focus 生命周期。

旧 Movement/Segment/Branch 类型仅在资产迁移中短暂并存；最终无 redirect/wrapper/fallback。
