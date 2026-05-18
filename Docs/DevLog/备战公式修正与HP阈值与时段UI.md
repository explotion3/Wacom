# 备战公式修正 + HP 阈值 flag 维护 + 时段 UI

> 时间：2026-05-17
> 范围：Stage 4 hotfix（备战区公式）、Stage 6（战内 HP 阈值跨越 flag）、Stage 5（探索 HUD 可视化）

## 任务 A：备战区容量公式修正

### 现象
Stage 4.3 加 B 类时把 `GetBattleDeckCapacity` 跟着改成"只算 A 类容器卡"，PIE 中虫妹背包显示备战 12/12，跟 GDD §11.5 例子（备战=15）不一致。

### 修
- `URunSession::GetBattleDeckCapacity()` 从 inline `return GetFluxCapacity()` 改为独立实现：Σ(全部容器卡 Capacity，A + B 都算)
- 加测试 `BattleDeckCapacityIncludesTypeB`
- 修正 Stage 4.3 加的 `FluxCapacityOnlyCountsTypeA` 测试中关于 BattleDeckCapacity 的旧断言
- 文档：WacomRun.md API 表 + 容量公式段
- TODO 项清掉

### 数值对照
- 玩家初始（虫妹）：通量 = 12（小布袋）+ 3（暮色引虫灯）= **15**；备战 = 12 + 3 + 3（蛛茧绒囊） = **18**

---

## 任务 C：Stage 6 战内 HP 阈值 flag 维护

### 规则分歧（先改文档）

GDD §10 写"LowHpThreshold = 0.8"，但 USER 在 #48 解释中说"血量减少 80% 触发 5% 伤口"——按这个含义 LowHpThreshold 应该是 **0.2**（HP 剩 20% 才触发，即减少了 80%）。

按项目铁律"先查文档→标注分歧→共识后先改文档再改代码"。本次走完整流程：

- 改 GDD §10 阈值常量化段：`LowHpThreshold 0.8 → 0.2`
- 加含义说明段：阈值是 `CurrentHp/MaxHp <` 比较，HighHpThreshold 大数值（伤得轻就触发，加成小），LowHpThreshold 小数值（伤得重才触发，加成大）
- 改 GDD §10 战内→战外回传段：`0.5 / 0.8 → 0.5 / 0.2`
- 改 WacomRun.md FRunState 字段表
- 改代码注释和默认值（RunState.h / BattleResultPacket.h / BattleState.h）

### 实现

- `FBattleInitParams` 加 `HighHpThreshold / LowHpThreshold` 字段（默认 0.5 / 0.2）
- `FBattleState` 加同名字段，`Initialize` 时从 InitParams 灌入
- `FBattleState::CheckHpThresholdsCrossed()` 内联辅助：检查 `CurrentHp / MaxHp < threshold`，首次跨越时设 flag（一次性 latching，不会回退）
- 玩家 HP 减少的两个路径调用：
  - `EffectHandlers::ApplyDamageToPlayer`（敌人意图伤害 + 卡牌伤害）
  - `PoisonResolver::ResolvePoisonForAllHosts`（玩家中毒结算）
- `URunSession::BuildInitParamsForBattle` 把 RunState 的阈值灌进 InitParams（让玩家自定义 RunState 字段时战内自动同步）

### 测试

新文件 `HpThresholdSpec.cpp`，5 个 case：

1. `NoCrossWhenAboveBoth` — HP=60 不跨任何阈值
2. `CrossesHighOnly` — HP=45 跨 High 不跨 Low
3. `CrossesBothInOneShot` — HP=10 一次跨两条
4. `FlagStaysTrueAfterHeal` — flag 是 latching，多次 EndTurn 不影响
5. `OnlyFirstTimeMatters` — 多次扣血只首次跨越

测试用自伤卡（Target.Player + Effect.Damage），`MakeSelfDamageCard` helper。

`Wacom.* 89 → 94，全过`。

---

## 任务 B：Stage 5 探索 HUD 可视化

### 改

`UWacomExplorationHUD` 从空占位扩成数据 HUD：

| 区域 | 内容 | 来源 |
|---|---|---|
| 左上 | 时段 / 剩余节点 / 第几天 | `URunSession::GetCurrentTimePhase / GetRemainingNodeCount / GetCurrentDayNumber` |
| 左下 | 手指数 / 经验进度条 / 已获得技能数 | `FRunState.FingerCount / ExperienceCurrent / ExperienceCapacity / AcquiredSkills` |
| 右上 | 8 条压力 + 总值 | `FRunState.Pressure` |
| 底部 | 操作提示（B / ESC） | 静态文字 |

实现方式：

- 全部 C++ `RebuildWidget` 构造 widget 树，纯文字 + 半透明黑底 `Border` 背景，避免压在天空盒上看不清
- `NativeTick` 每帧调 `RefreshFromRunSession` 全量刷新（RunState 更新频率低，开销可忽略）
- LOCTEXT 包裹所有显示文字（中文源）
- 经验值用 `UProgressBar` 配数字标签
- 时段显示用 `ETimePhase → FText` 的 namespace 内 helper

### 不做

- 美术样式 / 动画 / 图标（保留给美术阶段）
- 压力值条形图（第一版纯数字最直接）
- 阈值警示色（如压力 >70% 变红）
- 经验入账时的动画 / 音效

### 验证

PIE 重启进 L_Exploration 后，左右上下四个区域应同时显示 RunSession 数据。
战斗时 BattleHUD 叠在上面（Game 层 Push），ExplorationHUD 还是会显示在下层（CommonUI Stack 不自动隐藏底层 widget），后续 Stage 7+ 击倒 UI 时考虑加压制层。

---

## 验证

- 编译：`Build.bat WacomEditor` ✅
- 自动化测试：Wacom.* **94/94** 全绿（89 → 94，加了 5 个 HP 阈值测试 + 1 个备战公式测试 + Stage 4.3 的 1 个测试断言更新）
- 手动验收待用户做：
  - Standalone Game 进探索关卡，左右上下四个区域有数据显示
  - 按 B 打开背包，看到备战区 18 / 18 不再是 12 / 12
  - 战斗中触发玩家受伤跨越 50% 阈值后退出战斗，HUD 上 "伤口" 行显示 +1%

## 后续

- Stage 7：击倒事件 UI（玩家三选一：援助 / 破坏 / 撤离）
- Stage 8：地图系统（节点 + 通道 + 迷雾 + 撤离回路）
- Stage 9：节点事件（清晨规划 / 野炊 / 露营 / 探险 / 商店 / 事件）
- 美术阶段：HUD 视觉升级 / 动画 / 阈值警示色
