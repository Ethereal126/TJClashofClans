//
// Created by duby0 on 2025/12/7.
//

#include "Soldier.h"
#include <string>

// 构造函数实现
Soldier::Soldier(SoldierType type, int health, int damage, float move_speed, float attack_range, double attack_delay)
    : type_(type), health_(health), damage_(damage), move_speed_(move_speed), attack_range_(attack_range), attack_delay_(attack_delay)
{
    // 根据士兵类型设置默认名称
    switch (type_) {
        case SoldierType::kBarbarian:
            name = "Barbarian";
            break;
        case SoldierType::kArcher:
            name = "Archer";
            break;
        case SoldierType::kBomber:
            name = "Bomber";
            building_preference_ = "WallBuilding";
            break;
        case SoldierType::kGiant:
            name = "Giant";
            building_preference_ = "AttackBuilding";
            break;
        default:
            name = "Unknown Soldier";
            break;
    }
}

// 获取士兵类型
SoldierType Soldier::GetSoldierType() const
{
    return type_;
}

// 获取士兵生命值
int Soldier::GetHealth() const
{
    return health_;
}

// 设置士兵生命值
void Soldier::SetHealth(int health)
{
    health_ = health;
}

// 获取士兵伤害值
int Soldier::GetDamage() const
{
    return damage_;
}

// 设置士兵伤害值
void Soldier::SetDamage(int damage)
{
    damage_ = damage;
}

// 获取士兵移动速度
float Soldier::GetMoveSpeed() const
{
    return move_speed_;
}

// 设置士兵移动速度
void Soldier::SetMoveSpeed(double move_speed)
{
    move_speed_ = static_cast<float>(move_speed);
}

// 获取士兵攻击范围
float Soldier::GetAttackRange() const
{
    return attack_range_;
}

// 设置士兵攻击范围
void Soldier::SetAttackRange(double attack_range)
{
    attack_range_ = static_cast<float>(attack_range);
}

// 获取士兵攻击延迟
float Soldier::GetAttackDelay() const
{
    return attack_delay_;
}

// 设置士兵攻击延迟
void Soldier::SetAttackDelay(double attack_delay)
{
    attack_delay_ = static_cast<float>(attack_delay);
}

// 获取士兵名称
std::string Soldier::GetName() const
{
    return name;
}

// 设置士兵名称
void Soldier::SetName(std::string soldier_name)
{
    name = std::move(soldier_name);
}
