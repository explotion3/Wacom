# AGENTS.md

本文件是 Wacom 项目的协作规则。目标是让每次改动都能延续项目架构，而不是只解决眼前问题。
请你把用户当成一个什 么都不懂的傻子，用户 的要求可能很模糊 也可能不准确，甚至 会有一些专业性的错 误，你要以产品经理 的思维，先去理解用户的需求，请你根据自 己的判断协助用户完成 项目。
每当用户提出一个新的需求时，一定要反复向他提问，直到所有细节都安排妥当才能开工。
## 项目概况

- UE 5.8 第一人称 HD-2D 卡牌冒险游戏。
- 规则和设计真相优先看 `Docs/`，代码实现位于 `Source/`。
- 新增功能要服务长期扩展：能复用的系统不要写成一次性页面、一次性 Actor 或硬编码流程。

## 当前阶段重点

- 项目已从规则原型阶段进入“规则内核继续稳固，first-person hand / Battle UI / Run UI 表现和 WacomMap 规划逐步落地”的阶段。
- 近期卡牌表现主线是 HUD-rendered first-person card layer。涉及手牌、抽牌、出牌、悬停、选中、fake-3D 或发牌动效时，优先沿用现有链路：Battle/Run 事件或 Snapshot -> UI transition hint / ViewData -> Anchor/Layer/Slot/ViewWidget 表现。不要重新引入旧版 2D battle hand、世界 WidgetComponent 手牌或一次性 Widget 状态机。
- 外部 Demo、GitHub 项目和视频参考只作为行为、节奏、视觉目标参考；除非授权兼容且用户明确同意，不直接复制第三方代码、shader、资产或工程结构。复刻时应实现 Wacom-native 的 UE/UMG/Material 方案，并把关键假设写入对应 `Docs/`。
- 工作区可能包含上一轮 agent 的试验内容。开始实现前先确认 `git status` 和相关 diff，区分用户改动、试验残留和本次任务范围。

## 开始任务前

1. 先判断任务属于哪个领域。
2. 读取对应文档和代码。
3. 如果规则不明确，先列出假设和问题，不要悄悄写死行为。
4. 会影响规则、长期架构、策划口径或资产语义的关键细节，正式执行前必须向用户确认；普通实现细节可按现有文档和代码风格自行决策。
5. 代码完成后，按事实类型同步文档：长期规则、资产语义和制作合同写对应领域文档；短期任务写 `Docs/TODO.md`；技术债写 `Docs/TechDebt.md`；待确认设计问题写 `Docs/Questions.md`。

| 领域 | 文档 | 代码 |
|---|---|---|
| 战斗规则 | `Docs/WacomBattle.md` | `Source/WacomBattle/` |
| Run/探索 | `Docs/WacomRun.md` | `Source/WacomRun/` |
| 数据/卡牌 | `Docs/WacomData.md` | `Source/WacomData/` |
| Battle UI / first-person 手牌 | `Docs/WacomBattleUI.md`, `Docs/First_Person_Card_Layer_Design.md`, `Docs/UI_Battle_WBP_Binding.md` | `Source/WacomApp/Private/UI/Battle/`, `Source/WacomApp/Private/UI/Card/`, `Source/WacomApp/Public/UI/Card/` |
| UI/App shell | `Docs/WacomApp.md`, `Docs/WacomUI.md`, `Docs/WacomUIFoundation.md` | `Source/WacomApp/` |
| 世界交互 | `Docs/WacomWorldInteraction.md` | `Source/WacomApp/Private/Interaction/`, `Source/WacomApp/Private/Actors/`, `Source/WacomApp/Private/Components/` |
| 内容制作/校验 | `Docs/WacomDataAuthoring.md` | `Source/WacomEditor/`, `Source/WacomData/` |
| 架构/模块 | `Docs/Architecture.md` | `Source/*/` |
| 测试 | 对应领域文档 | `Source/WacomTests/` |

### CodeGraph 使用

- 项目已配置 `colbymchenry/codegraph` MCP，并在仓库内建立 `.codegraph` 索引。
- 探索“某个系统怎么工作、某个函数被谁调用、修改会影响哪里、从 A 怎么流到 B”时，优先使用 CodeGraph：`codegraph_context`、`codegraph_callers`、`codegraph_callees`、`codegraph_impact`、`codegraph_trace`。
- 查项目文件结构时优先使用 `codegraph_files` 或 `rg --files`，少做全仓库目录扫描；新增文件或刚改过的文件以 live 文件系统为准。
- CodeGraph 用于快速建立结构认知；真正修改前仍要读取 live 文件内容，以当前工作区文件为准，避免索引延迟或动态调用导致误判。
- 为避免 Windows 弹出命令框，CodeGraph MCP 已关闭 file watcher / auto-sync。每次调用 CodeGraph 前需要让 CodeGraph 反映新结构时，手动运行：
```powershell
& 'C:\Users\ahhh\AppData\Local\codegraph\current\node.exe' --liftoff-only 'C:\Users\ahhh\AppData\Local\codegraph\current\lib\dist\bin\codegraph.js' sync 'D:\UE_Project\5.7\Wacom'
```

### Spec Kit 使用

Spec Kit 是项目级规划工具，用来把较大的功能需求拆成 `spec.md`、`plan.md`、`tasks.md` 等设计工件；它不是默认实现入口，也不替代 `Docs/`、`AGENTS.md`、实时源码和工程判断。

适合主动考虑 Spec Kit 的场景：

- 新增中等以上功能，且会影响多个模块、UI 流程、数据语义或长期架构。
- 需求还比较口语化，需要先整理用户故事、验收标准、边界和非目标。
- 需要在正式编码前形成可审阅的计划、数据模型、contract 或任务拆分。
- 用户明确提到 `$speckit-*`、Spec Kit、spec、plan、tasks 或希望“先写规格/计划”。

不需要使用 Spec Kit 的场景：

- 小型 bugfix、低风险文案/布局调整、纯机械迁移、单文件小改。
- 已经有明确 `tasks.md` 或清晰实现路径时，直接按文档和代码实现。
- 用户要求快速验证或临时 spike，且不会改变长期规则。

推荐流程：

1. `$speckit-specify`：把需求写成规格；正文可优先使用中文，代码名、路径、命令、GameplayTag、类名保持英文。
2. `$speckit-plan`：结合 `Docs/`、模块边界和现有源码生成实现计划。
3. `$speckit-tasks`：生成任务清单；生成后必须再由主会话审阅，删除或改写过泛、条件化、跨错模块或会扩大巨型测试文件的任务。
4. 实现阶段默认不调用 `$speckit-implement`，而是由主会话读取 `tasks.md`、相关文档和 live 源码后逐项实现、验证并同步长期文档、`Docs/TODO.md` 或 `Docs/TechDebt.md`。

Spec Kit 输出只能作为草案。实现前必须重新确认：`WacomRun` / `WacomBattle` / `WacomData` / `WacomApp` 的职责边界、UI 是否保持 passive、测试是否放在合适的小型 spec 文件中，以及是否需要更新 `Docs/`、`Docs/TODO.md`、`Docs/TechDebt.md` 或 `Docs/Questions.md`。

`specs/` 目录里的 `spec.md / plan.md / tasks.md` 是阶段性规划工件，不是项目长期规则真相。功能完成后，凡是已经落地为规则、资产语义、UI 绑定、制作流程、验证口径或技术债的内容，都必须回写到对应 `Docs/` 文件、`Docs/TODO.md`、`Docs/TechDebt.md` 或 `Docs/Questions.md`；不要让长期事实只停留在某个 feature spec 里。

## 协作模式

默认采用“主对话直接负责”的工作方式。主会话完成日常探索、实现、测试和总结，避免为了普通任务额外开启 subagent 造成等待和集成成本。

主对话负责：

- 确认设计口径、架构边界、任务拆分和验收标准。
- 直接读取必要文档和代码，完成可控范围内的实现与验证。
- 执行最终验证，更新 `Docs/` 或 `Docs/TODO.md`。

复杂任务优先拆成可独立验证的小切片；普通小切片、文档同步、机械迁移和常规测试由主会话直接完成。

## Subagent 使用边界

默认不开 subagent。只有用户明确要求，或任务确实复杂、风险高、可并行且文件边界清楚时，才考虑开启。

使用 subagent 前，主会话必须先形成清晰任务说明，并明确：

- 目标和验收标准。
- 允许修改的文件或模块范围。
- 不允许触碰的边界。
- 预期验证方式。
- 最终输出：结论、改动文件列表、测试结果和剩余风险。

启用 subagent 后，主会话仍负责最终取舍、冲突解决、验证和文档同步。多个 subagent 并行时必须拥有互不重叠的文件范围；如果需要接管正在运行的任务，先中断或关闭对应 subagent，再由主会话继续。

## 模块职责

| 模块 | 职责 |
|---|---|
| `WacomCore` | 通用类型、GameplayTags、跨模块共享契约 |
| `WacomBattle` | 战斗规则、战斗状态、战斗 Snapshot/Event |
| `WacomRun` | 战斗外 Run 状态、背包 storage、商店、RunEvent、探索流程、战斗结果回传和 SaveGame schema |
| `WacomData` | 卡牌、敌人、角色等数据定义与注册 |
| `WacomApp` | GameMode、PlayerController、first-person、世界交互、输入、CommonUI shell、HUD/Screen/Toast 和 UI 表现协调 |
| `WacomEditor` | 编辑器工具、数据校验、开发辅助 |
| `WacomTests` | 自动化测试与测试夹具 |

不要让高层模块反向污染底层模块。例如 `WacomBattle` 不应该依赖 UI，`WacomCore` 不应该依赖具体玩法实现。

## 方案设计原则

- 做方案时默认提供成熟、长期可复用的设计，而不是临时绕过、一次性修补或只服务当前 Demo 的实现。
- 首选方案应说明可复用边界：所属模块、公共 contract、数据来源、UI/规则分层、扩展点、验证方式和需要回写的文档。
- 如果存在短期 workaround，只能作为用户明确要求、风险控制或分阶段交付时的备选项；不要把临时实现包装成正式设计。
- 提出临时备选时，必须同时给出正式方案、临时方案的适用范围、可能复发或阻塞后续扩展的风险，以及清理条件。
- 当长期方案成本明显更高或会影响大量资产/模块/策划口径时，先说明取舍并和用户确认分阶段路线；每一阶段都应朝最终架构靠近。

### 当前阶段重构授权

- 当前阶段允许为了正式架构大规模重构旧代码，不需要为了兼容早期原型、上一轮 agent 试验实现或短期 Demo 路径而保守设计。
- 方案设计可以采用更激进的正式实现：删除或替换脆弱原型路径、重划模块边界、拆分巨型类、收敛重复 UI flow、改造公共 contract、迁移数据结构或重新组织测试。
- 大规模重构前先说明目标架构、影响模块、计划删除/迁移的旧路径、验证策略和文档更新点；执行时尽量拆成可编译、可验证的小切片。
- 允许主动指出“这块应该重写而不是修补”。如果重写更符合长期架构，应优先给重写方案，并说明为什么继续补丁会拖累后续扩展。
- 即使允许激进重构，也不能回退用户已有改动或静默改变策划规则、资产语义、存档 schema、内容制作流程。此类变化仍需明确列出并获得确认。

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

如果必须采用临时方案，需要在代码中标注 `TODO(技术债)`，并写入 `Docs/TechDebt.md`，说明正式方案、影响范围和建议清理顺序。

## Bug 修复原则

修复 bug 时默认采用长期有效的方案，而不是只做当前最小短期补丁。除非用户明确要求“先临时绕过”“只做最小修复”或“不要扩大范围”，否则应按以下口径处理：

- 先复现和定位真实根因，再决定修改点；不要只在报错行、当前调用点或测试断言附近打补丁。
- 如果根因暴露的是职责边界、生命周期、输入所有权、状态同步、异步事务、资源注册/反注册等结构问题，应主动指出，并给出可落地的整理方案。
- 优先修正系统级所有权和公共路径，让同类问题不再在下一个界面、Actor、Widget 或测试里重复出现。
- 不把 prototype / spike 的临时假设继续扩散到正式路径；如果原型代码已经影响正式流程，应提出收口或迁移计划。
- 允许为了风险控制分阶段落地，但每一阶段都应朝正式版架构靠近，并在 `Docs/TechDebt.md` 或 `Docs/TODO.md` 记录剩余问题。
- 如果长期方案会明显影响策划口径、资产制作方式、模块边界或大量现有代码，先和用户讨论方案取舍；不要悄悄做大范围重构。
- 如果用户选择短期修复，也要在最终说明中标明它为何是临时方案、可能复发的场景，以及建议的正式修复方向。

遇到现有代码中不合理、脆弱或明显停留在原型阶段的实现时，不要为了完成当前任务而默认忽略。应在不阻塞当前目标的前提下，向用户说明问题、影响范围和建议的修正顺序，帮助项目逐步脱离原型阶段，向正式版靠近。

## UI 约定

- Passive Widget 不直接修改战斗或 Run 状态；Screen / HUD coordinator 接收 UI 意图，再通过私有 flow/helper 调用领域入口。
- 战斗 UI 使用 Snapshot 推送，玩家命令统一回传给 `UBattleHUD` / 上层控制者。
- Run/UI 长生命周期界面优先通过 ViewModel/Provider 获取只读数据；Shop / RunEvent / Backpack 等 Screen 可以读取 Run Snapshot/ViewData，并由 Screen flow 提交 `URunSession` 写 API。
- Widget 生命周期要考虑 CommonUI 的 Activate/Deactivate，不要只依赖 Construct。
- 新 UI 控件应明确：数据来源、刷新时机、输入焦点、反订阅时机。
- 暴露给编辑器的 UI / 调试 / 数值参数必须写清 `meta` 注释：`ToolTip` 说明用途，且 `ToolTip` 文案默认使用中文；数值型参数应在 `ToolTip` 文案中写明单位、作用范围、推荐调参区间和是否影响布局，不要把推荐范围直接写成 `ClampMin`、`ClampMax`、`UIMin` 或 `UIMax` 来顶死调参空间。只有存在真实规则、运行时安全、引擎 API 或资产合法性约束时，才在代码 meta 中使用硬限制，并在 `ToolTip` 中说明原因。
- 自动化测试访问 UI 内部状态时，优先使用 production 非反射 automation test view + `WacomTests/Private` access wrapper；不要在 `WacomApp/Public` 扩散散落 `ForTest` getter、测试字段或 Blueprint 可见测试 API。

### First-person card layer 当前边界

- `UWacomFirstPersonCardAnchorComponent` 是手牌布局和表现参数的主要制作入口；C++ 默认值应该服务 WBP 可调，不把单次 Demo 参数硬编码到规则层或 HUD 流程里。
- `UWacomFirstPersonCardLayerWidget` 负责整层布局、输入和 transition hint 分发；`UWacomFirstPersonCardLayerSlotWidget` 负责单槽运动；`UWacomFirstPersonCardViewWidget` 负责卡面内容。新增抽牌、发牌、hover、selected 或 fake-3D 表现时，优先保持这个边界。
- 抽牌/发牌动效应从领域事件或 UI transition hint 派生，表现层只消费语义和参数；不要让 Widget 自己推断抽牌规则、卡组状态或战斗结算。
- Godot Demo 0.2 一类参考效果应先拆成 Wacom 可复用的表现能力：入场 origin、stagger、arc/curve、scale/rotation/opacity、hover fake-3D、RetainerBox/Material 效果。每一层能力独立验证，避免把整套 Demo 一次性塞进 BattleHUD。

## 测试与验证

每个可运行切片完成后至少编译一次。涉及规则、存档、背包、UI 流程时补充或更新自动化测试，并优先运行相关命名空间而不是默认全量 `Wacom`。

纯文档、规划或 `AGENTS.md` 调整不需要编译；最终说明中写清未运行编译/测试即可。仅改 UMG/WBP/材质资产而没有 C++ 变更时，优先做编辑器或 PIE 验证，并在无法自动化时记录手动验收点。

验证分层口径保持简单：

- 自动化测试负责稳定契约：战斗规则、Run 状态、抽牌结果、卡牌 ID/顺序/数量、存档 schema、数据校验、公共 contract、UI transition hint 生成、CommonUI 生命周期、反订阅、边界状态和已经复现过的 bug。
- PIE / 人工验收负责表现和手感：手牌抽牌/发牌动效、hover 手感、fake-3D 视觉、UMG/WBP 绑定、材质表现、镜头/布局/节奏，以及需要判断“像不像、顺不顺、好不好看”的内容。
- 不补自动化测试时，也必须有明确验证路径。容易在 PIE 中稳定复现的表现类功能，可以交给用户验收；最终说明需要列出建议的 PIE 检查点和剩余风险。
- 不容易在 PIE 中稳定复现、牵涉边界条件、状态所有权、异步/生命周期或跨模块 contract 的内容，优先补自动化测试；不要只交给人工观察。
- 同时包含表现和契约的功能要拆开验证。例如抽牌动效的视觉节奏可以 PIE 验收，但 `CardsDrawn` 事件是否给新卡生成 `Drawn` transition hint、顺序是否稳定、重复刷新是否不重播等契约应补小型自动化测试。
- 可以阶段性集中补测试：大 UI/Actor 制作模块还在快速成型时，先保留清晰验收清单，等 2-4 个相关切片稳定后再集中写一组更高质量的 contract/integration tests。
- 不要继续塞进巨型测试文件：像 `BattleWidgetSpec.cpp` 这种已经太大了，后续应该拆成更小的 spec 文件，比如 `BattleSceneEnemyActorSpec.cpp`、`BattleHUDSceneTargetSpec.cpp`、`BattlePresentationSpec.cpp`。

```powershell
& 'E:\UE_5.8\Engine\Build\BatchFiles\Build.bat' WacomEditor Win64 Development -Project='D:\UE_Project\5.7\Wacom\Wacom.uproject' -WaitMutex -NoHotReloadFromIDE
```

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -ExecCmds='Automation RunTests Wacom; Quit' -Unattended -NoPause -NoSplash -NullRHI -DDC-ForceMemoryCache
```

常用 UI 测试前缀示例：

- RunEvent UI：`Wacom.UI.Event`
- Shop UI：`Wacom.UI.Shop`
- World interaction：`Wacom.UI.WorldInteraction`
- Run menu drop target：`Wacom.UI.RunMenuDropTarget`
- First-person card layer：`Wacom.UI.FirstPersonCardLayer` / `Wacom.UI.RunFirstPersonCardLayer`
- Battle UI：`Wacom.UI.Battle`

如果无法运行测试或编译，必须在最终说明中写清楚原因和剩余风险。

## Git 与文件安全

- 不要回退用户已有改动。
- 不要使用 `git reset --hard`、`git checkout --` 或批量删除，除非用户明确要求。
- 清理上一轮 agent 工作区时，先用 `git status` / `git diff` 确认范围；除非用户明确要求丢弃，否则优先保护用户改动，再做定向 restore / clean。
- 编辑文件优先使用小范围 patch。
- 生成文件、缓存、Binaries、Intermediate 不作为主要交付内容。

## 完成标准

一个任务完成时应满足：

- 行为符合对应文档。
- 代码没有跨错模块边界。
- 新能力能被后续内容复用。
- 编译和相关测试通过，或明确说明未验证原因。
- 重要设计变化已同步到 `Docs/`。

<!-- SPECKIT START -->
For additional context about technologies to be used, project structure,
shell commands, and other important information, read the current plan
at specs/006-backpack-real-card-piles/plan.md
<!-- SPECKIT END -->
