#include "UIManager/UIManager.h"
#include "Building/Building.h"
#include "TownHall/TownHall.h"
#include "AudioManager/AudioManager.h"
#include <sstream>
#include "MapManager/MapManager.h"      
#include "Combat/Combat.h"       
#include "Scene/BattleScene.h"   
#include "MainScene.h"          

USING_NS_CC;
using namespace cocos2d::ui;

// ==================== 静态成员初始化 ====================
UIManager* UIManager::_instance = nullptr;

// ==================== 单例管理 ====================
UIManager* UIManager::getInstance() {
    if (_instance == nullptr) {
        _instance = new (std::nothrow) UIManager();
    }
    return _instance;
}

void UIManager::destroyInstance() {
    if (_instance) {
        delete _instance;
        _instance = nullptr;
    }
}

// ==================== 构造/析构 ====================
UIManager::UIManager()
    : _rootScene(nullptr)
    , _scaleFactor(1.0f)
    , _selectedBuilding(nullptr)
    , _goldLabel(nullptr)
    , _elixirLabel(nullptr)
    , _loadingProgressBar(nullptr){ }

UIManager::~UIManager() {
    closeAllPanels();
}

// ==================== 初始化 ====================
bool UIManager::init(Scene* rootScene) {
    if (!rootScene) {
        return false;
    }

    _rootScene = rootScene;
    _visibleSize = Director::getInstance()->getVisibleSize();
    _visibleOrigin = Director::getInstance()->getVisibleOrigin();

    // 计算缩放因子（基于设计分辨率1280x720）
    float designWidth = 1280.0f;
    float designHeight = 720.0f;
    _scaleFactor = std::min(_visibleSize.width / designWidth, _visibleSize.height / designHeight);

    return true;
}

// ==================== 面板管理 ====================
void UIManager::showPanel(UIPanelType panelType, UILayer layer, bool modal) {
    // 检查是否已存在
    auto it = _panels.find(panelType);
    if (it != _panels.end() && it->second) {
        it->second->setVisible(true);
        playShowAnimation(it->second);
        return;
    }

    // 创建新面板
    Node* panel = createPanel(panelType);
    if (panel) {
        _panels[panelType] = panel;
        addPanelToScene(panel, layer, modal);
        playShowAnimation(panel);
    }
}

void UIManager::hidePanel(UIPanelType panelType, bool removeFromParent) {
    auto it = _panels.find(panelType);
    if (it != _panels.end() && it->second) {
        if (removeFromParent) {
            playHideAnimation(it->second, [this, panelType, it]() {
                it->second->removeFromParent();
                _panels.erase(panelType);

                // 移除对应的模态层
                auto modalIt = _modalLayers.find(panelType);
                if (modalIt != _modalLayers.end()) {
                    modalIt->second->removeFromParent();
                    _modalLayers.erase(modalIt);
                }
                });
        }
        else {
            playHideAnimation(it->second, [it]() {
                it->second->setVisible(false);
                });
        }
    }
}

void UIManager::togglePanel(UIPanelType panelType) {
    if (isPanelVisible(panelType)) {
        hidePanel(panelType);
    }
    else {
        showPanel(panelType);
    }
}

bool UIManager::isPanelVisible(UIPanelType panelType) const {
    auto it = _panels.find(panelType);
    return it != _panels.end() && it->second && it->second->isVisible();
}

Node* UIManager::getPanel(UIPanelType panelType) const {
    auto it = _panels.find(panelType);
    return (it != _panels.end()) ? it->second : nullptr;
}

void UIManager::closeAllPanels() {
    for (auto& pair : _panels) {
        if (pair.second) {
            pair.second->removeFromParent();
        }
    }
    _panels.clear();

    for (auto& pair : _modalLayers) {
        if (pair.second) {
            pair.second->removeFromParent();
        }
    }
    _modalLayers.clear();
}

// ==================== 面板创建工厂 ====================
Node* UIManager::createPanel(UIPanelType panelType) {
    switch (panelType) {
        case UIPanelType::LoadingScreen:
            return createLoadingScreen();
        case UIPanelType::GameHUD:
            return createGameHUD();
        case UIPanelType::ResourceBar:
            return createResourceBar();
        case UIPanelType::Settings:
            return createSettings();
        case UIPanelType::Shop:
            return createShop();
        case UIPanelType::MapSelection:
            return createMapSelection();
        case UIPanelType::BattleHUD:
            return createBattleHUD();
        case UIPanelType::BattleResult:
            return createBattleResult();
        default:
            return nullptr;
    }
}

// ==================== 加载界面 ====================
Node* UIManager::createLoadingScreen() {
    auto panel = Node::create();
    panel->setContentSize(_visibleSize);
    panel->setPosition(_visibleOrigin);

    // 背景图片
    auto bg = Sprite::create("..../Resource/loading_bg.png"); 
    if (bg) {
        bg->setPosition(_visibleSize / 2);
        // 缩放背景图以覆盖整个屏幕
        float scaleX = _visibleSize.width / bg->getContentSize().width;
        float scaleY = _visibleSize.height / bg->getContentSize().height;
        bg->setScale(std::max(scaleX, scaleY));
        panel->addChild(bg, 0);
    }
    else {
        // 如果没有背景图，使用纯色背景
        auto colorBg = LayerColor::create(Color4B(30, 30, 50, 255), _visibleSize.width, _visibleSize.height);
        panel->addChild(colorBg, 0);
    }

    // 进度条背景
    auto progressBg = Sprite::create();
    progressBg->setTextureRect(Rect(0, 0, 400 * _scaleFactor, 30 * _scaleFactor));
    progressBg->setColor(Color3B(50, 50, 50));
    progressBg->setPosition(Vec2(_visibleSize.width / 2, _visibleSize.height * 0.25f));
    panel->addChild(progressBg, 1);

    // 进度条
    _loadingProgressBar = LoadingBar::create();
    _loadingProgressBar->setScale9Enabled(true);
    _loadingProgressBar->setCapInsets(Rect(0, 0, 1, 1));
    _loadingProgressBar->setContentSize(Size(396 * _scaleFactor, 26 * _scaleFactor));    
    _loadingProgressBar->setDirection(LoadingBar::Direction::LEFT);
    _loadingProgressBar->setPercent(0);
    _loadingProgressBar->setPosition(Vec2(_visibleSize.width / 2, _visibleSize.height * 0.25f));
    panel->addChild(_loadingProgressBar, 2);

    // 加载提示文字
    auto loadingLabel = Label::createWithTTF("Loading...", "fonts/arial.ttf", 24 * _scaleFactor);
    loadingLabel->setPosition(Vec2(_visibleSize.width / 2, _visibleSize.height * 0.18f));
    loadingLabel->setColor(Color3B::WHITE);
    loadingLabel->setName("loadingLabel");
    panel->addChild(loadingLabel, 1);

    return panel;
}

void UIManager::showLoadingScreen() {
    showPanel(UIPanelType::LoadingScreen, UILayer::Loading, true);
}

void UIManager::updateLoadingProgress(float progress) {
    if (_loadingProgressBar) {
        _loadingProgressBar->setPercent(progress * 100);
    }
}

void UIManager::hideLoadingScreen() {
    hidePanel(UIPanelType::LoadingScreen, true);
    // 加载完成后显示GameHUD
    showPanel(UIPanelType::GameHUD, UILayer::HUD, false);
}

// ==================== GameHUD ====================
Node* UIManager::createGameHUD() {
    auto panel = Node::create();
    panel->setContentSize(_visibleSize);
    panel->setPosition(_visibleOrigin);

    // ===== 顶部资源栏 =====
    auto resourceBar = createResourceBar();
    if (resourceBar) {
        panel->addChild(resourceBar, 1);
    }

    // ===== 右侧按钮栏 =====
    float buttonSize = 60 * _scaleFactor;
    float buttonMargin = 20 * _scaleFactor;
    float rightX = _visibleSize.width - buttonSize / 2 - buttonMargin;
    float startY = _visibleSize.height * 0.6f;

    // 设置按钮
    auto settingsBtn = Button::create("..../Resource/UI/btn_settings.png", "..../Resource/UI/btn_settings_pressed.png"); 
    if (!settingsBtn->getVirtualRenderer()) {
        settingsBtn = Button::create();
        settingsBtn->setTitleText("Settings");
        settingsBtn->setTitleFontSize(14 * _scaleFactor);
        settingsBtn->setContentSize(Size(buttonSize, buttonSize));
        settingsBtn->setScale9Enabled(true);
    }
    settingsBtn->setPosition(Vec2(rightX, startY));
    settingsBtn->addClickEventListener([this](Ref* sender) {showPanel(UIPanelType::Settings, UILayer::Dialog, true);});
    panel->addChild(settingsBtn, 1);

    // 商店按钮
    auto shopBtn = Button::create("..../Resource/UI/btn_shop.png", "..../Resource/UI/btn_shop_pressed.png"); 
    if (!shopBtn->getVirtualRenderer()) {
        shopBtn = Button::create();
        shopBtn->setTitleText("Shop");
        shopBtn->setTitleFontSize(14 * _scaleFactor);
        shopBtn->setContentSize(Size(buttonSize, buttonSize));
        shopBtn->setScale9Enabled(true);
    }
    shopBtn->setPosition(Vec2(rightX, startY - buttonSize - buttonMargin));
    shopBtn->addClickEventListener([this](Ref* sender) {showShop();});
    panel->addChild(shopBtn, 1);

    // 进攻按钮
    auto attackBtn = Button::create("..../Resource/UI/btn_attack.png", "..../Resource/UI/btn_attack_pressed.png"); 
    if (!attackBtn->getVirtualRenderer()) {
        attackBtn = Button::create();
        attackBtn->setTitleText("Attack");
        attackBtn->setTitleFontSize(14 * _scaleFactor);
        attackBtn->setContentSize(Size(buttonSize, buttonSize));
        attackBtn->setScale9Enabled(true);
    }
    attackBtn->setPosition(Vec2(rightX, startY - 2 * (buttonSize + buttonMargin)));
    attackBtn->addClickEventListener([this](Ref* sender) {showMapSelection();});
    panel->addChild(attackBtn, 1);

    return panel;
}

// ==================== 资源栏 ====================
Node* UIManager::createResourceBar() {
    auto bar = Node::create();

    // 尺寸定义
    float barWidth = 200 * _scaleFactor;      // 资源条固定宽度
    float barHeight = 28 * _scaleFactor;      // 资源条高度
    float iconSize = 40 * _scaleFactor;       // 图标大小
    float margin = 10 * _scaleFactor;         // 边距
    float rowSpacing = 8 * _scaleFactor;      // 行间距

    // 整体容器大小
    float totalWidth = barWidth + iconSize + margin;
    float totalHeight = barHeight * 2 + rowSpacing;
    bar->setContentSize(Size(totalWidth, totalHeight));

    // 位置：右上角
    bar->setPosition(Vec2(_visibleSize.width - totalWidth - margin,
        _visibleSize.height - totalHeight - margin));

    
    TownHall* townHall = GetTownHall(); // 需要实现获取方式
    int goldCapacity = townHall->GetMaxGoldCapacity();
    int currentGold = townHall->GetGold();
    int elixirCapacity = townHall->GetMaxElixirCapacity();
    int currentElixir = townHall->GetElixir();

    // ==================== 金币行（上方）====================
    float goldRowY = barHeight + rowSpacing;

    // 金币条背景（半透明黑色，固定长度）
    auto goldBg = LayerColor::create(Color4B(0, 0, 0, 180), barWidth, barHeight);
    goldBg->setPosition(Vec2(0, goldRowY));
    bar->addChild(goldBg, 0);

    // 金币实际资源填充（右对齐）
    float goldRatio = (goldCapacity > 0) ? std::min(1.0f, (float)currentGold / goldCapacity) : 0;
    float goldFillWidth = barWidth * goldRatio;
    auto goldFill = LayerColor::create(Color4B(255, 215, 0, 200), goldFillWidth, barHeight - 4 * _scaleFactor);
    // 右对齐：从右侧开始填充
    goldFill->setPosition(Vec2(barWidth - goldFillWidth, goldRowY + 2 * _scaleFactor));
    bar->addChild(goldFill, 1);
    goldFill->setName("goldFill");

    // 金币数量文字（覆盖在资源条上，右对齐）
    _goldLabel = Label::createWithTTF(std::to_string(currentGold), "fonts/arial.ttf", 18 * _scaleFactor);
    _goldLabel->setPosition(Vec2(barWidth - 10 * _scaleFactor, goldRowY + barHeight / 2));
    _goldLabel->setAnchorPoint(Vec2(1.0f, 0.5f)); // 右对齐
    _goldLabel->setColor(Color3B::WHITE);
    _goldLabel->enableOutline(Color4B::BLACK, 2);
    bar->addChild(_goldLabel, 2);

    // 金币图标（在资源条右侧外部）
    auto goldIcon = Sprite::create("..../Resource/UI/icon_gold.png");
    if (goldIcon) {
        goldIcon->setScale(iconSize / goldIcon->getContentSize().width);
    }
    goldIcon->setPosition(Vec2(barWidth + margin + iconSize / 2, goldRowY + barHeight / 2));
    bar->addChild(goldIcon, 1);

    // ==================== 圣水行（下方）====================
    float elixirRowY = 0;

    // 圣水条背景（半透明黑色，固定长度）
    auto elixirBg = LayerColor::create(Color4B(0, 0, 0, 180), barWidth, barHeight);
    elixirBg->setPosition(Vec2(0, elixirRowY));
    bar->addChild(elixirBg, 0);

    // 圣水实际资源填充（右对齐）
    float elixirRatio = (elixirCapacity > 0) ? std::min(1.0f, (float)currentElixir / elixirCapacity) : 0;
    float elixirFillWidth = barWidth * elixirRatio;
    auto elixirFill = LayerColor::create(Color4B(200, 100, 255, 200), elixirFillWidth, barHeight - 4 * _scaleFactor);
    // 右对齐：从右侧开始填充
    elixirFill->setPosition(Vec2(barWidth - elixirFillWidth, elixirRowY + 2 * _scaleFactor));
    bar->addChild(elixirFill, 1);
    elixirFill->setName("elixirFill");

    // 圣水数量文字（覆盖在资源条上，右对齐）
    _elixirLabel = Label::createWithTTF(std::to_string(currentElixir), "fonts/arial.ttf", 18 * _scaleFactor);
    _elixirLabel->setPosition(Vec2(barWidth - 10 * _scaleFactor, elixirRowY + barHeight / 2));
    _elixirLabel->setAnchorPoint(Vec2(1.0f, 0.5f)); // 右对齐
    _elixirLabel->setColor(Color3B::WHITE);
    _elixirLabel->enableOutline(Color4B::BLACK, 2);
    bar->addChild(_elixirLabel, 2);

    // 圣水图标（在资源条右侧外部）
    auto elixirIcon = Sprite::create("..../Resource/UI/icon_elixir.png");
    if (elixirIcon) {
        elixirIcon->setScale(iconSize / elixirIcon->getContentSize().width);
    }
    elixirIcon->setPosition(Vec2(barWidth + margin + iconSize / 2, elixirRowY + barHeight / 2));
    bar->addChild(elixirIcon, 1);

    return bar;
}

void UIManager::updateResourceDisplay(ResourceType resourceType, int amount) {
    switch (resourceType) {
        case ResourceType::Gold:
            if (_goldLabel) {
                _goldLabel->setString(std::to_string(amount));
            }
            break;
        case ResourceType::Elixir:
            if (_elixirLabel) {
                _elixirLabel->setString(std::to_string(amount));
            }
            break;
    }
}

// ==================== 设置面板 ====================
Node* UIManager::createSettings() {
    auto panel = Node::create();

    // 面板大小
    Size panelSize(400 * _scaleFactor, 300 * _scaleFactor);
    panel->setContentSize(panelSize);
    panel->setPosition(Vec2((_visibleSize.width - panelSize.width) / 2 + _visibleOrigin.x,
        (_visibleSize.height - panelSize.height) / 2 + _visibleOrigin.y));

    // 背景
    auto bg = LayerColor::create(Color4B(40, 40, 60, 240), panelSize.width, panelSize.height);
    panel->addChild(bg, 0);

    // 边框
    auto border = DrawNode::create();
    border->drawRect(Vec2(0, 0), Vec2(panelSize.width, panelSize.height), Color4F::WHITE);
    panel->addChild(border, 1);

    // 标题
    auto title = Label::createWithTTF("Settings", "fonts/arial.ttf", 28 * _scaleFactor);
    title->setPosition(Vec2(panelSize.width / 2, panelSize.height - 30 * _scaleFactor));
    title->setColor(Color3B::WHITE);
    panel->addChild(title, 1);

    // 关闭按钮
    auto closeBtn = createCloseButton([this]() {
        hidePanel(UIPanelType::Settings, true);
        });
    closeBtn->setPosition(Vec2(panelSize.width - 25 * _scaleFactor, panelSize.height - 25 * _scaleFactor));
    panel->addChild(closeBtn, 2);

	AudioManager* audioManager = AudioManager::getInstance();

    // 音乐音量滑块
    float sliderY = panelSize.height - 100 * _scaleFactor;
    auto musicLabel = Label::createWithTTF("Music Volume", "fonts/arial.ttf", 18 * _scaleFactor);
    musicLabel->setPosition(Vec2(30 * _scaleFactor, sliderY));
    musicLabel->setAnchorPoint(Vec2(0, 0.5f));
    musicLabel->setColor(Color3B::WHITE);
    panel->addChild(musicLabel, 1);

    auto musicSlider = Slider::create();
    musicSlider->loadBarTexture("..../Resource/UI/slider_bg.png"); // 这里需要替换为实际滑块背景图片
    musicSlider->loadProgressBarTexture("..../Resource/UI/slider_progress.png"); // 这里需要替换为实际滑块进度图片
    musicSlider->loadSlidBallTextures("..../Resource/UI/slider_ball.png"); // 这里需要替换为实际滑块按钮图片
    musicSlider->setPosition(Vec2(panelSize.width - 120 * _scaleFactor, sliderY));
    musicSlider->setPercent(audioManager->getMusicVolume() * 100);
    musicSlider->addEventListener([](Ref* sender, Slider::EventType type) {
        if (type == Slider::EventType::ON_PERCENTAGE_CHANGED) {
            auto slider = dynamic_cast<Slider*>(sender);
            float volume = slider->getPercent() / 100.0f;
            AudioManager::getInstance()->setMusicVolume(volume);
            CCLOG("Music volume: %.2f", volume);
        }
        });
    panel->addChild(musicSlider, 1);

    // 音效音量滑块
    float sfxSliderY = sliderY - 60 * _scaleFactor;
    auto sfxLabel = Label::createWithTTF("SFX Volume", "fonts/arial.ttf", 18 * _scaleFactor);
    sfxLabel->setPosition(Vec2(30 * _scaleFactor, sfxSliderY));
    sfxLabel->setAnchorPoint(Vec2(0, 0.5f));
    sfxLabel->setColor(Color3B::WHITE);
    panel->addChild(sfxLabel, 1);

    auto sfxSlider = Slider::create();
    sfxSlider->loadBarTexture("..../Resource/UI/slider_bg.png"); // 这里需要替换为实际滑块背景图片
    sfxSlider->loadProgressBarTexture("..../Resource/UI/slider_progress.png"); // 这里需要替换为实际滑块进度图片
    sfxSlider->loadSlidBallTextures("..../Resource/UI/slider_ball.png"); // 这里需要替换为实际滑块按钮图片
    sfxSlider->setPosition(Vec2(panelSize.width - 120 * _scaleFactor, sfxSliderY));
    sfxSlider->setPercent(audioManager->getSoundEffectVolume() * 100);
    sfxSlider->addEventListener([](Ref* sender, Slider::EventType type) {
        if (type == Slider::EventType::ON_PERCENTAGE_CHANGED) {
            auto slider = dynamic_cast<Slider*>(sender);
            float volume = slider->getPercent() / 100.0f;
            AudioManager::getInstance()->setSoundEffectVolume(volume);
            CCLOG("SFX volume: %.2f", volume);
        }
        });
    panel->addChild(sfxSlider, 1);

    return panel;
}

// ==================== 商店面板 ====================
Node* UIManager::createShop() {
    auto panel = Node::create();

    // 面板大小
    Size panelSize(600 * _scaleFactor, 450 * _scaleFactor);
    panel->setContentSize(panelSize);
    panel->setPosition(Vec2((_visibleSize.width - panelSize.width) / 2 + _visibleOrigin.x,
        (_visibleSize.height - panelSize.height) / 2 + _visibleOrigin.y));

    // 背景
    auto bg = LayerColor::create(Color4B(40, 40, 60, 240), panelSize.width, panelSize.height);
    panel->addChild(bg, 0);

    // 边框
    auto border = DrawNode::create();
    border->drawRect(Vec2(0, 0), Vec2(panelSize.width, panelSize.height), Color4F::WHITE);
    panel->addChild(border, 1);

    // 标题
    auto title = Label::createWithTTF("Shop - Buildings", "fonts/arial.ttf", 28 * _scaleFactor);
    title->setPosition(Vec2(panelSize.width / 2, panelSize.height - 30 * _scaleFactor));
    title->setColor(Color3B::WHITE);
    panel->addChild(title, 1);

    // 关闭按钮
    auto closeBtn = createCloseButton([this]() {
        hidePanel(UIPanelType::Shop, true);
        });
    closeBtn->setPosition(Vec2(panelSize.width - 25 * _scaleFactor, panelSize.height - 25 * _scaleFactor));
    panel->addChild(closeBtn, 2);

    // 滚动视图
    auto scrollView = ScrollView::create();
    scrollView->setDirection(ScrollView::Direction::VERTICAL);
    scrollView->setBounceEnabled(true);
    scrollView->setContentSize(Size(panelSize.width - 40 * _scaleFactor, panelSize.height - 80 * _scaleFactor));
    scrollView->setPosition(Vec2(20 * _scaleFactor, 20 * _scaleFactor));
    panel->addChild(scrollView, 1);

    // 从 Building 获取所有建筑模板
    auto buildingTemplates = Building::GetAllBuildingTemplates();

    float itemHeight = 80 * _scaleFactor;
    float itemWidth = scrollView->getContentSize().width;
    float totalHeight = buildingTemplates.size() * itemHeight;

    auto container = Node::create();
    container->setContentSize(Size(itemWidth, std::max(totalHeight, scrollView->getContentSize().height)));
    scrollView->setInnerContainerSize(container->getContentSize());
    scrollView->addChild(container);

    for (size_t i = 0; i < buildingTemplates.size(); i++) {
        const auto& tmpl = buildingTemplates[i];
        float itemY = container->getContentSize().height - (i + 1) * itemHeight + itemHeight / 2;

        // 建筑项背景
        auto itemBg = LayerColor::create(Color4B(60, 60, 80, 200), itemWidth - 10 * _scaleFactor, itemHeight - 5 * _scaleFactor);
        itemBg->setPosition(Vec2(5 * _scaleFactor, itemY - itemHeight / 2 + 2.5f * _scaleFactor));
        container->addChild(itemBg, 0);

        // 建筑图标
        auto icon = Sprite::create(tmpl.iconPath);
        if (!icon) {
            icon = Sprite::create();
            icon->setTextureRect(Rect(0, 0, 50 * _scaleFactor, 50 * _scaleFactor));
            icon->setColor(Color3B(100, 100, 100));
        }
        icon->setPosition(Vec2(50 * _scaleFactor, itemY));
        icon->setScale(50 * _scaleFactor / std::max(icon->getContentSize().width, icon->getContentSize().height));
        container->addChild(icon, 1);

        // 建筑名称
        auto nameLabel = Label::createWithTTF(tmpl.name, "fonts/arial.ttf", 18 * _scaleFactor);
        nameLabel->setPosition(Vec2(100 * _scaleFactor, itemY + 10 * _scaleFactor));
        nameLabel->setAnchorPoint(Vec2(0, 0.5f));
        nameLabel->setColor(Color3B::WHITE);
        container->addChild(nameLabel, 1);

        // 建造成本
        auto costLabel = Label::createWithTTF("Cost: " + std::to_string(tmpl.cost), "fonts/arial.ttf", 14 * _scaleFactor);
        costLabel->setPosition(Vec2(100 * _scaleFactor, itemY - 10 * _scaleFactor));
        costLabel->setAnchorPoint(Vec2(0, 0.5f));
        costLabel->setColor(Color3B(255, 215, 0));
        container->addChild(costLabel, 1);

        // 购买按钮
        auto buyBtn = Button::create();
        buyBtn->setTitleText("Buy");
        buyBtn->setTitleFontSize(16 * _scaleFactor);
        buyBtn->setContentSize(Size(80 * _scaleFactor, 40 * _scaleFactor));
        buyBtn->setScale9Enabled(true);
        buyBtn->setPosition(Vec2(itemWidth - 60 * _scaleFactor, itemY));

        // 捕获模板信息（使用值拷贝）
        BuildingTemplate templateCopy = tmpl;
        buyBtn->addClickEventListener([this, templateCopy](Ref* sender) {
            // 检查金币是否足够
            TownHall* townHall = GetTownHall();
            int currentGold = townHall->GetGold();

            if (currentGold >= templateCopy.cost) {
                hidePanel(UIPanelType::Shop, true);

                // 使用工厂函数创建建筑实例
                Building* newBuilding = templateCopy.createFunc();
                if (newBuilding) {
                    // 保存待放置信息
                    _pendingPlacementBuilding = newBuilding;
                    _pendingPlacementCost = templateCopy.cost;

                    showToast("Drag to place " + templateCopy.name);
                    triggerUIEvent("OnEnterPlacementMode");
                }
            }
            else {
                showInfoDialog("Insufficient Gold",
                    "You need " + std::to_string(templateCopy.cost) + " gold to build " + templateCopy.name);
            }
            });
        container->addChild(buyBtn, 1);
    }
    return panel;
}

void UIManager::showShop() {
    showPanel(UIPanelType::Shop, UILayer::Dialog, true);
}

// ==================== 地图选择面板 ====================
Node* UIManager::createMapSelection() {
    auto panel = Node::create();

    // 面板大小
    Size panelSize(700 * _scaleFactor, 500 * _scaleFactor);
    panel->setContentSize(panelSize);
    panel->setPosition(Vec2((_visibleSize.width - panelSize.width) / 2 + _visibleOrigin.x,
        (_visibleSize.height - panelSize.height) / 2 + _visibleOrigin.y));

    // 背景（地图图片）
    auto bg = Sprite::create("..../Resource/UI/map_selection_bg.png"); // 这里需要替换为实际的地图选择背景图
    if (bg) {
        bg->setPosition(Vec2(panelSize.width / 2, panelSize.height / 2));
        float scaleX = panelSize.width / bg->getContentSize().width;
        float scaleY = panelSize.height / bg->getContentSize().height;
        bg->setScale(std::max(scaleX, scaleY));
        panel->addChild(bg, 0);
    }
    else {
        auto colorBg = LayerColor::create(Color4B(40, 60, 40, 240), panelSize.width, panelSize.height);
        panel->addChild(colorBg, 0);
    }

    // 边框
    auto border = DrawNode::create();
    border->drawRect(Vec2(0, 0), Vec2(panelSize.width, panelSize.height), Color4F::WHITE);
    panel->addChild(border, 1);

    // 标题
    auto title = Label::createWithTTF("Select Battle Map", "fonts/arial.ttf", 28 * _scaleFactor);
    title->setPosition(Vec2(panelSize.width / 2, panelSize.height - 30 * _scaleFactor));
    title->setColor(Color3B::WHITE);
    title->enableOutline(Color4B::BLACK, 2);
    panel->addChild(title, 1);

    // 关闭按钮
    auto closeBtn = createCloseButton([this]() {
        hidePanel(UIPanelType::MapSelection, true);
        });
    closeBtn->setPosition(Vec2(panelSize.width - 25 * _scaleFactor, panelSize.height - 25 * _scaleFactor));
    panel->addChild(closeBtn, 2);

    // 绘制连接线
    auto pathLine = DrawNode::create();
    Vec2 node1Pos(panelSize.width * 0.3f, panelSize.height * 0.5f);
    Vec2 node2Pos(panelSize.width * 0.7f, panelSize.height * 0.5f);
    pathLine->drawLine(node1Pos, node2Pos, Color4F(1, 1, 1, 0.5f));
    panel->addChild(pathLine, 1);

    // 地图节点1
    auto node1 = Button::create("..../Resource/UI/map_node.png", "..../Resource/UI/map_node_pressed.png"); // 这里需要替换为实际节点图片
    if (!node1->getVirtualRenderer()) {
        node1 = Button::create();
        auto circle1 = DrawNode::create();
        circle1->drawSolidCircle(Vec2::ZERO, 40 * _scaleFactor, 0, 32, Color4F(0.2f, 0.6f, 0.2f, 1));
        circle1->drawCircle(Vec2::ZERO, 40 * _scaleFactor, 0, 32, false, Color4F::WHITE);
        node1->addChild(circle1, -1);
        node1->setContentSize(Size(80 * _scaleFactor, 80 * _scaleFactor));
    }
    node1->setPosition(node1Pos);
    node1->addClickEventListener([this](Ref* sender) {
        playMapSelectAnimation(0, [this]() {
            hidePanel(UIPanelType::MapSelection, true);
            showBattleLoading("Loading Battle Map 1...");
            // 这里需要替换为加载战斗场景的逻辑
            triggerUIEvent("OnBattleStart_Map1");
            });
        });
    panel->addChild(node1, 2);

    auto node1Label = Label::createWithTTF("Map 1", "fonts/arial.ttf", 16 * _scaleFactor);
    node1Label->setPosition(Vec2(node1Pos.x, node1Pos.y - 55 * _scaleFactor));
    node1Label->setColor(Color3B::WHITE);
    panel->addChild(node1Label, 1);

    // 地图节点2
    auto node2 = Button::create("..../Resource/UI/map_node.png", "..../Resource/UI/map_node_pressed.png"); // 这里需要替换为实际节点图片
    if (!node2->getVirtualRenderer()) {
        node2 = Button::create();
        auto circle2 = DrawNode::create();
        circle2->drawSolidCircle(Vec2::ZERO, 40 * _scaleFactor, 0, 32, Color4F(0.6f, 0.2f, 0.2f, 1));
        circle2->drawCircle(Vec2::ZERO, 40 * _scaleFactor, 0, 32, false, Color4F::WHITE);
        node2->addChild(circle2, -1);
        node2->setContentSize(Size(80 * _scaleFactor, 80 * _scaleFactor));
    }
    node2->setPosition(node2Pos);
    node2->addClickEventListener([this](Ref* sender) {
        playMapSelectAnimation(1, [this]() {
            hidePanel(UIPanelType::MapSelection, true);
            showBattleLoading("Loading Battle Map 2...");
            // 这里需要替换为加载战斗场景的逻辑
            triggerUIEvent("OnBattleStart_Map2");
            });
        });
    panel->addChild(node2, 2);

    auto node2Label = Label::createWithTTF("Map 2", "fonts/arial.ttf", 16 * _scaleFactor);
    node2Label->setPosition(Vec2(node2Pos.x, node2Pos.y - 55 * _scaleFactor));
    node2Label->setColor(Color3B::WHITE);
    panel->addChild(node2Label, 1);

    return panel;
}

void UIManager::showMapSelection() {
    showPanel(UIPanelType::MapSelection, UILayer::Dialog, true);
}

void UIManager::playMapSelectAnimation(int mapIndex, const std::function<void()>& onComplete) {
    auto panel = getPanel(UIPanelType::MapSelection);
    if (!panel) {
        if (onComplete) onComplete();
        return;
    }

    // 创建选中效果动画
    auto flash = LayerColor::create(Color4B(255, 255, 255, 0), panel->getContentSize().width, panel->getContentSize().height);
    panel->addChild(flash, 100);

    auto fadeIn = FadeTo::create(0.2f, 200);
    auto fadeOut = FadeTo::create(0.3f, 0);
    auto delay = DelayTime::create(0.1f);
    auto callFunc = CallFunc::create([onComplete]() {
        if (onComplete) onComplete();
        });
    auto removeSelf = RemoveSelf::create();

    flash->runAction(Sequence::create(fadeIn, fadeOut, delay, callFunc, removeSelf, nullptr));
}

// ==================== 建筑操作面板 ====================
void UIManager::showBuildingOptions(const Vec2& position, BuildingCategory category, Building* building) {
    _selectedBuilding = building;

    // 先隐藏之前的BuildingOptions
    hidePanel(UIPanelType::BuildingOptions, true);

    auto panel = createBuildingOptions(position, category);
    if (panel) {
        _panels[UIPanelType::BuildingOptions] = panel;
        addPanelToScene(panel, UILayer::HUD, false);
        playShowAnimation(panel);
    }
}

Node* UIManager::createBuildingOptions(const Vec2& position, BuildingCategory category) {
    auto panel = Node::create();

    int buttonCount = (category == BuildingCategory::Military) ? 3 : 2;
    float buttonSize = 50 * _scaleFactor;
    float buttonMargin = 10 * _scaleFactor;
    float panelWidth = buttonCount * buttonSize + (buttonCount + 1) * buttonMargin;
    float panelHeight = buttonSize + 2 * buttonMargin;

    panel->setContentSize(Size(panelWidth, panelHeight));

    // 调整位置使面板在建筑下方居中
    Vec2 adjustedPos = position;
    adjustedPos.x -= panelWidth / 2;
    adjustedPos.y -= panelHeight + 20 * _scaleFactor;

    // 确保不超出屏幕边界
    adjustedPos.x = std::max(_visibleOrigin.x, std::min(adjustedPos.x, _visibleOrigin.x + _visibleSize.width - panelWidth));
    adjustedPos.y = std::max(_visibleOrigin.y, adjustedPos.y);

    panel->setPosition(adjustedPos);

    // 背景
    auto bg = LayerColor::create(Color4B(50, 50, 70, 220), panelWidth, panelHeight);
    panel->addChild(bg, 0);

    // 边框
    auto border = DrawNode::create();
    border->drawRect(Vec2(0, 0), Vec2(panelWidth, panelHeight), Color4F::WHITE);
    panel->addChild(border, 1);

    float currentX = buttonMargin + buttonSize / 2;
    float centerY = panelHeight / 2;

    // 信息按钮
    auto infoBtn = Button::create("..../Resource/UI/btn_info.png", "..../Resource/UI/btn_info_pressed.png"); // 这里需要替换为实际按钮图片
    if (!infoBtn->getVirtualRenderer()) {
        infoBtn = Button::create();
        infoBtn->setTitleText("Info");
        infoBtn->setTitleFontSize(12 * _scaleFactor);
        infoBtn->setContentSize(Size(buttonSize, buttonSize));
        infoBtn->setScale9Enabled(true);
    }
    infoBtn->setPosition(Vec2(currentX, centerY));
    infoBtn->addClickEventListener([this](Ref* sender) {
        hidePanel(UIPanelType::BuildingOptions, true);
        showBuildingInfo(_selectedBuilding);
        });
    panel->addChild(infoBtn, 1);

    currentX += buttonSize + buttonMargin;

    // 升级按钮
    auto upgradeBtn = Button::create("..../Resource/UI/btn_upgrade.png", "..../Resource/UI/btn_upgrade_pressed.png");
    if (!upgradeBtn->getVirtualRenderer()) {
        upgradeBtn = Button::create();
        upgradeBtn->setTitleText("Up");
        upgradeBtn->setTitleFontSize(12 * _scaleFactor);
        upgradeBtn->setContentSize(Size(buttonSize, buttonSize));
        upgradeBtn->setScale9Enabled(true);
    }
    upgradeBtn->setPosition(Vec2(currentX, centerY));

    // 检查是否允许升级
    bool canUpgrade = _selectedBuilding && _selectedBuilding->IsAllowedUpgrade();
    if (canUpgrade) {
        upgradeBtn->setEnabled(true);
        upgradeBtn->setColor(Color3B::WHITE);
        upgradeBtn->addClickEventListener([this](Ref* sender) {
            hidePanel(UIPanelType::BuildingOptions, true);
            showBuildingUpgrade(_selectedBuilding);
            });
    }
    else {
        // 已达最大等级，按钮变灰且不可点击
        upgradeBtn->setEnabled(false);
        upgradeBtn->setColor(Color3B(100, 100, 100));
        upgradeBtn->setTitleColor(Color3B::GRAY);
    }
    panel->addChild(upgradeBtn, 1);

    currentX += buttonSize + buttonMargin;

    // 训练按钮（仅军营）
    if (category == BuildingCategory::Military) {
        auto trainBtn = Button::create("..../Resource/UI/btn_train.png", "..../Resource/UI/btn_train_pressed.png"); // 这里需要替换为实际按钮图片
        if (!trainBtn->getVirtualRenderer()) {
            trainBtn = Button::create();
            trainBtn->setTitleText("Train");
            trainBtn->setTitleFontSize(12 * _scaleFactor);
            trainBtn->setContentSize(Size(buttonSize, buttonSize));
            trainBtn->setScale9Enabled(true);
        }
        trainBtn->setPosition(Vec2(currentX, centerY));
        trainBtn->addClickEventListener([this](Ref* sender) {
            hidePanel(UIPanelType::BuildingOptions, true);
            showArmyTraining(_selectedBuilding);
            });
        panel->addChild(trainBtn, 1);
    }

    // 点击面板外部关闭
    auto touchListener = EventListenerTouchOneByOne::create();
    touchListener->setSwallowTouches(true);
    touchListener->onTouchBegan = [this, panel](Touch* touch, Event* event) -> bool {
        Vec2 locationInNode = panel->convertToNodeSpace(touch->getLocation());
        Rect rect(Vec2::ZERO, panel->getContentSize());
        if (!rect.containsPoint(locationInNode)) {
            hidePanel(UIPanelType::BuildingOptions, true);
            return true;
        }
        return false;
        };
    Director::getInstance()->getEventDispatcher()->addEventListenerWithSceneGraphPriority(touchListener, panel);

    return panel;
}

// ==================== 建筑信息面板 ====================
void UIManager::showBuildingInfo(Building* building) {
    _selectedBuilding = building;

    hidePanel(UIPanelType::BuildingInfo, true);

    auto panel = createBuildingInfo(building);
    if (panel) {
        _panels[UIPanelType::BuildingInfo] = panel;
        addPanelToScene(panel, UILayer::Dialog, true);
        playShowAnimation(panel);
    }
}

Node* UIManager::createBuildingInfo(Building* building) {
    auto panel = Node::create();

    Size panelSize(500 * _scaleFactor, 350 * _scaleFactor);
    panel->setContentSize(panelSize);
    panel->setPosition(Vec2((_visibleSize.width - panelSize.width) / 2 + _visibleOrigin.x,
        (_visibleSize.height - panelSize.height) / 2 + _visibleOrigin.y));

    // 背景
    auto bg = LayerColor::create(Color4B(40, 40, 60, 240), panelSize.width, panelSize.height);
    panel->addChild(bg, 0);

    // 边框
    auto border = DrawNode::create();
    border->drawRect(Vec2(0, 0), Vec2(panelSize.width, panelSize.height), Color4F::WHITE);
    panel->addChild(border, 1);

    // 标题
    std::string buildingName = building->getName();
    auto title = Label::createWithTTF(buildingName + " Info", "fonts/arial.ttf", 24 * _scaleFactor);
    title->setPosition(Vec2(panelSize.width / 2, panelSize.height - 30 * _scaleFactor));
    title->setColor(Color3B::WHITE);
    panel->addChild(title, 1);

    // 关闭按钮
    auto closeBtn = createCloseButton([this]() {
        hidePanel(UIPanelType::BuildingInfo, true);
        });
    closeBtn->setPosition(Vec2(panelSize.width - 25 * _scaleFactor, panelSize.height - 25 * _scaleFactor));
    panel->addChild(closeBtn, 2);

    // 左侧：建筑图像
    float leftWidth = panelSize.width * 0.4f;
    auto buildingSprite = Sprite::create(building->GetPicture()); 
    if (!buildingSprite) {
        buildingSprite = Sprite::create();
        buildingSprite->setTextureRect(Rect(0, 0, 100 * _scaleFactor, 100 * _scaleFactor));
        buildingSprite->setColor(Color3B(80, 80, 80));
    }
    buildingSprite->setPosition(Vec2(leftWidth / 2, panelSize.height / 2));
    float spriteScale = (leftWidth - 40 * _scaleFactor) / std::max(buildingSprite->getContentSize().width, buildingSprite->getContentSize().height);
    buildingSprite->setScale(spriteScale);
    panel->addChild(buildingSprite, 1);

    // 右侧：属性列表
    float rightX = leftWidth + 20 * _scaleFactor;
    float lineHeight = 30 * _scaleFactor;
    float startY = panelSize.height - 80 * _scaleFactor;

    // 获取建筑属性 - 这里需要替换为实际的building方法调用
    int level = building->GetLevel();
    int health = building->GetHealth();
    int maxHealth = building->GetMaxHealth();
    int defense = building->GetDefense();

    auto levelLabel = Label::createWithTTF("Level: " + std::to_string(level), "fonts/arial.ttf", 18 * _scaleFactor);
    levelLabel->setPosition(Vec2(rightX, startY));
    levelLabel->setAnchorPoint(Vec2(0, 0.5f));
    levelLabel->setColor(Color3B::WHITE);
    panel->addChild(levelLabel, 1);

    auto healthLabel = Label::createWithTTF("Health: " + std::to_string(health) + "/" + std::to_string(maxHealth), "fonts/arial.ttf", 18 * _scaleFactor);
    healthLabel->setPosition(Vec2(rightX, startY - lineHeight));
    healthLabel->setAnchorPoint(Vec2(0, 0.5f));
    healthLabel->setColor(Color3B(100, 255, 100));
    panel->addChild(healthLabel, 1);

    auto defenseLabel = Label::createWithTTF("Defense: " + std::to_string(defense), "fonts/arial.ttf", 18 * _scaleFactor);
    defenseLabel->setPosition(Vec2(rightX, startY - 2 * lineHeight));
    defenseLabel->setAnchorPoint(Vec2(0, 0.5f));
    defenseLabel->setColor(Color3B(100, 100, 255));
    panel->addChild(defenseLabel, 1);

    // 这里可以根据建筑类型添加更多属性
    // 例如资源建筑的容量、生产速度
    // 攻击建筑的伤害、攻击范围等
    // 需要根据 BuildingCategory 或建筑子类来判断

    return panel;
}

// ==================== 建筑升级面板 ====================
void UIManager::showBuildingUpgrade(Building* building) {
    _selectedBuilding = building;

    hidePanel(UIPanelType::BuildingUpgrade, true);

    auto panel = createBuildingUpgrade(building);
    if (panel) {
        _panels[UIPanelType::BuildingUpgrade] = panel;
        addPanelToScene(panel, UILayer::Dialog, true);
        playShowAnimation(panel);
    }
}

Node* UIManager::createBuildingUpgrade(Building* building) {
    auto panel = Node::create();

    Size panelSize(500 * _scaleFactor, 400 * _scaleFactor);
    panel->setContentSize(panelSize);
    panel->setPosition(Vec2((_visibleSize.width - panelSize.width) / 2 + _visibleOrigin.x,
        (_visibleSize.height - panelSize.height) / 2 + _visibleOrigin.y));

    // 背景
    auto bg = LayerColor::create(Color4B(40, 40, 60, 240), panelSize.width, panelSize.height);
    panel->addChild(bg, 0);

    // 边框
    auto border = DrawNode::create();
    border->drawRect(Vec2(0, 0), Vec2(panelSize.width, panelSize.height), Color4F::WHITE);
    panel->addChild(border, 1);

    // 标题
    std::string buildingName = building->getName();
    auto title = Label::createWithTTF(buildingName + " Upgrade", "fonts/arial.ttf", 24 * _scaleFactor);
    title->setPosition(Vec2(panelSize.width / 2, panelSize.height - 30 * _scaleFactor));
    title->setColor(Color3B::WHITE);
    panel->addChild(title, 1);

    // 关闭按钮
    auto closeBtn = createCloseButton([this]() {
        hidePanel(UIPanelType::BuildingUpgrade, true);
        });
    closeBtn->setPosition(Vec2(panelSize.width - 25 * _scaleFactor, panelSize.height - 25 * _scaleFactor));
    panel->addChild(closeBtn, 2);

    // 左侧：建筑图像
    float leftWidth = panelSize.width * 0.4f;
    auto buildingSprite = Sprite::create(building->GetPicture()); 
    if (!buildingSprite) {
        buildingSprite = Sprite::create();
        buildingSprite->setTextureRect(Rect(0, 0, 100 * _scaleFactor, 100 * _scaleFactor));
        buildingSprite->setColor(Color3B(80, 80, 80));
    }
    buildingSprite->setPosition(Vec2(leftWidth / 2, panelSize.height / 2 + 30 * _scaleFactor));
    float spriteScale = (leftWidth - 40 * _scaleFactor) / std::max(buildingSprite->getContentSize().width, buildingSprite->getContentSize().height);
    buildingSprite->setScale(spriteScale);
    panel->addChild(buildingSprite, 1);

    // 右侧：属性对比
    float rightX = leftWidth + 20 * _scaleFactor;
    float lineHeight = 28 * _scaleFactor;
    float startY = panelSize.height - 80 * _scaleFactor;

    // 获取当前和升级后属性 - 这里需要替换为实际的building方法调用
    int currentLevel = building->GetLevel();
    int nextLevel = currentLevel + 1;
    int currentHealth = building->GetMaxHealth();
    int nextHealth = building->GetNextMaxHealth();
    int currentDefense = building->GetDefense();
    int nextDefense = building->GetNextDefence();
    int upgradeCost = building->GetBuildCost();
    int upgradeTime = building->GetBuildTime();

    auto levelLabel = Label::createWithTTF("Level: " + std::to_string(currentLevel) + " -> " + std::to_string(nextLevel), "fonts/arial.ttf", 18 * _scaleFactor);
    levelLabel->setPosition(Vec2(rightX, startY));
    levelLabel->setAnchorPoint(Vec2(0, 0.5f));
    levelLabel->setColor(Color3B::WHITE);
    panel->addChild(levelLabel, 1);

    auto healthLabel = Label::createWithTTF("Health: " + std::to_string(currentHealth) + " -> " + std::to_string(nextHealth), "fonts/arial.ttf", 18 * _scaleFactor);
    healthLabel->setPosition(Vec2(rightX, startY - lineHeight));
    healthLabel->setAnchorPoint(Vec2(0, 0.5f));
    healthLabel->setColor(Color3B(100, 255, 100));
    panel->addChild(healthLabel, 1);

    auto defenseLabel = Label::createWithTTF("Defense: " + std::to_string(currentDefense) + " -> " + std::to_string(nextDefense), "fonts/arial.ttf", 18 * _scaleFactor);
    defenseLabel->setPosition(Vec2(rightX, startY - 2 * lineHeight));
    defenseLabel->setAnchorPoint(Vec2(0, 0.5f));
    defenseLabel->setColor(Color3B(100, 100, 255));
    panel->addChild(defenseLabel, 1);

    // 升级成本
    auto costLabel = Label::createWithTTF("Cost: " + std::to_string(upgradeCost) + " Elixir", "fonts/arial.ttf", 18 * _scaleFactor);
    costLabel->setPosition(Vec2(rightX, startY - 4 * lineHeight));
    costLabel->setAnchorPoint(Vec2(0, 0.5f));
    costLabel->setColor(Color3B(200, 100, 255)); 
    panel->addChild(costLabel, 1);

    // 升级时间
    int minutes = upgradeTime / 60;
    int seconds = upgradeTime % 60;
    std::string timeStr = std::to_string(minutes) + "m " + std::to_string(seconds) + "s";
    auto timeLabel = Label::createWithTTF("Time: " + timeStr, "fonts/arial.ttf", 18 * _scaleFactor);
    timeLabel->setPosition(Vec2(rightX, startY - 5 * lineHeight));
    timeLabel->setAnchorPoint(Vec2(0, 0.5f));
    timeLabel->setColor(Color3B(200, 200, 200));
    panel->addChild(timeLabel, 1);

    // 确认升级按钮
    TownHall* townHall = GetTownHall();
    int currentElixir = townHall->GetElixir();
    bool canAfford = currentElixir >= upgradeCost;

    auto upgradeBtn = Button::create();
    upgradeBtn->setTitleText("Confirm Upgrade");
    upgradeBtn->setTitleFontSize(18 * _scaleFactor);
    upgradeBtn->setContentSize(Size(180 * _scaleFactor, 50 * _scaleFactor));
    upgradeBtn->setScale9Enabled(true);
    upgradeBtn->setPosition(Vec2(panelSize.width / 2, 50 * _scaleFactor));

    if (canAfford) {
        upgradeBtn->setEnabled(true);
        upgradeBtn->setColor(Color3B::WHITE);
        upgradeBtn->addClickEventListener([this, upgradeCost, upgradeTime](Ref* sender) {
            hidePanel(UIPanelType::BuildingUpgrade, true);

            TownHall* townHall = GetTownHall();
            townHall->SpendElixir(upgradeCost);
            updateResourceDisplay(ResourceType::Elixir, townHall->GetElixir());
            _selectedBuilding->startUpgrade(upgradeTime);

            showUpgradeProgress(_selectedBuilding, (float)upgradeTime, (float)upgradeTime);
            showToast("Upgrade started!");
            triggerUIEvent("OnUpgradeStarted");
            });
    }
    else {
        upgradeBtn->setEnabled(false);
        upgradeBtn->setColor(Color3B(100, 100, 100));
    }

    panel->addChild(upgradeBtn, 1);

    return panel;
}

// ==================== 军队训练面板 ====================
void UIManager::showArmyTraining(Building* building) {
    _selectedBuilding = building;

    hidePanel(UIPanelType::ArmyTraining, true);

    auto panel = createArmyTraining(building);
    if (panel) {
        _panels[UIPanelType::ArmyTraining] = panel;
        addPanelToScene(panel, UILayer::Dialog, true);
        playShowAnimation(panel);
    }
}

Node* UIManager::createArmyTraining(Building* building) {
    auto panel = Node::create();

    Size panelSize(550 * _scaleFactor, 450 * _scaleFactor);
    panel->setContentSize(panelSize);
    panel->setPosition(Vec2((_visibleSize.width - panelSize.width) / 2 + _visibleOrigin.x,
        (_visibleSize.height - panelSize.height) / 2 + _visibleOrigin.y));

    // 背景
    auto bg = LayerColor::create(Color4B(40, 40, 60, 240), panelSize.width, panelSize.height);
    panel->addChild(bg, 0);

    // 边框
    auto border = DrawNode::create();
    border->drawRect(Vec2(0, 0), Vec2(panelSize.width, panelSize.height), Color4F::WHITE);
    panel->addChild(border, 1);

    // 标题
    auto title = Label::createWithTTF("Army Training", "fonts/arial.ttf", 24 * _scaleFactor);
    title->setPosition(Vec2(panelSize.width / 2, panelSize.height - 30 * _scaleFactor));
    title->setColor(Color3B::WHITE);
    panel->addChild(title, 1);

    // 关闭按钮
    auto closeBtn = createCloseButton([this]() {
        hidePanel(UIPanelType::ArmyTraining, true);
        });
    closeBtn->setPosition(Vec2(panelSize.width - 25 * _scaleFactor, panelSize.height - 25 * _scaleFactor));
    panel->addChild(closeBtn, 2);

    // 分隔线
    auto divider = DrawNode::create();
    divider->drawLine(Vec2(panelSize.width / 2, 70 * _scaleFactor),
        Vec2(panelSize.width / 2, panelSize.height - 60 * _scaleFactor),
        Color4F(1, 1, 1, 0.5f));
    panel->addChild(divider, 1);

    // 左侧标题：当前部队
    auto leftTitle = Label::createWithTTF("Current Army", "fonts/arial.ttf", 18 * _scaleFactor);
    leftTitle->setPosition(Vec2(panelSize.width / 4, panelSize.height - 65 * _scaleFactor));
    leftTitle->setColor(Color3B(200, 200, 200));
    panel->addChild(leftTitle, 1);

    // 右侧标题：可选士兵
    auto rightTitle = Label::createWithTTF("Available Troops", "fonts/arial.ttf", 18 * _scaleFactor);
    rightTitle->setPosition(Vec2(panelSize.width * 3 / 4, panelSize.height - 65 * _scaleFactor));
    rightTitle->setColor(Color3B(200, 200, 200));
    panel->addChild(rightTitle, 1);

    // 获取士兵模板和人口上限
    TownHall* townHall = GetTownHall();
    auto soldierTemplates = townHall->GetSoldierCategory();
    int maxCapacity = townHall->GetArmyCapacity();

    // 加载保存的配置或初始化
    _tempArmyConfig = loadArmyConfig();
    _tempCurrentPopulation = 0;

    // 计算当前人口
    for (const auto& tmpl : soldierTemplates) {
        auto it = _tempArmyConfig.find(tmpl.name);
        if (it != _tempArmyConfig.end()) {
            _tempCurrentPopulation += it->second * tmpl.populationCost;
        }
        else {
            _tempArmyConfig[tmpl.name] = 0;
        }
    }

    // 人口显示
    auto popLabel = Label::createWithTTF(
        StringUtils::format("Population: %d/%d", _tempCurrentPopulation, maxCapacity),
        "fonts/arial.ttf", 16 * _scaleFactor);
    popLabel->setPosition(Vec2(panelSize.width / 4, panelSize.height - 90 * _scaleFactor));
    popLabel->setColor(Color3B(255, 200, 100));
    popLabel->setName("popLabel");
    panel->addChild(popLabel, 1);

    // 左右容器
    float containerTop = panelSize.height - 110 * _scaleFactor;
    float containerBottom = 70 * _scaleFactor;
    float containerHeight = containerTop - containerBottom;
    float halfWidth = panelSize.width / 2 - 20 * _scaleFactor;

    auto leftContainer = Node::create();
    leftContainer->setPosition(Vec2(10 * _scaleFactor, containerBottom));
    leftContainer->setContentSize(Size(halfWidth, containerHeight));
    leftContainer->setName("leftContainer");
    panel->addChild(leftContainer, 1);

    auto rightContainer = Node::create();
    rightContainer->setPosition(Vec2(panelSize.width / 2 + 10 * _scaleFactor, containerBottom));
    rightContainer->setContentSize(Size(halfWidth, containerHeight));
    rightContainer->setName("rightContainer");
    panel->addChild(rightContainer, 1);

    // 创建士兵项
    float iconSize = 60 * _scaleFactor;
    float itemSpacing = 10 * _scaleFactor;
    int columns = 3;

    for (size_t i = 0; i < soldierTemplates.size(); i++) {
        const auto& tmpl = soldierTemplates[i];
        int row = (int)i / columns;
        int col = (int)i % columns;
        float xPos = col * (iconSize + itemSpacing) + iconSize / 2 + 10 * _scaleFactor;
        float yPos = containerHeight - row * (iconSize + itemSpacing + 20 * _scaleFactor) - iconSize / 2 - 10 * _scaleFactor;

        // ===== 左侧：当前部队 =====
        auto leftItem = Node::create();
        leftItem->setContentSize(Size(iconSize, iconSize + 20 * _scaleFactor));
        leftItem->setPosition(Vec2(xPos, yPos));
        leftItem->setName("left_" + tmpl.name);

        // 左侧图标
        auto leftIcon = Sprite::create(tmpl.iconPath);
        if (!leftIcon) {
            leftIcon = Sprite::create();
            leftIcon->setTextureRect(Rect(0, 0, iconSize, iconSize));
            leftIcon->setColor(Color3B(100, 150, 100));
        }
        leftIcon->setScale(iconSize / std::max(leftIcon->getContentSize().width, leftIcon->getContentSize().height));
        leftIcon->setPosition(Vec2(iconSize / 2, iconSize / 2 + 15 * _scaleFactor));
        leftIcon->setName("icon");
        leftItem->addChild(leftIcon, 1);

        // 左侧数量标签
        int count = _tempArmyConfig[tmpl.name];
        auto leftCountLabel = Label::createWithTTF("x" + std::to_string(count), "fonts/arial.ttf", 14 * _scaleFactor);
        leftCountLabel->setPosition(Vec2(iconSize / 2, 5 * _scaleFactor));
        leftCountLabel->setColor(count > 0 ? Color3B::WHITE : Color3B(100, 100, 100));
        leftCountLabel->setName("countLabel");
        leftItem->addChild(leftCountLabel, 1);

        // 左侧点击事件（移除士兵）
        auto leftTouchListener = EventListenerTouchOneByOne::create();
        leftTouchListener->setSwallowTouches(true);
        leftTouchListener->onTouchBegan = [leftItem](Touch* touch, Event* event) -> bool {
            Vec2 locationInNode = leftItem->convertToNodeSpace(touch->getLocation());
            Rect rect(Vec2::ZERO, leftItem->getContentSize());
            return rect.containsPoint(locationInNode);
            };

        std::string soldierName = tmpl.name;
        int populationCost = tmpl.populationCost;
        leftTouchListener->onTouchEnded = [this, panel, soldierName, populationCost](Touch* touch, Event* event) {
            if (_tempArmyConfig[soldierName] > 0) {
                _tempArmyConfig[soldierName]--;
                _tempCurrentPopulation -= populationCost;
                refreshArmyTrainingUI(panel);
            }
            };
        Director::getInstance()->getEventDispatcher()->addEventListenerWithSceneGraphPriority(leftTouchListener, leftItem);

        leftContainer->addChild(leftItem, 1);

        // ===== 右侧：可选士兵 =====
        auto rightItem = Node::create();
        rightItem->setContentSize(Size(iconSize, iconSize));
        rightItem->setPosition(Vec2(xPos, yPos + 10 * _scaleFactor));
        rightItem->setName("right_" + tmpl.name);

        // 右侧图标
        auto rightIcon = Sprite::create(tmpl.iconPath);
        if (!rightIcon) {
            rightIcon = Sprite::create();
            rightIcon->setTextureRect(Rect(0, 0, iconSize, iconSize));
            rightIcon->setColor(Color3B(100, 100, 150));
        }
        rightIcon->setScale(iconSize / std::max(rightIcon->getContentSize().width, rightIcon->getContentSize().height));
        rightIcon->setPosition(Vec2(iconSize / 2, iconSize / 2));
        rightIcon->setName("icon");
        rightItem->addChild(rightIcon, 1);

        // 右侧点击事件（添加士兵）
        auto rightTouchListener = EventListenerTouchOneByOne::create();
        rightTouchListener->setSwallowTouches(true);
        rightTouchListener->onTouchBegan = [rightItem](Touch* touch, Event* event) -> bool {
            Vec2 locationInNode = rightItem->convertToNodeSpace(touch->getLocation());
            Rect rect(Vec2::ZERO, rightItem->getContentSize());
            return rect.containsPoint(locationInNode);
            };

        rightTouchListener->onTouchEnded = [this, panel, soldierName, populationCost, maxCapacity](Touch* touch, Event* event) {
            if (_tempCurrentPopulation + populationCost <= maxCapacity) {
                _tempArmyConfig[soldierName]++;
                _tempCurrentPopulation += populationCost;
                refreshArmyTrainingUI(panel);
            }
            };
        Director::getInstance()->getEventDispatcher()->addEventListenerWithSceneGraphPriority(rightTouchListener, rightItem);

        rightContainer->addChild(rightItem, 1);
    }

    // 刷新右侧可选状态
    refreshArmyTrainingUI(panel);

    // 训练部队按钮
    auto trainBtn = Button::create();
    trainBtn->setTitleText("Confirm Training");
    trainBtn->setTitleFontSize(18 * _scaleFactor);
    trainBtn->setContentSize(Size(180 * _scaleFactor, 45 * _scaleFactor));
    trainBtn->setScale9Enabled(true);
    trainBtn->setPosition(Vec2(panelSize.width / 2, 35 * _scaleFactor));
    trainBtn->addClickEventListener([this](Ref* sender) {
        saveArmyConfig();
        hidePanel(UIPanelType::ArmyTraining, true);
        showToast("Army configuration saved!");
        triggerUIEvent("OnArmyConfigSaved");
        });
    panel->addChild(trainBtn, 1);

    return panel;
}

void UIManager::refreshArmyTrainingUI(Node* panel) {
    if (!panel) return;

    TownHall* townHall = GetTownHall();
    auto soldierTemplates = townHall->GetSoldierCategory();
    int maxCapacity = townHall->GetArmyCapacity();

    // 更新人口显示
    auto popLabel = panel->getChildByName<Label*>("popLabel");
    if (popLabel) {
        popLabel->setString(StringUtils::format("Population: %d/%d", _tempCurrentPopulation, maxCapacity));
        popLabel->setColor(_tempCurrentPopulation >= maxCapacity ? Color3B(255, 100, 100) : Color3B(255, 200, 100));
    }

    auto leftContainer = panel->getChildByName("leftContainer");
    auto rightContainer = panel->getChildByName("rightContainer");

    for (const auto& tmpl : soldierTemplates) {
        int count = _tempArmyConfig[tmpl.name];
        bool canAdd = (_tempCurrentPopulation + tmpl.populationCost <= maxCapacity);

        // 更新左侧数量
        if (leftContainer) {
            auto leftItem = leftContainer->getChildByName("left_" + tmpl.name);
            if (leftItem) {
                auto countLabel = leftItem->getChildByName<Label*>("countLabel");
                if (countLabel) {
                    countLabel->setString("x" + std::to_string(count));
                    countLabel->setColor(count > 0 ? Color3B::WHITE : Color3B(100, 100, 100));
                }
                auto icon = leftItem->getChildByName<Sprite*>("icon");
                if (icon) {
                    icon->setColor(count > 0 ? Color3B::WHITE : Color3B(80, 80, 80));
                }
            }
        }

        // 更新右侧可选状态
        if (rightContainer) {
            auto rightItem = rightContainer->getChildByName("right_" + tmpl.name);
            if (rightItem) {
                auto icon = rightItem->getChildByName<Sprite*>("icon");
                if (icon) {
                    icon->setColor(canAdd ? Color3B::WHITE : Color3B(80, 80, 80));
                }
            }
        }
    }
}

// ==================== 军队配置存储 ====================
void UIManager::saveArmyConfig() {
    std::string configStr;
    for (const auto& pair : _tempArmyConfig) {
        if (pair.second > 0) {
            if (!configStr.empty()) configStr += ",";
            configStr += pair.first + ":" + std::to_string(pair.second);
        }
    }
    UserDefault::getInstance()->setStringForKey("army_config", configStr);
    UserDefault::getInstance()->flush();
}

std::map<std::string, int> UIManager::loadArmyConfig() {
    std::map<std::string, int> result;
    std::string configStr = UserDefault::getInstance()->getStringForKey("army_config", "");

    if (!configStr.empty()) {
        std::stringstream ss(configStr);
        std::string item;
        while (std::getline(ss, item, ',')) {
            size_t pos = item.find(':');
            if (pos != std::string::npos) {
                std::string name = item.substr(0, pos);
                int count = std::stoi(item.substr(pos + 1));
                result[name] = count;
            }
        }
    }
    return result;
}

// ==================== 战斗HUD ====================
Node* UIManager::createBattleHUD() {
    auto panel = Node::create();
    panel->setContentSize(_visibleSize);
    panel->setPosition(_visibleOrigin);

    // ==================== 底部士兵部署栏 ====================
    float bottomBarHeight = 80 * _scaleFactor;
    auto bottomBar = LayerColor::create(Color4B(0, 0, 0, 180), _visibleSize.width, bottomBarHeight);
    bottomBar->setPosition(Vec2(0, 0));
    panel->addChild(bottomBar, 1);

    // 从保存的配置读取士兵数据
    TownHall* townHall = GetTownHall();
    auto soldierTemplates = townHall->GetSoldierCategory();
    auto armyConfig = loadArmyConfig();

    // 构建部署数据（只显示数量 > 0 的士兵）
    struct DeployTroop {
        std::string name;
        std::string iconPath;
        int count;
    };
    std::vector<DeployTroop> deployTroops;
    for (const auto& tmpl : soldierTemplates) {
        auto it = armyConfig.find(tmpl.name);
        if (it != armyConfig.end() && it->second > 0) {
            deployTroops.push_back({ tmpl.name, tmpl.iconPath, it->second });
        }
    }

    // 存储当前战斗中的士兵数量
    _battleTroopCounts.clear();
    _battleTroopNames.clear();
    for (const auto& troop : deployTroops) {
        _battleTroopCounts[troop.name] = troop.count;
        _battleTroopNames.push_back(troop.name);
    }
    _selectedTroopIndex = -1;
    _selectedTroopName = "";

    // 士兵按钮布局
    float btnSize = 60 * _scaleFactor;
    float btnMargin = 15 * _scaleFactor;
    float totalWidth = deployTroops.size() * btnSize + (deployTroops.size() - 1) * btnMargin;
    float startX = (_visibleSize.width - totalWidth) / 2 + btnSize / 2;

    for (size_t i = 0; i < deployTroops.size(); i++) {
        const auto& troop = deployTroops[i];

        // 士兵按钮容器
        auto btnContainer = Node::create();
        btnContainer->setContentSize(Size(btnSize + 6 * _scaleFactor, btnSize + 6 * _scaleFactor));
        btnContainer->setPosition(Vec2(startX + i * (btnSize + btnMargin), bottomBarHeight / 2));
        btnContainer->setName("troopBtn_" + std::to_string(i));

        // 选中边框（默认隐藏）
        auto selectBorder = DrawNode::create();
        selectBorder->drawRect(
            Vec2(-btnSize / 2 - 3 * _scaleFactor, -btnSize / 2 - 3 * _scaleFactor),
            Vec2(btnSize / 2 + 3 * _scaleFactor, btnSize / 2 + 3 * _scaleFactor),
            Color4F::WHITE);
        selectBorder->setPosition(Vec2(btnContainer->getContentSize().width / 2,
            btnContainer->getContentSize().height / 2));
        selectBorder->setName("selectBorder");
        selectBorder->setVisible(false);
        btnContainer->addChild(selectBorder, 0);

        // 士兵图标
        auto icon = Sprite::create(troop.iconPath);
        if (!icon) {
            icon = Sprite::create();
            icon->setTextureRect(Rect(0, 0, btnSize, btnSize));
            icon->setColor(Color3B(100, 150, 100));
        }
        icon->setScale(btnSize / std::max(icon->getContentSize().width, icon->getContentSize().height));
        icon->setPosition(Vec2(btnContainer->getContentSize().width / 2,
            btnContainer->getContentSize().height / 2));
        icon->setName("icon");
        btnContainer->addChild(icon, 1);

        // 数量标签
        auto countLabel = Label::createWithTTF(std::to_string(troop.count), "fonts/arial.ttf", 14 * _scaleFactor);
        countLabel->setPosition(Vec2(btnContainer->getContentSize().width - 5 * _scaleFactor,
            8 * _scaleFactor));
        countLabel->setAnchorPoint(Vec2(1, 0.5f));
        countLabel->setColor(Color3B::WHITE);
        countLabel->enableOutline(Color4B::BLACK, 1);
        countLabel->setName("countLabel");
        btnContainer->addChild(countLabel, 2);

        // 点击事件（只负责选中士兵，不负责放置）
        auto touchListener = EventListenerTouchOneByOne::create();
        touchListener->setSwallowTouches(true);
        touchListener->onTouchBegan = [btnContainer](Touch* touch, Event* event) -> bool {
            Vec2 locationInNode = btnContainer->convertToNodeSpace(touch->getLocation());
            Rect rect(Vec2::ZERO, btnContainer->getContentSize());
            return rect.containsPoint(locationInNode);
            };

        std::string troopName = troop.name;
        int index = (int)i;
        touchListener->onTouchEnded = [this, troopName, index](Touch* touch, Event* event) {
            // 检查是否还有剩余
            if (_battleTroopCounts[troopName] <= 0) return;
            selectBattleTroop(index, troopName);
            };
        Director::getInstance()->getEventDispatcher()->addEventListenerWithSceneGraphPriority(
            touchListener, btnContainer);

        panel->addChild(btnContainer, 2);
    }

    // ==================== 士兵栏上方控制区 ====================
    float controlBarY = bottomBarHeight + 10 * _scaleFactor;
    float controlBarHeight = 40 * _scaleFactor;

    // 左侧：结束战斗按钮
    auto endBtn = Button::create();
    endBtn->setTitleText("End Battle");
    endBtn->setTitleFontSize(14 * _scaleFactor);
    endBtn->setContentSize(Size(100 * _scaleFactor, controlBarHeight));
    endBtn->setScale9Enabled(true);
    endBtn->setPosition(Vec2(70 * _scaleFactor, controlBarY + controlBarHeight / 2));
    endBtn->addClickEventListener([this](Ref* sender) {
        showConfirmDialog("End Battle", "Return to village?", [this]() {
            // 获取当前摧毁率计算星级
            int destroyPercent = Combat::GetInstance().GetDestroyPercent();
            int stars = 0;
            if (destroyPercent >= 50) stars = 1;
            if (destroyPercent >= 75) stars = 2;
            if (destroyPercent >= 100) stars = 3;
            endBattle(stars, destroyPercent);
            });
        });
    panel->addChild(endBtn, 2);

    // 右侧：摧毁率状态栏（半透明黑色背景）
    float statusBarWidth = 180 * _scaleFactor;
    float statusBarHeight = controlBarHeight;
    auto statusBar = LayerColor::create(Color4B(0, 0, 0, 150), statusBarWidth, statusBarHeight);
    statusBar->setPosition(Vec2(_visibleSize.width - statusBarWidth - 20 * _scaleFactor, controlBarY));
    statusBar->setName("statusBar");
    panel->addChild(statusBar, 1);

    // 星级显示（左侧）
    float starSize = 20 * _scaleFactor;
    float starStartX = 15 * _scaleFactor;
    for (int i = 0; i < 3; i++) {
        auto star = DrawNode::create();
        star->drawSolidCircle(Vec2::ZERO, starSize / 2, 0, 5, Color4F(0.3f, 0.3f, 0.3f, 1));
        star->setPosition(Vec2(starStartX + i * (starSize + 5 * _scaleFactor), statusBarHeight / 2));
        star->setName("star_" + std::to_string(i));
        statusBar->addChild(star, 1);
    }

    // 摧毁百分比（右侧）
    auto destroyLabel = Label::createWithTTF("0%", "fonts/arial.ttf", 18 * _scaleFactor);
    destroyLabel->setPosition(Vec2(statusBarWidth - 15 * _scaleFactor, statusBarHeight / 2));
    destroyLabel->setAnchorPoint(Vec2(1, 0.5f));
    destroyLabel->setColor(Color3B(255, 200, 100));
    destroyLabel->setName("destroyLabel");
    statusBar->addChild(destroyLabel, 1);

    return panel;
}

// 选中战斗士兵
void UIManager::selectBattleTroop(int index, const std::string& troopName) {
    auto panel = getPanel(UIPanelType::BattleHUD);
    if (!panel) return;

    // 取消之前的选中状态
    if (_selectedTroopIndex >= 0 && _selectedTroopIndex != index) {
        auto prevContainer = panel->getChildByName("troopBtn_" + std::to_string(_selectedTroopIndex));
        if (prevContainer) {
            auto prevBorder = prevContainer->getChildByName("selectBorder");
            if (prevBorder) prevBorder->setVisible(false);
        }
    }

    // 设置新的选中状态
    _selectedTroopIndex = index;
    _selectedTroopName = troopName;

    auto container = panel->getChildByName("troopBtn_" + std::to_string(index));
    if (container) {
        auto border = container->getChildByName("selectBorder");
        if (border) border->setVisible(true);
    }

    CCLOG("Selected troop: %s", troopName.c_str());
}

// 取消选中士兵
void UIManager::deselectBattleTroop() {
    if (_selectedTroopIndex >= 0) {
        auto panel = getPanel(UIPanelType::BattleHUD);
        if (panel) {
            auto container = panel->getChildByName("troopBtn_" + std::to_string(_selectedTroopIndex));
            if (container) {
                auto border = container->getChildByName("selectBorder");
                if (border) border->setVisible(false);
            }
        }
    }
    _selectedTroopIndex = -1;
    _selectedTroopName = "";
}

// 更新战斗中士兵数量
void UIManager::updateBattleTroopCount(const std::string& troopName, int newCount) {
    _battleTroopCounts[troopName] = newCount;
    refreshBattleHUDTroops();
}

// 获取指定士兵的剩余数量
int UIManager::getBattleTroopCount(const std::string& troopName) const {
    auto it = _battleTroopCounts.find(troopName);
    return (it != _battleTroopCounts.end()) ? it->second : 0;
}

// 刷新战斗HUD士兵显示
void UIManager::refreshBattleHUDTroops() {
    auto panel = getPanel(UIPanelType::BattleHUD);
    if (!panel) return;

    for (size_t i = 0; i < _battleTroopNames.size(); i++) {
        const std::string& troopName = _battleTroopNames[i];
        int count = _battleTroopCounts[troopName];

        auto container = panel->getChildByName("troopBtn_" + std::to_string(i));
        if (!container) continue;

        auto countLabel = container->getChildByName<Label*>("countLabel");
        auto icon = container->getChildByName<Sprite*>("icon");
        auto selectBorder = container->getChildByName("selectBorder");

        if (countLabel) {
            countLabel->setString(std::to_string(count));
        }

        // 数量为0时变灰并取消选中
        if (count <= 0) {
            if (icon) icon->setColor(Color3B(80, 80, 80));
            if (selectBorder) selectBorder->setVisible(false);
            if (_selectedTroopIndex == (int)i) {
                _selectedTroopIndex = -1;
                _selectedTroopName = "";
            }
        }
    }
}

// 更新摧毁百分比和星级
void UIManager::updateDestructionPercent(int percent) {
    auto panel = getPanel(UIPanelType::BattleHUD);
    if (!panel) return;

    auto statusBar = panel->getChildByName("statusBar");
    if (!statusBar) return;

    // 更新百分比
    auto destroyLabel = statusBar->getChildByName<Label*>("destroyLabel");
    if (destroyLabel) {
        destroyLabel->setString(std::to_string(percent) + "%");
    }

    // 计算星级（50%=1星，75%=2星，100%=3星）
    int stars = 0;
    if (percent >= 50) stars = 1;
    if (percent >= 75) stars = 2;
    if (percent >= 100) stars = 3;

    // 更新星级显示
    float starSize = 20 * _scaleFactor;
    for (int i = 0; i < 3; i++) {
        auto star = statusBar->getChildByName<DrawNode*>("star_" + std::to_string(i));
        if (star) {
            star->clear();
            Color4F starColor = (i < stars) ? Color4F(1, 0.8f, 0, 1) : Color4F(0.3f, 0.3f, 0.3f, 1);
            star->drawSolidCircle(Vec2::ZERO, starSize / 2, 0, 5, starColor);
        }
    }
}

// ==================== 战斗模式 ====================

void UIManager::enterBattleMode(MapManager* battleMap) {
    if (_isBattleMode) return;
    if (!battleMap) {
        CCLOG("UIManager::enterBattleMode - battleMap is null");
        return;
    }

    _isBattleMode = true;
    _currentBattleMap = battleMap;
    _selectedTroopIndex = -1;
    _selectedTroopName = "";

    // 显示战斗 HUD
    showPanel(UIPanelType::BattleHUD, UILayer::HUD, false);

    // 设置触摸监听（用于在地图上放置士兵）
    setupBattleTouchListener();

    CCLOG("UIManager: Entered battle mode");
}

void UIManager::exitBattleMode() {
    if (!_isBattleMode) return;

    _isBattleMode = false;
    _currentBattleMap = nullptr;
    _selectedTroopIndex = -1;
    _selectedTroopName = "";

    // 隐藏战斗 HUD
    hidePanel(UIPanelType::BattleHUD, true);

    // 移除触摸监听
    removeBattleTouchListener();

    // 清空士兵数量
    _battleTroopCounts.clear();
    _battleTroopNames.clear();

    CCLOG("UIManager: Exited battle mode");
}

void UIManager::setupBattleTouchListener() {
    if (_battleTouchListener) return;
    if (!_rootScene) return;

    _battleTouchListener = EventListenerTouchOneByOne::create();
    _battleTouchListener->setSwallowTouches(false);  // 不吞噬，让底部UI也能响应

    _battleTouchListener->onTouchBegan = [this](Touch* touch, Event* event) -> bool {
        // 战斗模式下，如果有选中士兵，就响应触摸
        return _isBattleMode && !_selectedTroopName.empty();
        };

    _battleTouchListener->onTouchEnded = [this](Touch* touch, Event* event) {
        if (!_isBattleMode || _selectedTroopName.empty()) return;

        Vec2 touchPos = touch->getLocation();

        // 检查是否点击在底部 UI 区域
        float uiHeight = 130.0f * _scaleFactor;
        if (touchPos.y < uiHeight) {
            return;  // 点击在 UI 区域，不处理
        }

        // 尝试放置士兵
        deploySoldierAt(touchPos);
        };

    Director::getInstance()->getEventDispatcher()
        ->addEventListenerWithSceneGraphPriority(_battleTouchListener, _rootScene);
}

void UIManager::removeBattleTouchListener() {
    if (_battleTouchListener) {
        Director::getInstance()->getEventDispatcher()
            ->removeEventListener(_battleTouchListener);
        _battleTouchListener = nullptr;
    }
}

bool UIManager::deploySoldierAt(const cocos2d::Vec2& screenPos) {
    if (!_isBattleMode || !_currentBattleMap || _selectedTroopName.empty()) {
        return false;
    }

    // 检查剩余数量
    int remaining = getBattleTroopCount(_selectedTroopName);
    if (remaining <= 0) {
        CCLOG("No remaining troops of type: %s", _selectedTroopName.c_str());
        return false;
    }

    // 屏幕坐标 → 地图本地坐标 → Vec 坐标
    Vec2 localPos = _currentBattleMap->convertToNodeSpace(screenPos);
    Vec2 vecPos = _currentBattleMap->worldToVec(localPos);

    // 转为格子坐标检查
    int gridX = static_cast<int>(std::floor(vecPos.x));
    int gridY = static_cast<int>(std::floor(vecPos.y));

    // 检查是否可放置
    if (!_currentBattleMap->isDeployAllowedGrid(gridX, gridY)) {
        CCLOG("Invalid deploy position: (%d, %d)", gridX, gridY);
        return false;
    }

    // 调用 Combat 创建士兵（无返回值）
    Combat::GetInstance().DeploySoldier(_selectedTroopName, vecPos);

    // 更新 UI 数量
    updateBattleTroopCount(_selectedTroopName, remaining - 1);

    CCLOG("Deployed soldier '%s' at vec (%.2f, %.2f), remaining: %d",
        _selectedTroopName.c_str(), vecPos.x, vecPos.y, remaining - 1);

    return true;
}

// ==================== 结束战斗 ====================

void UIManager::endBattle(int stars, int destroyPercent) {
    if (!_isBattleMode) return;

    // 隐藏战斗 HUD
    hidePanel(UIPanelType::BattleHUD, true);

    // 移除触摸监听
    removeBattleTouchListener();
    _isBattleMode = false;

    // 等待 1 秒后显示结算面板
    if (_rootScene) {
        _rootScene->scheduleOnce([this, stars, destroyPercent](float dt) {
            // 创建并显示结算面板
            auto panel = createBattleResult(stars, destroyPercent);
            if (panel) {
                _panels[UIPanelType::BattleResult] = panel;
                addPanelToScene(panel, UILayer::Dialog, true);
                playShowAnimation(panel);
            }
            }, 1.0f, "showBattleResult");
    }
}

Node* UIManager::createBattleResult(int stars, int destroyPercent) {
    auto panel = Node::create();

    Size panelSize(400 * _scaleFactor, 300 * _scaleFactor);
    panel->setContentSize(panelSize);
    panel->setPosition(Vec2((_visibleSize.width - panelSize.width) / 2 + _visibleOrigin.x,
        (_visibleSize.height - panelSize.height) / 2 + _visibleOrigin.y));

    // 背景
    auto bg = LayerColor::create(Color4B(30, 30, 50, 240), panelSize.width, panelSize.height);
    panel->addChild(bg, 0);

    // 边框
    auto border = DrawNode::create();
    border->drawRect(Vec2(0, 0), Vec2(panelSize.width, panelSize.height), Color4F::WHITE);
    panel->addChild(border, 1);

    // 标题
    std::string resultText = (stars > 0) ? "Victory!" : "Defeat";
    Color3B resultColor = (stars > 0) ? Color3B(100, 255, 100) : Color3B(255, 100, 100);

    auto title = Label::createWithTTF(resultText, "fonts/arial.ttf", 32 * _scaleFactor);
    title->setPosition(Vec2(panelSize.width / 2, panelSize.height - 50 * _scaleFactor));
    title->setColor(resultColor);
    panel->addChild(title, 1);

    // 星级显示
    float starSize = 35 * _scaleFactor;
    float starY = panelSize.height - 110 * _scaleFactor;
    for (int i = 0; i < 3; i++) {
        auto star = DrawNode::create();
        Color4F starColor = (i < stars) ? Color4F(1, 0.8f, 0, 1) : Color4F(0.3f, 0.3f, 0.3f, 1);
        star->drawSolidCircle(Vec2::ZERO, starSize / 2, 0, 5, starColor);
        star->setPosition(Vec2(panelSize.width / 2 + (i - 1) * (starSize + 15 * _scaleFactor), starY));
        panel->addChild(star, 1);
    }

    // 摧毁率
    auto destroyLabel = Label::createWithTTF("Destruction: " + std::to_string(destroyPercent) + "%",
        "fonts/arial.ttf", 22 * _scaleFactor);
    destroyLabel->setPosition(Vec2(panelSize.width / 2, panelSize.height - 170 * _scaleFactor));
    destroyLabel->setColor(Color3B(255, 200, 100));
    panel->addChild(destroyLabel, 1);

    // 确认按钮
    auto confirmBtn = Button::create();
    confirmBtn->setTitleText("Confirm");
    confirmBtn->setTitleFontSize(20 * _scaleFactor);
    confirmBtn->setContentSize(Size(150 * _scaleFactor, 50 * _scaleFactor));
    confirmBtn->setScale9Enabled(true);
    confirmBtn->setPosition(Vec2(panelSize.width / 2, 60 * _scaleFactor));
    confirmBtn->addClickEventListener([this](Ref* sender) {
        // 隐藏结算面板
        hidePanel(UIPanelType::BattleResult, true);

        // 播放白色过渡动画，然后返回主场景
        playWhiteTransition([this]() {
            // 清理状态
            _currentBattleMap = nullptr;
            _battleTroopCounts.clear();
            _battleTroopNames.clear();

            // 返回主场景
            auto mainScene = MainScene::create();
            Director::getInstance()->replaceScene(mainScene);
            });
        });
    panel->addChild(confirmBtn, 1);

    return panel;
}

void UIManager::playWhiteTransition(const std::function<void()>& onComplete) {
    if (!_rootScene) {
        if (onComplete) onComplete();
        return;
    }

    // 创建全屏白色遮罩
    auto flash = LayerColor::create(Color4B(255, 255, 255, 0), _visibleSize.width, _visibleSize.height);
    flash->setPosition(_visibleOrigin);
    _rootScene->addChild(flash, static_cast<int>(UILayer::Loading));

    // 动画：淡入 → 回调 → 淡出 → 移除
    auto fadeIn = FadeTo::create(0.3f, 255);
    auto callFunc = CallFunc::create([onComplete]() {
        if (onComplete) onComplete();
        });
    auto fadeOut = FadeTo::create(0.3f, 0);
    auto removeSelf = RemoveSelf::create();

    flash->runAction(Sequence::create(fadeIn, callFunc, fadeOut, removeSelf, nullptr));
}

// ==================== 升级进度覆盖层 ====================
Node* UIManager::createUpgradeProgressOverlay(Building* building, float totalTime, float remainingTime) {
    auto overlay = Node::create();
    overlay->setContentSize(Size(80 * _scaleFactor, 40 * _scaleFactor));

    // 进度条背景
    auto progressBg = LayerColor::create(Color4B(30, 30, 30, 200),
        overlay->getContentSize().width,
        15 * _scaleFactor);
    progressBg->setPosition(Vec2(0, 20 * _scaleFactor));
    overlay->addChild(progressBg, 0);

    // 进度条填充
    float progress = (totalTime - remainingTime) / totalTime;
    auto progressFill = LayerColor::create(Color4B(100, 200, 100, 255),
        overlay->getContentSize().width * progress,
        15 * _scaleFactor);
    progressFill->setPosition(Vec2(0, 20 * _scaleFactor));
    progressFill->setName("progressFill");
    overlay->addChild(progressFill, 1);

    // 剩余时间文字
    int minutes = (int)remainingTime / 60;
    int seconds = (int)remainingTime % 60;
    std::string timeStr = StringUtils::format("%02d:%02d", minutes, seconds);
    auto timeLabel = Label::createWithTTF(timeStr, "fonts/arial.ttf", 14 * _scaleFactor);
    timeLabel->setPosition(Vec2(overlay->getContentSize().width / 2, 8 * _scaleFactor));
    timeLabel->setColor(Color3B::WHITE);
    timeLabel->setName("timeLabel");
    overlay->addChild(timeLabel, 1);

    return overlay;
}

void UIManager::showUpgradeProgress(Building* building, float totalTime, float remainingTime) {
    if (!building || !_rootScene) return;

    // 移除已存在的进度条
    removeUpgradeProgress(building);

    auto overlay = createUpgradeProgressOverlay(building, totalTime, remainingTime);
    if (overlay) {
        // 获取建筑在世界坐标系的位置
        Vec2 buildingWorldPos = building->getParent()->convertToWorldSpace(building->getPosition());

        // 进度条显示在建筑上方
        overlay->setPosition(Vec2(buildingWorldPos.x - overlay->getContentSize().width / 2,
            buildingWorldPos.y + building->getContentSize().height / 2 + 10 * _scaleFactor));

        _rootScene->addChild(overlay, static_cast<int>(UILayer::HUD));
        _upgradeProgressNodes[building] = overlay;

        // 存储totalTime用于更新计算
        overlay->setUserData(new float(totalTime));

        // 启动更新定时器
        overlay->schedule([this, building, totalTime](float dt) {
            auto it = _upgradeProgressNodes.find(building);
            if (it != _upgradeProgressNodes.end()) {
                auto timeLabel = it->second->getChildByName<Label*>("timeLabel");
                auto progressFill = it->second->getChildByName<LayerColor*>("progressFill");

                if (timeLabel && progressFill) {
                    // 获取当前剩余时间 - 这里需要替换为 building->getRemainingUpgradeTime()
                    static float remaining = totalTime;
                    remaining -= dt;

                    if (remaining <= 0) {
                        remaining = 0;
                        removeUpgradeProgress(building);
                        showToast("Upgrade complete!");
                        triggerUIEvent("OnUpgradeComplete");
                        return;
                    }

                    int minutes = (int)remaining / 60;
                    int seconds = (int)remaining % 60;
                    timeLabel->setString(StringUtils::format("%02d:%02d", minutes, seconds));

                    float progress = (totalTime - remaining) / totalTime;
                    progressFill->setContentSize(Size(it->second->getContentSize().width * progress,
                        15 * _scaleFactor));
                }
            }
            }, "upgradeTimer");
    }
}

void UIManager::updateUpgradeProgress(Building* building, float remainingTime) {
    auto it = _upgradeProgressNodes.find(building);
    if (it != _upgradeProgressNodes.end() && it->second) {
        auto timeLabel = it->second->getChildByName<Label*>("timeLabel");
        auto progressFill = it->second->getChildByName<LayerColor*>("progressFill");

        if (timeLabel) {
            int minutes = (int)remainingTime / 60;
            int seconds = (int)remainingTime % 60;
            timeLabel->setString(StringUtils::format("%02d:%02d", minutes, seconds));
        }

        if (progressFill) {
            float* totalTimePtr = static_cast<float*>(it->second->getUserData());
            if (totalTimePtr) {
                float totalTime = *totalTimePtr;
                float progress = (totalTime - remainingTime) / totalTime;
                progressFill->setContentSize(Size(it->second->getContentSize().width * progress,
                    15 * _scaleFactor));
            }
        }
    }
}

void UIManager::removeUpgradeProgress(Building* building) {
    auto it = _upgradeProgressNodes.find(building);
    if (it != _upgradeProgressNodes.end()) {
        if (it->second) {
            // 清理userData
            float* totalTimePtr = static_cast<float*>(it->second->getUserData());
            if (totalTimePtr) {
                delete totalTimePtr;
            }
            it->second->unschedule("upgradeTimer");
            it->second->removeFromParent();
        }
        _upgradeProgressNodes.erase(it);
    }
}

// ==================== 提示与对话框 ====================
void UIManager::showToast(const std::string& message, float duration) {
    if (!_rootScene) return;

    auto toast = LayerColor::create(Color4B(0, 0, 0, 180),
        message.length() * 12 * _scaleFactor + 40 * _scaleFactor,
        40 * _scaleFactor);
    toast->setPosition(Vec2((_visibleSize.width - toast->getContentSize().width) / 2 + _visibleOrigin.x,
        _visibleSize.height * 0.2f + _visibleOrigin.y));

    auto label = Label::createWithTTF(message, "fonts/arial.ttf", 18 * _scaleFactor);
    label->setPosition(Vec2(toast->getContentSize().width / 2, toast->getContentSize().height / 2));
    label->setColor(Color3B::WHITE);
    toast->addChild(label);

    _rootScene->addChild(toast, static_cast<int>(UILayer::Tips));

    // 淡入淡出动画
    toast->setOpacity(0);
    auto fadeIn = FadeIn::create(0.2f);
    auto delay = DelayTime::create(duration);
    auto fadeOut = FadeOut::create(0.3f);
    auto removeSelf = RemoveSelf::create();
    toast->runAction(Sequence::create(fadeIn, delay, fadeOut, removeSelf, nullptr));
}

void UIManager::showConfirmDialog(const std::string& title, const std::string& content,
    const std::function<void()>& onConfirm, const std::function<void()>& onCancel) {

    auto panel = Node::create();
    Size panelSize(350 * _scaleFactor, 200 * _scaleFactor);
    panel->setContentSize(panelSize);
    panel->setPosition(Vec2((_visibleSize.width - panelSize.width) / 2 + _visibleOrigin.x,
        (_visibleSize.height - panelSize.height) / 2 + _visibleOrigin.y));

    // 背景
    auto bg = LayerColor::create(Color4B(40, 40, 60, 245), panelSize.width, panelSize.height);
    panel->addChild(bg, 0);

    // 边框
    auto border = DrawNode::create();
    border->drawRect(Vec2(0, 0), Vec2(panelSize.width, panelSize.height), Color4F::WHITE);
    panel->addChild(border, 1);

    // 标题
    auto titleLabel = Label::createWithTTF(title, "fonts/arial.ttf", 22 * _scaleFactor);
    titleLabel->setPosition(Vec2(panelSize.width / 2, panelSize.height - 30 * _scaleFactor));
    titleLabel->setColor(Color3B::WHITE);
    panel->addChild(titleLabel, 1);

    // 内容
    auto contentLabel = Label::createWithTTF(content, "fonts/arial.ttf", 16 * _scaleFactor);
    contentLabel->setPosition(Vec2(panelSize.width / 2, panelSize.height / 2 + 10 * _scaleFactor));
    contentLabel->setColor(Color3B(200, 200, 200));
    contentLabel->setMaxLineWidth(panelSize.width - 40 * _scaleFactor);
    contentLabel->setAlignment(TextHAlignment::CENTER);
    panel->addChild(contentLabel, 1);

    // 确认按钮
    auto confirmBtn = Button::create();
    confirmBtn->setTitleText("Confirm");
    confirmBtn->setTitleFontSize(16 * _scaleFactor);
    confirmBtn->setContentSize(Size(100 * _scaleFactor, 40 * _scaleFactor));
    confirmBtn->setScale9Enabled(true);
    confirmBtn->setPosition(Vec2(panelSize.width / 2 - 60 * _scaleFactor, 40 * _scaleFactor));
    confirmBtn->addClickEventListener([this, panel, onConfirm](Ref* sender) {
        panel->removeFromParent();
        if (onConfirm) onConfirm();
        });
    panel->addChild(confirmBtn, 1);

    // 取消按钮
    auto cancelBtn = Button::create();
    cancelBtn->setTitleText("Cancel");
    cancelBtn->setTitleFontSize(16 * _scaleFactor);
    cancelBtn->setContentSize(Size(100 * _scaleFactor, 40 * _scaleFactor));
    cancelBtn->setScale9Enabled(true);
    cancelBtn->setPosition(Vec2(panelSize.width / 2 + 60 * _scaleFactor, 40 * _scaleFactor));
    cancelBtn->addClickEventListener([this, panel, onCancel](Ref* sender) {
        panel->removeFromParent();
        if (onCancel) onCancel();
        });
    panel->addChild(cancelBtn, 1);

    // 添加模态层
    auto modalLayer = createModalLayer();
    _rootScene->addChild(modalLayer, static_cast<int>(UILayer::Dialog) - 1);
    _rootScene->addChild(panel, static_cast<int>(UILayer::Dialog));

    // 点击模态层关闭对话框
    auto touchListener = EventListenerTouchOneByOne::create();
    touchListener->setSwallowTouches(true);
    touchListener->onTouchBegan = [](Touch* touch, Event* event) -> bool {
        return true;
        };
    Director::getInstance()->getEventDispatcher()->addEventListenerWithSceneGraphPriority(touchListener, modalLayer);

    playShowAnimation(panel);
}

void UIManager::showInfoDialog(const std::string& title, const std::string& content) {
    auto panel = Node::create();
    Size panelSize(350 * _scaleFactor, 180 * _scaleFactor);
    panel->setContentSize(panelSize);
    panel->setPosition(Vec2((_visibleSize.width - panelSize.width) / 2 + _visibleOrigin.x,
        (_visibleSize.height - panelSize.height) / 2 + _visibleOrigin.y));

    // 背景
    auto bg = LayerColor::create(Color4B(40, 40, 60, 245), panelSize.width, panelSize.height);
    panel->addChild(bg, 0);

    // 边框
    auto border = DrawNode::create();
    border->drawRect(Vec2(0, 0), Vec2(panelSize.width, panelSize.height), Color4F::WHITE);
    panel->addChild(border, 1);

    // 标题
    auto titleLabel = Label::createWithTTF(title, "fonts/arial.ttf", 22 * _scaleFactor);
    titleLabel->setPosition(Vec2(panelSize.width / 2, panelSize.height - 30 * _scaleFactor));
    titleLabel->setColor(Color3B::WHITE);
    panel->addChild(titleLabel, 1);

    // 内容
    auto contentLabel = Label::createWithTTF(content, "fonts/arial.ttf", 16 * _scaleFactor);
    contentLabel->setPosition(Vec2(panelSize.width / 2, panelSize.height / 2 + 10 * _scaleFactor));
    contentLabel->setColor(Color3B(200, 200, 200));
    contentLabel->setMaxLineWidth(panelSize.width - 40 * _scaleFactor);
    contentLabel->setAlignment(TextHAlignment::CENTER);
    panel->addChild(contentLabel, 1);

    // 确定按钮
    auto okBtn = Button::create();
    okBtn->setTitleText("OK");
    okBtn->setTitleFontSize(16 * _scaleFactor);
    okBtn->setContentSize(Size(100 * _scaleFactor, 40 * _scaleFactor));
    okBtn->setScale9Enabled(true);
    okBtn->setPosition(Vec2(panelSize.width / 2, 40 * _scaleFactor));
    okBtn->addClickEventListener([panel](Ref* sender) {
        panel->removeFromParent();
        });
    panel->addChild(okBtn, 1);

    // 添加模态层
    auto modalLayer = createModalLayer();
    _rootScene->addChild(modalLayer, static_cast<int>(UILayer::Dialog) - 1);
    _rootScene->addChild(panel, static_cast<int>(UILayer::Dialog));

    playShowAnimation(panel);
}

// ==================== 战斗加载界面 ====================
void UIManager::showBattleLoading(const std::string& tips) {
    auto panel = Node::create();
    panel->setContentSize(_visibleSize);
    panel->setPosition(_visibleOrigin);

    // 全屏遮罩
    auto bg = LayerColor::create(Color4B(20, 20, 30, 255), _visibleSize.width, _visibleSize.height);
    panel->addChild(bg, 0);

    // 加载动画（旋转的圆圈）
    auto loadingCircle = DrawNode::create();
    loadingCircle->drawCircle(Vec2::ZERO, 30 * _scaleFactor, 0, 32, false, Color4F::WHITE);
    loadingCircle->setPosition(Vec2(_visibleSize.width / 2, _visibleSize.height / 2));
    panel->addChild(loadingCircle, 1);

    // 旋转动画
    auto rotate = RotateBy::create(1.0f, 360);
    auto repeatRotate = RepeatForever::create(rotate);
    loadingCircle->runAction(repeatRotate);

    // 提示文字
    auto tipsLabel = Label::createWithTTF(tips, "fonts/arial.ttf", 20 * _scaleFactor);
    tipsLabel->setPosition(Vec2(_visibleSize.width / 2, _visibleSize.height / 2 - 60 * _scaleFactor));
    tipsLabel->setColor(Color3B::WHITE);
    panel->addChild(tipsLabel, 1);

    _panels[UIPanelType::LoadingBattleField] = panel;
    addPanelToScene(panel, UILayer::Loading, true);
}

void UIManager::hideBattleLoading() {
    hidePanel(UIPanelType::LoadingBattleField, true);
}

// ==================== 通用关闭按钮 ====================
Button* UIManager::createCloseButton(const std::function<void()>& onClose) {
    auto closeBtn = Button::create("UI/btn_close.png", "UI/btn_close_pressed.png"); // 这里需要替换为实际关闭按钮图片
    if (!closeBtn->getVirtualRenderer()) {
        closeBtn = Button::create();
        closeBtn->setTitleText("X");
        closeBtn->setTitleFontSize(20 * _scaleFactor);
        closeBtn->setContentSize(Size(30 * _scaleFactor, 30 * _scaleFactor));
        closeBtn->setScale9Enabled(true);
    }
    closeBtn->addClickEventListener([onClose](Ref* sender) {
        if (onClose) onClose();
        });
    return closeBtn;
}

// ==================== 辅助方法 ====================
void UIManager::addPanelToScene(Node* panel, UILayer layer, bool modal) {
    if (!_rootScene || !panel) return;

    if (modal) {
        auto modalLayer = createModalLayer();
        _rootScene->addChild(modalLayer, static_cast<int>(layer) - 1);

        // 查找对应的面板类型并保存模态层
        for (auto& pair : _panels) {
            if (pair.second == panel) {
                _modalLayers[pair.first] = modalLayer;
                break;
            }
        }
    }

    _rootScene->addChild(panel, static_cast<int>(layer));
}

LayerColor* UIManager::createModalLayer() {
    auto layer = LayerColor::create(Color4B(0, 0, 0, 128), _visibleSize.width, _visibleSize.height);
    layer->setPosition(_visibleOrigin);

    // 阻挡触摸事件
    auto touchListener = EventListenerTouchOneByOne::create();
    touchListener->setSwallowTouches(true);
    touchListener->onTouchBegan = [](Touch* touch, Event* event) -> bool {
        return true;
        };
    Director::getInstance()->getEventDispatcher()->addEventListenerWithSceneGraphPriority(touchListener, layer);

    return layer;
}

void UIManager::playShowAnimation(Node* panel) {
    if (!panel) return;

    panel->setScale(0.8f);
    panel->setOpacity(0);

    auto scaleAction = ScaleTo::create(0.15f, 1.0f);
    auto fadeAction = FadeIn::create(0.15f);
    auto easeScale = EaseBackOut::create(scaleAction);

    panel->runAction(Spawn::create(easeScale, fadeAction, nullptr));
}

void UIManager::playHideAnimation(Node* panel, const std::function<void()>& callback) {
    if (!panel) {
        if (callback) callback();
        return;
    }

    auto scaleAction = ScaleTo::create(0.1f, 0.8f);
    auto fadeAction = FadeOut::create(0.1f);
    auto callFunc = CallFunc::create([callback]() {
        if (callback) callback();
        });

    panel->runAction(Sequence::create(Spawn::create(scaleAction, fadeAction, nullptr), callFunc, nullptr));
}

// ==================== UI事件系统 ====================
void UIManager::setUICallback(const std::string& eventName, const std::function<void()>& callback) {
    _callbacks[eventName] = callback;
}

void UIManager::triggerUIEvent(const std::string& eventName) {
    auto it = _callbacks.find(eventName);
    if (it != _callbacks.end() && it->second) {
        it->second();
    }
}

// ==================== 屏幕适配工具 ====================
Size UIManager::getVisibleSize() const {
    return _visibleSize;
}

Vec2 UIManager::getVisibleOrigin() const {
    return _visibleOrigin;
}

float UIManager::getScaleFactor() const {
    return _scaleFactor;
}

Vec2 UIManager::getUIPosition(float percentX, float percentY) const {
    return Vec2(_visibleOrigin.x + _visibleSize.width * percentX,
        _visibleOrigin.y + _visibleSize.height * percentY);
}