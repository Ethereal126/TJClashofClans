//
// Created by Faith_Oriest on 2025/12/1.
//

#pragma once
#ifndef __BUILDING_H__
#define __BUILDING_H__

#include <string>
#include <utility>
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
    int build_time_;                   // 建造时间（秒）
    int build_cost_;                   // 建造成本
    int width_;
    int length_;
    cocos2d::Vec2 position_;    // 建筑的位置信息 (x, y)

public:
    //napper:为了进行建筑渲染的测试临时添加
    std::string texture_;
    /**
     * @brief 构造函数
     * 初始化建筑名称、等级、生命值、防御、建造时间、建造成本和位置。
     */
    Building(std::string name, int level, int health, int defense,
        int buildtime, int build_cost, cocos2d::Vec2 position);

    /**
     * @brief 建筑升级接口
     * 包含等级提升、建造时间和费用变化、生命值上限和防御值提升等逻辑。
     */
    virtual void Upgrade();

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
     * @return 以 std::pair<int,int> 形式返回的建筑坐标。
     */
    cocos2d::Vec2 GetPosition() const;
	int GetWidth() const{
        return width_;
    }
	int GetLength() const {
        return length_;
    }
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
     * @brief 生产资源
     * @return 本次生产的资源数量。
     */
    virtual int ProduceResource();

    /**
     * @brief 显示资源建筑信息
     * 在基类显示信息基础上，附加资源生产相关信息。
     */
    virtual void ShowInfo() const override;
};

/**
 * @brief AttackBuilding类
 * 继承自Building类，表示一个攻击塔建筑。
 */
class AttackBuilding : public Building {
private:
    int Range_;  // 攻击范围

public:
    /**
     * @brief 构造函数
     * 初始化攻击塔的名称、基准数值、位置、纹理以及攻击范围。
     */
    AttackBuilding(std::string name, int base, cocos2d::Vec2 position, std::string texture, int range);

    /**
     * @brief 显示攻击塔信息
     * 在基类显示信息基础上，附加攻击范围等属性。
     */
    virtual void ShowInfo() const override;
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
private:
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
     * @brief 开始训练士兵
     * @param unit_type 要训练的士兵类型
     * @param count 训练数量
     * @return 是否成功开始训练
     */
    bool StartTraining(const std::string& unit_type, int count);

    /**
     * @brief 取消训练
     * @param unit_type 要取消的士兵类型
     * @param count 取消数量
     */
    void CancelTraining(const std::string& unit_type, int count);

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
