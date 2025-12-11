#include "UIManager/UIManager.h"
#include "Building/Building.h"

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
    , _loadingProgressBar(nullptr)
{
}

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
    auto settingsBtn = Button::create("UI/btn_settings.png", "UI/btn_settings_pressed.png"); // 这里需要替换为实际按钮图片
    if (!settingsBtn->getVirtualRenderer()) {
        settingsBtn = Button::create();
        settingsBtn->setTitleText("Settings");
        settingsBtn->setTitleFontSize(14 * _scaleFactor);
        settingsBtn->setContentSize(Size(buttonSize, buttonSize));
        settingsBtn->setScale9Enabled(true);
    }
    settingsBtn->setPosition(Vec2(rightX, startY));
    settingsBtn->addClickEventListener([this](Ref* sender) {
        showPanel(UIPanelType::Settings, UILayer::Dialog, true);
        });
    panel->addChild(settingsBtn, 1);

    // 商店按钮
    auto shopBtn = Button::create("UI/btn_shop.png", "UI/btn_shop_pressed.png"); // 这里需要替换为实际按钮图片
    if (!shopBtn->getVirtualRenderer()) {
        shopBtn = Button::create();
        shopBtn->setTitleText("Shop");
        shopBtn->setTitleFontSize(14 * _scaleFactor);
        shopBtn->setContentSize(Size(buttonSize, buttonSize));
        shopBtn->setScale9Enabled(true);
    }
    shopBtn->setPosition(Vec2(rightX, startY - buttonSize - buttonMargin));
    shopBtn->addClickEventListener([this](Ref* sender) {
        showShop();
        });
    panel->addChild(shopBtn, 1);

    // 进攻按钮
    auto attackBtn = Button::create("UI/btn_attack.png", "UI/btn_attack_pressed.png"); // 这里需要替换为实际按钮图片
    if (!attackBtn->getVirtualRenderer()) {
        attackBtn = Button::create();
        attackBtn->setTitleText("Attack");
        attackBtn->setTitleFontSize(14 * _scaleFactor);
        attackBtn->setContentSize(Size(buttonSize, buttonSize));
        attackBtn->setScale9Enabled(true);
    }
    attackBtn->setPosition(Vec2(rightX, startY - 2 * (buttonSize + buttonMargin)));
    attackBtn->addClickEventListener([this](Ref* sender) {
        showMapSelection();
        });
    panel->addChild(attackBtn, 1);

    return panel;
}

// ==================== 资源栏 ====================
Node* UIManager::createResourceBar() {
    auto bar = Node::create();
    float barHeight = 50 * _scaleFactor;
    bar->setContentSize(Size(_visibleSize.width, barHeight));
    bar->setPosition(Vec2(0, _visibleSize.height - barHeight));

    // 半透明背景
    auto bg = LayerColor::create(Color4B(0, 0, 0, 150), _visibleSize.width * 0.4f, barHeight);
    bg->setPosition(Vec2(10 * _scaleFactor, 0));
    bar->addChild(bg, 0);

    float iconSize = 32 * _scaleFactor;
    float startX = 30 * _scaleFactor;
    float centerY = barHeight / 2;

    // 金币图标
    auto goldIcon = Sprite::create("UI/icon_gold.png"); // 这里需要替换为实际金币图标
    if (goldIcon) {
        goldIcon->setScale(iconSize / goldIcon->getContentSize().width);
        goldIcon->setPosition(Vec2(startX, centerY));
        bar->addChild(goldIcon, 1);
    }

    // 金币数量
    int currentGold = 1000; // 这里需要替换为 resource->getGold() 获取实际金币数量
    _goldLabel = Label::createWithTTF(std::to_string(currentGold), "fonts/arial.ttf", 20 * _scaleFactor);
    _goldLabel->setPosition(Vec2(startX + iconSize + 10 * _scaleFactor, centerY));
    _goldLabel->setAnchorPoint(Vec2(0, 0.5f));
    _goldLabel->setColor(Color3B(255, 215, 0)); // 金色
    bar->addChild(_goldLabel, 1);

    float secondX = startX + 150 * _scaleFactor;

    // 圣水图标
    auto elixirIcon = Sprite::create("UI/icon_elixir.png"); // 这里需要替换为实际圣水图标
    if (elixirIcon) {
        elixirIcon->setScale(iconSize / elixirIcon->getContentSize().width);
        elixirIcon->setPosition(Vec2(secondX, centerY));
        bar->addChild(elixirIcon, 1);
    }

    // 圣水数量
    int currentElixir = 500; // 这里需要替换为 resource->getElixir() 获取实际圣水数量
    _elixirLabel = Label::createWithTTF(std::to_string(currentElixir), "fonts/arial.ttf", 20 * _scaleFactor);
    _elixirLabel->setPosition(Vec2(secondX + iconSize + 10 * _scaleFactor, centerY));
    _elixirLabel->setAnchorPoint(Vec2(0, 0.5f));
    _elixirLabel->setColor(Color3B(200, 100, 255)); // 紫色
    bar->addChild(_elixirLabel, 1);

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

    // 音乐音量滑块
    float sliderY = panelSize.height - 100 * _scaleFactor;
    auto musicLabel = Label::createWithTTF("Music Volume", "fonts/arial.ttf", 18 * _scaleFactor);
    musicLabel->setPosition(Vec2(30 * _scaleFactor, sliderY));
    musicLabel->setAnchorPoint(Vec2(0, 0.5f));
    musicLabel->setColor(Color3B::WHITE);
    panel->addChild(musicLabel, 1);

    auto musicSlider = Slider::create();
    musicSlider->loadBarTexture("UI/slider_bg.png"); // 这里需要替换为实际滑块背景图片
    musicSlider->loadProgressBarTexture("UI/slider_progress.png"); // 这里需要替换为实际滑块进度图片
    musicSlider->loadSlidBallTextures("UI/slider_ball.png"); // 这里需要替换为实际滑块按钮图片
    musicSlider->setPosition(Vec2(panelSize.width - 120 * _scaleFactor, sliderY));
    musicSlider->setPercent(80); // 这里需要替换为 audioManager->getMusicVolume() * 100
    musicSlider->addEventListener([](Ref* sender, Slider::EventType type) {
        if (type == Slider::EventType::ON_PERCENTAGE_CHANGED) {
            auto slider = dynamic_cast<Slider*>(sender);
            float volume = slider->getPercent() / 100.0f;
            // 这里需要替换为 audioManager->setMusicVolume(volume);
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
    sfxSlider->loadBarTexture("UI/slider_bg.png"); // 这里需要替换为实际滑块背景图片
    sfxSlider->loadProgressBarTexture("UI/slider_progress.png"); // 这里需要替换为实际滑块进度图片
    sfxSlider->loadSlidBallTextures("UI/slider_ball.png"); // 这里需要替换为实际滑块按钮图片
    sfxSlider->setPosition(Vec2(panelSize.width - 120 * _scaleFactor, sfxSliderY));
    sfxSlider->setPercent(80); // 这里需要替换为 audioManager->getSFXVolume() * 100
    sfxSlider->addEventListener([](Ref* sender, Slider::EventType type) {
        if (type == Slider::EventType::ON_PERCENTAGE_CHANGED) {
            auto slider = dynamic_cast<Slider*>(sender);
            float volume = slider->getPercent() / 100.0f;
            // 这里需要替换为 audioManager->setSFXVolume(volume);
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

    // 建筑列表数据 - 这里需要替换为从配置文件或BuildingManager获取
    struct BuildingData {
        std::string name;
        std::string icon;
        int cost;
    };
    std::vector<BuildingData> buildings = {
        {"Town Hall", "UI/building_townhall.png", 500},
        {"Gold Mine", "UI/building_goldmine.png", 200},
        {"Elixir Collector", "UI/building_elixir.png", 200},
        {"Barracks", "UI/building_barracks.png", 300},
        {"Cannon", "UI/building_cannon.png", 400},
        {"Archer Tower", "UI/building_archer.png", 450},
        {"Wall", "UI/building_wall.png", 50},
        {"Gold Storage", "UI/building_goldstorage.png", 250},
        {"Elixir Storage", "UI/building_elixirstorage.png", 250},
    };

    float itemHeight = 80 * _scaleFactor;
    float itemWidth = scrollView->getContentSize().width;
    float totalHeight = buildings.size() * itemHeight;

    auto container = Node::create();
    container->setContentSize(Size(itemWidth, std::max(totalHeight, scrollView->getContentSize().height)));
    scrollView->setInnerContainerSize(container->getContentSize());
    scrollView->addChild(container);

    for (size_t i = 0; i < buildings.size(); i++) {
        auto& data = buildings[i];
        float itemY = container->getContentSize().height - (i + 1) * itemHeight + itemHeight / 2;

        // 建筑项背景
        auto itemBg = LayerColor::create(Color4B(60, 60, 80, 200), itemWidth - 10 * _scaleFactor, itemHeight - 5 * _scaleFactor);
        itemBg->setPosition(Vec2(5 * _scaleFactor, itemY - itemHeight / 2 + 2.5f * _scaleFactor));
        container->addChild(itemBg, 0);

        // 建筑图标
        auto icon = Sprite::create(data.icon);
        if (!icon) {
            icon = Sprite::create();
            icon->setTextureRect(Rect(0, 0, 50 * _scaleFactor, 50 * _scaleFactor));
            icon->setColor(Color3B(100, 100, 100));
        }
        icon->setPosition(Vec2(50 * _scaleFactor, itemY));
        icon->setScale(50 * _scaleFactor / std::max(icon->getContentSize().width, icon->getContentSize().height));
        container->addChild(icon, 1);

        // 建筑名称
        auto nameLabel = Label::createWithTTF(data.name, "fonts/arial.ttf", 18 * _scaleFactor);
        nameLabel->setPosition(Vec2(100 * _scaleFactor, itemY + 10 * _scaleFactor));
        nameLabel->setAnchorPoint(Vec2(0, 0.5f));
        nameLabel->setColor(Color3B::WHITE);
        container->addChild(nameLabel, 1);

        // 建造成本
        auto costLabel = Label::createWithTTF("Cost: " + std::to_string(data.cost), "fonts/arial.ttf", 14 * _scaleFactor);
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

        std::string buildingName = data.name;
        int buildingCost = data.cost;
        buyBtn->addClickEventListener([this, buildingName, buildingCost](Ref* sender) {
            showPurchaseConfirm(buildingName, buildingCost, [this, buildingName]() {
                hidePanel(UIPanelType::Shop, true);
                // 这里需要替换为进入建筑放置模式的逻辑
                // map->enterPlacementMode(buildingName);
                showToast("Select a location to place " + buildingName);
                triggerUIEvent("OnEnterPlacementMode");
                });
            });
        container->addChild(buyBtn, 1);
    }

    return panel;
}

void UIManager::showShop() {
    showPanel(UIPanelType::Shop, UILayer::Dialog, true);
}

void UIManager::showPurchaseConfirm(const std::string& buildingName, int cost, const std::function<void()>& onConfirm) {
    std::string content = "Purchase " + buildingName + " for " + std::to_string(cost) + " gold?";

    int currentGold = 1000; // 这里需要替换为 resource->getGold() 获取实际金币

    if (currentGold >= cost) {
        showConfirmDialog("Confirm Purchase", content, [this, cost, onConfirm]() {
            // 这里需要替换为 resource->spendGold(cost);
            if (onConfirm) onConfirm();
            });
    }
    else {
        showInfoDialog("Insufficient Gold", "You don't have enough gold to purchase this building.");
    }
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
    auto bg = Sprite::create("UI/map_selection_bg.png"); // 这里需要替换为实际的地图选择背景图
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
    auto node1 = Button::create("UI/map_node.png", "UI/map_node_pressed.png"); // 这里需要替换为实际节点图片
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
    auto node2 = Button::create("UI/map_node.png", "UI/map_node_pressed.png"); // 这里需要替换为实际节点图片
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
    auto infoBtn = Button::create("UI/btn_info.png", "UI/btn_info_pressed.png"); // 这里需要替换为实际按钮图片
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
    auto upgradeBtn = Button::create("UI/btn_upgrade.png", "UI/btn_upgrade_pressed.png"); // 这里需要替换为实际按钮图片
    if (!upgradeBtn->getVirtualRenderer()) {
        upgradeBtn = Button::create();
        upgradeBtn->setTitleText("Up");
        upgradeBtn->setTitleFontSize(12 * _scaleFactor);
        upgradeBtn->setContentSize(Size(buttonSize, buttonSize));
        upgradeBtn->setScale9Enabled(true);
    }
    upgradeBtn->setPosition(Vec2(currentX, centerY));
    upgradeBtn->addClickEventListener([this](Ref* sender) {
        hidePanel(UIPanelType::BuildingOptions, true);
        showBuildingUpgrade(_selectedBuilding);
        });
    panel->addChild(upgradeBtn, 1);

    currentX += buttonSize + buttonMargin;

    // 训练按钮（仅军营）
    if (category == BuildingCategory::Military) {
        auto trainBtn = Button::create("UI/btn_train.png", "UI/btn_train_pressed.png"); // 这里需要替换为实际按钮图片
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
    std::string buildingName = building ? "Building" : "Unknown"; // 这里需要替换为 building->getName()
    if (building) {
        // buildingName = building->getName(); // 这里需要替换为实际获取建筑名称的方法
    }
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
    auto buildingSprite = Sprite::create("UI/building_placeholder.png"); // 这里需要替换为实际建筑图片
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
    int level = building ? building->GetLevel() : 1;
    int health = building ? building->GetHealth() : 100;
    int maxHealth = building ? building->GetMaxHealth() : 100;
    int defense = building ? building->GetDefense() : 10;

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
    std::string buildingName = "Building"; // 这里需要替换为 building->getName()
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
    auto buildingSprite = Sprite::create("UI/building_placeholder.png"); // 这里需要替换为实际建筑图片
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
    int currentLevel = building ? building->GetLevel() : 1;
    int nextLevel = currentLevel + 1;
    int currentHealth = building ? building->GetMaxHealth() : 100;
    int nextHealth = currentHealth + 50; // 这里需要替换为实际升级后的生命值
    int currentDefense = building ? building->GetDefense() : 10;
    int nextDefense = currentDefense + 5; // 这里需要替换为实际升级后的防御值
    int upgradeCost = building ? building->GetBuildCost() * currentLevel : 500; // 这里需要替换为实际升级成本
    int upgradeTime = building ? building->GetBuildTime() : 60; // 这里需要替换为实际升级时间

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
    auto costLabel = Label::createWithTTF("Cost: " + std::to_string(upgradeCost) + " Gold", "fonts/arial.ttf", 18 * _scaleFactor);
    costLabel->setPosition(Vec2(rightX, startY - 4 * lineHeight));
    costLabel->setAnchorPoint(Vec2(0, 0.5f));
    costLabel->setColor(Color3B(255, 215, 0));
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
    int currentGold = 1000; // 这里需要替换为 resource->getGold() 获取实际金币
    bool canAfford = currentGold >= upgradeCost;

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

            // 这里需要替换为实际扣除资源和开始升级的逻辑
            // resource->spendGold(upgradeCost);
            // _selectedBuilding->startUpgrade(upgradeTime);

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

// ==================== 军队训练面板（续） ====================
Node* UIManager::createArmyTraining(Building* building) {
    auto panel = Node::create();

    Size panelSize(600 * _scaleFactor, 400 * _scaleFactor);
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
    divider->drawLine(Vec2(panelSize.width / 2, 60 * _scaleFactor),
        Vec2(panelSize.width / 2, panelSize.height - 60 * _scaleFactor),
        Color4F(1, 1, 1, 0.5f));
    panel->addChild(divider, 1);

    // 左侧标题：当前部队
    auto leftTitle = Label::createWithTTF("Current Army", "fonts/arial.ttf", 18 * _scaleFactor);
    leftTitle->setPosition(Vec2(panelSize.width / 4, panelSize.height - 70 * _scaleFactor));
    leftTitle->setColor(Color3B(200, 200, 200));
    panel->addChild(leftTitle, 1);

    // 右侧标题：可用士兵
    auto rightTitle = Label::createWithTTF("Available Troops", "fonts/arial.ttf", 18 * _scaleFactor);
    rightTitle->setPosition(Vec2(panelSize.width * 3 / 4, panelSize.height - 70 * _scaleFactor));
    rightTitle->setColor(Color3B(200, 200, 200));
    panel->addChild(rightTitle, 1);

    // 士兵类型数据 - 这里需要替换为从配置或SoldierManager获取
    struct SoldierType {
        std::string name;
        std::string icon;
        int available;  // 可用数量
        int assigned;   // 已分配数量
    };

    // 这里需要替换为实际获取士兵数据的逻辑
    std::vector<SoldierType> soldiers = {
        {"Barbarian", "UI/soldier_barbarian.png", 10, 2},
        {"Archer", "UI/soldier_archer.png", 8, 3},
        {"Bomber", "UI/soldier_bomber.png", 4, 0},
        {"Giant", "UI/soldier_giant.png", 2, 1},
    };

    int soldierLimit = 20; // 这里需要替换为 getSoldierLimit() 获取全局人口上限
    int currentTotal = 0;
    for (const auto& s : soldiers) {
        currentTotal += s.assigned;
    }

    // 人口显示
    auto popLabel = Label::createWithTTF("Population: " + std::to_string(currentTotal) + "/" + std::to_string(soldierLimit),
        "fonts/arial.ttf", 16 * _scaleFactor);
    popLabel->setPosition(Vec2(panelSize.width / 4, 40 * _scaleFactor));
    popLabel->setColor(Color3B(255, 200, 100));
    popLabel->setName("popLabel");
    panel->addChild(popLabel, 1);

    float itemHeight = 60 * _scaleFactor;
    float leftStartY = panelSize.height - 110 * _scaleFactor;
    float rightStartY = panelSize.height - 110 * _scaleFactor;
    float leftWidth = panelSize.width / 2 - 20 * _scaleFactor;
    float rightWidth = panelSize.width / 2 - 20 * _scaleFactor;

    // ===== 左侧：已分配的部队 =====
    auto leftContainer = Node::create();
    leftContainer->setPosition(Vec2(10 * _scaleFactor, 60 * _scaleFactor));
    leftContainer->setContentSize(Size(leftWidth, leftStartY - 60 * _scaleFactor));
    leftContainer->setName("leftContainer");
    panel->addChild(leftContainer, 1);

    // ===== 右侧：可用士兵 =====
    auto rightContainer = Node::create();
    rightContainer->setPosition(Vec2(panelSize.width / 2 + 10 * _scaleFactor, 60 * _scaleFactor));
    rightContainer->setContentSize(Size(rightWidth, rightStartY - 60 * _scaleFactor));
    rightContainer->setName("rightContainer");
    panel->addChild(rightContainer, 1);

    // 创建士兵项的lambda函数
    auto createSoldierItem = [this, itemHeight, panel, soldierLimit](
        Node* container, const std::string& name, const std::string& icon,
        int count, bool isLeft, float yPos, int index) -> Node* {

            auto item = Node::create();
            item->setContentSize(Size(container->getContentSize().width, itemHeight - 5 * _scaleFactor));
            item->setPosition(Vec2(0, yPos));

            // 背景
            auto itemBg = LayerColor::create(Color4B(60, 60, 80, 180),
                item->getContentSize().width, item->getContentSize().height);
            item->addChild(itemBg, 0);

            // 士兵图标
            auto soldierIcon = Sprite::create(icon);
            if (!soldierIcon) {
                soldierIcon = Sprite::create();
                soldierIcon->setTextureRect(Rect(0, 0, 40 * _scaleFactor, 40 * _scaleFactor));
                soldierIcon->setColor(Color3B(100, 150, 100));
            }
            soldierIcon->setPosition(Vec2(30 * _scaleFactor, item->getContentSize().height / 2));
            soldierIcon->setScale(40 * _scaleFactor / std::max(soldierIcon->getContentSize().width,
                soldierIcon->getContentSize().height));
            item->addChild(soldierIcon, 1);

            // 士兵名称
            auto nameLabel = Label::createWithTTF(name, "fonts/arial.ttf", 14 * _scaleFactor);
            nameLabel->setPosition(Vec2(70 * _scaleFactor, item->getContentSize().height / 2 + 8 * _scaleFactor));
            nameLabel->setAnchorPoint(Vec2(0, 0.5f));
            nameLabel->setColor(Color3B::WHITE);
            item->addChild(nameLabel, 1);

            // 数量
            auto countLabel = Label::createWithTTF("x" + std::to_string(count), "fonts/arial.ttf", 14 * _scaleFactor);
            countLabel->setPosition(Vec2(70 * _scaleFactor, item->getContentSize().height / 2 - 8 * _scaleFactor));
            countLabel->setAnchorPoint(Vec2(0, 0.5f));
            countLabel->setColor(Color3B(200, 200, 200));
            countLabel->setName("countLabel");
            item->addChild(countLabel, 1);

            // 根据是左侧还是右侧设置颜色和交互
            if (isLeft) {
                // 左侧：已分配，点击可移除
                if (count > 0) {
                    itemBg->setColor(Color3B(80, 100, 80));

                    auto touchListener = EventListenerTouchOneByOne::create();
                    touchListener->setSwallowTouches(true);
                    touchListener->onTouchBegan = [item](Touch* touch, Event* event) -> bool {
                        Vec2 locationInNode = item->convertToNodeSpace(touch->getLocation());
                        Rect rect(Vec2::ZERO, item->getContentSize());
                        return rect.containsPoint(locationInNode);
                        };
                    touchListener->onTouchEnded = [this, name, index, panel](Touch* touch, Event* event) {
                        // 这里需要替换为实际移除士兵的逻辑
                        // soldierManager->removeSoldierFromArmy(name, 1);
                        CCLOG("Remove soldier: %s", name.c_str());
                        showToast("Removed 1 " + name);

                        // 刷新面板 - 这里简化处理，实际应该更新数据并刷新UI
                        triggerUIEvent("OnArmyChanged");
                        };
                    Director::getInstance()->getEventDispatcher()->addEventListenerWithSceneGraphPriority(touchListener, item);
                }
            }
            else {
                // 右侧：可用，点击可添加
                if (count > 0) {
                    itemBg->setColor(Color3B(80, 80, 100));

                    auto touchListener = EventListenerTouchOneByOne::create();
                    touchListener->setSwallowTouches(true);
                    touchListener->onTouchBegan = [item](Touch* touch, Event* event) -> bool {
                        Vec2 locationInNode = item->convertToNodeSpace(touch->getLocation());
                        Rect rect(Vec2::ZERO, item->getContentSize());
                        return rect.containsPoint(locationInNode);
                        };
                    touchListener->onTouchEnded = [this, name, index, panel, soldierLimit](Touch* touch, Event* event) {
                        // 检查是否超过人口上限
                        // int currentPop = soldierManager->getCurrentPopulation(); // 这里需要替换为实际获取当前人口
                        int currentPop = 6; // 临时值
                        if (currentPop >= soldierLimit) {
                            showToast("Population limit reached!");
                            return;
                        }

                        // 这里需要替换为实际添加士兵的逻辑
                        // soldierManager->addSoldierToArmy(name, 1);
                        CCLOG("Add soldier: %s", name.c_str());
                        showToast("Added 1 " + name);

                        triggerUIEvent("OnArmyChanged");
                        };
                    Director::getInstance()->getEventDispatcher()->addEventListenerWithSceneGraphPriority(touchListener, item);
                }
                else {
                    // 不可用 - 暗淡显示
                    itemBg->setColor(Color3B(40, 40, 50));
                    soldierIcon->setColor(Color3B(80, 80, 80));
                    nameLabel->setColor(Color3B(100, 100, 100));
                    countLabel->setColor(Color3B(80, 80, 80));
                }
            }

            container->addChild(item, 1);
            return item;
        };

    // 填充左侧（已分配）
    float leftY = leftContainer->getContentSize().height - itemHeight;
    for (size_t i = 0; i < soldiers.size(); i++) {
        if (soldiers[i].assigned > 0) {
            createSoldierItem(leftContainer, soldiers[i].name, soldiers[i].icon,
                soldiers[i].assigned, true, leftY, (int)i);
            leftY -= itemHeight;
        }
    }

    // 填充右侧（可用）
    float rightY = rightContainer->getContentSize().height - itemHeight;
    for (size_t i = 0; i < soldiers.size(); i++) {
        createSoldierItem(rightContainer, soldiers[i].name, soldiers[i].icon,
            soldiers[i].available, false, rightY, (int)i);
        rightY -= itemHeight;
    }

    return panel;
}

// ==================== 战斗HUD ====================
Node* UIManager::createBattleHUD() {
    auto panel = Node::create();
    panel->setContentSize(_visibleSize);
    panel->setPosition(_visibleOrigin);

    // 顶部信息栏
    auto topBar = LayerColor::create(Color4B(0, 0, 0, 150), _visibleSize.width, 60 * _scaleFactor);
    topBar->setPosition(Vec2(0, _visibleSize.height - 60 * _scaleFactor));
    panel->addChild(topBar, 1);

    // 倒计时
    auto timerLabel = Label::createWithTTF("3:00", "fonts/arial.ttf", 28 * _scaleFactor);
    timerLabel->setPosition(Vec2(_visibleSize.width / 2, _visibleSize.height - 30 * _scaleFactor));
    timerLabel->setColor(Color3B::WHITE);
    timerLabel->setName("timerLabel");
    panel->addChild(timerLabel, 2);

    // 星级进度（3颗星）
    float starSize = 30 * _scaleFactor;
    float starStartX = 50 * _scaleFactor;
    for (int i = 0; i < 3; i++) {
        auto star = Sprite::create("UI/star_empty.png"); // 这里需要替换为实际星星图片
        if (!star) {
            star = Sprite::create();
            auto starDraw = DrawNode::create();
            starDraw->drawSolidCircle(Vec2::ZERO, starSize / 2, 0, 5, Color4F(0.3f, 0.3f, 0.3f, 1));
            star->addChild(starDraw);
            star->setContentSize(Size(starSize, starSize));
        }
        star->setPosition(Vec2(starStartX + i * (starSize + 10 * _scaleFactor),
            _visibleSize.height - 30 * _scaleFactor));
        star->setName("star_" + std::to_string(i));
        panel->addChild(star, 2);
    }

    // 摧毁百分比
    auto destroyLabel = Label::createWithTTF("0%", "fonts/arial.ttf", 20 * _scaleFactor);
    destroyLabel->setPosition(Vec2(_visibleSize.width - 80 * _scaleFactor, _visibleSize.height - 30 * _scaleFactor));
    destroyLabel->setColor(Color3B(255, 200, 100));
    destroyLabel->setName("destroyLabel");
    panel->addChild(destroyLabel, 2);

    // 底部士兵部署栏
    auto bottomBar = LayerColor::create(Color4B(0, 0, 0, 180), _visibleSize.width, 80 * _scaleFactor);
    bottomBar->setPosition(Vec2(0, 0));
    panel->addChild(bottomBar, 1);

    // 士兵部署按钮 - 这里需要替换为实际的士兵数据
    struct DeploySoldier {
        std::string name;
        std::string icon;
        int count;
    };
    std::vector<DeploySoldier> deployTroops = {
        {"Barbarian", "UI/soldier_barbarian.png", 5},
        {"Archer", "UI/soldier_archer.png", 3},
        {"Bomber", "UI/soldier_bomber.png", 2},
        {"Giant", "UI/soldier_giant.png", 1},
    };

    float btnSize = 60 * _scaleFactor;
    float btnMargin = 15 * _scaleFactor;
    float totalWidth = deployTroops.size() * btnSize + (deployTroops.size() - 1) * btnMargin;
    float startX = (_visibleSize.width - totalWidth) / 2 + btnSize / 2;

    for (size_t i = 0; i < deployTroops.size(); i++) {
        auto& troop = deployTroops[i];

        auto btn = Button::create(troop.icon, troop.icon); // 这里需要替换为实际图片
        if (!btn->getVirtualRenderer()) {
            btn = Button::create();
            btn->setContentSize(Size(btnSize, btnSize));
            btn->setScale9Enabled(true);

            auto placeholder = DrawNode::create();
            placeholder->drawSolidRect(Vec2(-btnSize / 2, -btnSize / 2), Vec2(btnSize / 2, btnSize / 2),
                Color4F(0.3f, 0.4f, 0.3f, 1));
            btn->addChild(placeholder, -1);
        }
        btn->setPosition(Vec2(startX + i * (btnSize + btnMargin), 40 * _scaleFactor));
        btn->setName("deployBtn_" + std::to_string(i));

        // 数量标签
        auto countLabel = Label::createWithTTF(std::to_string(troop.count), "fonts/arial.ttf", 14 * _scaleFactor);
        countLabel->setPosition(Vec2(btnSize / 2 - 5 * _scaleFactor, -btnSize / 2 + 10 * _scaleFactor));
        countLabel->setColor(Color3B::WHITE);
        countLabel->setName("count");
        btn->addChild(countLabel, 1);

        std::string troopName = troop.name;
        btn->addClickEventListener([this, troopName, i](Ref* sender) {
            // 这里需要替换为实际选中士兵的逻辑
            // battleManager->selectTroopType(troopName);
            CCLOG("Selected troop: %s", troopName.c_str());
            triggerUIEvent("OnTroopSelected_" + std::to_string(i));
            });

        panel->addChild(btn, 2);
    }

    // 结束战斗按钮
    auto endBtn = Button::create();
    endBtn->setTitleText("End");
    endBtn->setTitleFontSize(16 * _scaleFactor);
    endBtn->setContentSize(Size(60 * _scaleFactor, 40 * _scaleFactor));
    endBtn->setScale9Enabled(true);
    endBtn->setPosition(Vec2(_visibleSize.width - 50 * _scaleFactor, 40 * _scaleFactor));
    endBtn->addClickEventListener([this](Ref* sender) {
        showConfirmDialog("End Battle", "Are you sure you want to end the battle?", [this]() {
            // 这里需要替换为实际结束战斗的逻辑
            triggerUIEvent("OnBattleEnd");
            });
        });
    panel->addChild(endBtn, 2);

    return panel;
}

// ==================== 战斗结果面板 ====================
Node* UIManager::createBattleResult() {
    auto panel = Node::create();

    Size panelSize(500 * _scaleFactor, 400 * _scaleFactor);
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

    // 标题（胜利/失败）- 这里需要替换为实际战斗结果
    bool isVictory = true; // 这里需要替换为 battleManager->isVictory()
    std::string resultText = isVictory ? "Victory!" : "Defeat";
    Color3B resultColor = isVictory ? Color3B(100, 255, 100) : Color3B(255, 100, 100);

    auto title = Label::createWithTTF(resultText, "fonts/arial.ttf", 36 * _scaleFactor);
    title->setPosition(Vec2(panelSize.width / 2, panelSize.height - 50 * _scaleFactor));
    title->setColor(resultColor);
    panel->addChild(title, 1);

    // 星级显示
    int stars = 2; // 这里需要替换为 battleManager->getStars()
    float starSize = 40 * _scaleFactor;
    float starY = panelSize.height - 120 * _scaleFactor;
    for (int i = 0; i < 3; i++) {
        auto star = Sprite::create(i < stars ? "UI/star_full.png" : "UI/star_empty.png");
        if (!star) {
            star = Sprite::create();
            auto starDraw = DrawNode::create();
            Color4F starColor = (i < stars) ? Color4F(1, 0.8f, 0, 1) : Color4F(0.3f, 0.3f, 0.3f, 1);
            starDraw->drawSolidCircle(Vec2::ZERO, starSize / 2, 0, 5, starColor);
            star->addChild(starDraw);
            star->setContentSize(Size(starSize, starSize));
        }
        star->setPosition(Vec2(panelSize.width / 2 + (i - 1) * (starSize + 10 * _scaleFactor), starY));
        panel->addChild(star, 1);
    }

    // 统计信息
    float lineHeight = 35 * _scaleFactor;
    float infoStartY = panelSize.height - 180 * _scaleFactor;
    float leftX = 80 * _scaleFactor;

    // 摧毁百分比
    int destroyPercent = 75; // 这里需要替换为 battleManager->getDestroyPercent()
    auto destroyLabel = Label::createWithTTF("Destruction: " + std::to_string(destroyPercent) + "%",
        "fonts/arial.ttf", 20 * _scaleFactor);
    destroyLabel->setPosition(Vec2(leftX, infoStartY));
    destroyLabel->setAnchorPoint(Vec2(0, 0.5f));
    destroyLabel->setColor(Color3B::WHITE);
    panel->addChild(destroyLabel, 1);

    // 获得金币
    int goldEarned = 500; // 这里需要替换为 battleManager->getGoldEarned()
    auto goldLabel = Label::createWithTTF("Gold: +" + std::to_string(goldEarned),
        "fonts/arial.ttf", 20 * _scaleFactor);
    goldLabel->setPosition(Vec2(leftX, infoStartY - lineHeight));
    goldLabel->setAnchorPoint(Vec2(0, 0.5f));
    goldLabel->setColor(Color3B(255, 215, 0));
    panel->addChild(goldLabel, 1);

    // 获得圣水
    int elixirEarned = 300; // 这里需要替换为 battleManager->getElixirEarned()
    auto elixirLabel = Label::createWithTTF("Elixir: +" + std::to_string(elixirEarned),
        "fonts/arial.ttf", 20 * _scaleFactor);
    elixirLabel->setPosition(Vec2(leftX, infoStartY - 2 * lineHeight));
    elixirLabel->setAnchorPoint(Vec2(0, 0.5f));
    elixirLabel->setColor(Color3B(200, 100, 255));
    panel->addChild(elixirLabel, 1);

    // 返回主城按钮
    auto returnBtn = Button::create();
    returnBtn->setTitleText("Return to Village");
    returnBtn->setTitleFontSize(20 * _scaleFactor);
    returnBtn->setContentSize(Size(200 * _scaleFactor, 50 * _scaleFactor));
    returnBtn->setScale9Enabled(true);
    returnBtn->setPosition(Vec2(panelSize.width / 2, 60 * _scaleFactor));
    returnBtn->addClickEventListener([this](Ref* sender) {
        hidePanel(UIPanelType::BattleResult, true);
        // 这里需要替换为返回主村庄的逻辑
        triggerUIEvent("OnReturnToVillage");
        });
    panel->addChild(returnBtn, 1);

    return panel;
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