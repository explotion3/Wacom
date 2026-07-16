# Research: 正式 Floor 1 内容设计与稳定身份冻结

## Decision 1 — 冻结三层骨架，只完整设计 Floor 1

**Decision**: 正式 Journey 使用 `Journey.Main.01`，Floor 顺序固定为 `Floor.Main.01`（蛇巢浅林）、`Floor.Main.02`（蛇蜕洞窟）、`Floor.Main.03`（毒巢核心）。本轮只完整定义 Floor 1。

**Rationale**: Floor 1 的正式入口必须拥有稳定目标身份；同时 Floor 2/3 内容尚未设计，创建空壳图会把临时内容伪装成可用 Production 规则。

**Alternatives considered**:

- 只冻结 Floor 1：无法冻结跨层出口目标，正式图仍不完整。
- 完整设计三层：显著扩大内容和资产范围，超出当前首层冻结目标。
- 创建最小 Floor 2/3 空壳：可能绕过 Validator 意图并污染正式身份权威。

## Decision 2 — Floor 1 使用 20 节点/21 边双分岔结构

**Decision**: 主线经过教学 Encounter、两轮可回访分支、保证蛇印、精英、Boss 守卫和 Floor Entrance。第一轮两个三节点支路；第二轮一个两节点支路和一个三节点支路。

**Rationale**: 最短路线包含 8–9 AP 的正式内容，完整清理包含 14–15 AP，能同时满足明确推进方向、分支探索和约 1.5 天完整探索目标。

**Alternatives considered**:

- 继续使用当前 8 节点基线：只有约 3–4 AP 内容，不能承担正式首层节奏。
- 中心 Hub 辐射：免费 Map Travel 会放大重复往返，首层空间和制作难度更高。
- 纯线性 16 节点：接近 1 天且探索选择不足。

## Decision 3 — 稳定身份使用结构化、主题无关命名

**Decision**: Journey/Floor 使用 `Main` 序列；Node 使用 `Entry/Main/Junction/Route/Key/Guardian/Exit` 结构角色；Edge 使用 `Main/Route` 序列。展示文案和具体内容不进入稳定身份。

**Rationale**: 日志与制作面板仍可读，同时替换蛇种、奖励、文案或地图坐标不会迫使未来存档迁移。

**Alternatives considered**:

- 纯编号：最稳定但制作和诊断可读性差。
- 内容语义 ID：内容变更后容易出现 ID 与实际含义漂移。
- 复用 Authoring/Debug ID：会把过渡夹具身份冻结进正式内容。

## Decision 4 — Host PersistentId 从 FloorId 与 NodeId 派生

**Decision**: 所有非 Navigation 正式内容 Host 使用 `<FloorId>.<NodeId>` 作为 PersistentId，例如 `Floor.Main.01.Node.Route.A.01`。

**Rationale**: NodeId 只保证 Floor 内唯一；加入 FloorId 后可作为跨层唯一运行状态 key，同时避免再维护与逻辑节点无关的人工 Actor 身份。

**Alternatives considered**:

- 直接使用 NodeId：不同 Floor 可拥有相同结构 NodeId，存在跨层冲突。
- 使用资产名或 Actor Label：不属于规则身份，重命名和场景复制会漂移。
- 手工维护独立 PersistentId：增加第二套身份注册表和错绑风险。

## Decision 5 — Production 内容只冻结职责和预留 ID

**Decision**: 为 15 个内容节点冻结新的 Encounter/Event/Shop/Pickup ID、内容角色、风险/收益类别和 AP 口径；具体敌人槽、事件选项、库存、奖励数值留给后续内容制作。

**Rationale**: 当前只有 `DA_Encounter_SnakeSingle` 不带 Debug 语义，其余 Event/Shop/Reward 主要是 Debug 夹具。直接引用会把测试内容晋升为权威；本轮若同时冻结全部数值又会扩大到完整内容设计。

**Alternatives considered**:

- 允许 Debug 占位：污染 Production 权威并让后续替换难以审计。
- 同轮冻结全部规则数值：需要新的卡牌、事件、商店和 Encounter 设计审阅。
- 只写节点类型、不预留内容 ID：后续各工种仍会产生命名分叉。

## Decision 6 — 四个 Navigation 节点都允许 Camp

**Decision**: `Node.Entry` 与三个 Junction 的 `bAllowsCamp` 都为 true；Camp 仍是 Night 事务，不成为节点类型。

**Rationale**: 首层承担教学和内容基线，全结构落点提供清晰恢复位置；具体 Camp Activity 与恢复数值尚未交付，不应借地图设计写死。

**Alternatives considered**:

- 只允许 Entry/Boss 前：路线压力更强，但首次首层容错较低。
- 新增专用 Camp 节点：违背当前 Camp 不是 Map Node 的正式合同。

## Decision 7 — 分支不固定左右价值语义

**Decision**: 两轮分支的内容组合独立设计，不把左路永久定义为援助、右路永久定义为风险。

**Rationale**: 用户明确选择保留分支内容的不确定性；地图仍可通过 DisplayName、轮廓和揭示信息表达当前路线差异。

**Alternatives considered**:

- 两轮统一左援助/右风险：易学但路线过度可预测。
- 只在第一轮教学左右语义：仍会形成半固定空间承诺。

## Decision 8 — 保留非消耗蛇印门槛，但阻塞生产实现

**Decision**: `Node.Key.01` 保证奖励 `Card.Run.SerpentSigil`；`Node.Exit.01` 非消耗检查该 CardId。正式 DataAsset/地图不得实现，直到任务凭证不可移除或可恢复的规则完成。

**Rationale**: 当前 Validator 能证明必经 Treasure 保证获得卡牌，却不能证明玩家之后不会永久移除它。Pickup 完成后也不会自动补发，因此直接落地会产生不可恢复软锁。

**Alternatives considered**:

- 取消钥匙，只保留守卫：最安全，但用户选择保留钥匙身份和入口语义。
- 静默把蛇印做成普通卡：无法消除软锁。
- 本轮扩展任务卡 schema/Run 规则：违反本轮纯设计冻结边界。

## Decision 9 — 本轮只修改规划与长期文档

**Decision**: 不编译、不运行 Automation/PIE、不加载或保存资产；只做静态合同验证、跨文档一致性和 Git 审计。

**Rationale**: 没有源码、资产、Config 或运行时变更。启动 Unreal 不能增加有效覆盖，反而可能产生二进制序列化漂移。

**Alternatives considered**:

- 创建完整 Production Journey DataAsset：被蛇印软锁和缺失 Floor 2/3 有效图共同阻塞。
- 先把 `L_Exploration` 改成正式 Floor 1：会盲目覆盖尚未制作的 20 节点场景。
- 运行 Debug builder 验证：不涉及本轮文档合同且会写二进制资产。
