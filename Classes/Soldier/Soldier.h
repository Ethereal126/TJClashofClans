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

class Soldier {
public:
    // 构造函数
    Soldier(SoldierType type, int health, int damage, float move_speed, float attack_range, float attack_delay);
    std::string building_preference_;
    float size_;
    
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
    std::string name_;
};

// 士兵模板结构体（用于工厂函数）
struct SoldierTemplate {
    SoldierType type_;
    std::string name_;
    std::string icon_path_;
    int housing_space_;  // 人口占用
    int training_cost_;  // 训练费用
    int training_time_;  // 训练时间（秒）

    // 工厂函数，用于创建士兵
    std::function<Soldier* ()> factory_;

    // 获取士兵属性的函数（可选，如果需要的话）
    std::function<int()> get_health_func_;
    std::function<int()> get_damage_func_;
    std::function<float()> get_move_speed_func_;
    std::function<float()> get_attack_range_func_;
    std::function<float()> get_attack_delay_func_;

    // 构造函数1：包含工厂函数和获取属性的函数
    SoldierTemplate(SoldierType t, std::string n, std::string p, int hs, int tc, int tt,
        std::function<Soldier* ()> factory,
        std::function<int()> get_health = nullptr,
        std::function<int()> get_damage = nullptr,
        std::function<float()> get_move_speed = nullptr,
        std::function<float()> get_attack_range = nullptr,
        std::function<float()> get_attack_delay = nullptr)
        : type_(t), name_(std::move(n)), icon_path_(std::move(p)),
        housing_space_(hs), training_cost_(tc), training_time_(tt),
        factory_(std::move(factory)),
        get_health_func_(get_health), get_damage_func_(get_damage),
        get_move_speed_func_(get_move_speed), get_attack_range_func_(get_attack_range),
        get_attack_delay_func_(get_attack_delay) {
    }

    // 构造函数2：直接存储所有属性值（向后兼容）
    SoldierTemplate(SoldierType t, std::string n, std::string p, int hp, int dmg,
        float ms, float ar, float ad, int hs, int tc, int tt)
        : type_(t), name_(std::move(n)), icon_path_(std::move(p)),
        housing_space_(hs), training_cost_(tc), training_time_(tt) {

        // 创建工厂函数，使用传入的属性值
        factory_ = [t, hp, dmg, ms, ar, ad]() -> Soldier* {
            return new Soldier(t, hp, dmg, ms, ar, ad);
            };

        // 创建属性获取函数
        get_health_func_ = [hp]() -> int { return hp; };
        get_damage_func_ = [dmg]() -> int { return dmg; };
        get_move_speed_func_ = [ms]() -> float { return ms; };
        get_attack_range_func_ = [ar]() -> float { return ar; };
        get_attack_delay_func_ = [ad]() -> float { return ad; };
    }

    // 创建士兵的工厂函数
    Soldier* Create() const {
        return factory_ ? factory_() : nullptr;
    }

    // 获取属性的辅助函数
    int GetHealth() const {
        return get_health_func_ ? get_health_func_() : 0;
    }

    int GetDamage() const {
        return get_damage_func_ ? get_damage_func_() : 0;
    }

    float GetMoveSpeed() const {
        return get_move_speed_func_ ? get_move_speed_func_() : 0.0f;
    }

    float GetAttackRange() const {
        return get_attack_range_func_ ? get_attack_range_func_() : 0.0f;
    }

    float GetAttackDelay() const {
        return get_attack_delay_func_ ? get_attack_delay_func_() : 0.0f;
    }
};

#endif //TJCLASHOFCLANS_SOLDIER_H
