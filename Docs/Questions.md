---
type: question-index
scope: wacom-open-decisions
status: active
updated: 2026-07-18
tags:
  - wacom/questions
  - wacom/docs
---

# Questions

> [!info] 本文职责
> 本文只记录会影响规则、策划口径或长期架构的未决问题。当前规则事实看领域文档，短期任务看 [`TODO.md`](TODO.md)，未来方向看 [`Roadmap.md`](Roadmap.md)，已有技术债看 [`TechDebt.md`](TechDebt.md)。

> [!warning] 执行约束
> 本文不是规则真相、未来愿望池或短期任务清单。实现前必须先在对应领域文档中收口；不要把这里的问题静默写死到代码里。

<a id="questions-status"></a>
## 状态与触发时机

| 问题 | 当前事实 / 约束 | 需要确认 |
|---|---|---|
| 暮蛉被动修改对象 | `OnTwilightTriggered` 当前只发事件，不改 Magnitude | 是修改一张中毒卡牌、下一次中毒效果、所有中毒效果，还是按其他选择规则 |

---

<a id="questions-hand"></a>
## 手牌、区域与抽牌

| 问题 | 当前事实 / 约束 | 需要确认 |
|---|---|---|
| 手牌满时拂晓飞蛾回手 | 当前随机插入手牌后立即执行普通卡上限，超限卡进弃牌区 | 是否改为“手牌满时不触发”或“保证回手牌不被上限弃掉” |
| 右手牌永久删除 | 当前规则未建模左右手永久缺失字段；手牌锚点事实见 [WacomBattle.md](./WacomBattle.md) | 是否完全对称处理右手永久删除后的区域、击倒分支和 UI 可用性 |
| 左右手都永久删除 | 当前规则未建模左右手永久缺失字段；手牌锚点事实见 [WacomBattle.md](./WacomBattle.md) | 是否只剩普通手牌区，双手区和左右手区全部失效 |
| `Effect.Shuffle.ToRandomZone` 锚点缺失回退 | 当前腾挪可用区域随锚点存在性缩减 | 指向缺失区域时应失败、随机插入可用区域，还是降级到普通手牌区 |
| 是否需要正式 `DrawToZone` | `Effect.Draw` 当前从 Draw / Discard / Exhaust 入手后随机插入当前手牌 | 是否新增直接抽到指定 `HandZone.*` 的效果，还是保持 `Draw + Shuffle` 组合 |
| `DrawToZone` 指向不存在区域 | 依赖上一个问题 | 如果目标是不存在的 `HandZone.Both`、Left 或 Right，应失败、随机插入，还是降级到可用区域 |
| `DrawToZone` 与普通手牌上限 | 当前中途入手后立即执行普通卡上限 | 如果普通手牌已满，是先弃牌再放入保证新卡进手，还是先放入再按上限规则可能弃掉新卡 |

---

<a id="questions-knockdown"></a>
## 击倒与战后结算

| 问题 | 当前事实 / 约束 | 需要确认 |
|---|---|---|
| 击倒事件正式触发条件 | 部位 HP 归零时已有击倒事件框架；撤离只在仍有存活部位时可选 | 未来是否还受敌人类型、阶段、节点、剧情状态或特殊状态影响 |
| Aid / Destroy / Withdraw 其它具体效果 | Aid/Destroy 分支奖励字段、原子授予与文本预览已经完成；正式 Part 必须显式配置双分支，Withdraw 不获得卡 | Aid 是否另给左手 buff，Destroy 是否永久强化 / 破坏部位，Withdraw 是否触发特殊节点或地图回路 |
| 最后存活部位击倒 | 当前最后部位不可选 Withdraw | 是否所有敌人都保持该规则，还是部分 Boss / 事件敌人有例外 |
| 背包容量不足时的战斗奖励 | 当前 Victory 后 `AcquireCardToRun()` 加入 Run，再由负重 / 容量重算兜底 | 是否正式接受“溢出进负重区”，还是需要奖励选择、丢弃、邮寄或临时缓存 |
| 战内玩家受扣血事件 | 现有高 / 低 HP 阈值 flag 回传 Run 压力 | 是否还需要 `Passive.Trigger.OnPlayerDamaged` 表达每次受伤触发 |
| Run 失败统一总结交接 | Journey success 已有独立 Outcome/summary/event、passive Screen 与主菜单 handoff；Defeat、压力满和手指耗尽目前只结束活动 Run，不进入该成功页面 | 三种失败是否共用一个失败摘要；失败原因、战斗细节、进度统计与返回目标分别显示什么 |

Floor 1 八张分支奖励卡的费用、稀有度、关键词、效果、描述模板、package leaf、十一 Part 映射与 `14–17 / 20` 奖励量已由 Spec 013 关闭，不再作为开放问题。背包容量和其它非卡牌击倒后果仍按上表独立确认。

---

<a id="questions-run-map"></a>
## Run、探索与地图

| 问题 | 当前事实 / 约束 | 需要确认 |
|---|---|---|
| 自由探索 Run 边界 | 当前自由探索仍复用 `RunSession` | 是否新建区域探索 session，或继续让 `RunSession` 承载所有战外状态 |
| 突袭正式规则 | 文档中尚未收口 | 触发来源、先手规则、地图消耗、战斗初始化参数和逃离规则 |
| Camp Activity 内容 | Camp ticket、最近合法节点、取消、typed handler seam 与 Night→Morning 已落地；普通活动不减 Decay | Rest 的 Hunger / Fatigue 恢复值、资源成本、重复使用、卡牌强化正式事务及 Camp Screen |
| RunEvent 完成状态生命周期 | 当前按场景 `PersistentId` 记录，内存态保存 | 是否跨存档、跨日、跨地图保留；重复访问是否允许不同事件类型覆盖 |
| Shop 库存生命周期 | 当前按场景 `PersistentId` 在内存态保留 | 是否跨存档、跨日、跨地图保留；随机库存何时刷新 |

Floor 1 世界资产权威已由 Spec 015 关闭：采用独立新建的 `/Game/Wacom/Maps/Run/L_Run_Floor_Main_01`，不迁移、不覆盖 `L_Exploration`，也不复用 Debug map 作为 Production 权威。跨层 world handoff 和 Floor 2/3 场景仍是实现任务，不再是 Floor 1 权威选择问题。

Floor 1 本地启动口径由 Spec 016 关闭：完整 Production Journey 缺失期间，只允许 Editor PIE-only Preview GameMode 从关卡唯一 Descriptor 构造 transient 单层 Journey；主菜单和发行启动不切到该地图。地图上的 Preview override 是明确 release blocker，不再把“临时改用 GM_Wacom 或创建空壳正式 Journey”作为候选方案。

---

<a id="questions-ui"></a>
## UI 与功能可用性口径

| 问题 | 当前事实 / 约束 | 需要确认 |
|---|---|---|
| 删牌功能可用性 | 当前删牌规则、接口和 UI 技术债见 [WacomRun.md](./WacomRun.md) / [WacomUI.md](./WacomUI.md) / [TechDebt.md](./TechDebt.md) | 何时切换为“需要 DeleteProvider 才可删牌”，以及 UI 是隐藏、禁用还是提示来源 |
| AppToast 是否进入全局日志 | AppToast 当前只做战斗外即时反馈，不进 CommonUI Stack | 是否需要统一全局事件日志；哪些反馈应入日志，哪些只即时显示 |
| 战斗 Combat Log 保留范围 | `BattleHUD` 当前只在常驻滚动 `CombatLogFeed` 中保留本场最近命令块 | 是否需要战后回放、跨战斗历史或 Run 级日志 |
