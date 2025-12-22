//
// Created by Faith_Oriest on 2025/12/16.
//

#include "ResourceStorage/ResourceStorage.h"
#include "TownHall/TownHall.h"
#include "Soldier/Soldier.h"
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
    if (!Building::initWithFile("buildings/" + GetName() + ".png")) {
        return false;
    }

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
        auto collector = Sprite::create("buildings/elixirmine.png");
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

ElixirStorage* ElixirStorage::Create(const std::string& name, int base,
    cocos2d::Vec2 position,
    const std::string& texture,
    const std::string& resourceType) {
    // 使用nothrow避免分配失败时抛出异常
    auto storage = new (std::nothrow) ElixirStorage(name, base, position, texture);

    if (storage) {
        // 初始化纹理和资源类型
        if (storage->Init()) {
            // 标记为自动释放（Cocos2d-x的内存管理机制）
            storage->autorelease();

            // 设置资源类型相关属性
            if (resourceType == "Gold") {
                storage->setColor(cocos2d::Color3B::YELLOW);
            }
            else if (resourceType == "Elixir") {
                storage->setColor(cocos2d::Color3B(200, 100, 255)); // 紫色
                // 圣水储罐特有的初始化
                storage->SetCollectionRadius(100.0f + base * 10.0f);  // 根据等级设置收集半径
            }

            cocos2d::log("创建圣水储罐: %s (类型: %s, 位置: %f,%f)",
                name.c_str(), resourceType.c_str(),
                position.x, position.y);
            return storage;
        }

        // 初始化失败，删除对象
        delete storage;
    }

    cocos2d::log("创建圣水储罐失败: %s", name.c_str());
    return nullptr;
}

ElixirStorage::~ElixirStorage() {
}

ElixirStorage::ElixirStorage(const std::string& name, int base, cocos2d::Vec2 position, const std::string& texture)
    : ProductionBuilding(name, base, position, texture.empty() ? "buildings/elixirmine.png" : texture, "Elixir")
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


void ElixirStorage::PlayCollectionAnimation(const Vec2& targetPosition) {
    // 创建圣水收集动画
    auto elixirSprite = Sprite::create("buildings/elixirmine0.png");
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
    auto ripple = Sprite::create("buildings/elixirmine.png");
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

GoldStorage* GoldStorage::Create(const std::string& name, int base,
    cocos2d::Vec2 position,
    const std::string& texture,
    const std::string& resourceType) {
    // 使用nothrow避免分配失败时抛出异常
    auto storage = new (std::nothrow) GoldStorage(name, base, position, texture);

    if (storage) {
        // 初始化纹理和资源类型
        if (storage->Init()) {
            // 标记为自动释放（Cocos2d-x的内存管理机制）
            storage->autorelease();

            // 设置资源类型相关属性
            if (resourceType == "Gold") {
                storage->setColor(cocos2d::Color3B::YELLOW);
                // 金币储罐特有的初始化
                storage->ActivateProtection(base >= 3);  // 3级及以上自动激活保护
            }
            else if (resourceType == "Elixir") {
                storage->setColor(cocos2d::Color3B(200, 100, 255)); // 紫色
            }

            cocos2d::log("创建金币储罐: %s (类型: %s, 位置: %f,%f)",
                name.c_str(), resourceType.c_str(),
                position.x, position.y);
            return storage;
        }

        // 初始化失败，删除对象
        delete storage;
    }

    cocos2d::log("创建金币储罐失败: %s", name.c_str());
    return nullptr;
}

GoldStorage::~GoldStorage() {
    if (protectionShield_) protectionShield_->removeFromParent();
    if (shieldEffect_) shieldEffect_->removeFromParent();
}

GoldStorage::GoldStorage(const std::string& name, int base, cocos2d::Vec2 position, const std::string& texture)
    : ProductionBuilding(name, base, position, texture.empty() ? "buildings/goldpool1.png" : texture, "Gold")
    , isVaultProtected_(false)
    , protectionPercentage_(0.3f)
    , protectionShield_(nullptr)
    , shieldEffect_(nullptr) {
}

bool GoldStorage::Init() {
    if (!ProductionBuilding::Init()) {
        return false;
    }
    StartProduction();

    return true;
}

void GoldStorage::ActivateProtection(bool active) {
    isVaultProtected_ = active;

    if (isVaultProtected_) {}
    else if (protectionShield_) {
        protectionShield_->setVisible(false);
    }
}

void GoldStorage::UpgradeProtection(float additionalPercentage) {
    protectionPercentage_ = std::min(protectionPercentage_ + additionalPercentage, 0.8f);
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

// ==================== Barracks 实现 ====================

static std::string SoldierTypeToString(SoldierType type) {
    switch (type) {
    case SoldierType::kBarbarian: return "Barbarian";
    case SoldierType::kArcher: return "Archer";
    case SoldierType::kBomber: return "Bomber";
    case SoldierType::kGiant: return "Giant";
    default: return "Unknown";
    }
}

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
    : TrainingBuilding(name, base, position, "buildings/barrack.png", 5, 30) {
    // 初始化默认兵种
    availableTroops_.push_back("Barbarian");
    availableTroops_.push_back("Archer");

    troopCapacity_["Barbarian"] = 20;
    troopCapacity_["Archer"] = 15;
}

bool Barracks::Init() {
    // 直接调用 Building 的初始化方法
    if (!Sprite::initWithFile("buildings/barrack.png")) {
        return false;
    }

    // 设置建筑尺寸和位置
    setContentSize(cocos2d::Size(width_ * 32, length_ * 32));
    setAnchorPoint(cocos2d::Vec2(0.5f, 0.5f));

    // 初始化可用兵种列表
    availableTroops_.clear();
    availableTroops_.push_back("Barbarian");
    availableTroops_.push_back("Archer");
    availableTroops_.push_back("Giant");
    availableTroops_.push_back("Bomber");
    // 将兵种添加到基类列表（如果需要）
    AddAvailableUnitName("Barbarian");
    AddAvailableUnitName("Archer");
    AddAvailableUnitName("Giant");
    AddAvailableUnitName("Bomber");

    // 初始化兵种图标
    for (const auto& troop : availableTroops_) {
        std::string iconPath = "others/" + troop + ".png";
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
    const auto& availableUnits = GetAvailableUnitNames();
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
    available_unit_names_.push_back(unit_type);
}

bool Barracks::IsTroopAvailable(const std::string& troopType) const {
    // 先检查自己的列表
    for (const auto& troop : availableTroops_) {
        if (troop == troopType) {
            return true;
        }
    }

    // 再检查从基类继承的列表
    const auto& baseUnits = GetAvailableUnitNames();
    for (const auto& unit : baseUnits) {
        if (unit == troopType) {
            return true;
        }
    }

    return false;
}

std::vector<std::string> Barracks::GetAvailableTroopTypes() const {
    std::vector<std::string> allTroops = availableTroops_;

    // 合并基类的可用单位
    const auto& baseUnits = GetAvailableUnitNames();
    for (const auto& unit : baseUnits) {
        // 避免重复添加
        if (std::find(allTroops.begin(), allTroops.end(), unit) == allTroops.end()) {
            allTroops.push_back(unit);
        }
    }

    return allTroops;
}

std::vector<std::string> Barracks::GetAvailableSoldierNames() const {
    std::vector<std::string> soldierNames;

    // 将 SoldierType 枚举转换为字符串
    for (const auto& soldierType : available_soldier_types_) {
        soldierNames.push_back(SoldierTypeToString(soldierType));
    }

    return soldierNames;
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
        auto troopSprite = Sprite::create("others/Barbarian.png");
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