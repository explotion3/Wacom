# AGENTS.md

本文件是 Wacom 项目的协作规则。目标是让每次改动都能延续项目架构，而不是只解决眼前问题。

## 项目概况

- UE 5.7 第一人称 HD-2D 卡牌冒险游戏。
- 规则和设计真相优先看 `Docs/`，代码实现位于 `Source/`。
- 新增功能要服务长期扩展：能复用的系统不要写成一次性页面、一次性 Actor 或硬编码流程。

## 开始任务前

1. 先判断任务属于哪个领域。
2. 读取对应文档和代码。
3. 如果规则不明确，先列出假设和问题，不要悄悄写死行为。
4. 会影响规则、长期架构、策划口径或资产语义的关键细节，正式执行前必须向用户确认；普通实现细节可按现有文档和代码风格自行决策。
5. 代码完成后，同步更新相关文档或 `Docs/TODO.md`。

| 领域 | 文档 | 代码 |
|---|---|---|
| 战斗规则 | `Docs/WacomBattle.md` | `Source/WacomBattle/` |
| Run/探索 | `Docs/WacomRun.md` | `Source/WacomRun/` |
| 数据/卡牌 | `Docs/WacomData.md` | `Source/WacomData/` |
| UI/App | `Docs/WacomApp.md` | `Source/WacomApp/` |
| 架构/模块 | `Docs/Architecture.md` | `Source/*/` |
| 测试 | 对应领域文档 | `Source/WacomTests/` |

## 协作模式

默认采用“主对话作为指挥台”的工作方式。

主对话负责：

- 确认设计口径、架构边界、任务拆分和验收标准。
- 分配 subagent 的探索、实现、测试、审查任务。
- 汇总 subagent 结果，解决冲突，并做最终取舍。
- 执行最终验证，更新 `Docs/` 或 `Docs/TODO.md`。

主对话应尽量保持上下文干净，不长期承载某个模块的细碎探索过程。复杂任务优先拆成可独立验证的小切片。

## Subagent 协作规则

默认新开 subagent 执行独立任务，避免旧上下文污染判断。只有同一问题连续深挖时才复用已有 subagent。

子 agent 默认使用 xhigh。只有纯检索、简单格式化或低风险机械任务，才主动降低强度。

常用角色：

| 角色 | 职责 | 限制 |
|---|---|---|
| Explorer | 只读探索代码、文档、调用链，输出结论和文件行号 | 不修改文件，不做长期设计决定 |
| Worker | 实现指定切片 | 必须声明 owned files，不回退他人改动，不越界修改 |
| Reviewer | 审查当前改动 | 只报告 bug、架构越界、测试缺口和风险，不直接改代码 |
| Validator | 编译、运行测试、分析日志 | 不改业务代码，只输出命令、结果、失败摘要 |

Worker 任务必须包含：

- 明确目标。
- 允许修改的文件或模块范围。
- 不允许触碰的边界。
- 预期测试或验证方式。
- 最终输出改动文件列表和剩余风险。

### 文件租约与接管

- Worker 一旦拿到 owned files，主对话和其他 worker 不得同时修改这些文件。
- 主对话默认不替仍在运行的 worker 完成实现；主对话只做分派、等待、审查 diff、集成判断和最终验证。
- 如果 worker 超时、卡住或产出方向错误，主对话先等待或追加更窄指令；需要接管时，必须先中断或关闭该 worker，再由主对话或新 worker 继续。
- Reviewer 发现问题后，优先把修复回派给对应 worker；只有相关 worker 已关闭、问题很小且不会和其他租约冲突时，主对话才直接修改。
- 大任务优先拆成 10-20 分钟内可完成并可独立验证的小切片；一个切片完成并审查后，再下发下一切片。
- 多个 worker 并行时必须拥有互不重叠的文件范围；如果后续发现需要改到他人租约文件，先停下并回报主对话重新分配。

## 模块职责

| 模块 | 职责 |
|---|---|
| `WacomCore` | 通用类型、GameplayTags、跨模块共享契约 |
| `WacomBattle` | 战斗规则、战斗状态、战斗 Snapshot/Event |
| `WacomRun` | Run 状态、背包、探索流程、战斗外持久数据 |
| `WacomData` | 卡牌、敌人、角色等数据定义与注册 |
| `WacomApp` | UI、HUD、输入入口、界面层协调 |
| `WacomEditor` | 编辑器工具、数据校验、开发辅助 |
| `WacomTests` | 自动化测试与测试夹具 |

不要让高层模块反向污染底层模块。例如 `WacomBattle` 不应该依赖 UI，`WacomCore` 不应该依赖具体玩法实现。

## 可复用扩展原则

新增模块、系统或功能时默认按以下方式设计：

- 规则逻辑放在领域模块内，UI 只读 Snapshot/ViewModel 并发送 Command。
- 公共协议放 `Public/`，具体算法、规则执行、内部状态放 `Private/`。
- 优先组合小型服务、组件、数据结构，不把所有流程堆进一个大类。
- 可配置内容使用 DataAsset、DataTable、GameplayTag 或清晰的数据结构，不把策划数值散落在 UI 或 Actor 中。
- 新增 GameplayTag 必须在 `WacomCore/Public/Tags/WacomGameplayTags.h` 声明，并同步 `Docs/WacomData.md`。
- 只有蓝图访问、序列化、GC、DataAsset、UMG、反射遍历需要时才使用 `UCLASS/USTRUCT/UENUM/UPROPERTY`。
- 头文件尽量前向声明，具体 include 放到 `.cpp`。
- 不为赶进度引入临时单例、全局状态、字符串拼 tag、Tick 轮询 UI 数据。

如果必须采用临时方案，需要在代码中标注 `TODO(技术债)`，并写入 `Docs/TODO.md`，说明正式方案。

## UI 约定

- UI 不直接修改战斗或 Run 状态。
- 战斗 UI 使用 Snapshot 推送，命令回传给上层控制者。
- Run/UI 长生命周期界面优先通过 ViewModel/Provider 获取数据。
- Widget 生命周期要考虑 CommonUI 的 Activate/Deactivate，不要只依赖 Construct。
- 新 UI 控件应明确：数据来源、刷新时机、输入焦点、反订阅时机。
- 暴露给编辑器的 UI / 调试 / 数值参数必须写清 `meta` 注释：`ToolTip` 说明用途；数值型参数需要尽量提供 `ClampMin`、`ClampMax` 或 `UIMin`、`UIMax`，并在说明中写明单位、作用范围和是否影响布局。

## 测试与验证

每个可运行切片完成后至少编译一次。涉及规则、存档、背包、UI 流程时补充或更新自动化测试。

```powershell
& 'E:\UE_5.7\Engine\Build\BatchFiles\Build.bat' WacomEditor Win64 Development -Project='D:\UE_Project\5.7\Wacom\Wacom.uproject' -WaitMutex -NoHotReloadFromIDE
```

```powershell
& 'E:\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -ExecCmds='Automation RunTests Wacom; Quit' -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache
```

如果无法运行测试或编译，必须在最终说明中写清楚原因和剩余风险。

## Git 与文件安全

- 不要回退用户已有改动。
- 不要使用 `git reset --hard`、`git checkout --` 或批量删除，除非用户明确要求。
- 编辑文件优先使用小范围 patch。
- 生成文件、缓存、Binaries、Intermediate 不作为主要交付内容。

## 完成标准

一个任务完成时应满足：

- 行为符合对应文档。
- 代码没有跨错模块边界。
- 新能力能被后续内容复用。
- 编译和相关测试通过，或明确说明未验证原因。
- 重要设计变化已同步到 `Docs/`。
