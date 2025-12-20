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
    Soldier(SoldierType type, int health, int damage, float move_speed, float attack_range, double attack_delay);
    std::string building_preference_;
    
    // 获取士兵类型
    SoldierType GetSoldierType() const;
    
    // 获取士兵生命值
    int GetHealth() const;
    
    // 设置士兵生命值
    void SetHealth(int health);
    
    // 获取士兵伤害值
    int GetDamage() const;
    
    // 设置士兵伤害值
    void SetDamage(int damage);
    
    // 获取士兵移动速度
    float GetMoveSpeed() const;
    
    // 设置士兵移动速度
    void SetMoveSpeed(double move_speed);
    
    // 获取士兵攻击范围
    float GetAttackRange() const;
    
    // 设置士兵攻击范围
    void SetAttackRange(double attack_range);
    
    // 获取士兵攻击延迟
    float GetAttackDelay() const;
    
    // 设置士兵攻击延迟
    void SetAttackDelay(double attack_delay);
    
    // 获取士兵名称
    std::string GetName() const;
    
    // 设置士兵名称
    void SetName(std::string name);
    
    // TODO: Village类尚未实现，暂时注释掉相关方法
    // void SetVillage(Village* village);
    // Village* GetVillage() const;
    
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
