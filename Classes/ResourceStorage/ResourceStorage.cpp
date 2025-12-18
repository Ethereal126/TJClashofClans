//
// Created by Faith_Oriest on 2025/12/16.
//

#include "ResourceStorage/ResourceStorage.h"
#include "TownHall/TownHall.h"
#include "cocos2d.h"
#include <string>
#include <cmath>

USING_NS_CC;

// ==================== ResourceStorage 实现 ====================

ResourceStorage::~ResourceStorage() {
    // 清理资源
    if (storageEffect_) storageEffect_->release();
    if (particleEffect_) particleEffect_->removeFromParent();
    if (uiIcon_) uiIcon_->removeFromParent();
    if (uiLabel_) uiLabel_->removeFromParent();
    if (progressBar_) progressBar_->removeFromParent();
}

ResourceStorage::ResourceStorage(const std::string& name, int base, cocos2d::Vec2 position,
    const std::string& texture, const std::string& resourceType)
    : Building(name, 1, base * 8, base, 0, base * 10, 3, 3, position)
    , resourceType_(resourceType)
    , capacity_(base * 100)
    , currentAmount_(0)
    , isActive_(true)
    , uiIcon_(nullptr)
    , uiLabel_(nullptr)
    , progressBar_(nullptr)
    , storageEffect_(nullptr)
    , particleEffect_(nullptr) {
}

ResourceStorage* ResourceStorage::Create(const std::string& name, int base, cocos2d::Vec2 position,
    const std::string& texture, const std::string& resourceType) {
    auto storage = new (std::nothrow) ResourceStorage(name, base, position, texture, resourceType);
    if (storage && storage->Init()) {
        storage->autorelease();
        return storage;
    }
    CC_SAFE_DELETE(storage);
    return nullptr;
}

bool ResourceStorage::Init() {
    if (!Building::initWithFile("textures/" + GetName() + ".png")) {
        return false;
    }

    InitUIComponents();
    InitAnimations();

    // 设置建筑尺寸和位置
    setContentSize(Size(GetWidth() * 32, GetLength() * 32));
    setAnchorPoint(Vec2(0.5f, 0.5f));

    return true;
}

bool ResourceStorage::AddResource(int amount) {
    if (amount <= 0 || !isActive_) {
        return false;
    }

    int oldAmount = currentAmount_;
    currentAmount_ = std::min(currentAmount_ + amount, capacity_);

    // 触发回调
    TriggerAmountChanged(oldAmount, currentAmount_);

    // 更新UI
    UpdateUI();

    // 如果满了，触发满容量回调
    if (currentAmount_ >= capacity_) {
        TriggerCapacityFull();
        PlayFullCapacityAnimation();
    }
    else {
        PlayStorageAnimation();
    }

    return true;
}

bool ResourceStorage::UseResource(int amount) {
    if (amount <= 0 || !isActive_) {
        return false;
    }

    if (currentAmount_ < amount) {
        PlayNotEnoughAnimation();
        return false;
    }

    int oldAmount = currentAmount_;
    currentAmount_ -= amount;

    // 触发回调
    TriggerAmountChanged(oldAmount, currentAmount_);

    // 更新UI
    UpdateUI();

    return true;
}

bool ResourceStorage::CanAfford(int amount) const {
    return isActive_ && currentAmount_ >= amount;
}

void ResourceStorage::UpgradeCapacity(int additionalCapacity) {
    capacity_ += additionalCapacity;
    UpdateUI();
}

float ResourceStorage::GetFillPercentage() const {
    if (capacity_ <= 0) return 0.0f;
    return static_cast<float>(currentAmount_) / static_cast<float>(capacity_);
}

void ResourceStorage::UpdateUI() {
    if (uiLabel_) {
        char buffer[50];
        snprintf(buffer, sizeof(buffer), "%s: %d/%d",
            resourceType_.c_str(), currentAmount_, capacity_);
        uiLabel_->setString(buffer);
    }

    if (progressBar_) {
        progressBar_->setPercentage(GetFillPercentage() * 100.0f);
    }
}

void ResourceStorage::PlayStorageAnimation() {
    if (storageEffect_) {
        stopAction(storageEffect_);
    }

    // 创建存储动画
    auto scaleUp = ScaleTo::create(0.1f, 1.05f);
    auto scaleDown = ScaleTo::create(0.1f, 1.0f);
    auto sequence = Sequence::create(scaleUp, scaleDown, nullptr);
    storageEffect_ = sequence;
    storageEffect_->retain();

    runAction(sequence);
}

void ResourceStorage::PlayFullCapacityAnimation() {
    if (particleEffect_) {
        particleEffect_->stopSystem();
        particleEffect_->removeFromParent();
    }

    // 创建满容量粒子效果
    particleEffect_ = ParticleSystemQuad::create("particles/full_capacity.plist");
    if (particleEffect_) {
        particleEffect_->setPosition(getContentSize().width / 2, getContentSize().height / 2);
        addChild(particleEffect_, 10);
    }
}

void ResourceStorage::PlayNotEnoughAnimation() {
    auto tintRed = TintTo::create(0.1f, 255, 100, 100);
    auto tintBack = TintTo::create(0.1f, 255, 255, 255);
    auto sequence = Sequence::create(tintRed, tintBack, nullptr);
    runAction(sequence);

    // 震动效果
    auto moveRight = MoveBy::create(0.05f, Vec2(5, 0));
    auto moveLeft = MoveBy::create(0.05f, Vec2(-10, 0));
    auto moveCenter = MoveBy::create(0.05f, Vec2(5, 0));
    auto shake = Sequence::create(moveRight, moveLeft, moveCenter, nullptr);
    runAction(shake);
}

void ResourceStorage::Upgrade() {
    // 保存旧等级
    int oldLevel = GetLevel();

    // 调用基类升级
    Building::Upgrade();

    // 容量随等级提升
    int newCapacity = capacity_ * 2;
    UpgradeCapacity(newCapacity - capacity_);

    // 更新UI
    UpdateUI();
}

void ResourceStorage::ShowInfo() const {
    Building::ShowInfo();

    CCLOG("Resource Type: %s", resourceType_.c_str());
    CCLOG("Capacity: %d", capacity_);
    CCLOG("Current Amount: %d", currentAmount_);
    CCLOG("Fill Percentage: %.1f%%", GetFillPercentage() * 100.0f);
}

void ResourceStorage::InitUIComponents() {
    // 创建UI标签
    uiLabel_ = Label::createWithTTF("", "fonts/Marker Felt.ttf", 16);
    uiLabel_->setPosition(Vec2(getContentSize().width / 2, -20));
    uiLabel_->setTextColor(Color4B::WHITE);
    addChild(uiLabel_, 100);

    // 创建进度条背景
    auto progressBg = Sprite::create("ui/progress_bg.png");
    progressBg->setPosition(Vec2(getContentSize().width / 2, -50));
    addChild(progressBg, 99);

    // 创建进度条
    auto progressFg = Sprite::create("ui/progress_fg.png");
    progressBar_ = ProgressTimer::create(progressFg);
    progressBar_->setType(ProgressTimer::Type::BAR);
    progressBar_->setMidpoint(Vec2(0, 0.5f));
    progressBar_->setBarChangeRate(Vec2(1, 0));
    progressBar_->setPercentage(0);
    progressBar_->setPosition(Vec2(getContentSize().width / 2, -50));
    addChild(progressBar_, 100);
}

void ResourceStorage::InitAnimations() {
    // 初始化动画相关
    storageEffect_ = nullptr;
    particleEffect_ = nullptr;
}

void ResourceStorage::SetOnAmountChangedCallback(const std::function<void(int, int)>& callback) {
    onAmountChangedCallback_ = callback;
}

void ResourceStorage::SetOnCapacityFullCallback(const std::function<void()>& callback) {
    onCapacityFullCallback_ = callback;
}

void ResourceStorage::SetUIIcon(const std::string& iconPath) {
    if (uiIcon_) {
        uiIcon_->removeFromParent();
    }

    uiIcon_ = Sprite::create(iconPath);
    if (uiIcon_) {
        uiIcon_->setPosition(Vec2(30, getContentSize().height - 30));
        uiIcon_->setScale(0.5f);
        addChild(uiIcon_, 101);
    }
}

void ResourceStorage::SetUILabel(Label* label) {
    if (uiLabel_) {
        uiLabel_->removeFromParent();
    }
    uiLabel_ = label;
    if (uiLabel_) {
        uiLabel_->retain();
        addChild(uiLabel_, 100);
    }
}

void ResourceStorage::SetProgressBar(ProgressTimer* progressBar) {
    if (progressBar_) {
        progressBar_->removeFromParent();
    }
    progressBar_ = progressBar;
    if (progressBar_) {
        progressBar_->retain();
        addChild(progressBar_, 100);
    }
}

void ResourceStorage::TriggerAmountChanged(int oldAmount, int newAmount) {
    if (onAmountChangedCallback_) {
        onAmountChangedCallback_(oldAmount, newAmount);
    }
}

void ResourceStorage::TriggerCapacityFull() {
    if (onCapacityFullCallback_) {
        onCapacityFullCallback_();
    }
}

// ==================== ProductionBuilding 实现 ====================

ProductionBuilding::~ProductionBuilding() {
    for (auto collector : collectors_) {
        if (collector) collector->removeFromParent();
    }
}

ProductionBuilding::ProductionBuilding(const std::string& name, int base, cocos2d::Vec2 position,
    const std::string& texture, const std::string& resourceType)
    : ResourceStorage(name, base, position, texture, resourceType)
    , productionRate_(base * 10)
    , productionTimer_(0.0f) {
}

ProductionBuilding* ProductionBuilding::Create(const std::string& name, int base, cocos2d::Vec2 position,
    const std::string& texture, const std::string& resourceType) {
    auto building = new (std::nothrow) ProductionBuilding(name, base, position, texture, resourceType);
    if (building && building->Init()) {
        building->autorelease();
        return building;
    }
    CC_SAFE_DELETE(building);
    return nullptr;
}

bool ProductionBuilding::Init() {
    if (!ResourceStorage::Init()) {
        return false;
    }

    InitProductionSystem();
    return true;
}

void ProductionBuilding::StartProduction() {
    if (!isActive_) return;

    // 启动生产定时器
    schedule([this](float dt) {
        OnProductionUpdate(dt);
        }, 1.0f, "production_timer");
}

void ProductionBuilding::StopProduction() {
    unschedule("production_timer");
}

void ProductionBuilding::CollectResources() {
    if (!isActive_ || currentAmount_ <= 0) return;

    int collected = currentAmount_;
    currentAmount_ = 0;

    TriggerAmountChanged(currentAmount_ + collected, currentAmount_);
    UpdateUI();

    // 播放收集动画
    PlayStorageAnimation();
}

int ProductionBuilding::GetProductionRate() const {
    return productionRate_;
}

void ProductionBuilding::UpgradeProductionRate(int additionalRate) {
    productionRate_ += additionalRate;
}

void ProductionBuilding::UpgradeCapacity(int additionalCapacity) {
    ResourceStorage::UpgradeCapacity(additionalCapacity);

    // 生产建筑升级时也提升生产速率
    productionRate_ = static_cast<int>(productionRate_ * 1.2f);
}

void ProductionBuilding::PlayStorageAnimation() {
    ResourceStorage::PlayStorageAnimation();

    // 添加额外的粒子效果
    if (currentAmount_ > 0) {
        auto particles = ParticleSystemQuad::create("particles/resource_drop.plist");
        if (particles) {
            particles->setPosition(Vec2::ZERO);
            particles->setAutoRemoveOnFinish(true);
            addChild(particles, 5);
        }
    }
}

void ProductionBuilding::Upgrade() {
    // 保存旧的生产速率
    int oldProductionRate = productionRate_;

    // 调用基类升级
    ResourceStorage::Upgrade();

    // 提升生产速率
    productionRate_ = static_cast<int>(productionRate_ * 1.3f);

    CCLOG("%s升级: 生产速率 %d -> %d", GetName().c_str(), oldProductionRate, productionRate_);
}

void ProductionBuilding::ShowInfo() const {
    ResourceStorage::ShowInfo();

    CCLOG("Production Rate: %d/hour", productionRate_);
    CCLOG("Collectors: %d", static_cast<int>(collectors_.size()));
}

void ProductionBuilding::InitProductionSystem() {
    // 创建收集器
    int collectorCount = GetLevel() + 1;
    for (int i = 0; i < collectorCount; ++i) {
        auto collector = Sprite::create("textures/collector.png");
        if (collector) {
            float angle = 2 * M_PI * i / collectorCount;
            float radius = 40.0f;
            collector->setPosition(Vec2(radius * cos(angle), radius * sin(angle)));
            collector->setScale(0.5f);
            addChild(collector, 10);
            collectors_.push_back(collector);
        }
    }
}

void ProductionBuilding::OnProductionUpdate(float deltaTime) {
    if (!isActive_) return;

    productionTimer_ += deltaTime;

    // 每小时生产一次
    if (productionTimer_ >= 3600.0f) {
        int productionAmount = productionRate_;

        // 根据等级调整产量
        productionAmount = static_cast<int>(productionAmount * (1.0f + 0.1f * GetLevel()));

        AddResource(productionAmount);
        productionTimer_ = 0.0f;
    }
}

// ==================== ElixirStorage 实现 ====================

ElixirStorage* ElixirStorage::Create(std::string name, int base, cocos2d::Vec2 position) {
    auto storage = new (std::nothrow) ElixirStorage(name, base, position);
    if (storage && storage->Init()) {
        storage->autorelease();
        return storage;
    }
    CC_SAFE_DELETE(storage);
    return nullptr;
}

ElixirStorage::~ElixirStorage() {
}

ElixirStorage::ElixirStorage(const std::string& name, int base, cocos2d::Vec2 position)
    : ProductionBuilding(name, base, position, "textures/elixir_storage.png", "Elixir")
    , collectionRadius_(100.0f)
    , elixirColor_(Color4F(0.8f, 0.2f, 0.8f, 1.0f)) {  // 紫色
}

bool ElixirStorage::Init() {
    if (!ProductionBuilding::Init()) {
        return false;
    }

    InitElixirSpecificComponents();
    StartProduction();

    return true;
}

void ElixirStorage::SetCollectionRadius(float radius) {
    collectionRadius_ = radius;
}

float ElixirStorage::GetCollectionRadius() const {
    return collectionRadius_;
}

void ElixirStorage::PlayCollectionAnimation(const Vec2& targetPosition) {
    // 创建圣水收集动画
    auto elixirSprite = Sprite::create("particles/elixir_drop.png");
    if (!elixirSprite) return;

    elixirSprite->setPosition(getPosition());
    getParent()->addChild(elixirSprite, 1000);

    // 移动到收集点
    auto moveTo = MoveTo::create(0.5f, targetPosition);
    auto fadeOut = FadeOut::create(0.2f);
    auto remove = CallFuncN::create([](Node* node) {
        node->removeFromParent();
        });

    auto sequence = Sequence::create(moveTo, fadeOut, remove, nullptr);
    elixirSprite->runAction(sequence);

    // 粒子效果
    auto particles = ParticleSystemQuad::create("particles/elixir_collect.plist");
    if (particles) {
        particles->setPosition(targetPosition);
        particles->setAutoRemoveOnFinish(true);
        getParent()->addChild(particles, 1001);
    }
}

void ElixirStorage::PlayStorageAnimation() {
    ProductionBuilding::PlayStorageAnimation();

    // 圣水特有的波动效果
    auto waveAction = RepeatForever::create(
        Sequence::create(
            ScaleTo::create(1.0f, 1.02f, 0.98f),
            ScaleTo::create(1.0f, 0.98f, 1.02f),
            nullptr
        )
    );
    waveAction->setTag(999);
    runAction(waveAction);
}

void ElixirStorage::Upgrade() {
    int oldCapacity = GetCapacity();
    int oldProductionRate = GetProductionRate();

    ProductionBuilding::Upgrade();

    // 圣水建筑升级额外增加收集半径
    collectionRadius_ += 20.0f;

    CCLOG("圣水储罐升级完成");
    CCLOG("容量: %d -> %d", oldCapacity, GetCapacity());
    CCLOG("生产速率: %d -> %d", oldProductionRate, GetProductionRate());
    CCLOG("收集半径: %.1f", collectionRadius_);
}

void ElixirStorage::ShowInfo() const {
    ProductionBuilding::ShowInfo();

    CCLOG("Collection Radius: %.1f", collectionRadius_);
    CCLOG("Elixir Color: (%.1f, %.1f, %.1f, %.1f)",
        elixirColor_.r, elixirColor_.g, elixirColor_.b, elixirColor_.a);
}

int ElixirStorage::GetNextProductionRate() const {
    return static_cast<int>(productionRate_ * 1.3f);
}

int ElixirStorage::GetNextCapacity() const {
    return static_cast<int>(capacity_ * 2.0f);
}

void ElixirStorage::InitElixirSpecificComponents() {
    // 添加圣水波纹效果
    auto ripple = Sprite::create("effects/elixir_ripple.png");
    if (ripple) {
        ripple->setPosition(getContentSize().width / 2, getContentSize().height / 2);
        ripple->setOpacity(100);

        auto rotate = RotateBy::create(10.0f, 360);
        auto repeatRotate = RepeatForever::create(rotate);
        ripple->runAction(repeatRotate);

        addChild(ripple, -1);
    }

    // 设置圣水颜色特效
    auto tint = TintTo::create(0, elixirColor_.r * 255, elixirColor_.g * 255, elixirColor_.b * 255);
    runAction(tint);
}

// ==================== GoldStorage 实现 ====================

GoldStorage* GoldStorage::Create(std::string name, int base, cocos2d::Vec2 position) {
    auto storage = new (std::nothrow) GoldStorage(name, base, position);
    if (storage && storage->Init()) {
        storage->autorelease();
        return storage;
    }
    CC_SAFE_DELETE(storage);
    return nullptr;
}

GoldStorage::~GoldStorage() {
    if (protectionShield_) protectionShield_->removeFromParent();
    if (shieldEffect_) shieldEffect_->removeFromParent();
}

GoldStorage::GoldStorage(const std::string& name, int base, cocos2d::Vec2 position)
    : ProductionBuilding(name, base, position, "textures/gold_storage.png", "Gold")
    , isVaultProtected_(false)
    , protectionPercentage_(0.3f)
    , protectionShield_(nullptr)
    , shieldEffect_(nullptr) {
}

bool GoldStorage::Init() {
    if (!ProductionBuilding::Init()) {
        return false;
    }

    InitGoldSpecificComponents();
    StartProduction();

    return true;
}

void GoldStorage::ActivateProtection(bool active) {
    isVaultProtected_ = active;

    if (isVaultProtected_) {
        PlayProtectionActivationAnimation();
    }
    else if (protectionShield_) {
        protectionShield_->setVisible(false);
    }
}

bool GoldStorage::IsVaultProtected() const {
    return isVaultProtected_;
}

void GoldStorage::UpgradeProtection(float additionalPercentage) {
    protectionPercentage_ = std::min(protectionPercentage_ + additionalPercentage, 0.8f);
}

float GoldStorage::GetProtectionPercentage() const {
    return protectionPercentage_;
}

int GoldStorage::CalculateRaidLoss(int attemptedSteal) const {
    if (!isVaultProtected_) {
        return std::min(attemptedSteal, GetCurrentAmount());
    }

    // 有保护时，损失减少
    int actualLoss = static_cast<int>(attemptedSteal * (1.0f - protectionPercentage_));
    return std::min(actualLoss, GetCurrentAmount());
}

void GoldStorage::PlayStorageAnimation() {
    ProductionBuilding::PlayStorageAnimation();

    // 金币特有的闪光效果
    auto flash = Sequence::create(
        CallFunc::create([this]() {
            setColor(Color3B(255, 255, 200));  // 金色闪光
            }),
        DelayTime::create(0.1f),
        CallFunc::create([this]() {
            setColor(Color3B::WHITE);
            }),
        nullptr
    );

    runAction(flash);
}

void GoldStorage::PlayProtectionActivationAnimation() {
    if (!protectionShield_) {
        protectionShield_ = Sprite::create("effects/protection_shield.png");
        if (protectionShield_) {
            protectionShield_->setPosition(getContentSize().width / 2, getContentSize().height / 2);
            protectionShield_->setScale(1.2f);
            addChild(protectionShield_, 5);
        }
    }

    protectionShield_->setVisible(true);

    // 护盾激活动画
    auto scaleUp = ScaleTo::create(0.2f, 1.5f);
    auto scaleDown = ScaleTo::create(0.2f, 1.2f);
    auto fadeIn = FadeIn::create(0.3f);
    auto spawn = Spawn::create(scaleUp, fadeIn, nullptr);
    auto sequence = Sequence::create(spawn, scaleDown, nullptr);

    protectionShield_->runAction(sequence);
}

void GoldStorage::PlayRaidDefenseAnimation() {
    // 防御效果粒子
    auto particles = ParticleSystemQuad::create("particles/shield_defense.plist");
    if (particles) {
        particles->setPosition(getContentSize().width / 2, getContentSize().height / 2);
        particles->setAutoRemoveOnFinish(true);
        addChild(particles, 6);
    }

    // 震动效果
    auto shake = Sequence::create(
        MoveBy::create(0.05f, Vec2(5, 0)),
        MoveBy::create(0.05f, Vec2(-10, 0)),
        MoveBy::create(0.05f, Vec2(5, 0)),
        nullptr
    );
    runAction(shake);
}

void GoldStorage::Upgrade() {
    int oldCapacity = GetCapacity();
    int oldProductionRate = GetProductionRate();
    float oldProtection = protectionPercentage_;

    ProductionBuilding::Upgrade();

    // 金币建筑升级提升保护效果
    if (GetLevel() >= 3) {  // 3级开始有保护
        if (!isVaultProtected_) {
            ActivateProtection(true);
        }
        UpgradeProtection(0.1f);
    }

    CCLOG("金币储罐升级完成");
    CCLOG("容量: %d -> %d", oldCapacity, GetCapacity());
    CCLOG("生产速率: %d -> %d", oldProductionRate, GetProductionRate());
    CCLOG("保护效果: %.1f%% -> %.1f%%", oldProtection * 100, protectionPercentage_ * 100);
}

void GoldStorage::ShowInfo() const {
    ProductionBuilding::ShowInfo();

    CCLOG("Vault Protected: %s", isVaultProtected_ ? "Yes" : "No");
    CCLOG("Protection Percentage: %.1f%%", protectionPercentage_ * 100);
}

int GoldStorage::GetNextProductionRate() const {
    return static_cast<int>(productionRate_ * 1.25f);
}

int GoldStorage::GetNextCapacity() const {
    return static_cast<int>(capacity_ * 1.8f);
}

float GoldStorage::GetNextProtectionPercentage() const {
    if (GetLevel() < 2) return protectionPercentage_;
    return std::min(protectionPercentage_ + 0.1f, 0.8f);
}

void GoldStorage::InitGoldSpecificComponents() {
    // 添加金币堆叠视觉效果
    for (int i = 0; i < 3; ++i) {
        auto goldPile = Sprite::create("textures/gold_pile.png");
        if (goldPile) {
            float offsetX = (i - 1) * 20.0f;
            float offsetY = i * 5.0f;
            goldPile->setPosition(getContentSize().width / 2 + offsetX, 20 + offsetY);
            goldPile->setScale(0.5f);
            goldPile->setOpacity(150);
            addChild(goldPile, i);
        }
    }

    // 创建剪切节点用于护盾效果
    shieldEffect_ = ClippingNode::create();
    if (shieldEffect_) {
        auto stencil = DrawNode::create();
        Vec2 vertices[] = {
            Vec2(-50, -50), Vec2(50, -50),
            Vec2(50, 50), Vec2(-50, 50)
        };
        stencil->drawPolygon(vertices, 4, Color4F::GREEN, 1, Color4F::GREEN);

        shieldEffect_->setStencil(stencil);
        shieldEffect_->setAlphaThreshold(0.5f);
        shieldEffect_->setPosition(getContentSize().width / 2, getContentSize().height / 2);
        addChild(shieldEffect_, 4);
    }
}

// ==================== Barracks 实现 ====================

Barracks* Barracks::Create(const std::string& name, int base, cocos2d::Vec2 position) {
    auto barracks = new (std::nothrow) Barracks(name, base, position);
    if (barracks && barracks->Init()) {
        barracks->autorelease();
        return barracks;
    }
    CC_SAFE_DELETE(barracks);
    return nullptr;
}

Barracks::~Barracks() {
    // 清理资源
    for (auto& pair : troopIcons_) {
        if (pair.second) pair.second->removeFromParent();
    }
    for (auto& unit : trainingQueue_) {
        if (unit.icon) unit.icon->removeFromParent();
    }
}

Barracks::Barracks(const std::string& name, int base, cocos2d::Vec2 position)
    : TrainingBuilding(name, base, position, "textures/barracks.png", 5, 30) {
    // 初始化默认兵种
    availableTroops_.push_back("Barbarian");
    availableTroops_.push_back("Archer");

    troopCapacity_["Barbarian"] = 20;
    troopCapacity_["Archer"] = 15;
}

bool Barracks::Init() {
    // 直接调用 Building 的初始化方法
    if (!Sprite::initWithFile("textures/barracks.png")) {
        return false;
    }

    // 设置建筑尺寸和位置
    if (!TrainingBuilding::Init()) {
        return false;
    }

    // 初始化默认兵种（使用基类的方法）
    AddAvailableUnit("Barbarian");
    AddAvailableUnit("Archer");

    // 初始化兵种图标
    for (const auto& troop : GetAvailableUnits()) {
        std::string iconPath = "icons/" + troop + "_icon.png";
        auto icon = CreateTroopIcon(troop, iconPath);
        if (icon) {
            troopIcons_[troop] = icon;
        }
    }

    // 设置容量
    troopCapacity_["Barbarian"] = 20;
    troopCapacity_["Archer"] = 15;

    // 启动训练更新定时器
    schedule([this](float dt) {
        OnTrainingUpdate(dt);
        }, 1.0f, "training_timer");

    return true;
}

bool Barracks::UnlockTroopType(const std::string& troopType, const std::string& iconPath) {
    // 检查是否已经解锁
    const auto& availableUnits = GetAvailableUnits();
    for (const auto& troop : availableUnits) {
        if (troop == troopType) {
            return false;  // 已经解锁
        }
    }

    // 使用基类方法添加兵种
    AddAvailableUnit(troopType);

    // 设置容量
    troopCapacity_[troopType] = 10;  // 默认容量

    // 创建图标
    auto icon = CreateTroopIcon(troopType, iconPath);
    if (icon) {
        troopIcons_[troopType] = icon;
    }

    // 添加到自己的可用兵种列表
    availableTroops_.push_back(troopType);

    return true;
}

void Barracks::AddAvailableUnit(const std::string& unit_type) {
    // 直接访问基类的 available_units_
    available_units_.push_back(unit_type);
}

bool Barracks::IsTroopAvailable(const std::string& troopType) const {
    for (const auto& troop : availableTroops_) {
        if (troop == troopType) {
            return true;
        }
    }
    return false;
}

bool Barracks::StartTraining(const std::string& unitType, int count) {
    // 先调用基类方法
    if (!TrainingBuilding::StartTraining(unitType, count)) {
        return false;
    }

    // 添加到训练队列
    for (int i = 0; i < count; ++i) {
        Sprite* icon = nullptr;
        auto it = troopIcons_.find(unitType);
        if (it != troopIcons_.end()) {
            icon = Sprite::createWithTexture(it->second->getTexture());
            if (icon) {
                icon->setScale(0.5f);
                addChild(icon, 10);
            }
        }

        TrainingUnit unit(unitType, GetTrainingSpeed(), icon);
        trainingQueue_.push_back(unit);
    }

    // 播放训练开始动画
    PlayTrainingStartAnimation(unitType);

    // 更新UI
    UpdateUI();

    return true;
}

void Barracks::CancelTraining(const std::string& unitType, int count) {
    // 先调用基类方法
    TrainingBuilding::CancelTraining(unitType, count);

    // 从训练队列中移除
    int removed = 0;
    for (auto it = trainingQueue_.begin(); it != trainingQueue_.end(); ) {
        if (removed >= count) break;

        if (it->troopType == unitType) {
            if (it->icon) {
                it->icon->removeFromParent();
            }
            it = trainingQueue_.erase(it);
            removed++;
        }
        else {
            ++it;
        }
    }

    UpdateUI();
}

void Barracks::UpgradeTroopCapacity(const std::string& troopType, int additionalCapacity) {
    auto it = troopCapacity_.find(troopType);
    if (it != troopCapacity_.end()) {
        it->second += additionalCapacity;
    }
    else {
        troopCapacity_[troopType] = additionalCapacity;
    }

    UpdateUI();
}

bool Barracks::DeployTroops(const std::string& troopType, int count) {
    auto it = trainedTroops_.find(troopType);
    if (it == trainedTroops_.end() || it->second < count) {
        return false;
    }

    it->second -= count;
    if (it->second <= 0) {
        trainedTroops_.erase(it);
    }

    // 播放部署动画
    PlayTroopDeployAnimation(getPosition());

    UpdateUI();

    return true;
}

int Barracks::GetTrainedTroopCount(const std::string& troopType) const {
    auto it = trainedTroops_.find(troopType);
    return (it != trainedTroops_.end()) ? it->second : 0;
}

int Barracks::GetTroopCapacity(const std::string& troopType) const {
    auto it = troopCapacity_.find(troopType);
    return (it != troopCapacity_.end()) ? it->second : 0;
}

const std::vector<std::string>& Barracks::GetAvailableTroops() const {
    return availableTroops_;
}

void Barracks::UpdateUI() {
    // 更新训练队列显示
    for (size_t i = 0; i < trainingQueue_.size(); ++i) {
        if (trainingQueue_[i].icon) {
            float x = 20 + (i % 5) * 25;
            float y = getContentSize().height - 40 - (i / 5) * 25;
            trainingQueue_[i].icon->setPosition(x, y);

            // 显示剩余时间
            if (trainingQueue_[i].remainingTime > 0) {
                auto label = Label::createWithTTF(
                    std::to_string(trainingQueue_[i].remainingTime),
                    "fonts/Marker Felt.ttf", 10
                );
                label->setPosition(Vec2(12, 12));
                trainingQueue_[i].icon->addChild(label, 11);
            }
        }
    }

    // 更新已训练士兵显示
    int index = 0;
    for (const auto& pair : trainedTroops_) {
        auto it = troopIcons_.find(pair.first);
        if (it != troopIcons_.end() && it->second) {
            float x = getContentSize().width - 40 - (index % 3) * 30;
            float y = 40 + (index / 3) * 30;
            it->second->setPosition(x, y);

            // 显示数量
            auto countLabel = Label::createWithTTF(
                std::to_string(pair.second),
                "fonts/Marker Felt.ttf", 12
            );
            countLabel->setPosition(Vec2(15, -10));
            countLabel->setTextColor(Color4B::WHITE);
            it->second->addChild(countLabel, 11);

            index++;
        }
    }
}

void Barracks::PlayTrainingStartAnimation(const std::string& troopType) {
    // 创建训练烟雾效果
    auto smoke = ParticleSystemQuad::create("particles/training_smoke.plist");
    if (smoke) {
        smoke->setPosition(getContentSize().width / 2, 30);
        smoke->setAutoRemoveOnFinish(true);
        addChild(smoke, 9);
    }

    // 播放音效
    // CocosDenshion::SimpleAudioEngine::getInstance()->playEffect("sounds/training_start.wav");
}

void Barracks::PlayTrainingCompleteAnimation(const std::string& troopType) {
    // 创建完成特效
    auto completeEffect = ParticleSystemQuad::create("particles/training_complete.plist");
    if (completeEffect) {
        completeEffect->setPosition(getContentSize().width / 2, 30);
        completeEffect->setAutoRemoveOnFinish(true);
        addChild(completeEffect, 10);
    }

    // 文字提示
    auto popup = Label::createWithTTF(troopType + " Trained!", "fonts/Marker Felt.ttf", 24);
    if (popup) {
        popup->setPosition(getContentSize().width / 2, getContentSize().height + 30);
        popup->setTextColor(Color4B::GREEN);
        addChild(popup, 100);

        auto moveUp = MoveBy::create(1.0f, Vec2(0, 50));
        auto fadeOut = FadeOut::create(0.5f);
        auto remove = CallFuncN::create([](Node* node) {
            node->removeFromParent();
            });

        auto sequence = Sequence::create(moveUp, fadeOut, remove, nullptr);
        popup->runAction(sequence);
    }
}

void Barracks::PlayTroopDeployAnimation(const Vec2& deployPosition) {
    // 创建士兵部署特效
    for (int i = 0; i < 3; ++i) {
        auto troopSprite = Sprite::create("effects/troop_spawn.png");
        if (troopSprite) {
            troopSprite->setPosition(deployPosition);
            troopSprite->setScale(0.5f);
            getParent()->addChild(troopSprite, 1000);

            // 随机方向移动
            float angle = CCRANDOM_0_1() * 2 * M_PI;
            float distance = 50 + CCRANDOM_0_1() * 100;
            Vec2 target = deployPosition + Vec2(distance * cos(angle), distance * sin(angle));

            auto move = MoveTo::create(0.5f, target);
            auto fade = FadeOut::create(0.3f);
            auto remove = CallFuncN::create([](Node* node) {
                node->removeFromParent();
                });

            auto sequence = Sequence::create(move, fade, remove, nullptr);
            troopSprite->runAction(sequence);
        }
    }
}

void Barracks::Upgrade() {
    int oldCapacity = GetTrainingCapacity();
    int oldSpeed = GetTrainingSpeed();

    // 调用基类升级
    TrainingBuilding::Upgrade();

    // 升级时解锁新兵种
    if (GetLevel() == 2 && !IsTroopAvailable("Giant")) {
        UnlockTroopType("Giant", "icons/giant_icon.png");
    }
    else if (GetLevel() == 3 && !IsTroopAvailable("Wizard")) {
        UnlockTroopType("Wizard", "icons/wizard_icon.png");
    }
    else if (GetLevel() == 4 && !IsTroopAvailable("Dragon")) {
        UnlockTroopType("Dragon", "icons/dragon_icon.png");
    }

    CCLOG("军营升级完成");
    CCLOG("训练容量: %d -> %d", oldCapacity, GetTrainingCapacity());
    CCLOG("训练速度: %d -> %d", oldSpeed, GetTrainingSpeed());
    CCLOG("可用兵种: %d种", static_cast<int>(availableTroops_.size()));
}

void Barracks::ShowInfo() const {
    TrainingBuilding::ShowInfo();

    CCLOG("可用兵种列表:");
    for (const auto& troop : availableTroops_) {
        int trained = GetTrainedTroopCount(troop);
        int capacity = GetTroopCapacity(troop);
        CCLOG("  %s: %d/%d", troop.c_str(), trained, capacity);
    }

    CCLOG("训练队列: %d个单位", static_cast<int>(trainingQueue_.size()));
}

int Barracks::GetNextTrainingCapacity() const {
    return GetTrainingCapacity() + 2;
}

int Barracks::GetNextTrainingSpeed() const {
    return static_cast<int>(GetTrainingSpeed() * 0.9f);  // 速度更快
}

void Barracks::OnTrainingComplete(const std::string& troopType, int count) {
    trainedTroops_[troopType] += count;

    // 检查是否超过容量
    int capacity = GetTroopCapacity(troopType);
    if (trainedTroops_[troopType] > capacity) {
        trainedTroops_[troopType] = capacity;
    }

    // 通知TownHall更新军队人数
    TownHall* town_hall = TownHall::GetInstance();
    if (town_hall) {
        town_hall->UpdateArmyCount(count);
    }

    // 播放完成动画
    PlayTrainingCompleteAnimation(troopType);

    UpdateUI();
}

Sprite* Barracks::CreateTroopIcon(const std::string& troopType, const std::string& iconPath) {
    auto icon = Sprite::create(iconPath);
    if (icon) {
        icon->setScale(0.3f);
        icon->setVisible(false);  // 默认隐藏，训练完成时显示
        addChild(icon, 10);
    }
    return icon;
}

void Barracks::OnTrainingUpdate(float deltaTime) {
    for (auto it = trainingQueue_.begin(); it != trainingQueue_.end(); ) {
        it->remainingTime -= static_cast<int>(deltaTime);

        if (it->remainingTime <= 0) {
            // 训练完成
            OnTrainingComplete(it->troopType, 1);

            // 移除图标
            if (it->icon) {
                it->icon->removeFromParent();
            }

            it = trainingQueue_.erase(it);
        }
        else {
            ++it;
        }
    }
}