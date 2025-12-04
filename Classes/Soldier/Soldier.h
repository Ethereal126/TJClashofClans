//
// Created by duby0 on 2025/12/5.
//

#ifndef TJCLASHOFCLANS_SOLDIER_H
#define TJCLASHOFCLANS_SOLDIER_H

#include <cocos2d.h>

//TODO:完善此类实现，使其真正满足Soldier所需具备的完整能力
class Soldier : public cocos2d::Sprite {
public:
    // 构造函数
    Soldier();
    // 析构函数
    virtual ~Soldier();

    // 创建士兵
    static Soldier* Create(const std::string& filename);

    // 初始化函数
    virtual bool Init(const std::string& filename);

    // 获取生命值
    int GetHealth() const;
    // 设置生命值
    void SetHealth(int health);

    // 获取攻击力
    int GetDamage() const;
    // 设置攻击力
    void SetDamage(int damage);

    // 获取攻击距离
    double GetAttackRange() const;
    // 设置攻击距离
    void SetAttackRange(float range);

    // 获取移动速度
    double GetMoveSpeed() const;
    // 设置移动速度
    void SetMoveSpeed(float speed);

protected:
    int health_;
    int damage_;
    double attack_range_;//攻击范围
    double attack_time_;//一次攻击所花费的时间
    double move_speed_;
};
#endif //TJCLASHOFCLANS_SOLDIER_H
