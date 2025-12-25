# WHUDR

基于 SFML 3 的 2D RPG/冒险项目，包含标题界面、地图探索、回合制战斗与对话系统。项目使用固定逻辑视口 640×480，并通过 letterboxing 保持像素风格呈现。

## 特性
- 4:3 视口与自动黑边：统一视觉比例与像素锐利度。
- 栈式状态机：`Title` / `Overworld` / `Battle` / `Cutscene` 场景切换。
- 回合制战斗 UI：角色→行动→子选项→目标→确认，支持撤销与物品占用。
- 对话与音效：文本滚动、角色语音提示音、常用交互音效预载。
- 数据层：英雄/装备模板、全局队伍与背包、读档与存档示例。

## 技术栈
- 语言：C++20
- 框架：SFML 3（Graphics / Window / System / Audio）
- 数据：nlohmann/json（单头文件）
- 构建：CMake + Visual Studio / VS Code 任务

## 目录结构
- `src/`：核心源码（`Game`、`States`、`UI`、`Battle`、`Manager`、`Utils`）
- `assets/`：字体、音乐、音效、精灵等资源
- `external/JSON/json.hpp`：JSON 头文件
- `external/SFML/SFML-3.0.2/`：SFML 3 SDK
- `savedata/file_0.json`：示例存档
- `doc/vscode_template/`：VS Code C/C++ 调试与任务模板

## 运行机制简述
- 入口：`src/main.cpp` 创建 `Game` 并调用 `Game::run()`
- 渲染：在 `Game` 中固定逻辑视口 640×480，`updateViewViewport()` 保持 4:3 并加黑边
- 状态：使用 `std::stack<std::unique_ptr<BaseState>>` 管理当前场景，`push/change/pop` 切换
- 标题：`TitleState` 处理菜单与欢迎对话，选择新游戏/继续游戏后进入 `Overworld`
- 战斗：`BattleMenu` 通过分阶段游标与音效反馈收集指令，生成 `BattleCommand`

## 构建与运行（Windows / VS 2022）
### 先决条件
- CMake ≥ 3.20
- Visual Studio 2022（MSVC，x64）
- 已包含 `external/SFML/SFML-3.0.2`（CMake 包路径在 `CMakeLists.txt` 已设置）

### 命令行构建
```powershell
# 生成工程（VS 2022, x64）
cmake -S . -B build -G "Visual Studio 17 2022" -A x64

# 编译 Debug
cmake --build build --config Debug

# 运行（路径因生成器不同可能为 build/Debug 或 x64/Debug）
# 可在资源旁运行或通过 VS Code 调试（见下文）
```

### VS Code 任务
- 在 VS Code 中打开工作区后，运行任务 “CMake Build”（Debug）。
- 调试模板位于 `doc/vscode_template/`，可按需复制到 `.vscode/` 并调整。

### 常见问题
- 找不到 SFML：检查 `CMakeLists.txt` 的 `SFML_DIR` 指向 `external/SFML/SFML-3.0.2/lib/cmake/SFML`
- 运行时缺少 DLL：确保 `sfml-graphics-3.dll`、`sfml-window-3.dll`、`sfml-system-3.dll`、`sfml-audio-3.dll` 在可执行文件旁或在 `PATH`

## 交互与控制
- 标题提示：按 `Z` 或 `Enter` 开始（来自标题界面文案）
- 通用交互（建议）：`Z/Enter` 确认、`X/Esc` 取消、方向键导航
- 键位映射：可在 `InputManager` 中调整与扩展（支持手柄可作为后续增强）

## 扩展建议
- 输入配置：将键位外置到配置文件，增加手柄与可重绑定支持
- UI 主题：统一 `BattleMenu` / `DialogBox` 主题与过渡动画参数
- 数据驱动：将 Act/物品/敌人技能外置到 JSON，降低扩展难度
- 调试工具：添加开发者控制台或快速切场命令以便测试

## 致谢
- SFML 团队与开源社区
- JSON for Modern C++（nlohmann/json）

> 提示：本项目未附带许可证文本；如需开源发布，请补充许可证并梳理第三方资源条款。

### 用户操作&注意事项
确认/交互：Z 或 Enter 
取消/撤销：X 或 Shift 
菜单： C 或 Control 
移动： ↑ ↓ ← →
切换Debug模式（显示部分碰撞箱）：D

神圣斗篷需要拿到后在主世界按C打开物品栏使用后才会装备。