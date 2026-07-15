# Research: Run 探索规则核心重构

## Decision 1: Logical Map Graph 与 Run Path 表现图分离

**Decision**: Journey/Floor DataAsset 保存稳定 Node/Directed Edge；Spline 和 Actor 只映射 EdgeId/NodeId。

**Rationale**: 非连续地图、死胡同、同层传送和不可逆跨层无法由当前关卡 Spline 连线可靠表达。规则图不依赖世界布局，才能独立测试并为未来地图 UI/存档保留稳定身份。

**Alternatives considered**:
- 继续把 Segment Actor 连线作为图真相：依赖关卡加载，无法可靠验证或保存。
- 从世界坐标自动生成图：会把美术摆放误当策划规则，不适合手工内容阶段。

## Decision 2: 使用两阶段 traversal ticket

**Decision**: 选择 Edge 只创建 ticket；到达终点才切 CurrentNode，回到起点取消。

**Rationale**: 玩家允许沿局部通道后退。点击即切节点会让中途返回、场景加载失败或暂停流程产生规则/画面分叉。

**Alternatives considered**:
- 点击分支即提交：实现简单但无法表达取消。
- 进入通道首帧提交：仍然无法正确恢复中途返回。

## Decision 3: 规则事务使用 working state 与显式结果

**Decision**: 初始化、地图命令和节点活动先在 working state 验证/执行，成功后一次提交并返回 Status、version、events 和 post snapshot。

**Rationale**: 当前 bool/void 入口与外部 `ConsumeNode` 允许部分状态提前修改。显式结果能让 App 幂等应用，失败结果不需要回滚 UI 猜测。

**Alternatives considered**:
- 继续广播后由 UI 拉 `FRunState`：无法区分本次事务事件，也难以防重复表现。
- 建立 Session 累计事件队列：重引入输出所有权和重复消费问题。

## Decision 4: 固定成本活动开始前预留 Action Point

**Decision**: Encounter/Camp 开始前预留；Victory/Camp completion 提交，启动失败或 Withdraw 释放。RunEvent/Shop/Treasure 在单次成功事务中直接验证并消费。

**Rationale**: 玩家不应完成整场战斗后才发现没有合法行动点；预留又能避免 Battle UI/加载失败时提前扣除。

**Alternatives considered**:
- 开始时立即扣除：异常启动需要补偿事务。
- 结束时再检查：可能产生无法提交的完整内容结果。

## Decision 5: Withdraw 不消费 Action Point但保留现有战果

**Decision**: Withdraw 释放预留，保留破坏部位、经验和卡牌奖励；不增加撤离专属压力，继续使用每场战斗 Fatigue +1、HP 阈值 Wound 与同归于尽 Wound +10。

**Rationale**: 这是用户最终确认的风险/收益口径。长期约束来自可恢复的战斗压力与每日 Decay，而不是额外撤离时间成本。

**Alternatives considered**:
- Withdraw 消费 1：被用户否决。
- 免费但回滚进度/奖励：被用户否决。
- 添加额外固定撤离压力：用户要求沿用现有压力来源。

## Decision 6: 新 Traversal 类整体替换旧 Run Tunnel 类型

**Decision**: 新建 PathTraversal/PathSegment/BranchTarget/NodeAnchor，资产迁移成功后删除旧类和 Redirect；复用 CursorLookDriver、CameraShake asset、ViewStage 和 first-person layer。

**Rationale**: 旧 MovementComponent 同时承担 Spline、输入 profile、settings、look、shake、stage request 和隐式 active/suspended 状态。保留类壳会继续传播旧 public API 和资产语义。

**Alternatives considered**:
- 原类内部瘦身：资产风险较低，但用户选择整体替换。
- 外包一层完全不改旧类：会保留直接 SwitchToSegment 和非法状态组合。

## Decision 7: 固定 v1 tagged node payload

**Decision**: `MapNodeType` 决定 Encounter/RunEvent/Shop/Treasure/FloorEntrance 的内联 typed payload；Validator 要求只配置匹配 payload。

**Rationale**: v1 节点类型已经固定，内联数据便于 UE Details 制作和校验，也不需要引入新的结构容器依赖或为每个节点创建子 DataAsset。

**Alternatives considered**:
- `FInstancedStruct` 多态 payload：扩展性强，但增加制作复杂度和依赖，当前类型集合不需要。
- 通用 UObject/content id：类型安全和编辑器提示不足。

## Decision 8: Camp 只落地生命周期和 typed handler seam

**Decision**: 核心管理合法性、最近节点、预留、active token 和 Night→Morning；具体 Rest/CardUpgrade/SpecialEvent/Backpack/Skill handler 后续设计。

**Rationale**: Hunger/Fatigue 恢复值、资源成本和卡牌强化事务尚未确认。写入临时数值会把未决内容伪装成正式规则。

**Alternatives considered**:
- 实现无效果 Rest：对玩家产生误导。
- 通用 effect array：会允许 UI/内容绕过正式卡牌和 Run 事务。

## Decision 9: SaveGame 明确延期

**Decision**: 新地图状态仅在内存运行，不提升 schema；Debug Journey 每次初始化为新图。

**Rationale**: 玩家档案、活动旅程和滚动备份已明确延后，当前先稳定必须持久化的身份和事务。

**Alternatives considered**:
- 同轮升级 SaveGame：扩大迁移面并过早冻结仍在变化的规则。

## Decision 10: 不新增 UE Module

**Decision**: 在 WacomRun 内建立 Private deep modules，而不是创建 `WacomMap` Build.cs Module。

**Rationale**: Map runtime 与 RunState、压力、背包持有区、Battle settlement 和事件事务高度内聚；独立 UE Module 会增加公共 API 和依赖边界而不带来独立复用价值。

**Alternatives considered**:
- 新建 WacomMap Module：会需要大量 Run 反向调用或把玩家状态抽到更低层，当前没有收益。
