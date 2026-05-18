# 删牌卡骨架 Stage 4.4

> 时间：2026-05-17
> 范围：GDD §11.7 删牌能力的"卡牌承载体"接口骨架 + 暮色引虫灯卡

## 做了什么

跟 Stage 4.3 同款骨架思路：把"删牌能力由谁提供"做成关键词 + 业务接口，UI 仍按 GDD §11.7 第一阶段约定始终显示删牌区，不接入识别。

### 数据契约

- **新 GameplayTag** `Card.Keyword.DeleteProvider`（`WacomCore`）：删牌能力的关键词承载

### 业务层（`URunSession`）

| 新 API | 行为 |
|---|---|
| `IsDeleteProviderCard(Card)` static | 卡是否带 DeleteProvider 关键词 |
| `IsDeleteFunctionAvailable() const` | Backpack 至少一张 DeleteProvider 卡 |

**当前不被任何代码路径调用**，纯接口就位。等规则从"始终显示"切换到"按需可用"再连。

### 内容

- **暮色引虫灯** `DA_Card_MuseiYinchongdeng`
  - Capacity = 3，**A 类容器卡**（CapacityEffect 为空，计入 Flux 公式）
  - Keywords = `[DeleteProvider]`（不带 BagProvider，与小布袋职责正交）
  - 白色 / Cost 0 / 无主动效果
  - 进 BugGirl `StarterDeck`，Initialize 后落入 Backpack
  - 加进去后玩家初始 Flux = 12（小布袋）+ 3（暮色引虫灯）= **15**，与 GDD §11.5 例子一致
  - 蛛茧绒囊（B，Cap=3）仍不算入 Flux

第一阶段简化（GDD §4.4 → 留待长期）：
- 不读 `Durability`（TODO 已记）
- 不实现战斗主动效果
- 任务后升级远期不做

### 测试

`BackpackSpec.cpp` 新增 4 个 case：

1. `IsDeleteProviderCard` — 静态判定正反例（含同时带 Bag+Delete 的卡）
2. `IsDeleteFunctionAvailable` — Bag-only / 含 Lantern 两个场景
3. `DeleteFunctionLostAfterDestroy` — 销毁 Lantern 后 false
4. `BagAndDeleteProvidersIndependent` — 只有 DeleteProvider 时删牌可用、背包 UI 不可用

`Wacom.* 测试 84 → 88，全过`。

## 关键决策

### 暮色引虫灯第一阶段是 A 类

GDD §11.5 例子里就写了"第一阶段简化为 A 类"，它的"删牌能力"用关键词承担，不是用 CapacityEffect 承担。
两个职责正交：CapacityEffect = "放进特殊存放区的卡获得效果修饰"，DeleteProvider = "背包获得删牌能力"。

### `IsDeleteFunctionAvailable` 暂不接入调用点

GDD §11.7 第一阶段约定"删牌区始终显示，不与具体卡关联"。
强行让 UI 读这个判定 → 玩家初始没暮色引虫灯就没删牌区，违反 GDD。
强行让 `DeleteCardForGold` 校验 → 同上。
所以接口就位，调用点等规则切换。这避免了"造假行为"和将来必然返工。

### 暮色引虫灯不带 BagProvider

GDD 没给它 BagProvider，只是个"开了删牌能力的容器卡"。两个关键词在数据层正交。
测试用 `BagAndDeleteProvidersIndependent` 锚定这个不变量。

## 踩坑

- 第一次 str_replace 时不小心把蛛茧绒囊段也吞了，立刻补回。教训：替换 oldStr 要选小不选大。

## 验证

- 编译：`Build.bat WacomEditor` 通过
- 资产重生：`-run=WacomRegenerateContent` 成功，暮色引虫灯 .uasset 落盘
- 自动化测试：Wacom.* **88/88** 全绿（84 → 88）
- 手动验收待用户做：Standalone Game 中按 B 打开背包应看到 8 张卡（5 伙伴 + 小布袋 + 蛛茧绒囊 + 暮色引虫灯），Flux 显示 `15 / 15`

## 待办

- 等具体卡设计落地后可能新加：暮色引虫灯任务后升级、耐久消耗、战斗主动效果
- Stage 4.5：BackpackScreen 渲染 B 类特殊存放区 + 接入 IsDeleteFunctionAvailable 判定（如果届时 GDD 切换为按需可用）
- TODO §1：备战区容量公式应改为 ΣAll（A+B 都算），跟通量公式分开
