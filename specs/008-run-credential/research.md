# Research: Run 持久任务凭证

## R1 — 权威状态归属

**Decision**: Credential 是 `FRunState` 的独立稳定 `FName` 集合，由 `WacomRun` 唯一写入。

**Why**: 实体卡可以被销毁、卖出、支付或消耗；把入口资格建立在当前持有卡上会再次产生软锁。集合天然支持幂等和常数级查询。

**Rejected**:

- 继续使用 `OwnedCardRequirements`：卡牌离开持有区后资格丢失。
- 从 `CollectedPickupIds` 反推：把内容实例完成状态错误提升为通用任务资格，且未来多来源/任务奖励无法复用。
- GameplayTag：本轮身份是内容数据 FName，不需要扩展全局 tag 注册表。
- Actor/Widget 保存 bool：跨场景和 SaveGame 不可靠，也跨错所有权。

## R2 — 稳定蛇印身份

**Decision**: 使用 `Credential.Run.SerpentSigil`；表现卡继续使用 `Card.Run.SerpentSigil`。

**Why**: 独立命名明确区分授权与表现，且不把凭证寿命绑定到 Journey 资产路径。

**Rejected**: 复用 CardId 会鼓励调用者从卡牌推断资格；Journey-scoped 名称会把通用规则绑定到当前内容层级。

## R3 — 授予入口与事务边界

**Decision**: `UWacomRunPickupDefinition` 声明零到多个 `GrantedCredentialIds`；`AWacomRunRewardPickupActor` 提交完整 Definition；`URunSession` 在复制的 working state 中依次校验主奖励、授予 Credential、标记 Pickup、结算 Treasure 节点/AP，成功后一次替换权威状态并广播一次。

**Why**: 在旧 `CollectCardPickup` 成功后再次调用 Grant 会留下部分提交窗口。完整 Definition 入口同时保持旧 Card/Gold API 和旧 Debug 内容兼容。

**Rejected**:

- Actor 先拾卡再补 Credential：不是原子事务。
- Blueprint 公开 `GrantCredential`：允许任意表现层改写权威任务资格。
- 只支持单 Credential 字段：限制未来一个任务奖励解锁多个权限的复用能力。

## R4 — 卡牌移除独立性

**Decision**: Credential 只增不减，本轮不修改卡牌销毁、删牌换金币、RunEvent 支付或世界交互消耗实现。

**Why**: 这些路径只操作物理卡牌 storage；只要 Credential 不由 storage 重算，它们天然不会撤销凭证。回归测试负责证明边界。

**Rejected**: 在每个移除调用点加“保留蛇印”特判会重复规则并锁死具体 CardId。

## R5 — FloorEntrance 求值

**Decision**: `RequiredCredentialIds` 全部满足且所有 `OwnedCardRequirements` 满足时，入口条件通过；已解锁入口保持当前永久解锁行为。Preview、Request 和 Confirm 都读取当时最新 RunState。

**Why**: Credential 是新增通用条件，不应破坏现有持有卡条件。AND 语义最明确，也能组合不同门槛。

**Rejected**: OR 会意外放宽旧入口；按同名 CardId 自动满足 Credential 会重新耦合表现卡。

## R6 — SaveGame v4

**Decision**: 运行时 `TSet<FName>` 写为排序 `TArray<FName>`；v4 读取拒绝空或重复 ID。v3→v4 初始化为空，不从保存卡牌推断。

**Why**: 数组具备稳定磁盘布局，排序保证确定性；v3 时尚无合法 Production 蛇印资产，不存在可以可信迁移的旧资格事实。

**Rejected**:

- 根据蛇印卡推断：旧卡牌只代表表现/storage，不证明任务已合法授予。
- 拒绝所有 v3：没有必要破坏已有兼容链。
- 保存 TSet：磁盘顺序不稳定，不利于回归和审计。

## R7 — 制作校验

**Decision**: Pickup validator 验证 GrantedCredentialIds 非空唯一；Map validator 验证 RequiredCredentialIds 非空唯一，并复用当前图分析证明入口要求在其前方有固定 Pickup grant 节点支配入口。

**Why**: “存在某个来源”不足以防止玩家绕过来源到达门；支配关系才是保证获得。

**Rejected**: 只用路径顺序或 MapPosition 判断；两者都不是图规则真相。

## R8 — 验证与资产边界

**Decision**: 每个反射/规则切片编译 WacomEditor 并运行定向 Automation；不运行 builder、不保存 `.uasset/.umap`、不做 PIE。

**Why**: 本轮没有正式蛇印资产或入口可供 PIE；规则、迁移和静态校验应由自动化稳定覆盖。现有 Debug 资产用于零字段兼容/加载回归，不进行重建。

## Live-source facts

- 当前 Pickup 的 Gold/Card 结算已经使用 working-state 并统一提交，可抽取为数据驱动公共内部路径。
- 当前 FloorEntrance 只检查 `OwnedCardRequirements`，且 Confirm 会重新构建 Preview。
- 当前 SaveGame 版本为 3，序列化器先迁移再用临时状态应用，具备失败原子性基础。
- 正常 SaveGame 总开关仍关闭；探索图、金币、时间和 Pickup 完成状态等仍不是完整磁盘 schema，本轮不扩张。
- 当前 Map validator 已能检查保证卡牌来源和图支配关系，可按相同图事实增加 Credential 来源校验。
