//
// Created by Faith_Oriest on 2025/12/7.
//

// ResourceManager.h
#pragma once

#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include "cocos2d.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

USING_NS_CC;

/**
 * @brief 资源管理器基类
 * 继承自 Node，支持 Cocos2d-x 的自动内存管理和 UI 集成
 */
class ResourceManager : public Node {
public:

    virtual ~ResourceManager() = 0;

    virtual bool Init();

    // 资源操作
    virtual bool AddResource(int amount);
    virtual bool UseResource(int amount);
    virtual bool CanAfford(int amount) const;

    // 容量管理
    virtual void UpgradeCapacity(int additional_capacity);
    virtual float GetFillPercentage() const;

    // UI 更新
    virtual void UpdateUi();

    // 动画效果
    virtual void PlayStorageAnimation();
    virtual void PlayFullCapacityAnimation();
    virtual void PlayNotEnoughAnimation();

    // Get 函数
    virtual const std::string& GetName() const { return name_; }
    virtual int GetCapacity() const { return capacity_; }
    virtual int GetCurrentAmount() const { return current_amount_; }
    virtual bool IsActive() const { return is_active_; }

    // Set 函数
    virtual void SetName(const std::string& name) { name_ = name; }
    virtual void SetActive(bool active) { is_active_ = active; }

    // 事件回调设置
    void SetOnAmountChangedCallback(const std::function<void(int, int)>& callback);
    void SetOnCapacityFullCallback(const std::function<void()>& callback);

    // UI 组件设置
    void SetUiIcon(const std::string& icon_path);
    void SetUiLabel(Label* label);
    void SetProgressBar(ProgressTimer* progress_bar);

protected:
    ResourceManager();

    // 初始化方法
    virtual void InitUiComponents();
    virtual void InitAnimations();

    // 回调触发
    virtual void TriggerAmountChanged(int old_amount, int new_amount);
    virtual void TriggerCapacityFull();

    // 成员变量
    std::string name_;
    int capacity_;
    int current_amount_;
    bool is_active_;

    // UI 组件
    Sprite* ui_icon_;
    Label* ui_label_;
    ProgressTimer* progress_bar_;

    // 动画组件
    Action* storage_effect_;
    ParticleSystemQuad* particle_effect_;

    // 事件回调
    std::function<void(int, int)> on_amount_changed_callback_;
    std::function<void()> on_capacity_full_callback_;

private:
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
};

// ==================== 非战斗资源管理基类 ====================

/**
 * @brief 非战斗资源管理基类
 * 管理圣水和金币等非战斗资源的共同功能
 */
class NonCombatResourceManager : public ResourceManager {
public:
    virtual ~NonCombatResourceManager() = 0;

    virtual bool Init();

    // 生产管理
    virtual void StartProduction();
    virtual void StopProduction();
    virtual void CollectResources();

    // 生产速率相关
    virtual int GetProductionRate() const { return production_rate_; }
    virtual void UpgradeProductionRate(int additional_rate);

    // 升级相关
    virtual void UpgradeCapacity(int additional_capacity) override;

    // 动画效果重写
    virtual void PlayStorageAnimation() override;

protected:
    NonCombatResourceManager();

    // 初始化方法
    virtual void InitProductionSystem();

    // 生产更新（由定时器调用）
    virtual void OnProductionUpdate(float delta_time);

    // 成员变量
    int production_rate_;
    float production_timer_;
    std::vector<Sprite*> collectors_;

private:
    NonCombatResourceManager(const NonCombatResourceManager&) = delete;
    NonCombatResourceManager& operator=(const NonCombatResourceManager&) = delete;
};

// ==================== 圣水储罐类 ====================

/**
 * @brief 圣水储罐类
 * 管理圣水资源的存储和生产
 */
class ElixirStorage : public NonCombatResourceManager {
public:
    virtual ~ElixirStorage() = 0;

    virtual bool Init() override;

    // 圣水特有功能
    void SetCollectionRadius(float radius);
    float GetCollectionRadius() const { return collection_radius_; }

    // 收集动画（圣水特有）
    void PlayCollectionAnimation(const Vec2& target_position);

    // 动画重写
    virtual void PlayStorageAnimation() override;

protected:
    ElixirStorage();

    // 初始化圣水特有组件
    virtual void InitElixirSpecificComponents();

    // 成员变量
    float collection_radius_;
    Color4F elixir_color_;  // 圣水颜色，用于特效

private:
    ElixirStorage(const ElixirStorage&) = delete;
    ElixirStorage& operator=(const ElixirStorage&) = delete;
};

// ==================== 金币储罐类 ====================

/**
 * @brief 金币储罐类
 * 管理金币资源的存储和生产
 */
class GoldStorage : public NonCombatResourceManager {
public:
    virtual ~GoldStorage() override = default;

    virtual bool Init() override;

    // 金库保护功能
    void ActivateProtection(bool active);
    bool IsVaultProtected() const { return is_vault_protected_; }

    void UpgradeProtection(float additional_percentage);
    float GetProtectionPercentage() const { return protection_percentage_; }

    // 被攻击时资源损失计算
    int CalculateRaidLoss(int attempted_steal) const;

    // 动画重写
    virtual void PlayStorageAnimation() override;

    // 保护相关动画
    void PlayProtectionActivationAnimation();
    void PlayRaidDefenseAnimation();

protected:
    GoldStorage();

    // 初始化金币特有组件
    virtual void InitGoldSpecificComponents();

    // 成员变量
    bool is_vault_protected_;
    float protection_percentage_;

    // 保护罩视觉效果
    Sprite* protection_shield_;
    ClippingNode* shield_effect_;

private:
    GoldStorage(const GoldStorage&) = delete;
    GoldStorage& operator=(const GoldStorage&) = delete;
};

// ==================== 战斗资源管理类 ====================

/**
 * @brief 兵营类
 * 管理士兵的训练和存储
 */
class Barracks : public ResourceManager {
public:
    // 训练队列结构体 - 需要在公有部分定义，以便外部可以访问
    struct TrainingUnit {
        std::string troop_type;
        int remaining_time;
        Sprite* icon;

        TrainingUnit() : troop_type(""), remaining_time(0), icon(nullptr) {}
        TrainingUnit(const std::string& type, int time, Sprite* sprite)
            : troop_type(type), remaining_time(time), icon(sprite) {
        }
    };

    virtual ~Barracks() = 0;

    virtual bool Init();

    // 兵种管理
    bool UnlockTroopType(const std::string& troop_type, const std::string& icon_path);
    bool IsTroopAvailable(const std::string& troop_type) const;

    // 训练管理
    bool TrainTroop(const std::string& troop_type, int count);
    void CancelTraining(int queue_index);

    // 容量管理重写
    virtual void UpgradeCapacity(int additional_capacity) override;
    void UpgradeTroopCapacity(const std::string& troop_type, int additional_capacity);

    // 部署士兵
    bool DeployTroops(const std::string& troop_type, int count);

    // 队列管理
    const std::vector<TrainingUnit>& GetTrainingQueue() const;
    int GetQueueSize() const;

    // 信息获取
    int GetTrainedTroopCount(const std::string& troop_type) const;
    int GetTroopCapacity(const std::string& troop_type) const;
    const std::vector<std::string>& GetAvailableTroops() const;

    // UI 更新（重写基类方法）
    virtual void UpdateUi() override;

    // 动画效果
    void PlayTrainingStartAnimation(const std::string& troop_type);
    void PlayTrainingCompleteAnimation(const std::string& troop_type);
    void PlayTroopDeployAnimation(const Vec2& deploy_position);

protected:
    Barracks();

    // 训练完成回调
    void OnTrainingComplete(const std::string& troop_type, int count);

    // 创建兵种图标
    Sprite* CreateTroopIcon(const std::string& troop_type, const std::string& icon_path);

    // 训练更新
    void OnTrainingUpdate(float delta_time);

    // 成员变量
    std::unordered_map<std::string, int> troop_capacity_;
    std::vector<std::string> available_troops_;
    std::unordered_map<std::string, Sprite*> troop_icons_;
    std::vector<TrainingUnit> training_queue_;
    std::unordered_map<std::string, int> trained_troops_;

private:
    Barracks(const Barracks&) = delete;
    Barracks& operator=(const Barracks&) = delete;
};

#endif  // RESOURCE_MANAGER_H