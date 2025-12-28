# TJClashofClans

本项目是“程序设计范式”课程的期末大项目，基于 **Cocos2d-x (C++)** 框架开发的一款《部落冲突》(Clash of Clans) 复刻版游戏。项目旨在通过游戏开发实践，深度应用面向对象设计原则、设计模式以及模块化软件架构。

## 🎮 游戏核心功能

- **村庄建设系统**：支持建筑拖拽摆放、重叠检测、网格对齐及升级逻辑。
- **实时战斗引擎**：多兵种 AI 协同、自动寻路算法、防御建筑自动反击。
- **资源管理体系**：金币与圣水的动态产出、存储上限管理及大本营核心数据同步。
- **战斗重放系统**：基于指令流记录与回溯的战斗模拟复现功能。
- **持久化存储**：基于 JSON 格式的玩家存档与关卡配置管理。
- **视听反馈**：多层级 UI 框架、动态血条显示及事件驱动的音效系统。


## 🛠 技术栈

- **引擎**：Cocos2d-x v4.0+
- **语言**：C++ 11/14
- **数据格式**：JSON (nlohmann/json)
- **测试框架**：Google Test (GTest)
- **开发环境**：Visual Studio 2022 (Windows)


## 📂 项目结构

```text
Classes/
├── AudioManager/    # 音频管理器实现
├── Building/        # 建筑基类及通用逻辑
├── Combat/          # 战斗引擎、AI 寻路、血条组件及单元测试
├── MapManager/      # 网格系统、坐标映射与存档读写
├── ResourceStorage/ # 资源建筑与生产逻辑
├── Soldier/         # 士兵模板与工厂配置
├── TownHall/        # 大本营核心枢纽与全局数据
├── UIManager/       # UI 框架、面板管理与事件回调
├── MainScene.*      # 村庄主场景
├── BattleScene.*    # 战斗场景
└── ReplayScene.*    # 重放场景
```


## 🚀 快速开始

### 环境依赖
- **操作系统**：Windows 10/11
- **开发工具**：Visual Studio 2022 (必须安装“使用 C++ 的桌面开发”组件)
- **构建工具**：CMake 3.10+
- **脚本环境**：Python 2.7 (Cocos2d-x 某些编译脚本可能需要)

### 编译运行步骤
1. **获取代码**：
   ```bash
   git clone https://github.com/YourRepo/TJClashofClans.git
   cd TJClashofClans
   ```
2. **初始化引擎环境**：
   进入 `cocos2d` 目录，依次执行环境配置和依赖下载脚本（需联网）：
   ```bash
   cd cocos2d
   python setup.py
   python download-deps.py
   cd ..
   ```
3. **生成 Visual Studio 工程**：
   打开 **Developer Command Prompt for VS 2022**，进入项目根目录执行：
   ```bash
   mkdir build
   cd build
   cmake -S .. -B . -G "Visual Studio 17 2022" -A win32 -T v143 -Wno-deprecated
   ```
4. **编译与运行**：
   - 在 `build` 文件夹中双击 `TJClashofClans.sln` 打开工程。
   - 右键点击 `TJClashofClans` 设为“启动项目”。
   - 点击“本地 Windows 调试器”或按 `Ctrl+F5` 开始运行。

## 👥 开发团队

- **napper-d**：负责战斗系统逻辑、士兵 AI 实现。
- **fhzjzbx**：负责建筑基类、大本营及资源系统实现。
- **Ethereal126**：负责地图管理器、UI 框架、音效系统及战斗重放系统。


---
*本项目仅用于学术交流与课程学习，相关资源版权归原作者所有。*

