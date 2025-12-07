//
// Created by duby0 on 2025/12/5.
//

#ifndef TJCLASHOFCLANS_SOLDIER_H
#define TJCLASHOFCLANS_SOLDIER_H

#include <cocos2d.h>

enum class SoldierType {
    kBarbarian,  // 野蛮人
    kArcher,     // 弓箭手
    kBomber,     // 炸弹人
    kGiant       // 巨人
};

//TODO:完善此类实现，使其真正满足Soldier所需具备的完整能力
class Soldier {
public:
    // 构造函数
    Soldier();

    // 获取生命值
    int GetHealth() const {return health_;}
    // 设置生命值
    void SetHealth(int health){health_ = health;};

    // 获取攻击力
    int GetDamage() const;
    // 设置攻击力
    void SetDamage(int damage);

    // 获取攻击距离
    float GetAttackRange() const;
    // 设置攻击距离
    void SetAttackRange(double range);
    // 获取攻击距离
    float GetAttackDelay() const;
    // 设置攻击距离
    void SetAttackDelay(double delay);
    // 获取移动速度
    float GetMoveSpeed() const;
    // 设置移动速度
    void SetMoveSpeed(double speed);

    std::string GetName() const;
    void SetName(std::string name);


protected:
    SoldierType type_;
    int health_;
    int damage_;
    float attack_range_;//攻击范围
    float attack_delay_;//一次攻击所花费的时间
    float move_speed_;
    std::string name;
};
#endif //TJCLASHOFCLANS_SOLDIER_H
