# B 类容器卡骨架 Stage 4.3

> 时间：2026-05-17
> 范围：GDD §11.2 B 类容器卡的数据契约 + 业务识别 + 测试卡

## 做了什么

按方案 B：把 B 类容器卡的"容量公式分流 + 枚举 API"做出来，UI 暂不渲染特殊存放区。

### 数据契约

- **新 GameplayTag** `Card.CapacityEffect.Placeholder`（`WacomCore`）：B 类容器卡的容量效果占位 tag，第一阶段用它走通骨架，等具体卡牌设计落地后扩展为 `Card.CapacityEffect.CostMinusOne` 等
- **`FCardPhysique` 加 `CapacityEffect: FGameplayTag`**：空 tag = A 类，非空 = B 类。`UPROPERTY` 加 `meta=(Categories="Card.CapacityEffect")` 限定可选范围

### 业务层（`URunSession`）

| 新 API | 行为 |
|---|---|
| `IsTypeAContainerCard(Card)` | 容器卡 + CapacityEffect 为空 |
| `IsTypeBContainerCard(Card)` | 容器卡 + CapacityEffect 有效 |
| `GetSpecialZoneCapacity(BCard)` | `Capacity - 1`（clamp 到 0）|
| `CollectTypeBContainers(OutCards)` | 枚举 Backpack + BattleDeck 里所有 B 类容器卡 |

**`GetFluxCapacity` 改为只 Σ A 类容器卡**。原来"所有 Capacity>0 卡都算"的实现替换。

### 内容

- **蛛茧绒囊** `DA_Card_ZhujianRongnang`（B 类占位测试卡）
  - Capacity = 3（特殊存放区容量 = 2）
  - CapacityEffect = `Card.CapacityEffect.Placeholder`
  - 白色 / Cost 0 / 不带 BagProvider
  - 进 BugGirl `StarterDeck`，Initialize 后落入 Backpack（容器卡走 a2 规则）
  - 玩家手上同时有小布袋（A，12）+ 蛛茧绒囊（B，3）时：Flux 仍 = 12

### 测试

`BackpackSpec.cpp` 新增 5 个 case：

1. `TypeAVsTypeBContainerPredicates` — 静态判定正反例
2. `SpecialZoneCapacity` — Cap=3→2、Cap=1→0、Cap=0→0、nullptr→0
3. `FluxCapacityOnlyCountsTypeA` — 混合 A/B/Normal 时 Flux 只算 A
4. `CollectTypeBContainers` — 跨 Backpack/BattleDeck 枚举 + 移动后仍可见
5. `OnlyTypeBProvidersStillUnlockBackpack` — 只有 B 类 BagProvider 时背包 UI 仍可用（Flux=0）

`Wacom.* 测试 79 → 84，全过`。

## 关键决策

### `CapacityEffect` 用 `FGameplayTag`

不用 FName / enum：
- 项目铁律：所有 tag 走 GameplayTag，避免字符串拼
- 扩展只需注册新 tag（`Card.CapacityEffect.CostMinusOne` / `AddTouchedKeyword` 等）
- `IsValid()` 天然区分 A/B

### Flux 公式从"全部 Capacity"改为"只 A 类"

是个语义修正：旧实现把 B 类也算进 Flux 是因为 B 类还没落地，临时简化为 A。
现在落地了 B 类机制，Flux 必须只算 A，否则 B 类既贡献通量又开辟特殊区，就重复计数了。

虫妹现状（小布袋 A=12）→ Flux 仍 = 12，无回归；
加蛛茧绒囊（B=3）后 → Flux 仍 = 12（B 不算），符合预期。

### UI 不渲染特殊存放区

Stage 4.3 范围限定为"骨架 + 数据契约"。BackpackScreen 的特殊存放区渲染留到 Stage 4.5，
等经济 / 具体容量效果设计明确后一起做（避免给将来必然返工的 UI 占位）。
玩家在背包界面看到蛛茧绒囊，照样能 Move/Delete，但它不在自己的特殊区里展开。

### 蛛茧绒囊不带 BagProvider

故意的：测 IsBackpackUiAvailable 判定与"是 A/B 类"是两条独立维度。
小布袋是 A + BagProvider，蛛茧绒囊是 B + 无 BagProvider，覆盖了对角线两个组合。

## 踩坑

- `CardPhysique.h` 新加 `FGameplayTag` 字段，需要 include `GameplayTagContainer.h`，否则 UHT 报错
- `FCardPhysique::CapacityEffect` 配 `meta=(Categories="Card.CapacityEffect")`，让蓝图编辑器只能选这个命名空间下的 tag

## 验证

- 编译：`Build.bat WacomEditor Win64 Development` 通过
- 资产重生：`-run=WacomRegenerateContent` 成功，蛛茧绒囊 .uasset 落盘
- 自动化测试：Wacom.* 84/84 全绿
- 手动验收待用户做：Standalone Game 中按 B 打开背包，应看到 7 张卡（5 伙伴 + 小布袋 + 蛛茧绒囊），Flux 显示 12 / 12

## 待办

- Stage 4.4：删牌卡（暮色引虫灯类，等具体卡设计）
- Stage 4.5：BackpackScreen 渲染 B 类特殊存放区 UI
- Stage 4.5：Effect 系统应用 CapacityEffect（让特殊存放区放进去的卡获得效果）
- 等具体容量效果设计落地后，扩展 `Card.CapacityEffect.*` 子 tag
