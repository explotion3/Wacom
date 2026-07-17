# Contract: Journey terminal definition

## Public static contract

- `DisplayName` 是总结页显示标题；空值回退 `JourneyId`。
- `SuccessTerminalNode` 是唯一成功终局的 Floor-qualified handle。
- 未配置终局：Editor warning，Runtime 可启动，不会自动成功。
- 已配置非法终局：Editor error，Runtime 初始化原子拒绝。

## Configured terminal invariants

1. Floor 是 `Floors.Last()`。
2. NodeId 存在于该 Floor。
3. NodeType 为 Encounter。
4. Encounter payload `bBoss=true`。
5. 从 Floor Entry 可达。
6. 出度为 0。
7. 最后一层不包含任何 FloorEntrance。

Validator 与 Runtime 允许实现不同报告载体，但必须使用相同语义；Runtime 不依赖 Editor module。
