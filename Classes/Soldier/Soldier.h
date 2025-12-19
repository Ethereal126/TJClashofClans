//
// Created by duby0 on 2025/12/5.
//

#ifndef TJCLASHOFCLANS_SOLDIER_H
#define TJCLASHOFCLANS_SOLDIER_H

#include <cocos2d.h>
#include <string>
#include <vector>

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
    
    // 获取士兵类型
    SoldierType GetSoldierType() const;
    
    // 获取士兵生命值
    virtual int GetHealth() const;
    
    // 设置士兵生命值
    virtual void SetHealth(int health);
    
    // 获取士兵伤害值
    virtual int GetDamage() const;
    
    // 设置士兵伤害值
    virtual void SetDamage(int damage);
    
    // 获取士兵移动速度
    virtual float GetMoveSpeed() const;
    
    // 设置士兵移动速度
    virtual void SetMoveSpeed(double move_speed);
    
    // 获取士兵攻击范围
    virtual float GetAttackRange() const;
    
    // 设置士兵攻击范围
    virtual void SetAttackRange(double attack_range);
    
    // 获取士兵攻击延迟
    virtual float GetAttackDelay() const;
    
    // 设置士兵攻击延迟
    virtual void SetAttackDelay(double attack_delay);
    
    // 获取士兵名称
    virtual std::string GetName() const;
    
    // 设置士兵名称
    virtual void SetName(std::string name);
    
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
    std::string name_;
};

// 士兵模板结构体（用于工厂函数）
struct SoldierTemplate {
    SoldierType type_;
    std::string name_;
    std::string icon_path_;
    int health_;
    int damage_;
    float move_speed_;
    float attack_range_;
    float attack_delay_;
    int housing_space_;  // 人口占用
    int training_cost_;  // 训练费用
    int training_time_;  // 训练时间（秒）

    SoldierTemplate(SoldierType t, std::string n, std::string p, int hp, int dmg,
        float ms, float ar, float ad, int hs, int tc, int tt)
        : type_(t), name_(std::move(n)), icon_path_(p), health_(hp), damage_(dmg),
        move_speed_(ms), attack_range_(ar), attack_delay_(ad),
        housing_space_(hs), training_cost_(tc), training_time_(tt) {
    }

    // 创建士兵的工厂函数
    Soldier* Create() const {
        return new Soldier(type_, health_, damage_, move_speed_, attack_range_, attack_delay_);
    }
};

#endif //TJCLASHOFCLANS_SOLDIER_H
