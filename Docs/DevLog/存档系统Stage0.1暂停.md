# Stage 0.1 - 存档系统暂停

## 目标

GDD 重写后引入了大量新字段（手指数 / 压力 / 经验 / 备战卡组 / 时段位置 / 已揭示节点 等）。
继续维护当前存档迁移会让每改一次字段就要改 `UWacomSaveGame` + `MigrateIfNeeded` + 30 个测试，技术债摊得很快。

第一阶段先停用存档系统，等到 demo 完善（节点 / 压力 / 经验 等系统稳定）后再恢复。

## 实现策略

保守屏蔽：底层 `UWacomSaveGame` / `FRunState` 拷贝 / 迁移机制全保留，只在入口层加一个总开关。

**总开关**：`AWacomGameMode::bSaveSystemEnabled = false`（编译期常量）。

## 屏蔽点

| 入口 | 启用时行为 | 暂停时行为 |
|---|---|---|
| `AWacomGameMode::BootstrapRunFromSave` | 读 Main / Auto，失败才新 Run | 直接走新 Run |
| `AWacomGameMode::SaveRunToSlot` | 写盘 | 静默 return false |
| `AWacomGameMode::EndPlay` 退出存档 | 走 SaveRunToSlot | 通过 SaveRunToSlot 已屏蔽 |
| `AWacomGameMode::ExitBattle` 自动存档 | 写 Auto + Main | 通过 SaveRunToSlot 已屏蔽 |
| `WacomMainMenuScreen::Continue` 按钮 | 有存档时启用 | 永远禁用，Handle 也提前返回 |
| `WacomMainMenuScreen::NewGame` | 有存档时弹 ConfirmDialog | 直接开新游戏，不弹 |
| `WacomPauseMenuScreen::Save` 按钮 | 创建并写存档 | 不创建按钮 |
| `WacomMenuGameMode::RequestStartNewGame` | 删旧存档 + OpenLevel | 跳过删存档 + OpenLevel |
| `WacomMenuGameMode::RequestContinueGame` | 读 Main + OpenLevel | 直接 return（理论上 UI 已禁用）|

## 保留可用

- `UWacomSaveGame` / `FRunState` 数据契约不动
- `URunSession::SaveToSlot` / `LoadFromSlot` / `BuildSaveGameFromRunState` / `ApplySaveGameToRunState` / `MigrateIfNeeded` 全留可用
- `Wacom.Run.Save.Roundtrip` 自动化测试照常跑（验证底层契约）

## 关联修复（非本次 Stage 目标）

`DreamShader` 插件 `DreamShaderEditorMaterialGeneratorCodeParsing.cpp` 与 `DreamShaderMaterialGeneratorCodeShared.h` 之间的 ODR 冲突：
- 头文件里 `inline bool IsIdentifierBoundary` / `inline void SkipWhitespace` / `inline bool FindMatchingDelimiter`
- Parsing.cpp 里又拷了一份 `static` 同名定义
- Adaptive Build 把 Parsing.cpp 单独编译时报重复定义

修法：删掉 Parsing.cpp 三个重名 static 函数，include `DreamShaderMaterialGeneratorCodeShared.h` 让 inline 版本可见。
保留 Parsing.cpp 自己的 `MatchesKeywordAt`（Shared.h 没有）。

## 验证

- 编译：`Build.bat WacomEditor Win64 Development` PASS
- 自动化测试：`Automation RunTests Wacom` PASS（30/30 成功）

## 恢复步骤

要重启存档系统时：
1. `AWacomGameMode::bSaveSystemEnabled = true`
2. 重新评估 `UWacomSaveGame` 字段是否覆盖新需求（手指数 / 压力 / 经验 / 时段位置 / 节点状态 / 已揭示节点 / 备战卡组）
3. 必要时 `CurrentSaveVersion++` + `MigrateIfNeeded` 增加分支

## 文件改动

- `Source/WacomApp/Public/GameFramework/WacomGameMode.h`
- `Source/WacomApp/Private/GameFramework/WacomGameMode.cpp`
- `Source/WacomApp/Private/GameFramework/WacomMenuGameMode.cpp`
- `Source/WacomApp/Private/UI/Menus/WacomMainMenuScreen.cpp`
- `Source/WacomApp/Private/UI/Menus/WacomPauseMenuScreen.cpp`
- `Plugins/DreamShader/Source/DreamShaderEditor/Private/DreamShaderMaterialGeneratorCodeParsing.cpp`（关联 ODR 修复）
