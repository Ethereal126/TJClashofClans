// ResourceBuildings.h
#pragma once

#ifndef RESOURCE_BUILDINGS_H
#define RESOURCE_BUILDINGS_H

#include "Building/Building.h"
#include <unordered_map>
#include <vector>
#include <functional>

/**
 * @brief 资源建筑基类
 * 继承自Building类，管理资源的存储和生产
 */
class ResourceStorage : public Building {
public:
    virtual ~ResourceStorage();

    // 创建方法
    static ResourceStorage* Create(const std::string& name, int base, cocos2d::Vec2 position,
        const std::string& texture, const std::string& resourceType);

    // 初始化声明
    virtual bool Init();

    // 资源操作
    virtual bool AddResource(int amount);
    virtual bool UseResource(int amount);
    virtual bool CanAfford(int amount) const;

    // 容量管理
    virtual void UpgradeCapacity(int additionalCapacity);
    virtual float GetFillPercentage() const;

    // UI 更新
    virtual void UpdateUI();

    // 动画效果
    virtual void PlayStorageAnimation();
    virtual void PlayFullCapacityAnimation();
    virtual void PlayNotEnoughAnimation();

    // Get 函数
    virtual int GetCapacity() const { return capacity_; }
    virtual int GetCurrentAmount() const { return currentAmount_; }
    virtual bool IsActive() const { return isActive_; }
    virtual const std::string& GetResourceType() const { return resourceType_; }

    // Set 函数
    virtual void SetActive(bool active) { isActive_ = active; }
    virtual void SetResourceType(const std::string& type) { resourceType_ = type; }

    // 事件回调设置
    void SetOnAmountChangedCallback(const std::function<void(int, int)>& callback);
    void SetOnCapacityFullCallback(const std::function<void()>& callback);

    // UI 组件设置
    void SetUIIcon(const std::string& iconPath);
    void SetUILabel(cocos2d::Label* label);
    void SetProgressBar(cocos2d::ProgressTimer* progressBar);

    // 重写建筑方法
    virtual void Upgrade() override;
    virtual void ShowInfo() const override;

protected:
    ResourceStorage(const std::string& name, int base, cocos2d::Vec2 position,
        const std::string& texture, const std::string& resourceType);

    // 初始化方法
    virtual void InitUIComponents();
    virtual void InitAnimations();

    // 回调触发
    virtual void TriggerAmountChanged(int oldAmount, int newAmount);
    virtual void TriggerCapacityFull();

    // 成员变量
    std::string resourceType_;
    int capacity_;
    int currentAmount_;
    bool isActive_;

    // UI 组件
    cocos2d::Sprite* uiIcon_;
    cocos2d::Label* uiLabel_;
    cocos2d::ProgressTimer* progressBar_;

    // 动画组件
    cocos2d::Action* storageEffect_;
    cocos2d::ParticleSystemQuad* particleEffect_;

    // 事件回调
    std::function<void(int, int)> onAmountChangedCallback_;
    std::function<void()> onCapacityFullCallback_;

private:
    ResourceStorage(const ResourceStorage&) = delete;
    ResourceStorage& operator=(const ResourceStorage&) = delete;
};

// ==================== 非战斗资源管理基类 ====================

/**
 * @brief 非战斗资源生产建筑
 * 管理圣水和金币等非战斗资源的共同生产功能
 */
class ProductionBuilding : public ResourceStorage {
public:
    virtual ~ProductionBuilding();

    // 创建方法
    static ProductionBuilding* Create(const std::string& name, int base, cocos2d::Vec2 position,
        const std::string& texture, const std::string& resourceType);

    // 初始化声明
    virtual bool Init() override;

    // 生产管理
    virtual void StartProduction();
    virtual void StopProduction();
    virtual void CollectResources();

    // 生产速率相关
    virtual int GetProductionRate() const { return productionRate_; }
    virtual void UpgradeProductionRate(int additionalRate);

    // 升级相关（重写基类）
    virtual void UpgradeCapacity(int additionalCapacity) override;

    // 动画效果重写
    virtual void PlayStorageAnimation() override;

    // 重写建筑方法
    virtual void Upgrade() override;
    virtual void ShowInfo() const override;

protected:
    ProductionBuilding(const std::string& name, int base, cocos2d::Vec2 position,
        const std::string& texture, const std::string& resourceType);

    // 初始化生产系统
    virtual void InitProductionSystem();

    // 生产更新（由定时器调用）
    virtual void OnProductionUpdate(float deltaTime);

    // 成员变量
    int productionRate_;
    float productionTimer_;
    std::vector<cocos2d::Sprite*> collectors_;

private:
    ProductionBuilding(const ProductionBuilding&) = delete;
    ProductionBuilding& operator=(const ProductionBuilding&) = delete;
};

// ==================== 圣水储罐类 ====================

/**
 * @brief 圣水储罐类
 * 管理圣水资源的存储和生产
 */
class ElixirStorage : public ProductionBuilding {
public:
    static ElixirStorage* Create(std::string name, int base, cocos2d::Vec2 position);

    virtual ~ElixirStorage();

    virtual bool Init() override;

    // 圣水特有功能
    void SetCollectionRadius(float radius);
    float GetCollectionRadius() const { return collectionRadius_; }

    // 收集动画（圣水特有）
    void PlayCollectionAnimation(const cocos2d::Vec2& targetPosition);

    // 动画重写
    virtual void PlayStorageAnimation() override;

    // 重写建筑方法
    virtual void Upgrade() override;
    virtual void ShowInfo() const override;

    // 获取下一级信息
    int GetNextProductionRate() const;
    int GetNextCapacity() const;

protected:
    ElixirStorage(const std::string& name, int base, cocos2d::Vec2 position);

    // 初始化圣水特有组件
    virtual void InitElixirSpecificComponents();

    // 成员变量
    float collectionRadius_;
    cocos2d::Color4F elixirColor_;  // 圣水颜色，用于特效

private:
    ElixirStorage(const ElixirStorage&) = delete;
    ElixirStorage& operator=(const ElixirStorage&) = delete;
};

// ==================== 金币储罐类 ====================

/**
 * @brief 金币储罐类
 * 管理金币资源的存储和生产
 */
class GoldStorage : public ProductionBuilding {
public:
    static GoldStorage* Create(std::string name, int base, cocos2d::Vec2 position);

    virtual ~GoldStorage();

    virtual bool Init() override;

    // 金库保护功能
    void ActivateProtection(bool active);
    bool IsVaultProtected() const { return isVaultProtected_; }

    void UpgradeProtection(float additionalPercentage);
    float GetProtectionPercentage() const { return protectionPercentage_; }

    // 被攻击时资源损失计算
    int CalculateRaidLoss(int attemptedSteal) const;

    // 动画重写
    virtual void PlayStorageAnimation() override;

    // 保护相关动画
    void PlayProtectionActivationAnimation();
    void PlayRaidDefenseAnimation();

    // 重写建筑方法
    virtual void Upgrade() override;
    virtual void ShowInfo() const override;

    // 获取下一级信息
    int GetNextProductionRate() const;
    int GetNextCapacity() const;
    float GetNextProtectionPercentage() const;

protected:
    GoldStorage(const std::string& name, int base, cocos2d::Vec2 position);

    // 初始化金币特有组件
    virtual void InitGoldSpecificComponents();

    // 成员变量
    bool isVaultProtected_;
    float protectionPercentage_;

    // 保护罩视觉效果
    cocos2d::Sprite* protectionShield_;
    cocos2d::ClippingNode* shieldEffect_;

private:
    GoldStorage(const GoldStorage&) = delete;
    GoldStorage& operator=(const GoldStorage&) = delete;
};

// ==================== 军营类 ====================

/**
 * @brief 军营类
 * 管理士兵的训练和存储，继承自TrainingBuilding以保持原有逻辑
 */
class Barracks : public TrainingBuilding {
public:
    // 训练队列结构体
    struct TrainingUnit {
        std::string troopType;
        int remainingTime;
        cocos2d::Sprite* icon;

        TrainingUnit() : troopType(""), remainingTime(0), icon(nullptr) {}
        TrainingUnit(const std::string& type, int time, cocos2d::Sprite* sprite)
            : troopType(type), remainingTime(time), icon(sprite) {
        }
    };

    // 创建方法声明
    static Barracks* Create(const std::string& name, int base, cocos2d::Vec2 position);
    
    virtual ~Barracks();

    // 初始化声明
    virtual bool Init() override;

    // 访问可用单位
    void AddAvailableUnit(const std::string& unit_type);

    //检查军营是否活跃
    bool IsActive() const { return GetHealth() > 0; }

    // 兵种管理
    bool UnlockTroopType(const std::string& troopType, const std::string& iconPath);
    bool IsTroopAvailable(const std::string& troopType) const;

    // 训练管理（扩展原有方法）
    virtual bool StartTraining(const std::string& unitType, int count) override;
    virtual void CancelTraining(const std::string& unitType, int count) override;

    // 容量管理重写
    void UpgradeTroopCapacity(const std::string& troopType, int additionalCapacity);

    // 部署士兵
    bool DeployTroops(const std::string& troopType, int count);

    // 队列管理
    const std::vector<TrainingUnit>& GetTrainingQueue() const { return trainingQueue_; }
    int GetQueueSize() const { return static_cast<int>(trainingQueue_.size()); }

    // 信息获取
    int GetTrainedTroopCount(const std::string& troopType) const;
    int GetTroopCapacity(const std::string& troopType) const;
    const std::vector<std::string>& GetAvailableTroops() const { return availableTroops_; }

    // UI 更新
    virtual void UpdateUI();

    // 动画效果
    void PlayTrainingStartAnimation(const std::string& troopType);
    void PlayTrainingCompleteAnimation(const std::string& troopType);
    void PlayTroopDeployAnimation(const cocos2d::Vec2& deployPosition);

    // 重写建筑方法
    virtual void Upgrade() override;
    virtual void ShowInfo() const override;

    // 获取下一级信息
    int GetNextTrainingCapacity() const;
    int GetNextTrainingSpeed() const;

protected:
    Barracks(const std::string& name, int base, cocos2d::Vec2 position);

    // 训练完成回调
    void OnTrainingComplete(const std::string& troopType, int count);

    // 创建兵种图标
    cocos2d::Sprite* CreateTroopIcon(const std::string& troopType, const std::string& iconPath);

    // 训练更新
    void OnTrainingUpdate(float deltaTime);

    // 成员变量
    std::unordered_map<std::string, int> troopCapacity_;
    std::vector<std::string> availableTroops_;
    std::unordered_map<std::string, cocos2d::Sprite*> troopIcons_;
    std::vector<TrainingUnit> trainingQueue_;
    std::unordered_map<std::string, int> trainedTroops_;

private:
    Barracks(const Barracks&) = delete;
    Barracks& operator=(const Barracks&) = delete;
};

#endif  // RESOURCE_BUILDINGS_H