---
inclusion: always
---

# Wacom 项目总约定

本文是项目级的持久约定。每次对话默认加载。

## 项目身份

- 单人第一人称 HD-2D 卡牌冒险游戏。
- UE 5.7。
- 主导程序：单人。
- 核心设计文档位于 `Docs/`：
  - `Architecture.md`：模块与依赖
  - `Battle_Rules.md`：战斗流程
  - `Hand_Zone_Rules.md`：手牌区域
  - `Game_Design_Document.md`：GDD
  - `Data_Schema_Draft.md`：数据结构与字段（第一阶段定稿）
  - `Characters/BugGirl.md`：角色虫妹
  - `FirstPerson_HD2D_Card_Variant.md`：方案背景

## 模块结构（7 模块，非插件化）

```
Source/
├── WacomCore/      Runtime, PreDefault     依赖: Core, CoreUObject, GameplayTags (+Engine private)
├── WacomData/      Runtime, PreDefault     依赖: WacomCore
├── WacomBattle/    Runtime, Default        依赖: WacomData
├── WacomRun/       Runtime, Default        依赖: WacomBattle
├── WacomApp/       Runtime, Default        依赖: WacomRun (主游戏模块, PRIMARY_GAME_MODULE)
├── WacomEditor/    Editor, PostEngineInit  依赖: WacomBattle + UnrealEd/AssetTools/...
└── WacomTests/     DeveloperTool, Default  依赖: WacomRun
```

**依赖方向由 `Build.cs` 硬约束，严禁反向依赖。**

```
WacomCore <- WacomData <- WacomBattle <- WacomRun <- WacomApp
WacomEditor / WacomTests 位于依赖链之外，只向运行时模块单向依赖。
```

## 切片实现顺序（Architecture.md §11）

当前处于 **S1 已完成**，下一步是 S2。

1. ✅ S1：BattleState / BattleCommand / BattleSnapshot / BattleEvent
2. ⏳ S2：BattleSession + BattleResolver 骨架
3. S3：起始阶段抽牌 + 等待值
4. S4：HandZoneService
5. S5：PlayCard / Wait / EndTurn 三个命令
6. S6：敌方部位行动子流程
7. S7：卡牌效果执行器（伤害、腾挪、施加状态占位）
8. S8：先机命中、抵抗、完美释放
9. S9：蛇 + 虫妹最小卡组 DataAsset
10. S10：测试场景战斗 Actor
11. S11：自动化测试覆盖 Architecture.md §12

每个切片结束必须编译通过。

## 反射 / UCLASS 使用门槛

**只有以下理由才使用 UCLASS / USTRUCT / UENUM / UPROPERTY：**

- 需要蓝图访问
- 需要 DataAsset / DataTable 资产承载
- 需要序列化（存档、网络）
- 需要 UObject GC 引用管理
- 需要反射遍历

否则用纯 C++ struct / enum class / 原生容器。例如 `FBattleState` 是非反射 struct；`FCardEffect` 要进 DataAsset 所以是 USTRUCT。

## Public / Private 边界

- `Public/`：外部模块可以 include，只放协议、轻量类型、对外入口。
- `Private/`：规则实现、内部服务类，外部编译期不可见。
- 头文件优先前向声明；具体 include 放 `.cpp`。

战斗真相 (`BattleState`、`BattleResolver`、各 Resolver、Executor) 必须留在 `WacomBattle/Private/`。UI 和 Actor 只能通过 `UBattleSession` + `FBattleSnapshot` + `FBattleEvent` 访问战斗。

## 术语表（统一用词）

对齐 `Data_Schema_Draft.md §1`。代码和新文档禁止混用：

- ✅ `EnemyPart` / 敌方部位。❌ `enemy unit` / `Unit` / `Part`（单用时）
- ✅ `HandZone` / 手牌区域
- ✅ `HandQueue` / 手牌队列
- ✅ `RuntimeCost` vs `BaseCost`
- ✅ `HandAnchor`（左手牌 / 右手牌）

## GameplayTag 约束

- 所有 tag 必须在 `WacomCore/Public/Tags/WacomGameplayTags.h` 用 `UE_DECLARE_GAMEPLAY_TAG_EXTERN` 声明。
- 严禁业务代码里用字符串拼 tag（`FGameplayTag::RequestGameplayTag("Foo.Bar")` 是反模式）。
- 新增 tag 时同步更新 `Data_Schema_Draft.md §2`。

## 完成即验证（硬要求）

**每个切片写完必须在本机编译通过才算完成。**

编译命令（非阻断验证）：

```
"e:\UE_5.7\Engine\Build\BatchFiles\Build.bat" WacomEditor Win64 Development -Project="d:\UE_Project\5.7\Wacom\Wacom.uproject" -WaitMutex -FromMsBuild
```

- 成功标记：`Result: Succeeded`
- 预计第一次全量 40-50s，增量 5-10s

如果编译失败，必须先修到通过再汇报完成，不得把"未验证"的代码当作完成。

## 随机与确定性

- 所有腾挪、随机插入、随机目标使用 `BattleState.Rng`（`FRandomStream`）。
- 不得调用 `FMath::Rand`、`FMath::RandRange` 等全局随机。
- 测试可注入 seed 复现。

## 不引入的东西（第一阶段）

- GAS（GameplayAbilitySystem）：不作为战斗核心。
- DataTable：第一阶段用 PrimaryDataAsset。
- SaveGame / 网络复制：先不写 USTRUCT serialize 支持。
- 完整 UI 美术：第一阶段靠 `BattleTestActor` + 打印。
- GAS/UI/Animation 插件化：延后。

## 开发日志

重要里程碑写入 `Docs/Dev_Log.md`，仅记录：
- 哪个切片完成
- 遇到并绕过的约束
- 规则文档因为实现需要做的修订点

不写每日流水账。

## 规则问题的唯一真相

战斗、手牌、卡牌规则的真相在 `Docs/*.md`，不在脑内。遇到规则分歧时：

1. 先查 `Docs/Battle_Rules.md` / `Hand_Zone_Rules.md` / `Data_Schema_Draft.md`。
2. 如果文档没覆盖或矛盾，标注为"规则分歧"，向主导提问，不自行发挥。
3. 达成共识后先改文档，再改代码。

绝不在代码里悄悄引入未写入文档的规则分支。
