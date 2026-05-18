# Stage 0.2 - HP 系统重构

## 目标

按 Game_Design.md §3.1 / §3.4 重构 HP 上限计算：

- **本体 HP 上限**：从硬编码 `BaseMaxHp` 改为 `FingerCount × HpPerFinger`
- **战内 MaxHp 累加**：增加 Companion 关键词过滤，只有带 `Card.Keyword.Companion` 的卡才计入 `MaxHpBonus`

为 Stage 1.1 战外状态容器（手指 / 残疾压力等）铺路。

## 字段变更

### `UCharacterDefinition`

**移除**：
- `int32 BaseMaxHp = 20;`

**新增**：
- `int32 FingerCount = 10;`（角色起始手指数）
- `int32 HpPerFinger = 2;`（每指 HP，按角色可调）
- `int32 GetBasePlayerMaxHp() const`（计算本体上限）

虫妹 `FingerCount = 10, HpPerFinger = 2` → 本体上限 20，与重构前 `BaseMaxHp = 20` 完全等值。
测试 fixture `FingerCount = 50, HpPerFinger = 2` → 本体上限 100，与重构前 `BaseMaxHp = 100` 完全等值。

## 行为变更

### `UBattleSession::Initialize`

战内 MaxHp 累加逻辑：

```
战内 MaxHp = Character->GetBasePlayerMaxHp()
           + Σ(备战卡组中带 Companion 关键词的卡的 MaxHpBonus)
```

显式过滤 `Card.Keyword.Companion`。武器 / 工具 / 中立卡即便填了 MaxHpBonus 也不计入。
烁光蝶（伙伴+武器+连击）因为带伙伴关键词，仍然计入 +6。

虫妹累加（重构前后等值）：
- 本体：`10 × 2 = 20`
- 朝光暮蝶 +1（伙伴）
- 拂晓飞蛾 +1（伙伴）
- 赤腹工蚁 +1（伙伴）
- 烁光蝶 +6（伙伴+武器+连击 → 仍计入）
- 暮蛉 +23（伙伴）
- 战内 MaxHp = `20 + 1+1+1+6+23 = 52`

## 兼容性

`UCharacterDefinition` 是 DataAsset，旧 uasset 已重新生成（`WacomRegenerateContent` commandlet）。
若不重新生成，新引擎读旧 uasset 时：
- `BaseMaxHp` 字段在新 header 已删除，反序列化时被忽略
- `FingerCount` / `HpPerFinger` 不在旧 uasset 中，使用默认值 10 / 2 → 本体上限 20

刚好对齐重构前虫妹 `BaseMaxHp = 20`。运行时无差异。

## 验证

- 编译：`Build.bat WacomEditor Win64 Development` PASS
- 自动化测试：`Automation RunTests Wacom` PASS（30/30 成功）
- 资产重新生成：`-run=WacomRegenerateContent` PASS

## 文件改动

- `Source/WacomData/Public/Characters/CharacterDefinition.h`（字段变更 + 新接口）
- `Source/WacomBattle/Private/Session/BattleSession.cpp`（accumulate 加 Companion 过滤）
- `Source/WacomEditor/Private/ContentBuilders/BugGirlBuilder.cpp`（虫妹定义改用 FingerCount）
- `Source/WacomTests/Private/Fixtures/BattleTestFixtures.cpp`（fixture MakeCharacter 改用 FingerCount）
- `Content/Wacom/Characters/DA_Character_BugGirl.uasset`（commandlet 重新生成）

## 同步文档

`WacomData.md` 待 Stage 1.1 一并同步（届时还会引入压力 / 经验 / 节点等字段）。
