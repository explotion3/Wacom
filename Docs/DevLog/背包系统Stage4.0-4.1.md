# Stage 4.0 / 4.1 - 背包 + 备战卡组业务层

## 目标

按 GDD §11 重写后的规则实现"万物成卡"模式的背包系统：

- 背包能力本身由"BagProvider 卡牌"赋予，不是常驻系统
- 容量动态计算（容器卡贡献），不再是固定常量
- Backpack / BattleDeck 互斥（一张卡同时只能在一个区）
- 引入金币、删牌区入口
- 战斗用 BattleDeck 而非 StarterDeck

## Stage 4.0 数据契约迁移

- `FCardPhysique.BagCapacity` → `Capacity`（对齐 GDD 用词）
- 新增 `Card.Keyword.BagProvider` 关键词
- `FRunState.Gold: int32 = 0`

## Stage 4.1 业务层

### 数据契约

**`UCharacterDefinition` 不再加字段**（按用户决定，基础背包卡进 StarterDeck，方案 B）。

**`FBattleInitParams` 加字段**：
```cpp
TArray<TObjectPtr<const UCardDefinition>> BattleDeckOverride;
```

BattleSession::Initialize 优先用 Override 作为 StarterDeck 来源，空时回退 `Character->StarterDeck`（向后兼容现有 fixture）。

**`FRunState` 字段调整**：
- 删除 `BackpackCapacity`（容量改为动态计算）
- 加 `Gold` / `TheftCount`（Stage 2 已加）

### Initialize 行为（a2 规则）

```
StarterDeck 遍历：
  - 容器卡（Capacity > 0）→ Backpack
  - 非容器卡（Capacity = 0）→ BattleDeck
  - 一张卡同时只能在一个区
```

虫妹默认配置：
- 5 张伙伴卡（Capacity=0）→ BattleDeck
- 虫妹的小布袋（Capacity=12, BagProvider）→ Backpack
- 暮色引虫灯（Capacity=3）→ Backpack（容器卡，第一阶段简化为 A 类）

### URunSession 新增 API

| API | 行为 |
|---|---|
| `GetFluxCapacity()` | `Σ(玩家拥有所有 A 类容器卡 Capacity)` |
| `GetBattleDeckCapacity()` | = GetFluxCapacity（同公式） |
| `IsContainerCard(Card)` | 静态：Capacity > 0 |
| `IsBagProviderCard(Card)` | 静态：含 BagProvider 关键词 |
| `IsIntrinsicCard(Card)` | 静态：Rarity = Intrinsic |
| `IsBackpackUiAvailable()` | Backpack 至少含一张 BagProvider |
| `IsCardInBackpack / IsCardInBattleDeck` | 查询 |
| `AddCardToBackpack` | 加卡 + RecomputeBurden |
| `DestroyCardFromBackpack` | 永久销毁（Backpack 或 BattleDeck 任一找到都可，Intrinsic / 最后 BagProvider 拒绝，Companion 加嗜血） |
| `DeleteCardForGold` | 销毁 + 按稀有度发金币（白=1 / 蓝=2，占位） |
| `AddCardToBattleDeck` | 从 Backpack 移到 BattleDeck（互斥；容量上限拒绝） |
| `RemoveCardFromBattleDeck` | 从 BattleDeck 移回 Backpack（Intrinsic 拒绝） |
| `AddGold / RemoveGold / GetGold` | 经济操作 |

### RecomputeBurden 公式（GDD §3.2 / §11.4）

```
超出 GetFluxCapacity 的 Backpack 卡数 n → Burden = n*(n+1)/2
```

由 AddCardToBackpack / DestroyCardFromBackpack / Add/RemoveCardToBattleDeck / 玩家拥有容器卡变化时自动重算。

### 战斗联动

`URunSession::BuildInitParamsForBattle` 把 `RunState.BattleDeck` 拷给 `OutParams.BattleDeckOverride`。
现有 fixture 不填 Override 自动走老路读 `Character->StarterDeck`，零破坏。

## 虫妹的小布袋

新增资产：`/Game/Wacom/Cards/BugGirl/DA_Card_BugGirlBag`

- Cost = 0
- Capacity = 12
- Keyword: BagProvider
- Rarity: White
- 无主动效果

加进 BugGirlBuilder StarterDeck，Initialize 后进 Backpack。

## 测试

新加 `Wacom.Run.Deck.*`（17 个）：

静态判定：
- StaticPredicates

容量 / Initialize：
- InitializeA2SeparatesContainerFromNormal
- CapacitySumsAcrossBackpackAndBattleDeck（含 Add 移到备战后容量不变）
- BackpackUiAvailability

AddCard / DestroyCard：
- AddCardToBackpackRecomputesBurden
- DestroyIntrinsicRejected
- DestroyLastBagProviderRejected
- DestroyOneOfTwoBagProvidersAllowed
- DestroyCompanionAddsBloodlust
- DestroyAlsoRemovesFromBattleDeck

DeleteCardForGold：
- DeleteCardForGoldByRarity

BattleDeck 操作：
- AddToBattleDeckRequiresBackpack
- AddToBattleDeckRespectsCapacity
- RemoveFromBattleDeckIntrinsicRejected
- RemoveFromBattleDeckSucceeds

战斗联动：
- BuildInitParamsUsesBattleDeck

经济：
- GoldAddRemove

## 修过的旧测试

Stage 1.1 / Stage 2 的部分压力 / Initialize 测试因为新规则下"普通卡只在 BattleDeck 不在 Backpack"做了适配（构造时显式给容器卡或调整断言）：

- `Wacom.Run.State.InitializePopulatesFromCharacter`
- `Wacom.Run.State.PressureCapFailsRun`
- `Wacom.Run.Pressure.BurdenZeroWhenWithinCapacity`
- `Wacom.Run.Pressure.BurdenSetByOverCount`

## 验证

- 编译：`Build.bat WacomEditor Win64 Development` PASS
- 自动化测试：`Automation RunTests Wacom` PASS（79/79 成功，62 旧 + 17 新）
- 资产重新生成：`-run=WacomRegenerateContent` PASS（虫妹现含小布袋 6 张 StarterDeck）

## 文件改动

新增：
- `Source/WacomTests/Private/Run/BackpackSpec.cpp`

修改：
- `Source/WacomData/Public/Cards/CardPhysique.h`：BagCapacity → Capacity
- `Source/WacomCore/Public/Tags/WacomGameplayTags.h`：加 BagProvider
- `Source/WacomCore/Private/Tags/WacomGameplayTags.cpp`：定义 BagProvider
- `Source/WacomBattle/Public/Session/BattleSession.h`：FBattleInitParams 加 BattleDeckOverride
- `Source/WacomBattle/Private/Session/BattleSession.cpp`：Initialize 用 Override 优先
- `Source/WacomRun/Public/RunState.h`：删 BackpackCapacity，加 Gold；TheftCount 字段位置移动
- `Source/WacomRun/Public/RunSession.h`：加 16 个新 API 声明
- `Source/WacomRun/Private/RunSession.cpp`：a2 规则 Initialize、新公式 RecomputeBurden、所有新 API 实现
- `Source/WacomEditor/Private/ContentBuilders/BugGirlBuilder.cpp`：BuildBugGirlBag + StarterDeck 加入
- `Source/WacomTests/Private/Run/RunStateSpec.cpp`：兼容 a2 规则
- `Source/WacomTests/Private/Run/PressureSpec.cpp`：兼容动态容量
- `Content/Wacom/Cards/BugGirl/DA_Card_BugGirlBag.uasset`：新资产
- `Content/Wacom/Characters/DA_Character_BugGirl.uasset`：StarterDeck 含小布袋
- `Docs/Game_Design.md`：§11.4 互斥约定
- `Docs/WacomData.md`：FCardPhysique 字段表 + Card.Keyword 表
- `Docs/WacomRun.md`：FRunState 字段 / API 表全面更新

## 不做什么（留给 Stage 4.2 / 4.3）

- 背包 UI（Stage 4.2）
- B 类容器卡 + 容量效果 + 特殊存放区（Stage 4.3，等具体卡设计）
- 删牌区"由卡赋予"的关联（GDD §11.7：第一阶段始终显示，不绑定卡）
- 节点事件 / 商店出售触发 DestroyCardFromBackpack（Stage 9）
