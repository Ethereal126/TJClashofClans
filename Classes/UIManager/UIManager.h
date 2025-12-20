#pragma once
#ifndef __UI_MANAGER_H__
#define __UI_MANAGER_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include <string>
#include <map>
#include <functional>

// 前向声明
class Building;
class MapManager;

// UI层级枚举（用于控制显示顺序）
enum class UILayer {
    Background = 0,   // 背景层
    Game = 10,        // 游戏主界面层
    HUD = 20,         // HUD（抬头显示）层
    Dialog = 30,      // 对话框层
    Tips = 40,        // 提示信息层
    Loading = 50      // 加载界面层（最顶层）
};

// UI面板类型枚举
enum class UIPanelType {
    // ===== 加载界面 =====
    LoadingScreen,    // 游戏启动加载界面（背景图+进度条）

    // ===== 主界面 =====
    GameHUD,          // 游戏HUD（资源栏+设置/商店/进攻按钮）

    // ===== 建筑相关 =====
    BuildingOptions,  // 建筑操作浮窗（信息/升级/训练按钮）
    BuildingInfo,     // 建筑信息面板（建筑图像+属性列表）
    BuildingUpgrade,  // 建筑升级面板（属性对比+所需资源+确认按钮）

    // ===== 资源相关 =====
    ResourceBar,      // 资源栏（金币/圣水）

    // ===== 军队相关 =====
    ArmyTraining,     // 军队训练界面（左侧已分配/右侧可选士兵）

    // ===== 战斗相关 =====
    BattleHUD,        // 战斗界面（部署部队+倒计时+星级）
    BattleResult,     // 战斗结果（胜利/失败统计）
    MapSelection,     // 地图选择界面（线路图+可选节点）

    // ===== 系统界面 =====
    Settings,         // 设置面板（音量滑块）
    Shop,             // 商店（建筑列表+确认购买对话框）

    // ===== 提示与对话框 =====
    ToastMessage,     // 提示文本（短暂显示的操作反馈）
    ConfirmDialog,    // 确认对话框（确认/取消按钮）

    // ===== 其他 =====
    LoadingBattleField, // 加载战斗地图界面
};

// 资源类型枚举（用于资源显示更新）
enum class ResourceType {
    Gold,             // 金币
    Elixir,           // 圣水
};

// 建筑类型枚举（用于判断BuildingOptions显示哪些按钮）
enum class BuildingCategory {
    Normal,           // 普通建筑（信息/升级）
    Resource,         // 资源建筑（信息/升级）
    Military,         // 军事建筑/军营（信息/升级/训练）
    Defense,          // 防御建筑（信息/升级）
};

// UI管理器：单例模式，负责管理所有UI界面的创建、显示、隐藏和销毁
class UIManager {
public:
    // ========== 单例管理 ==========
    static UIManager* getInstance();
    static void destroyInstance();

    // ========== 初始化 ==========
    // 初始化UI管理器（设置设计分辨率、屏幕适配等）
    // 调用时机：AppDelegate::applicationDidFinishLaunching()
    bool init(cocos2d::Scene* rootScene);

    // ========== 面板管理 ==========
    // 显示指定类型的UI面板
    void showPanel(UIPanelType panelType, UILayer layer = UILayer::Dialog, bool modal = false);

    // 隐藏指定类型的UI面板
    void hidePanel(UIPanelType panelType, bool removeFromParent = false);

    // 切换面板显示状态（显示↔隐藏）
    void togglePanel(UIPanelType panelType);

    // 检查指定面板是否正在显示
    bool isPanelVisible(UIPanelType panelType) const;

    // 获取指定面板的节点指针（用于外部操作面板内容）
    cocos2d::Node* getPanel(UIPanelType panelType) const;

    // 关闭所有面板（场景切换时使用）
    void closeAllPanels();

    // ========== 建筑操作面板 ==========
    // 显示建筑操作浮窗（根据建筑类型显示不同按钮）
    // position: 建筑在屏幕上的位置（浮窗显示在建筑下方）
    // category: 建筑类别（决定显示哪些按钮）
    // building: 当前选中的建筑指针（用于后续操作）
    void showBuildingOptions(const cocos2d::Vec2& position, BuildingCategory category, Building* building);

    // 显示建筑信息面板
    void showBuildingInfo(Building* building);

    // 显示建筑升级面板
    void showBuildingUpgrade(Building* building);

    // 显示军队训练面板
    void showArmyTraining(Building* building);

    // ========== 升级进度覆盖层 ==========
    // 在建筑上方显示升级进度条
    // building: 正在升级的建筑
    // totalTime: 升级总时间（秒）
    // remainingTime: 剩余时间（秒）
    void showUpgradeProgress(Building* building, float totalTime, float remainingTime);

    // 更新升级进度
    void updateUpgradeProgress(Building* building, float remainingTime);

    // 移除升级进度覆盖层
    void removeUpgradeProgress(Building* building);

    // ========== 提示与对话框 ==========
    // 创建并显示提示文本（Toast）
    void showToast(const std::string& message, float duration = 2.0f);

    // 创建并显示确认对话框
    void showConfirmDialog(const std::string& title,
        const std::string& content,
        const std::function<void()>& onConfirm,
        const std::function<void()>& onCancel = nullptr);

    // 显示信息对话框（仅显示信息，只有"确定"按钮）
    void showInfoDialog(const std::string& title, const std::string& content);

    // ========== 加载界面 ==========
    // 显示游戏启动加载界面
    void showLoadingScreen();

    // 更新加载进度（0.0 ~ 1.0）
    void updateLoadingProgress(float progress);

    // 隐藏加载界面并进入游戏
    void hideLoadingScreen();

    // 显示战斗地图加载界面
    void showBattleLoading(const std::string& tips = "Loading...");

    // 隐藏战斗地图加载界面
    void hideBattleLoading();

    // ========== 资源显示更新 ==========
    // 更新资源显示（金币、圣水）
    void updateResourceDisplay(ResourceType resourceType, int amount);

    // ========== 商店相关 ==========
    // 显示商店面板
    void showShop();

    // 获取待放置建筑信息
    Building* getPendingPlacementBuilding() const { return _pendingPlacementBuilding; }
    int getPendingPlacementCost() const { return _pendingPlacementCost; }
    void clearPendingPlacement() {
        _pendingPlacementBuilding = nullptr; _pendingPlacementCost = 0;
    }

    // ========== 地图选择相关 ==========
    // 显示地图选择界面
    void showMapSelection();

    // 播放节点选择动画并进入战斗
    // mapIndex: 选择的地图索引（0或1）
    void playMapSelectAnimation(int mapIndex, const std::function<void()>& onComplete);

    // ========== UI事件系统 ==========
    void setUICallback(const std::string& eventName, const std::function<void()>& callback);
    void triggerUIEvent(const std::string& eventName);

    // ========== 屏幕适配工具 ==========
    cocos2d::Size getVisibleSize() const;
    cocos2d::Vec2 getVisibleOrigin() const;
    float getScaleFactor() const;
    cocos2d::Vec2 getUIPosition(float percentX, float percentY) const;

    // ========== 战斗模式（新增/修改）==========
    // 进入战斗模式，传入当前战斗地图
    void enterBattleMode(MapManager* battleMap);
    // 退出战斗模式
    void exitBattleMode();
    // 是否处于战斗模式
    bool isInBattleMode() const { return _isBattleMode; }

    // 结束战斗（由 Combat 调用，显示结算界面）
    void endBattle(int stars, int destroyPercent);

    // ========== 战斗 HUD 相关接口 ==========
    // 更新战斗中士兵数量（MapManager 放置士兵后调用）
    void updateBattleTroopCount(const std::string& troopName, int newCount);
    // 更新摧毁百分比（自动计算星级）
    void updateDestructionPercent(int percent);
    // 获取当前选中的士兵类型名称
    std::string getSelectedTroopName() const { return _selectedTroopName; }
    // 获取指定士兵的剩余数量
    int getBattleTroopCount(const std::string& troopName) const;
    // 取消选中士兵
    void deselectBattleTroop();

protected:
    UIManager();
    virtual ~UIManager();

    // 创建具体的UI面板（工厂方法）
    cocos2d::Node* createPanel(UIPanelType panelType);

    // 各面板创建方法
    cocos2d::Node* createLoadingScreen();
    cocos2d::Node* createGameHUD();
    cocos2d::Node* createResourceBar();
    cocos2d::Node* createSettings();
    cocos2d::Node* createShop();
    cocos2d::Node* createMapSelection();
    cocos2d::Node* createBuildingOptions(const cocos2d::Vec2& position, BuildingCategory category);
    cocos2d::Node* createBuildingInfo(Building* building);
    cocos2d::Node* createBuildingUpgrade(Building* building);
    cocos2d::Node* createArmyTraining(Building* building);
    cocos2d::Node* createBattleHUD();
    cocos2d::Node* createBattleResult(int stars, int destroyPercent);
    cocos2d::Node* createUpgradeProgressOverlay(Building* building, float totalTime, float remainingTime);

    // 创建通用关闭按钮（右上角叉）
    cocos2d::ui::Button* createCloseButton(const std::function<void()>& onClose);

    // 添加面板到场景
    void addPanelToScene(cocos2d::Node* panel, UILayer layer, bool modal);

    // 创建模态遮罩层
    cocos2d::LayerColor* createModalLayer();

    // 面板显示/隐藏动画
    void playShowAnimation(cocos2d::Node* panel);
    void playHideAnimation(cocos2d::Node* panel, const std::function<void()>& callback = nullptr);

private:
    static UIManager* _instance;

    cocos2d::Scene* _rootScene;              // 根场景引用
    cocos2d::Size _visibleSize;              // 可见区域大小
    cocos2d::Vec2 _visibleOrigin;            // 可见区域原点
    float _scaleFactor;                      // 缩放因子

    // 面板缓存（面板类型 -> 面板节点）
    std::map<UIPanelType, cocos2d::Node*> _panels;

    // 模态遮罩层缓存
    std::map<UIPanelType, cocos2d::LayerColor*> _modalLayers;

    // UI事件回调映射
    std::map<std::string, std::function<void()>> _callbacks;

    // 当前选中的建筑（用于BuildingOptions/Info/Upgrade）
    Building* _selectedBuilding;

    // 升级进度覆盖层缓存（建筑指针 -> 进度条节点）
    std::map<Building*, cocos2d::Node*> _upgradeProgressNodes;

    // 资源栏标签引用（用于快速更新）
    cocos2d::Label* _goldLabel;
    cocos2d::Label* _elixirLabel;

    // 加载界面进度条引用
    cocos2d::ui::LoadingBar* _loadingProgressBar;

    // 待放置建筑信息（用于传递给MapManager）
    Building* _pendingPlacementBuilding = nullptr;
    int _pendingPlacementCost = 0;

    // 军队配置临时存储（士兵名称 -> 选中数量）
    std::map<std::string, int> _tempArmyConfig;
    int _tempCurrentPopulation = 0;
    // 军队配置存储
    void saveArmyConfig();
    std::map<std::string, int> loadArmyConfig();
    void refreshArmyTrainingUI(cocos2d::Node* panel);

    // ========== 战斗模式相关（新增）==========
    bool _isBattleMode = false;
    MapManager* _currentBattleMap = nullptr;  // 当前战斗地图指针
    cocos2d::EventListenerTouchOneByOne* _battleTouchListener = nullptr;

    // 战斗模式内部方法
    void setupBattleTouchListener();
    void removeBattleTouchListener();
    bool deploySoldierAt(const cocos2d::Vec2& screenPos);

    // ========== 战斗 HUD 相关 ==========
    // 战斗中的士兵数量（士兵名称 -> 剩余数量）
    std::map<std::string, int> _battleTroopCounts;
    std::vector<std::string> _battleTroopNames;
    // 当前选中的士兵按钮索引，-1 表示未选中
    int _selectedTroopIndex = -1;
    // 当前选中的士兵类型名称
    std::string _selectedTroopName;

    // 战斗 HUD 内部方法
    void selectBattleTroop(int index, const std::string& troopName);
    void refreshBattleHUDTroops();

    void playWhiteTransition(const std::function<void()>& onComplete);
};

#endif // __UI_MANAGER_H__