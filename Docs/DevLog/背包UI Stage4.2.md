# 背包 UI Stage 4.2

> 时间：2026-05-17
> 范围：GDD §11 背包系统的 UI 表现层（点击切换语义）

## 做了什么

围绕 GDD §11 的"万物成卡"背包，把已有的 RunSession 业务 API 接到 UI 上。

### 新建文件

- `Source/WacomApp/Public/UI/Backpack/WacomDeckCardWidget.h`
- `Source/WacomApp/Private/UI/Backpack/WacomDeckCardWidget.cpp`
- `Source/WacomApp/Public/UI/Backpack/WacomBackpackScreen.h`
- `Source/WacomApp/Private/UI/Backpack/WacomBackpackScreen.cpp`

### 改动文件

- `Source/WacomApp/Public/GameFramework/WacomPlayerController.h`：加 `IA_OpenBackpack` 字段、`OnOpenBackpackPressed` / `TryOpenBackpackFromConsole` 方法声明
- `Source/WacomApp/Private/GameFramework/WacomPlayerController.cpp`：实现 + 注册两个 console command
- `Docs/WacomApp.md`：§7 加 BackpackScreen 小节、§8 IMC/IA 表加 IA_OpenBackpack

## 关键决策

### 点击切换替代拖拽（Q4）

- 第一版完全不做 DragDropOperation
- 卡片主按钮 = Move（Backpack ↔ BattleDeck 互换），由 RunSession 自己识别当前在哪一区
- 卡片右上角 X = Delete（弹 Confirm → DeleteCardForGold）
- 节省了 OnDragDetected / NativeOnDrop / DragVisual 一整套样板

### Console command 临时入口（Q7）

- `IA_OpenBackpack` 资产由用户手动建（避免 commandlet 写资产引发的 GUID 问题）
- 建好前用 `Wacom.OpenBackpack` / `Wacom.CloseBackpack` 在 PIE 控制台触发
- 命令最终统一进入 `TryOpenBackpackFromConsole`：与 IA 触发同一条路径
- 控制台 lambda 无法访问 protected 成员，所以拆出 public `TryOpenBackpackFromConsole`

### 全量 RebuildFromRunState 刷新（GDD §11.5）

- 任何 Move/Delete 完成后整个 BackpackScreen 重建子控件
- 不写增量 diff，省掉很多边界情况
- 数百张卡的 RunState 不需要担心性能

### Move/Delete 按钮启用判定提前到 UI

业务规则（IsIntrinsicCard / IsBagProviderCard / 容量上限 / 最后 BagProvider）已经在 RunSession 层把守。
UI 同步禁用按钮只是为了视觉一致，不替代业务校验。

### 战斗 IMC 不绑 IA_OpenBackpack（Q3）

- 战斗中按 B 不会触发 OnOpenBackpackPressed
- 即便误触发，OnOpenBackpackPressed 内部也防御了 `GetGameFlowState() != Exploration` 早返回

### 暂停菜单和背包共用 GameMenu 层

- "GameMenu 层有顶层 widget" 时按 B 直接 Deactivate（关闭语义）
- 与 ESC 走 OnOpenMenuPressed 的逻辑同构

## 踩坑

- 控制台 lambda 调用 protected 方法编译失败：拆 public 包装
- DeckCardWidget 的"右上角 X"用 CanvasPanel + 锚点（1,0）+ Alignment(1,0) + 负偏移实现，不用额外 SizeBox
- `FAutoConsoleCommandWithWorld` 的 lambda 形参签名是 `(UWorld*)`，不要写 `(const TArray<FString>&, UWorld*)`

## 验证

- 编译：`Build.bat WacomEditor Win64 Development` 通过
- 自动化测试：Wacom.* 79/79 全绿（无新增测试，UI 层暂无单测必要性）
- 手动测试：用户在 IA 资产建好后 Standalone Game 中按 B 打开背包，验证 Move / Delete 按钮、ConfirmDialog 流程。建好前可用 Console `Wacom.OpenBackpack`

## 待办

- 用户手动创建 `Content/Wacom/Input/IA_OpenBackpack` 资产
- 用户手动在 `IMC_Exploration` 中给 IA_OpenBackpack 绑 B 键
- 在 `WacomPlayerController.cpp` 取消 `LazyLoadIA(IA_OpenBackpack, ...)` 那行注释
- Stage 4.3：B 类容器卡 + 容量效果（等具体卡设计）
- Stage 4.4：删牌卡（等具体卡设计）
