# Implementation Plan: Stage 4.5 背包 B 类容器卡特殊存放区

> Convert the feature design into a series of prompts for a code-generation LLM that will implement each step with incremental progress. Make sure that each prompt builds on the previous prompts, and ends with wiring things together. There should be no hanging or orphaned code that isn't integrated into a previous step. Focus ONLY on tasks that involve writing, modifying, or testing code.

## Overview

实现路径按 design §Architecture 的五个切片顺序展开（4.5.0 → 4.5.1 → 4.5.2 → 4.5.3a → 4.5.3b），每个切片内部按"数据层结构 → 操作 API → 属性测试 → 例子/边界测试 → 切片末尾编译+全测全绿"组织。

模块边界（design §Architecture 已锁定）：

- `WacomRun`：`FCardInstance` / `FSpecialZone` / `EZoneKind` / `RunState` 字段升级 / `RunSession` 新 API / `WacomSaveGame v2`
- `WacomBattle`：`FBattleDeckEntry` / `FRuntimeCardInstance.CapacityEffectTags` / `FCardEffectDispatcher` Damage 修正
- `WacomCore`：`Card.CapacityEffect.WeaponDamagePlus3` / `Card.Keyword.Weapon` 注册（如缺）
- `WacomEditor`：蛛茧绒囊 `BugGirlBuilder.cpp` CapacityEffect tag 单点改动
- `WacomApp`：`UWacomCardDragOperation` / `UWacomZoneDropTarget` / `UWacomDeleteZoneDropTarget` / `UWacomBackpackScreen` 重构 / `UWacomDeckCardWidget` 拖拽源化
- `WacomTests`：`BackpackSpec` / `BattleSpec` / `SaveGameRoundtripSpec` / `BackpackScreenSpec` 新增覆盖

属性测试通过项目内 `FWacomBattleFixture` 工厂 + 自实现的小型 PBT runner（≥100 次迭代，`FRandomStream` 注入种子），按 design §Testing Strategy 部署。

切片末尾"完成即验证"使用项目约定命令：

```
编译: "e:\UE_5.7\Engine\Build\BatchFiles\Build.bat" WacomEditor Win64 Development -Project="d:\UE_Project\5.7\Wacom\Wacom.uproject" -WaitMutex -FromMsBuild
测试: "e:\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "d:\UE_Project\5.7\Wacom\Wacom.uproject" -ExecCmds="Automation RunTests Wacom; Quit" -Unattended -NoPause -NoSplash -NullRHI
```

## Tasks

### Slice 4.5.0 — Instance ID 重构 + SaveGame v1→v2 升档骨架

- [x] 1. 定义 `FCardInstance` 与 `EZoneKind` 类型（WacomRun/Public/RunStateTypes.h）
  - [x] 1.1 在 `RunStateTypes.h` 中新增 `USTRUCT(BlueprintType) FCardInstance`（字段 `FGuid InstanceId` / `TObjectPtr<UCardDefinition> Definition` / `bool bBattleEnabledInSpecialZone = false`），三字段均 UPROPERTY，默认值与 R1.1 一致
    - 头文件遵循"前向声明 + cpp 内 include"原则，`UCardDefinition` 用前向声明
    - _Requirements: 1.1_

  - [x] 1.2 在 `RunStateTypes.h` 中新增 `UENUM(BlueprintType) EZoneKind { Backpack, BattleDeck, SpecialZone, BurdenZone }`，DisplayName 与 design §2 一致
    - _Requirements: 1.6, 1.7_

- [x] 2. 升级 `FRunState.Backpack` / `FRunState.BattleDeck` 字段类型
  - [x] 2.1 把 `Backpack` / `BattleDeck` 从 `TArray<TObjectPtr<UCardDefinition>>` 改为 `TArray<FCardInstance>`，更新所有调用点编译
    - 编辑 `WacomRun/Public/RunState.h` + `WacomRun/Private/RunSession.cpp`（含 `Initialize` / `ResetRunState` / 各 Add/Remove/Destroy/Move 入口的内部代码）
    - 仅做字段级类型升级 + 旧调用点最小适配（不暴露 instance 概念给外部按 Definition 的 API）
    - 现有按 Definition 的 public API 签名保持不变
    - _Requirements: 1.2, 1.13_

  - [x] 2.2 重写 `URunSession::Initialize` 把 StarterDeck 灌入 Backpack/BattleDeck 时为每张非空 Definition `FGuid::NewGuid()` 生成 InstanceId
    - StarterDeck 中 nullptr 跳过；空 StarterDeck / nullptr Character 走 fallback（清空两数组 + UE_LOG Warning）
    - _Requirements: 1.3, 1.4_

  - [x] 2.3 重写 `URunSession::AddCardToBackpack(Card)` 在 `Card != nullptr` 时分配新 InstanceId 包成 `FCardInstance` 追加；nullptr 拒绝并 Warning
    - Initialize 与 AddCardToBackpack 内部对生成的 instance 做 `ensureMsgf(InstanceId.IsValid())` 兜底
    - _Requirements: 1.5, 1.14_

  - [x] 2.4 改写 `IsCardInBackpack(Card)` / `IsCardInBattleDeck(Card)` / `AddCardToBattleDeck(Card)` / `RemoveCardFromBattleDeck(Card)` / `DestroyCardFromBackpack(Card)` / `DeleteCardForGold(Card)` 内部按"下标升序 Definition 匹配第一个 instance"操作，public 签名不变
    - 保持现有 BackpackSpec 105 单测语义不变
    - _Requirements: 1.9, 1.10, 1.13_

- [x] 3. 新增 `MoveInstance` / `FindInstance` API + 4.5.0 测试
  - [x] 3.1 在 `RunSession.h` 新增 `bool FindInstance(FGuid InstanceId, FCardInstance& OutInstance, EZoneKind& OutZone, FGuid& OutZoneOwnerInstanceId) const`
    - 命中写入 out 三参数；未命中保持 out 不变并 return false
    - 内部遍历 Backpack / BattleDeck（4.5.0 阶段先两区；BurdenZone / SpecialZones 在 4.5.1 接入）
    - _Requirements: 1.8_

  - [x] 3.2 在 `RunSession.h` 新增 `UFUNCTION(BlueprintCallable) bool MoveInstance(FGuid InstanceId, EZoneKind ToZone, FGuid ToZoneOwnerInstanceId)`
    - 4.5.0 阶段仅支持 Backpack ↔ BattleDeck（其他 zone 暂返回 false 占位）
    - 校验失败 / InstanceId 不存在 / ToZone 容量满 → return false 且 RunState 不变（含不广播 OnRunStateChangedNative）
    - 成功路径尾部统一 `NotifyRunStateChanged()` 一次
    - _Requirements: 1.6, 1.7_

  - [x] 3.3 Property 1 — InstanceId 全局唯一且非零
    - **Property 1: InstanceId 全局唯一且非零**
    - **Validates: Requirements 1.3, 1.5, 7.2**
    - 文件：`Source/WacomTests/Private/Run/BackpackSpec.cpp`
    - 在测试函数注释固定头部：`// Feature: backpack-special-zone-stage-4-5, Property 1: ...`
    - 至少 100 次迭代，`FRandomStream` 注入种子
    - _Requirements: 1.3, 1.5_

  - [x] 3.4 Property 2 — MoveInstance 原子成功 / 完全失败
    - **Property 2: MoveInstance 原子成功 / 完全失败**
    - **Validates: Requirements 1.6, 1.7, 2.7, 5.5**
    - 4.5.0 阶段先覆盖 Backpack ↔ BattleDeck 路径与失败路径（InstanceId 不存在 / 目标容量满）；SpecialZone / 自指拒绝在 4.5.1 扩展
    - _Requirements: 1.6, 1.7, 5.5_

  - [x] 3.5 Property 3 — FindInstance 一致性
    - **Property 3: FindInstance 一致性**
    - **Validates: Requirements 1.8**
    - _Requirements: 1.8_

  - [x] 3.6 Property 4 — 按 Definition 操作选第一个匹配 instance
    - **Property 4: 按 Definition 操作选第一个匹配 instance**
    - **Validates: Requirements 1.9, 1.10**
    - _Requirements: 1.9, 1.10_

  - [x] 3.7 EXAMPLE / EDGE_CASE — 4.5.0 结构与 fallback
    - 默认构造结构断言（R1.1 / R1.2 字段值为预期默认值）
    - nullptr Character / 空 StarterDeck fallback（R1.4 → Backpack 与 BattleDeck Num()==0 + Warning）
    - zero GUID `ensureMsgf`（R1.14，仅 Editor / Debug build 触发）
    - BackpackSpec 既有 105 条全过（回归校验）
    - _Requirements: 1.1, 1.2, 1.4, 1.13, 1.14_

- [x] 4. SaveGame v2 升档骨架（R7.1 / R7.2 / R7.3）
  - [x] 4.1 把 `UWacomSaveGame::CurrentSaveVersion` 编译期常量从 1 升到 2
    - 添加 `static_assert` 防止后续误改
    - _Requirements: 7.1_

  - [x] 4.2 在 `WacomSaveGame.h` 新增 `USTRUCT FCardInstanceSaveEntry { FGuid InstanceId; FSoftObjectPath DefinitionAssetPath; bool bBattleEnabledInSpecialZone; }`（带 `UPROPERTY(SaveGame)`）
    - 同步新增 `USTRUCT FSpecialZoneSaveEntry { FGuid OwnerInstanceId; TArray<FCardInstanceSaveEntry> Cards; }`
    - 两个 USTRUCT 不带 BlueprintType（design 反射门槛对照表）
    - _Requirements: 7.2_

  - [x] 4.3 在 `UWacomSaveGame` 新增字段：`TArray<FCardInstanceSaveEntry> Backpack/BattleDeck/BurdenZone` + `TArray<FSpecialZoneSaveEntry> SpecialZones`，全部 `UPROPERTY(SaveGame)`
    - _Requirements: 7.2_

  - [x] 4.4 扩展 `MigrateIfNeeded` 的 switch 链：v0→v1 保持原行为；v1→v2 把四个新数组初始化为空 + SaveVersion=2 + fallthrough；v2→v2 return true；>2 return false
    - 不修改任何 v0/v1 已存在字段原值
    - _Requirements: 7.3, 7.7_

  - [x] 4.5 重写 `BuildSaveGameFromRunState` 把 Backpack / BattleDeck 写入 v2 字段（4.5.0 阶段 BurdenZone / SpecialZones 先输出空）
    - 校验：每条 entry 的 InstanceId 非 zero GUID；写入前用临时 set 检查全表合并后无重复；违反则 UE_LOG Error 并跳过该条
    - _Requirements: 7.2_

  - [x] 4.6 重写 `ApplySaveGameToRunState` 三分支决策树（R7.4 / R7.5 / R7.6）
    - 步骤 1：先在临时 `FRunState` 上做所有还原 + 校验，只有全部通过才赋给 `this->RunState`
    - 步骤 2：SaveVersion=2 且四数组全为空 → 按 StarterDeck 重建 instances（每张新 InstanceId）
    - 步骤 3：SaveVersion=2 且任一数组非空 → 按 SaveEntry 还原 InstanceId / Definition / flag（4.5.0 阶段 SpecialZones 输出空，因此走简单两区还原即可）
    - 步骤 4：损坏档（DefinitionAssetPath 失效 / OwnerInstanceId dangling / InstanceId 重复）→ 拒绝加载，RunState 不变，UE_LOG Error
    - _Requirements: 7.4, 7.5, 7.6_

  - [x] 4.7 EXAMPLE / EDGE_CASE — SaveGame 升档与拒绝
    - 文件：`Source/WacomTests/Private/Run/SaveGameRoundtripSpec.cpp`（追加）
    - SMOKE：`static_assert(UWacomSaveGame::CurrentSaveVersion == 2)`（R7.1）
    - EXAMPLE：v0 → v2 与 v1 → v2 迁移后新字段全部为空容器 + SaveVersion=2（R7.3 / R7.8a）
    - EDGE_CASE：SaveVersion = 3 → MigrateIfNeeded false（R7.7 / R7.8d）
    - EDGE_CASE：v2 + 四数组全空 + 当前 Character.StarterDeck → 按 StarterDeck 重建路径（R7.4）
    - _Requirements: 7.1, 7.3, 7.4, 7.7_

- [x] 5. Slice 4.5.0 检查点
  - 编译：`Build.bat WacomEditor Win64 Development`
  - 测试：`Automation RunTests Wacom`
  - Ensure all tests pass, ask the user if questions arise.

### Slice 4.5.1 — SpecialZone + BurdenZone 数据层 + RecomputeBurden 重写

- [x] 6. 定义 `FSpecialZone` + `FRunState` 字段升级
  - [x] 6.1 在 `RunStateTypes.h` 新增 `USTRUCT(BlueprintType) FSpecialZone { FGuid OwnerInstanceId; TArray<FCardInstance> Cards; }`，两字段 UPROPERTY 蓝图可读
    - _Requirements: 2.1_

  - [x] 6.2 在 `RunState.h` 新增字段 `TArray<FCardInstance> BurdenZone` 与 `TArray<FSpecialZone> SpecialZones`，构造态 Num()==0
    - _Requirements: 2.2, 2.11_

- [x] 7. SpecialZone 自动建立 / 销毁回退
  - [x] 7.1 在 `Initialize` / `AddCardToBackpack` 路径里：B 主卡 instance 进入 Backpack/BattleDeck 时自动追加 `FSpecialZone{OwnerInstanceId=该 InstanceId, Cards={}}` 到 `RunState.SpecialZones`（幂等）
    - 同步在 `MoveInstance` 把 B 主卡 instance 跨入 Backpack/BattleDeck 的路径上确保 entry 存在（R5.1 跟随依赖）
    - _Requirements: 2.3_

  - [x] 7.2 重写 `DestroyCardFromBackpack` 内 B 主卡分支：通过 §11.8 校验后，按 `FSpecialZone.Cards` 数组下标升序逐张退回；Backpack 未满 → 追加 Backpack；Backpack 满 → 追加 BurdenZone；处理完后从 `SpecialZones` 移除该 entry
    - 退回前每张 instance `bBattleEnabledInSpecialZone` 重置为 false（R8.6 / Property 10）
    - _Requirements: 2.4, 5.4_

  - [x] 7.3 在 `RunSession.h` 新增 `int32 GetSpecialZoneCapacityFor(FGuid OwnerInstanceId) const`（公式 = `FMath::Max(0, OwnerDef->Physique.Capacity - 1)`，未命中返回 0）
    - 与现有 `static GetSpecialZoneCapacity(BCard)` 数值一致
    - _Requirements: 2.5_

  - [x] 7.4 在 `RunSession.h` 新增 `bool GetSpecialZone(FGuid OwnerInstanceId, FSpecialZone& Out) const`
    - 命中写入 + return true；未命中不修改 + return false
    - _Requirements: 2.6_

- [x] 8. `MoveInstance` 扩展到 SpecialZone / BurdenZone + 校验表
  - [x] 8.1 在 `MoveInstance` 中加入 SpecialZone 分支 + 校验表（R2.7 a-d）：a) ToOwner 在 SpecialZones 中存在；b) InstanceId != ToOwner；c) `Cards.Num() < GetSpecialZoneCapacityFor(ToOwner)`；d) InstanceId 在所有 zone 中存在
    - 校验失败 → return false + 不修改 + 不广播
    - _Requirements: 2.7, 2.8_

  - [x] 8.2 在 `MoveInstance` 中加入"从 SpecialZone 移出时把 instance.bBattleEnabledInSpecialZone 重置为 false"逻辑（R8.6 / Property 10）
    - 扩展 `FindInstance` 内部遍历也包含 `BurdenZone` 与 `⋃SpecialZones.Cards`
    - 进入 SpecialZone 时若是新进入（之前不在该 SpecialZone）默认 flag = false（R2.9）
    - _Requirements: 2.8, 2.9, 8.6_

  - [x] 8.3 在 `RunSession.h` 新增 `UFUNCTION(BlueprintCallable) bool SetSpecialZoneCardBattleEnabled(FGuid InstanceId, bool bEnabled)`
    - InstanceId 在某 SpecialZone → 设 flag + 广播 + return true；否则不修改 + 不广播 + return false
    - _Requirements: 2.10, 8.1, 8.5_

- [x] 9. `RecomputeBurden` 重写（含 BurdenZone 溢出 + 回填 + 压力公式）
  - [x] 9.1 实现 ① 超容溢出（R2.12）：`while Backpack.Num() > GetFluxCapacity()` 弹末尾追加 BurdenZone；同样处理 `BattleDeck > GetBattleDeckCapacity()`
    - _Requirements: 2.12_

  - [x] 9.2 实现 ② 回填（R2.14）：BurdenZone 头部 instance 按"通量 → 备战 → SpecialZones（数组下标升序，第一个有空位的）"优先序回填，回填到 SpecialZone 时强制 `bBattleEnabledInSpecialZone = false`
    - 第二次调用幂等（同一稳态下不再迁移任何 instance）
    - _Requirements: 2.13, 2.14_

  - [x] 9.3 实现 ③ 压力写入（R9.1）：`SetPressure(EWacomPressureType::Burden, FMath::Clamp(n*(n+1)/2, 0, 100))`，n=BurdenZone.Num()
    - SetPressure 内部 dedupe 同值不广播；本入口尾部统一 `NotifyRunStateChanged()` 一次
    - _Requirements: 9.1_

  - [x] 9.4 改 `AddCardToBackpack` 路径：内部调 `RecomputeBurden` 后不重复广播（私有路径不发，public 入口尾部统一发一次，R2.16）
    - 同步审计所有 zone-modifying public 入口的广播规则与 design §4 广播规则表一致
    - _Requirements: 2.16_

- [x] 10. `BattleDeckCapacity` 公式回归 GDD §11.4 + `CollectTypeBContainers` 重命名签名
  - [x] 10.1 改 `URunSession::GetBattleDeckCapacity` 回归 `Σ(玩家拥有的所有 A 类容器卡 Capacity)`，与 `GetFluxCapacity` 严格相等
    - 玩家无任何容器卡或全是 B 类时返回 0
    - _Requirements: 3.1, 3.2, 3.3, 3.4_

  - [x] 10.2 改 `CollectTypeBContainers(TArray<UCardDefinition*>&)` 签名为 `CollectTypeBContainers(TArray<FGuid>& OutOwnerInstanceIds) const`
    - 输出按 `RunState.SpecialZones` 数组下标升序、去重、不含悬空 InstanceId
    - 玩家无 B 主卡 → 输出空数组
    - 更新所有调用点（4.5.1 阶段唯一调用点：UI 渲染 SpecialZone 区块在 4.5.3b 才接入；编辑搜索调用 → 临时调用方按需适配）
    - _Requirements: 3.5, 3.6_

- [x] 11. SaveGame v2 完整序列化（接入 BurdenZone + SpecialZones）
  - [x] 11.1 把 `BuildSaveGameFromRunState` 中 BurdenZone / SpecialZones 输出从 4.5.0 的"空数组占位"补全为完整字段写入
    - 全表合并后 InstanceId 唯一性继续校验
    - _Requirements: 7.2, 7.5_

  - [x] 11.2 把 `ApplySaveGameToRunState` 还原四区 + SpecialZones 归属关系
    - 校验损坏档：DefinitionAssetPath 失效 / OwnerInstanceId 在还原 Backpack ∪ BattleDeck 中找不到 / InstanceId 重复 → 任一命中 → 拒绝加载，RunState 保留调用前状态
    - _Requirements: 7.5, 7.6_

- [x] 12. 4.5.1 切片测试（属性测试 + 例子测试）
  - [x] 12.1 Property 5 — B 主卡 ↔ SpecialZone 双射不变量
    - **Property 5: B 主卡 ↔ SpecialZone 双射不变量**
    - **Validates: Requirements 2.2, 2.3, 3.5, 3.6, 5.6**
    - 在每次随机 op 之后断言双射两端 + `CollectTypeBContainers` 输出顺序与去重
    - _Requirements: 2.2, 2.3, 3.5, 3.6, 5.6_

  - [x] 12.2 Property 6 — B 主卡销毁内含卡退回流
    - **Property 6: B 主卡销毁内含卡退回流**
    - **Validates: Requirements 2.4**
    - _Requirements: 2.4_

  - [x] 12.3 Property 7 — GetSpecialZoneCapacityFor 公式
    - **Property 7: GetSpecialZoneCapacityFor 公式**
    - **Validates: Requirements 2.5**
    - _Requirements: 2.5_

  - [x] 12.4 Property 8 — RecomputeBurden 输出契约
    - **Property 8: RecomputeBurden 输出契约**
    - **Validates: Requirements 2.12, 2.13, 2.14, 9.1**
    - 含溢出 / 回填 / 优先序 / 压力公式 / 幂等五条 sub-clause
    - _Requirements: 2.12, 2.13, 2.14, 9.1_

  - [x] 12.5 Property 9 — 广播计数与 Burden 通道写入唯一性
    - **Property 9: 广播计数与 Burden 通道写入唯一性**
    - **Validates: Requirements 2.8, 2.16, 9.2**
    - 用一个监听 `OnRunStateChangedNative` 的计数器 + `Pressure.Burden` 前后差校验
    - _Requirements: 2.8, 2.16, 9.2_

  - [x] 12.6 Property 10 — 进入 / 离开 SpecialZone 重置 bBattleEnabledInSpecialZone
    - **Property 10: 进入 / 离开 SpecialZone 重置 bBattleEnabledInSpecialZone**
    - **Validates: Requirements 2.9, 8.6**
    - _Requirements: 2.9, 8.6_

  - [x] 12.7 Property 11 — SetSpecialZoneCardBattleEnabled 切 flag 不移卡
    - **Property 11: SetSpecialZoneCardBattleEnabled 切 flag 不移卡**
    - **Validates: Requirements 2.10, 8.1, 8.5**
    - _Requirements: 2.10, 8.1, 8.5_

  - [x] 12.8 Property 12 — BattleDeckCapacity == FluxCapacity == Σ A 类容器 Capacity
    - **Property 12: BattleDeckCapacity == FluxCapacity == Σ A 类容器 Capacity**
    - **Validates: Requirements 3.1, 3.2, 3.3, 3.4**
    - _Requirements: 3.1, 3.2, 3.3, 3.4_

  - [x] 12.9 EXAMPLE / EDGE_CASE — 4.5.1 具体场景与 SaveGame round-trip
    - 文件：`BackpackSpec.cpp` + `SaveGameRoundtripSpec.cpp`（追加）
    - R2.1 / R2.11 默认构造结构断言
    - R2.6 `GetSpecialZone` 命中 / 未命中
    - R2.15 a~h 八条具体场景（B 主卡加入自动建空 SZ / 合法放入 / 自指拒绝 / 容量满拒绝 / 销毁退回 Backpack / 销毁溢出 BurdenZone / 回填优先序 / 容量回退溢出）
    - R9.3 n=0 / 1 / 3 / 14 → Burden = 0 / 1 / 6 / 100
    - R3.7 BackpackSpec 既有 + 新增全过（回归）
    - SaveGame v2 完整 round-trip example（含 BurdenZone + SpecialZones）
    - SaveGame 三类损坏档拒绝（R7.6 / R7.8c）
    - _Requirements: 2.1, 2.6, 2.11, 2.15, 3.7, 7.5, 7.6, 9.3_

- [x] 13. Slice 4.5.1 检查点
  - 编译：`Build.bat WacomEditor Win64 Development`
  - 测试：`Automation RunTests Wacom`
  - Ensure all tests pass, ask the user if questions arise.

### Slice 4.5.2 — BattleSession 集成 + 蛛茧绒囊武器+3

- [x] 14. GameplayTag 注册 + 蛛茧绒囊 builder 单点改动
  - [x] 14.1 在 `WacomCore/Public/Tags/WacomGameplayTags.h` + `.cpp` 用 `UE_DECLARE_GAMEPLAY_TAG_EXTERN` + `UE_DEFINE_GAMEPLAY_TAG` 注册 `Card.CapacityEffect.WeaponDamagePlus3` 与 `Card.Keyword.Weapon`（如缺）
    - 同步更新 `Docs/WacomData.md §5` 的 tag 表
    - _Requirements: 4.1_

  - [x] 14.2 在 `Source/WacomEditor/.../BugGirlBuilder.cpp` 把蛛茧绒囊 DataAsset 的 `Physique.CapacityEffect` 从 `Card.CapacityEffect.Placeholder` 改为 `Card.CapacityEffect.WeaponDamagePlus3`（单点改动）
    - _Requirements: 4.2_

- [x] 15. `FBattleDeckEntry` + `FBattleInitParams` + `FRuntimeCardInstance` 扩展（WacomBattle）
  - [x] 15.1 在 `WacomBattle/Public/Session/BattleSession.h` 新增 `USTRUCT(BlueprintType) FBattleDeckEntry { TObjectPtr<const UCardDefinition> Definition; FGameplayTagContainer CapacityEffectTags; }`
    - _Requirements: 4.3_

  - [x] 15.2 在 `FBattleInitParams` 新增字段 `TArray<FBattleDeckEntry> BattleDeckEntries`（保留旧 `BattleDeckOverride` 作为 fixture 向后兼容）
    - _Requirements: 4.3_

  - [x] 15.3 在 `WacomBattle/Public/Runtime/RuntimeCardInstance.h` 新增字段 `FGameplayTagContainer CapacityEffectTags`（仅 UPROPERTY，不暴露 Blueprint）
    - _Requirements: 4.4_

  - [x] 15.4 改 `UBattleSession::Initialize` 选择规则：`BattleDeckEntries.Num() > 0` → 用 entries 创建 RuntimeCardInstance（拷贝 CapacityEffectTags）；否则若 `BattleDeckOverride.Num() > 0` → 旧路径（CapacityEffectTags 为空）；否则 → 用 `Character->StarterDeck`
    - _Requirements: 4.3_

- [x] 16. `URunSession::BuildInitParamsForBattle` 重写为输出 `BattleDeckEntries`
  - [x] 16.1 收集 BattleDeck 原生 instances → 每张转成 `FBattleDeckEntry{Definition, /*Tags=*/{}}`
    - _Requirements: 4.3, 5.2, 8.2_

  - [x] 16.2 遍历 `RunState.SpecialZones`：仅当 `OwnerInstanceId` 当前位于 BattleDeck（用 `FindInstance`）→ 把 SpecialZone.Cards 中 `bBattleEnabledInSpecialZone == true` 的 instance 转成 `FBattleDeckEntry{Definition, {OwnerDef.Physique.CapacityEffect}}`
    - 主卡仍在 Backpack → 不输出（R5.3 / R8.3 默认提案）
    - 主卡在 BurdenZone → 不输出（仅 BattleDeck 才参战）
    - _Requirements: 4.3, 4.7, 5.2, 5.3, 8.2, 8.3, 8.4_

  - [x] 16.3 不再写 `BattleDeckOverride`（保留兼容 fixture，但 RunSession 路径只走 entries）
    - _Requirements: 4.3_

- [x] 17. `FCardEffectDispatcher::Execute` Damage 路径 +3 修正（4.5.2 关键改动）
  - [x] 17.1 在 `WacomBattle/Private/Effects/CardEffectDispatcher.cpp` Damage 分支末尾加：若 `Self->Definition->Keywords.HasTag(Card.Keyword.Weapon)` 且 `Self->CapacityEffectTags.HasTag(Card.CapacityEffect.WeaponDamagePlus3)` → `FinalMag += 3`，最后 `FMath::Max(0, FinalMag)` clamp
    - 单 instance 至多被一个 SpecialZone 持有（R5.6），叠加 N=1
    - 注释引用 design §9 的归属理由（cross-cutting 修正放 Dispatcher 而非 Resolver）
    - _Requirements: 4.4, 4.5, 4.6_

- [x] 18. 4.5.2 切片测试
  - [x] 18.1 Property 13 — BuildInitParamsForBattle 入战清单组成
    - **Property 13: BuildInitParamsForBattle 入战清单组成**
    - **Validates: Requirements 4.3, 4.7, 5.2, 5.3, 8.2, 8.3, 8.4**
    - 文件：`Source/WacomTests/Private/Run/BackpackSpec.cpp`（追加）
    - _Requirements: 4.3, 4.7, 5.2, 5.3, 8.2, 8.3, 8.4_

  - [x] 18.2 Property 14 — 武器 + WeaponDamagePlus3 伤害修正
    - **Property 14: 武器 + WeaponDamagePlus3 伤害修正**
    - **Validates: Requirements 4.4, 4.5, 4.6**
    - 文件：`Source/WacomTests/Private/Battle/`（新增或追加合适 spec）
    - 注入 mock 卡（带 / 不带 `Card.Keyword.Weapon` 关键词）+ 不同 base damage + 不同 modifier 组合
    - _Requirements: 4.4, 4.5, 4.6_

  - [x] 18.3 Property 15 — B 主卡跨 zone 移动 SpecialZone 内容保持
    - **Property 15: B 主卡跨 Backpack ↔ BattleDeck 移动 SpecialZone 内容保持**
    - **Validates: Requirements 5.1, 5.4**
    - _Requirements: 5.1, 5.4_

  - [x] 18.4 SMOKE / EXAMPLE — Tag 注册 + 蛛茧绒囊 Builder + 四条具体场景
    - SMOKE：`WacomTags::Card_CapacityEffect_WeaponDamagePlus3.IsValid() == true` 与 `Card_Keyword_Weapon.IsValid() == true`（R4.1）
    - EXAMPLE：蛛茧绒囊 builder 输出 CapacityEffect == WeaponDamagePlus3（R4.2）
    - EXAMPLE R4.8 a~d 四条具体场景：a) flag=false 武器卡不入战；b) flag=true 武器卡 = base+3；c) flag=true 非武器卡 = base；d) 主卡仍在背包时其 SpecialZone 内含卡不入战
    - 回归：BattleSpec / BackpackSpec 既有用例全过（R4.9）
    - _Requirements: 4.1, 4.2, 4.8, 4.9_

- [x] 19. Slice 4.5.2 检查点
  - 编译：`Build.bat WacomEditor Win64 Development`
  - 测试：`Automation RunTests Wacom`
  - Ensure all tests pass, ask the user if questions arise.

### Slice 4.5.3a — 拖拽框架（备战 ↔ 通量）

- [x] 20. 拖拽 Operation + DropTarget widget 类
  - [x] 20.1 在 `WacomApp/Public/UI/Backpack/WacomCardDragOperation.h` + `.cpp` 定义 `UWacomCardDragOperation : UDragDropOperation`
    - 字段：`FGuid InstanceId / EZoneKind FromZone / FGuid FromZoneOwnerInstanceId / TObjectPtr<UCardDefinition> Definition`，全部 `UPROPERTY(BlueprintReadOnly)`
    - 头文件类注释含五项 widget 生命周期声明（数据源 / 更新触发 / 订阅时机 / 反订阅时机 / 焦点输入）— 这里 `UDragDropOperation` 不是 widget，用"N/A"标注
    - _Requirements: 6.1_

  - [x] 20.2 在 `WacomApp/Public/UI/Backpack/WacomZoneDropTarget.h` + `WacomApp/Private/UI/Backpack/WacomZoneDropTarget.cpp` 定义 `UWacomZoneDropTarget : UUserWidget`
    - 字段：`EZoneKind ZoneKind / FGuid OwnerInstanceId / TWeakObjectPtr<UWacomBackpackScreen> OwnerScreen`
    - 实现 `SetOwnerScreen` 注入 + `NativeOnDragOver` / `NativeOnDrop`
    - 头文件类注释完整声明五项 widget 生命周期契约
    - `NativeOnDrop`：`Cast<UWacomCardDragOperation>(Op)` 失败 → return false；成功 → 调 `OwnerScreen->GetRunSession()->MoveInstance(...)`，按返回值返回
    - _Requirements: 6.3, 6.5_

- [x] 21. `UWacomDeckCardWidget` 拖拽源化
  - [x] 21.1 改 `SetCard` 签名接受 `(const FCardInstance& Inst, EZoneKind FromZone, FGuid FromZoneOwnerInstanceId)` 三参数
    - 删除原"主按钮 Move 委托绑定" + 主按钮变为纯展示（R6.6）
    - _Requirements: 6.2, 6.6_

  - [x] 21.2 覆写 `NativeOnMouseButtonDown`：仅响应鼠标左键 → 调 `DetectDragIfPressed`
    - _Requirements: 6.2_

  - [x] 21.3 覆写 `NativeOnDragDetected`：构造 `UWacomCardDragOperation` 实例，填 `InstanceId / FromZone / FromZoneOwner / Definition` 并返回
    - 当 `FromZone != SpecialZone` 时 `FromZoneOwnerInstanceId = FGuid()`（R6.1 约束）
    - _Requirements: 6.1, 6.2_

- [x] 22. `UWacomBackpackScreen` 4.5.3a 重构
  - [x] 22.1 把 BattleDeckZone 与 BackpackZone WrapBox 包进各自 `UWacomZoneDropTarget` 实例（C++ 父类创建，调 `SetOwnerScreen(this)`）
    - 4.5.3a 只接入这两个 zone（DeleteZone / SpecialZone / BurdenZone 在 4.5.3b 接入）
    - _Requirements: 6.3, 6.4_

  - [x] 22.2 把 `RebuildAll()` 拆出独立函数；订阅 `OnRunViewModelRefreshedNative` → `HandleViewModelRefreshed` → `RebuildAll`
    - RebuildAll：清空两个 WrapBox + 按 `RunState.BattleDeck` / `RunState.Backpack` 顺序重建 widget；不做 widget 增量 patch（R6.4）
    - 失败路径（DropTarget MoveInstance 返回 false）→ 不广播 → RebuildAll 不被调 → UI 自然保持原状（R6.5）
    - _Requirements: 6.4, 6.5_

  - [x] 22.3 EXAMPLE — 4.5.3a 拖拽框架单测
    - 文件：`Source/WacomTests/Private/UI/BackpackScreenSpec.cpp`（新增）
    - R6.1：DragOperation 四种 FromZone 字段约束（非 SpecialZone 时 OwnerInstanceId 必须为 invalid GUID）
    - R6.2：DeckCardWidget `NativeOnDragDetected` 输出非空 `UWacomCardDragOperation` 且四字段已填
    - R6.3：DropTarget `Cast` 失败时不调 RunSession（用 mock RunSession 计数）
    - R6.4：mock RunSession `MoveInstance` 返回 true → BackpackScreen 收到 OnRunViewModelRefreshedNative → RebuildAll 调用计数 == 1
    - R6.5：mock 返回 false → 不广播 → RebuildAll 调用计数 == 0
    - R6.6：DeckCardWidget 主按钮的 Move 委托绑定已删除（widget tree 中主按钮 OnClicked 监听器为空）
    - _Requirements: 6.1, 6.2, 6.3, 6.4, 6.5, 6.6_

- [x] 23. Slice 4.5.3a 检查点
  - 编译：`Build.bat WacomEditor Win64 Development`
  - 测试：`Automation RunTests Wacom`
  - Ensure all tests pass, ask the user if questions arise.

### Slice 4.5.3b — SpecialZone / BurdenZone 渲染 + 删牌拖拽

- [x] 24. DeleteZone + SpecialZone 区块渲染 + BurdenZone 区块
  - [x] 24.1 新增 `UWacomDeleteZoneDropTarget : UWacomZoneDropTarget`（独立子类路径，design §10 Note）
    - `NativeOnDrop` 改调 `RunSession->DeleteCardForGold(Op->Definition)`，按返回值返回
    - 头文件类注释含完整五项 widget 生命周期声明
    - _Requirements: 6.9_

  - [x] 24.2 在 `UWacomBackpackScreen` `RebuildAll()` 末尾按 `RunState.SpecialZones` 顺序动态创建 SpecialZone 区块
    - 每区块：标题 `特殊存放区 [Definition.DisplayName]  n/(Capacity-1)` + WrapBox + 包外的 `UWacomZoneDropTarget(SpecialZone, OwnerInstanceId)`
    - WrapBox 按 `SZ.Cards` 顺序逐张创建 `UWacomDeckCardWidget`，FromZone=SpecialZone，FromZoneOwnerInstanceId=该 OwnerInstanceId
    - `bBattleEnabledInSpecialZone == true` 的卡 widget 显示 widget tree 可识别的"已选"角标元素（命名 `BattleEnabledBadge` 或独立类型 `UWacomBattleEnabledBadge`）
    - _Requirements: 6.7_

  - [x] 24.3 新增"已入战"标记 widget 元素：当 SpecialZone 主卡 instance 当前位于 BattleDeck 时区块标题显示该元素，主卡回 Backpack 时元素 visibility=collapsed
    - widget tree 中元素可被名称 / 类型查询识别（design §10 SpecialZonesPanel 布局）
    - _Requirements: 6.13_

  - [x] 24.4 在 `RebuildAll()` 末尾按 `RunState.BurdenZone` 顺序构建 BurdenZone 区块（标题"负重区 n" + WrapBox）
    - WrapBox 子项与 `RunState.BurdenZone` 数组按顺序逐张对应（Definition / InstanceId 一致）
    - 包外 `UWacomZoneDropTarget(BurdenZone, FGuid())`（API 允许，UI 层不主动让玩家拖入；按 design §5 校验表"无额外校验"通过）
    - _Requirements: 6.8_

- [x] 25. BattleDeckZone 视觉合并 + 右键单击 + 容量满拒绝
  - [x] 25.1 在 `RebuildAll` 中 BattleDeckZone WrapBox 渲染时同步追加：所有 B 主卡 instance 当前位于 BattleDeck 时，其 SpecialZone 中 `bBattleEnabledInSpecialZone == true` 的卡 widget（带 widget tree 可识别的"来自 [B 主卡名]"角标）
    - 这些卡同时在原 SpecialZone 区块中保留并显示"已选"标记（不消失）
    - _Requirements: 6.10_

  - [x] 25.2 在 `UWacomDeckCardWidget` 覆写 `NativeOnPreviewMouseButtonDown` 或 `NativeOnMouseButtonDown` 处理鼠标右键单击
    - 仅当 `FromZone == EZoneKind::SpecialZone` 时响应；调用 `OwnerScreen->GetRunSession()->SetSpecialZoneCardBattleEnabled(InstanceId, !current_flag)`
    - 切换成功后通过 `OnRunViewModelRefreshedNative` → `RebuildAll` 重建；"已选"角标 visibility 严格等于 RunState 中切换后的 flag 值
    - _Requirements: 6.11_

  - [x] 25.3 在 `UWacomZoneDropTarget::NativeOnDragOver` 内对 BattleDeckZone + 来源 Backpack + 容量满场景返回 false（视觉上拒绝接收）
    - 即便 NativeOnDragOver 返回 true，`NativeOnDrop` 调 `MoveInstance` 也会因容量满返回 false → 仍走 R6.5 路径保留原位
    - _Requirements: 6.12_

- [x] 26. 4.5.3b 切片测试 + 贯穿 Property 16
  - [x] 26.1 EXAMPLE / EDGE_CASE — 4.5.3b 渲染与交互
    - 文件：`Source/WacomTests/Private/UI/BackpackScreenSpec.cpp`（追加）
    - R6.7：SpecialZone 区块 widget tree 含标题 + `n/(Capacity-1)` + `BattleEnabledBadge`（断言 widget 元素存在性）
    - R6.8：BurdenZone 区块 widget tree 子项与 `RunState.BurdenZone` 顺序 / Definition / InstanceId 对应
    - R6.9：删牌 DropTarget mock 返回 false（金币不足 / Intrinsic / 最后 BagProvider）→ widget 子项数量、顺序、引用全保持
    - R6.10：mock RunState 含 BattleDeck B 主卡 + 其 SpecialZone 内 BattleEnabled 卡 → BattleDeckZone WrapBox 同时含两类 widget；后者带"来自 X"角标；原 SpecialZone 区块仍含该卡 + "已选"标记
    - R6.11：mock 右键单击 → `SetSpecialZoneCardBattleEnabled` 调用计数 == 1 + 角标 visibility 与 flag 一致
    - R6.12：mock BattleDeck 满 + 来源 Backpack → DropTarget 拒绝 + widget 位置不变
    - R6.13：B 主卡在 BattleDeck → SpecialZone 区块标题"已入战"标记 visible；主卡回 Backpack → 标记 collapsed
    - _Requirements: 6.7, 6.8, 6.9, 6.10, 6.11, 6.12, 6.13_

  - [x] 26.2 Property 16 — SaveGame v2 round-trip 完整保留 instance 归属
    - **Property 16: SaveGame v2 round-trip 完整保留 instance 归属**
    - **Validates: Requirements 7.2, 7.5**
    - 文件：`Source/WacomTests/Private/Run/SaveGameRoundtripSpec.cpp`（追加）
    - 生成器：`MakeRandomRunState` 含 0..n 张 B 主卡 + 各 SpecialZone 0..Capacity-1 张内含卡 + 0..k 张 BurdenZone 卡
    - 断言：Save → Apply 后四个映射（InstanceId 集合 / InstanceId→Definition / InstanceId→(zone, ownerInstanceId) / InstanceId→bBattleEnabledInSpecialZone）逐项相等；SaveA 内 InstanceId 非零且全表唯一
    - _Requirements: 7.2, 7.5_

- [x] 27. Slice 4.5.3b 最终检查点 + 文档同步
  - 编译：`Build.bat WacomEditor Win64 Development`
  - 测试：`Automation RunTests Wacom`
  - 更新 `Docs/WacomRun.md §3 / §4`：FRunState 字段升级 + SaveGame v2 字段
  - 更新 `Docs/WacomApp.md §7 BackpackScreen`：拖拽交互模型
  - 更新 `Docs/WacomData.md §5`：新 tag 列表
  - 在 `Docs/DevLog/` 新建 `背包SpecialZone-Stage4.5.md` 记录关键决策与踩坑
  - Ensure all tests pass, ask the user if questions arise.

## Notes

- 任务标记 `*`（如 `3.3`、`12.1`、`22.3` 等）为可选测试任务，可跳过加快 MVP 落地；非可选任务为核心实现任务，必须完成。
- 每条 property 任务都注明对应 design §Correctness Properties 的 Property 编号 + Validates 的 Requirement 子句号。
- Property 测试通过项目内自实现的小型 PBT runner，每 property ≥100 次迭代，`FRandomStream` 注入种子；测试函数注释固定头部为 `// Feature: backpack-special-zone-stage-4-5, Property N: <text>`。
- 每个 slice 末尾的"检查点"任务（5 / 13 / 19 / 23 / 27）在编译 + RunTests Wacom 全绿前不要进入下一切片。
- UI 视觉精度（颜色 / 字体 / 排版）、本地化文本、性能（>100 张卡）不在本 stage 测试范围（design §Testing Strategy 末注）。
- 反射使用门槛严格按 design §Architecture 反射对照表执行；任何新 UCLASS / USTRUCT / UENUM / UPROPERTY 都需在该表内有对应理由。
- 所有新建 widget 头文件类注释必须含五项生命周期声明（数据源 / 更新触发 / 订阅时机 / 反订阅时机 / 焦点输入），缺一不可。
- BackpackScreen 数据流仍走 MVVM：写命令 → RunSession → `OnRunStateChangedNative` → `UWacomRunViewModelProvider` → `OnRunViewModelRefreshedNative` → BackpackScreen `RebuildAll`，不直接订阅 RunSession（design §14 时序图）。

## Task Dependency Graph

```json
{
  "waves": [
    { "id": 0, "tasks": ["1.1", "1.2", "4.1", "14.1", "14.2", "15.1", "15.3", "20.1"] },
    { "id": 1, "tasks": ["2.1", "4.2", "15.2"] },
    { "id": 2, "tasks": ["2.2", "2.3", "2.4", "4.3"] },
    { "id": 3, "tasks": ["3.1", "3.2", "4.4", "4.5", "4.6"] },
    { "id": 4, "tasks": ["3.3", "3.4", "3.5", "3.6", "3.7", "4.7"] },
    { "id": 5, "tasks": ["6.1", "6.2"] },
    { "id": 6, "tasks": ["7.1", "7.2", "7.3", "7.4", "10.1", "10.2"] },
    { "id": 7, "tasks": ["8.1", "8.2", "8.3"] },
    { "id": 8, "tasks": ["9.1", "9.2", "9.3", "9.4"] },
    { "id": 9, "tasks": ["11.1", "11.2"] },
    { "id": 10, "tasks": ["12.1", "12.2", "12.3", "12.4", "12.5", "12.6", "12.7", "12.8", "12.9"] },
    { "id": 11, "tasks": ["15.4", "16.1", "16.2", "16.3"] },
    { "id": 12, "tasks": ["17.1"] },
    { "id": 13, "tasks": ["18.1", "18.2", "18.3", "18.4"] },
    { "id": 14, "tasks": ["20.2", "21.1"] },
    { "id": 15, "tasks": ["21.2", "21.3", "22.1"] },
    { "id": 16, "tasks": ["22.2"] },
    { "id": 17, "tasks": ["22.3"] },
    { "id": 18, "tasks": ["24.1", "24.2", "24.3", "24.4"] },
    { "id": 19, "tasks": ["25.1", "25.2", "25.3"] },
    { "id": 20, "tasks": ["26.1", "26.2"] }
  ]
}
```
