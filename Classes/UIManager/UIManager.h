#pragma once
#ifndef __UI_MANAGER_H__
#define __UI_MANAGER_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include <string>
#include <map>
#include <functional>

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
    // ===== 主界面 =====
    MainMenu,         // 主菜单（启动后第一个界面）
    GameHUD,          // 游戏HUD（资源栏+快捷按钮）

    // ===== 建筑相关 =====
    BuildMenu,        // 建造菜单（选择建筑类型）
    BuildingInfo,     // 建筑信息面板（点击建筑面板的信息按钮显示详情）
	BuildingOptions,  // 建筑操作面板（升级/查看详细信息/收集）
    BuildingUpgrade,  // 建筑升级面板（升级确认+需求资源）

    // ===== 资源相关 =====
    ResourceBar,      // 资源栏（金币/圣水/人口容量）

    // ===== 军队相关 =====
    ArmyTraining,     // 军队训练界面（弓箭手/野蛮人/炸弹人/巨人）

    // ===== 战斗相关 =====
    BattlePreparation,// 战斗准备（选择攻击目标）
    BattleHUD,        // 战斗界面（部署部队+技能按钮）
    BattleResult,     // 战斗结果（胜利/失败统计）
	MapSelection,    // 地图选择界面（选择战斗地图）

    // ===== 系统界面 =====
    Settings,         // 设置面板（音乐/音效/语言）
    Pause,            // 暂停菜单（继续/退出）
    Shop,             // 商店（购买宝石/资源包）

	// ===== 提示与对话框 =====
	ToastMessage,     // 提示文本（短暂显示的操作反馈）

    // ===== 其他 =====
    LoadingBattleField,          // 加载战斗地图界面

};

// 资源类型枚举（用于资源显示更新）
enum class ResourceType {
    Gold,             // 金币
    Elixir,           // 圣水
    DarkElixir,       // 暗黑重油
    Gem,              // 宝石
    Population,       // 人口容量
    CurrentPopulation // 当前人口
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
    // 参数：面板类型、层级、是否模态（阻挡底层交互）
    // 调用时机：点击按钮、触发事件时
    // 例如：点击"建造"按钮 -> showPanel(UIPanelType::BuildMenu)
    void showPanel(UIPanelType panelType, UILayer layer = UILayer::Game, bool modal = false);

    // 隐藏指定类型的UI面板
    // removeFromParent=false：只隐藏，保留在内存中（快速再次显示）
    // removeFromParent=true：从父节点移除，释放内存
    // 调用时机：关闭面板时
    void hidePanel(UIPanelType panelType, bool removeFromParent = false);

    // 切换面板显示状态（显示↔隐藏）
    // 调用时机：快捷键切换（如ESC键切换暂停菜单）
    void togglePanel(UIPanelType panelType);

    // 检查指定面板是否正在显示
    bool isPanelVisible(UIPanelType panelType) const;

    // 获取指定面板的节点指针（用于外部操作面板内容）
    cocos2d::Node* getPanel(UIPanelType panelType) const;

    // 关闭所有面板（场景切换时使用）
    // 调用时机：切换场景前（如主菜单->游戏场景）
    void closeAllPanels();

    // ========== 提示与对话框 ==========
    // 创建并显示提示文本（Toast）
    // 调用时机：操作反馈（如"资源不足"、"建造成功"）
    // 自动消失，不需要用户操作
    void showToast(const std::string& message, float duration = 2.0f);

    // 创建并显示确认对话框
    // 调用时机：需要用户确认的操作（如"是否花费500金币升级？"）
    // 区别于Toast：需要用户点击"确定"或"取消"
    void showConfirmDialog(const std::string& title,
        const std::string& content,
        const std::function<void()>& onConfirm,
        const std::function<void()>& onCancel = nullptr);

    // 显示信息对话框（仅显示信息，只有"确定"按钮）
    // 调用时机：提示性信息（如"恭喜升级到2级！"）
    void showInfoDialog(const std::string& title, const std::string& content);

    // ========== 加载界面 ==========
    // 显示加载界面（阻挡所有交互）
    // 调用时机：加载资源时、网络请求时
    void showLoading(const std::string& tips = "Loading...");

    // 隐藏加载界面
    void hideLoading();

    // ========== 资源显示更新 ==========
    // 更新资源显示（金币、宝石、人口等）
    // 调用时机：资源变化时（建造消耗、战斗获得、定时生产）
    // 参数：resourceType使用ResourceType枚举，amount为当前数值
    void updateResourceDisplay(ResourceType resourceType, int amount);


    // ========== UI事件系统 ==========
    // 设置UI事件回调
    // 例如：setUICallback("OnBuildingSelected", []() { showPanel(BuildingInfo); })
    // 调用时机：初始化时注册回调
    void setUICallback(const std::string& eventName, const std::function<void()>& callback);

    // 触发UI事件
    // 调用时机：事件发生时（如点击建筑、完成训练）
    void triggerUIEvent(const std::string& eventName);

    // ========== 屏幕适配工具 ==========
    cocos2d::Size getVisibleSize() const;
    cocos2d::Vec2 getVisibleOrigin() const;
    float getScaleFactor() const;

    // 坐标转换工具（百分比 -> 实际坐标）
    // percentX=0.5, percentY=0.5 表示屏幕中心
    cocos2d::Vec2 getUIPosition(float percentX, float percentY) const;

protected:
    UIManager();
    virtual ~UIManager();

    // 创建具体的UI面板（工厂方法）
    cocos2d::Node* createPanel(UIPanelType panelType);

    // 添加面板到场景
    void addPanelToScene(cocos2d::Node* panel, UILayer layer, bool modal);

    // 创建模态遮罩层（阻挡底层交互的半透明黑色层）
    cocos2d::LayerColor* createModalLayer();

    // 面板显示/隐藏动画
    void playShowAnimation(cocos2d::Node* panel);
    void playHideAnimation(cocos2d::Node* panel, const std::function<void()>& callback = nullptr);

private:
    static UIManager* _instance;

    cocos2d::Scene* _rootScene;              // 根场景引用
    cocos2d::Size _visibleSize;              // 可见区域大小
    cocos2d::Vec2 _visibleOrigin;            // 可见区域原点
    float _scaleFactor;                       // 缩放因子

    // 面板缓存（面板类型 -> 面板节点）
    std::map<UIPanelType, cocos2d::Node*> _panels;

    // 模态遮罩层缓存
    std::map<UIPanelType, cocos2d::LayerColor*> _modalLayers;

    // UI事件回调映射（事件名 -> 回调函数）
    std::map<std::string, std::function<void()>> _callbacks;

    // 当前正在显示的加载界面
    cocos2d::Node* _loadingPanel;
};

#endif // __UI_MANAGER_H__