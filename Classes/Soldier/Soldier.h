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
    std::string name;
};

// 士兵模板结构体（用于工厂函数）
struct SoldierTemplate {
    SoldierType type;
    std::string name;
    int health;
    int damage;
    float move_speed;
    float attack_range;
    float attack_delay;
    int housing_space;  // 人口占用
    int training_cost;  // 训练费用
    int training_time;  // 训练时间（秒）

    SoldierTemplate(SoldierType t, std::string n, int hp, int dmg,
        float ms, float ar, float ad, int hs, int tc, int tt)
        : type(t), name(std::move(n)), health(hp), damage(dmg),
        move_speed(ms), attack_range(ar), attack_delay(ad),
        housing_space(hs), training_cost(tc), training_time(tt) {
    }

    // 创建士兵的工厂函数
    Soldier* Create() const {
        return new Soldier(type, health, damage, move_speed, attack_range, attack_delay);
    }
};

// 添加获取所有士兵模板的静态函数（在类外部）
static std::vector<SoldierTemplate> GetSoldierTemplates() {
    std::vector<SoldierTemplate> templates;

    // 野蛮人
    templates.emplace_back(SoldierType::kBarbarian, "Barbarian",
        45, 8, 1.0f, 0.4f, 1.0f, 1, 25, 20);
    // 弓箭手
    templates.emplace_back(SoldierType::kArcher, "Archer",
        20, 7, 0.8f, 3.5f, 1.0f, 1, 50, 25);
    // 炸弹人
    templates.emplace_back(SoldierType::kBomber, "Bomber",
        20, 6, 1.2f, 0.4f, 1.0f, 2, 1000, 60);
    // 巨人
    templates.emplace_back(SoldierType::kGiant, "Giant",
        300, 22, 0.6f, 1.0f, 2.0f, 5, 500, 120);

    return templates;
}

#endif //TJCLASHOFCLANS_SOLDIER_H
