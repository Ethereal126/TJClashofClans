#pragma once
#ifndef __BUILDING_H__
#define __BUILDING_H__

#include <string>
#include <utility>
#include "cocos2d.h"

/*
Building类 - 基类，所有建筑物都会继承这个类
*/
class Building : public cocos2d::Sprite {
protected:
    std::string name_;                // 建筑名称
    int level_;                       // 建筑等级
    int health_;                      // 当前生命值
    int defense_;                     // 防御值
    int build_time_;                   // 建造时间（秒）
    int build_cost_;                   // 建造成本
    cocos2d::Vec2 position_;    // 建筑的位置信息 (x, y)

public:
    // 构造函数：初始化建筑名称、等级、生命值、防御、建造时间、建造成本和位置
    Building(std::string name, int level, int health, int defense,
        int buildtime, int build_cost, std::pair<int, int> position);

    // 虚函数：建筑更新（包含等级提升，建筑时间和费用变换，建筑血量上限和防御值提升）
    virtual void Upgrade();

    // 接受伤害，减少生命值
    void TakeDamage(int damage);

    // 修复建筑，恢复至最大生命值
    void Repair();

    // 检查建筑是否被摧毁（1表示摧毁，0表示没有）
    bool IsDamaged() const;

    // 虚函数：显示建筑信息（例如名称、等级、生命值等）
    virtual void ShowInfo() const;

    // 获取最大生命值（可能根据建筑等级变化）
    int GetMaxHealth() const;

    // 虚析构函数，确保子类对象正确销毁
    virtual ~Building() = default;

    // Getter函数（提供常量访问，不修改状态）
    const std::string& GetName() const;
    int GetLevel() const;
    int GetHealth() const;
    int GetDefense() const;
    int GetBuildTime() const;
    int GetBuildCost() const;
    std::pair<int, int> GetPosition() const;
};

/*
SourceBuilding类 - 继承自Building类，表示一个资源类建筑
*/
class SourceBuilding : public Building{
private:
    int production_rate_;   // 每小时生产的金币数量

public:
    // 构造函数：初始化资源建筑的等级、位置，设置资源生产速率
    SourceBuilding(int level, std::pair<int, int> position);

    // 虚函数：生产资源
    virtual int ProduceResource();

    // override虚函数：显示资源建筑信息
    virtual void ShowInfo() const override;
};

/*
AttackBuilding类 - 继承自Building类，表示一个攻击塔
*/
class AttackBuilding : public Building {
private:
    int Range_;  // 攻击范围

public:
    // 构造函数：初始化攻击塔的等级、位置，设置攻击范围
    AttackBuilding(int level, std::pair<int, int> position);

    // override虚函数：显示攻击塔信息
    virtual void ShowInfo() const override;
};

//napper:尽管好像不需要做什么额外实现但是为了炸弹兵对墙体的特殊索敌最好还是拆出来
class WallBuilding : public Building{

};
#endif // __BUILDING_H__