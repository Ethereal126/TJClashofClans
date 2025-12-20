//
// Created by Faith_Oriest on 2025/12/1.
//

#pragma once
#ifndef __BUILDING_H__
#define __BUILDING_H__

#include <string>
#include "cocos2d.h"

/**
 * @brief Building类
 * 基类，所有建筑物都会继承自该类。
 */
class Building : public cocos2d::Sprite {
protected:
    std::string name_;                // 建筑名称
    int level_;                       // 建筑等级
    int health_;                      // 当前生命值
    int defense_;                     // 防御值
    int build_time_;                  // 建造时间（秒）
    int build_cost_;                  // 建造成本
    int width_;                       // 建造宽度
    int length_;                      // 建造长度
    bool is_upgrading_;               // 是否正在升级中
    float upgrade_remaining_time_;    // 升级剩余时间（秒）
    cocos2d::Vec2 position_;    // 建筑的位置信息 (x, y)


public:
    //napper:临时添加，用于绑定建筑对应的图片
    std::string texture_;

    /**
     * @brief 构造函数
     * 初始化建筑名称、等级、生命值、防御、建造时间、建造成本和位置。
     */
    Building(std::string name, int level, int health, int defense,
        int buildtime, int build_cost, int width, int length, cocos2d::Vec2 position);

    /**
     * @brief 建筑升级接口
     * 包含等级提升、建造时间和费用变化、生命值上限和防御值提升等逻辑。
     */
    virtual void Upgrade();

    /**
     * @brief 开始升级
     * 启动升级过程，在指定时间内无法再次升级
     * @param upgrade_time 升级所需时间（秒）
     */
    void StartUpgrade(float upgrade_time);

    /**
     * @brief 检查是否允许升级
     * @return true 表示允许升级，false 表示正在升级中
     */
    bool IsAllowedUpgrade() const;

    /**
     * @brief 获取建筑图片路径
     * @return 建筑图片路径字符串
     */
    std::string GetBuildingImagePath() const;

    /**
     * @brief 建筑承受伤害
     * 根据传入伤害值减少当前生命值。
     * @param damage 外部传入的伤害值。
     */
    void TakeDamage(int damage);

    /**
     * @brief 修复建筑
     * 将当前生命值恢复至最大生命值。
     */
    void Repair();

    /**
     * @brief 检查建筑是否被摧毁
     * @return true 表示建筑被摧毁（生命值小于等于 0），false 表示未被摧毁。
     */
    bool IsDamaged() const;

    /**
     * @brief 显示建筑信息
     * 例如名称、等级、生命值等，用于调试或 UI 显示。
     */
    virtual void ShowInfo() const;

    /**
     * @brief 获取最大生命值
     * 最大生命值可能会随着建筑等级变化。
     * @return 返回最大生命值。
     */
    int GetMaxHealth() const;

    /**
     * @brief 虚析构函数
     * 确保通过基类指针删除派生类对象时能够正确析构。
     */
    virtual ~Building() = default;

    /**
     * @brief 获取建筑名称
     * @return 建筑名称的常量引用。
     */
    const std::string& GetName() const;

    /**
     * @brief 获取建筑等级
     * @return 当前等级数值。
     */
    int GetLevel() const;

    /**
     * @brief 获取当前生命值
     * @return 当前生命值数值。
     */
    int GetHealth() const;

    /**
     * @brief 获取防御值
     * @return 当前防御数值。
     */
    int GetDefense() const;

    /**
     * @brief 获取建造时间
     * @return 建造所需时间（单位：秒）。
     */
    int GetBuildTime() const;

    /**
     * @brief 获取建造成本
     * @return 建造所需资源成本。
     */
    int GetBuildCost() const;

    /**
     * @brief 获取建筑位置
     * @return 以 cocos2d::Vec2 形式返回的建筑坐标。
     */
    cocos2d::Vec2 GetPosition() const;

    /**
     * @brief 获取建筑宽度
     * @return 以 int 形式返回的建筑宽度。
     */
    int GetWidth() const {
        return width_;
    }

    /**
     * @brief 获取建筑长度
     * @return 以 int 形式返回的建筑长度。
     */
    int GetLength() const {
        return length_;
    }

    /**
     * @brief 获取建筑下一级等级
     * @return 以 int 形式返回的建筑下一级等级。
     */
    int GetNextLevel() const {
        return level_ + 1;
    }

    /**
     * @brief 获取建筑下一级建筑时间
     * @return 以 int 形式返回的建筑下一级建筑时间。
     */
    int GetNextBuildTime() const {
        return static_cast<int>(build_time_ * (1.2 + 0.3 * level_));
    }

    /**
     * @brief 获取建筑下一级建筑费用
     * @return 以 int 形式返回的建筑下一级建筑费用。
     */
    int GetNextBuildCost() const {
        return static_cast<int>(build_cost_ * (1.2 + 0.3 * level_));
    }

    /**
     * @brief 获取建筑下一级防御
     * @return 以 int 形式返回的建筑下一级防御。
     */
    int GetNextDefense() {
        return static_cast<int>(defense_ * (1.1 + 0.2 * level_));
    }

    /**
     * @brief 获取建筑下一级血量
     * @return 以 int 形式返回的建筑下一级血量。
     */
    int GetNextHealth() {
        return 8 * GetNextDefense();
    }

    /**
     * @brief 获取是否正在升级中
     * @return true 表示正在升级，false 表示未在升级
     */
    bool IsUpgrading() const { return is_upgrading_; }

    /**
     * @brief 获取升级剩余时间
     * @return 升级剩余时间（秒）
     */
    float GetUpgradeRemainingTime() const { return upgrade_remaining_time_; }
};

/**
 * @brief SourceBuilding类
 * 继承自Building类，表示一个资源类建筑，用于生产资源。
 */
class SourceBuilding : public Building {
private:
    int production_rate_;   // 每小时生产的资源数量

public:
    /**
     * @brief 构造函数
     * 使用基准值 base 和位置初始化资源建筑，并设置资源生产速率与纹理。
     */
    SourceBuilding(std::string name, int base, cocos2d::Vec2 position, std::string texture);

    /**
     * @brief 创建资源建筑实例
     * @param name 建筑名称
     * @param base 基准数值
     * @param position 位置坐标
     * @param texture 纹理路径
     * @param resourceType 资源类型 ("Gold" 或 "Elixir")
     * @return 创建的SourceBuilding指针，失败返回nullptr
     */
    static SourceBuilding* Create(const std::string& name, int base,
        cocos2d::Vec2 position,
        const std::string& texture,
        const std::string& resourceType);

    /**
     * @brief 生产资源
     * @return 本次生产的资源数量。
     */
    virtual int ProduceResource () const;

    /**
     * @brief 显示资源建筑信息
     * 在基类显示信息基础上，附加资源生产相关信息。
     */
    virtual void ShowInfo() const override;

    /**
     * @brief 检查建筑是否活跃（是否在工作）
     * @return true 表示建筑活跃，false 表示建筑被摧毁或暂停工作
     */
    bool IsActive() const { return GetHealth() > 0; }

    /**
     * @brief 获取生产速率
     * @return 每小时生产的资源数量
     */
    int GetProductionRate() const { return production_rate_; }

    /**
     * @brief 设置生产速率
     * @param rate 新的生产速率
     */
    void SetProductionRate(int rate) { production_rate_ = rate; }
};

/**
 * @brief AttackBuilding类
 * 继承自Building类，表示一个攻击塔建筑。
 */
class AttackBuilding : public Building {
public:
    float attack_interval_;
    int attack_damage_;
    float attack_range_;

    /**
     * @brief 构造函数
     * 初始化攻击塔的名称、基准数值、位置、纹理以及攻击范围。
     */
    AttackBuilding(std::string name, int base, cocos2d::Vec2 position, std::string texture,
                   float attack_interval,int attack_damage,float attack_range_);

    /**
     * @brief 创建攻击建筑实例
     * @param name 建筑名称
     * @param base 基准数值
     * @param position 位置坐标
     * @param texture 纹理路径
     * @param range 攻击范围
     * @return 创建的AttackBuilding指针，失败返回nullptr
     */
    static AttackBuilding* Create(const std::string& name, int base,
        cocos2d::Vec2 position,
        const std::string& texture, float attack_interval,int attack_damage,float attack_range);

    /**
     * @brief 显示攻击塔信息
     * 在基类显示信息基础上，附加攻击范围等属性。
     */
    virtual void ShowInfo() const override;

    /**
     * @brief 检查建筑是否活跃
     * @return true 表示建筑活跃，false 表示建筑被摧毁
     */
    bool IsActive() const { return GetHealth() > 0; }
};

/**
 * @brief WallBuilding类
 * 继承自Building类，表示墙体建筑。
 * napper: 尽管好像不需要做什么额外实现，但是为了炸弹兵对墙体的特殊索敌逻辑，最好还是拆成单独类。
 */
class WallBuilding : public Building {

};

/**
 * @brief TrainingBuilding类
 * 继承自Building类，表示训练营建筑，用于训练士兵。
 */
class TrainingBuilding : public Building {
protected:
    int training_capacity_;     // 同时训练的最大士兵数量
    int training_speed_;        // 训练速度（单位：秒/每兵）
    std::vector<std::string> available_units_;  // 可训练的单位类型列表

public:
    /**
     * @brief 构造函数
     * 初始化训练营的名称、基准数值、位置、纹理以及训练属性。
     * @param name 建筑名称
     * @param base 基准数值（用于计算生命值、防御等）
     * @param position 建筑位置坐标
     * @param texture 建筑纹理路径
     * @param capacity 训练容量
     * @param speed 训练速度（秒/每兵）
     */
    TrainingBuilding(std::string name, int base, cocos2d::Vec2 position,
        std::string texture, int capacity, int speed);

    /**
     * @brief 初始化
     * @return 是否成功初始化
     */
    virtual bool Init();

    /**
     * @brief 创建训练营实例
     * @param name 建筑名称
     * @param base 基准数值
     * @param position 位置坐标
     * @param texture 纹理路径
     * @param capacity 训练容量
     * @param speed 训练速度
     * @return 创建的TrainingBuilding指针，失败返回nullptr
     */
    static TrainingBuilding* Create(const std::string& name, int base,
        cocos2d::Vec2 position,
        const std::string& texture, int capacity, int speed);

    /**
     * @brief 添加可训练单位
     * @param unit_type 要添加的士兵类型
     */
    void AddAvailableUnit(const std::string& unit_type);

    /**
     * @brief 获取可训练单位列表
     * @return 可训练单位列表
     */
    std::vector<std::string>& GetAvailableUnits();

    /**
     * @brief 开始训练士兵
     * @param unit_type 要训练的士兵类型
     * @param count 训练数量
     * @return 是否成功开始训练
     */
    virtual bool StartTraining(const std::string& unit_type, int count);

    /**
     * @brief 取消训练
     * @param unit_type 要取消的士兵类型
     * @param count 取消数量
     */
    virtual void CancelTraining(const std::string& unit_type, int count);

    /**
     * @brief 获取当前训练队列信息
     * @return 训练队列中各类士兵的数量映射
     */
    std::map<std::string, int> GetTrainingQueue() const;

    /**
     * @brief 检查是否有训练完成
     * @return 完成训练的士兵类型和数量
     */
    std::pair<std::string, int> CheckCompletedTraining();

    /**
     * @brief 升级训练营
     * 重写基类升级方法，提升训练容量和速度
     */
    virtual void Upgrade() override;

    /**
     * @brief 显示训练营信息
     * 在基类显示信息基础上，附加训练相关属性。
     */
    virtual void ShowInfo() const override;

    /**
     * @brief 获取训练容量
     * @return 同时训练的最大士兵数量
     */
    int GetTrainingCapacity() const;

    /**
     * @brief 获取训练速度
     * @return 训练速度（秒/每兵）
     */
    int GetTrainingSpeed() const;

    /**
     * @brief 获取可训练单位列表
     * @return 可训练单位类型的常量引用
     */
    const std::vector<std::string>& GetAvailableUnits() const;
};
#endif // __BUILDING_H__
