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
4. 正式执行前，先向用户确认会影响规则、长期架构或策划口径的关键细节；不能把不确定的设计点直接写死到代码里。
5. 代码完成后，同步更新相关文档或 `Docs/TODO.md`。

| 领域 | 文档 | 代码 |
|---|---|---|
| 战斗规则 | `Docs/WacomBattle.md` | `Source/WacomBattle/` |
| Run/探索 | `Docs/WacomRun.md` | `Source/WacomRun/` |
| 数据/卡牌 | `Docs/WacomData.md` | `Source/WacomData/` |
| UI/App | `Docs/WacomApp.md` | `Source/WacomApp/` |
| 架构/模块 | `Docs/Architecture.md` | `Source/*/` |
| 测试 | 对应领域文档 | `Source/WacomTests/` |

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
