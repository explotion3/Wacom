# 背包 SpecialZone - Stage 4.5

## 目标

Stage 4.5 将背包系统从按 `UCardDefinition*` 的简单列表升级为按 `FCardInstance` 管理的多 zone 结构，并接入 B 类容器卡的特殊存放区。

## 核心决策

- 每张卡都有 `InstanceId`，同款卡可以被独立放入不同 zone。
- Run 层维护四类物理归属：`Backpack`、`BattleDeck`、`BurdenZone`、`SpecialZones[*].Cards`。
- B 类容器卡不计入通量 / 备战容量公式，而是开辟一个 `SpecialZone`，容量为 `Capacity - 1`。
- SpecialZone 内卡默认不入战，只有 `bBattleEnabledInSpecialZone == true` 且主卡位于 BattleDeck 时才进入战斗初始化。
- `BuildInitParamsForBattle` 输出 `BattleDeckEntries`，SpecialZone 入战卡会携带主卡 `CapacityEffect`。
- `WeaponDamagePlus3` 放在战斗效果派发层处理：武器卡且拥有该容量效果时，`Effect.Damage` 最终值 +3。

## UI 决策

- 背包 UI 改为拖拽模型。
- `UWacomCardDragOperation` 只保存 payload，不含业务逻辑。
- `UWacomZoneDropTarget` 只负责 drop surface，不固定内部布局。
- `UWacomBackpackScreen` 负责标题、WrapBox、角标、全量重建。
- `DragOver` 只做视觉预判，真实规则以 `RunSession::MoveInstance` 或 `DeleteCardForGold` 返回值为准。
- SpecialZone 内右键切换入战标记。
- BattleDeck 区额外显示已入战 SpecialZone 卡的视觉投影，并显示“来自 [B 主卡名]”。

## 存档

SaveGame 升到 v2，新增：

- `FCardInstanceSaveEntry`
- `FSpecialZoneSaveEntry`
- `Backpack`
- `BattleDeck`
- `BurdenZone`
- `SpecialZones`

旧档四个新数组为空时，读取路径按当前角色 StarterDeck 重建 instance。

## 验证状态

截至本记录：

- `Build.bat WacomEditor Win64 Development` 通过。
- `Automation RunTests Wacom` 通过。
- 剩余工作集中在 Stage 4.5 测试补齐与最终检查点。
