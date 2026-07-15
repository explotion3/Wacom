# Contract: Map Authoring and Validation

## Data validation errors

以下情况必须使 Journey/Floor 无效：

- 空或重复 JourneyId/FloorId/NodeId/EdgeId。
- EntryNode 不存在。
- Edge 端点不存在、自环或引用其它 Floor 的 NodeId。
- 强制/入口节点从 Entry 不可达。
- NodeType 与 payload 缺失或错配。
- Treasure 同时配置 Pickup 和 Card Interaction，或两者都为空。
- FloorEntrance target 不存在、指向当前/过去 Floor、requirements 配置无有效正向筛选。
- 已声明支持角色和前置保证奖励均无法满足入口 requirements。

无法静态证明但不一定错误的随机/条件内容使用 Warning，不得静默当作已保证。

## Scene validation errors

- 当前 Floor 每个 Node 缺少唯一 NodeAnchor。
- 每条 Edge 缺少唯一 PathSegment。
- PathSegment/BranchTarget 使用未声明 EdgeId。
- Content node 缺少匹配 Host，或同一 Node 有重复权威 Host。
- Scene Actor 为复用现有 Trigger / Screen flow 可以保留内容 Definition façade mirror，但它必须与 Floor typed payload 完全一致；Floor DataAsset 始终是规则真相，Actor 字段不得生成或覆盖地图定义。

## Debug content builder

Editor-private 命令必须可重复运行：

- 创建/更新单一 Debug Journey 和 Debug Floor，不按次数追加节点或边。
- 迁移现有 `L_Exploration` 的可用 Battle/RunEvent/Shop/Treasure 内容引用到 Floor payload。
- 创建/更新新 Path Segment/Branch BP，复制已确认的 Spline、视觉和 CameraShake tuning。
- 写入稳定 NodeId/EdgeId/NodeBinding，配置 `GM_Wacom.DefaultJourneyDefinition`。
- 编译、保存并运行 Data/Scene validation。

命令不得从 Actor 连线反向生成正式图；Debug graph 由 builder 中的明确 fixture 定义，DataAsset 创建后仍为规则真相。

## Migration gate

删除旧反射类前必须同时满足：

1. Debug builder 成功。
2. `L_Exploration` 和相关 BP 全部重存。
3. Blueprint compile 0 error/0 failed load。
4. AssetRegistry 未发现旧类/字段/enum 引用。
5. 新 Traversal 与 first-person focused tests 通过。

最终源码、配置和资产中的旧 NodeCount/ConsumeNode、RunTunnelMovement/Segment/Branch 和 Prototype Redirect 为零引用。
