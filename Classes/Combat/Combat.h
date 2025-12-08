//
// Created by napper-d on 2025/12/1.
//

#ifndef PROGRAMMING_PARADIGM_FINAL_PROJECT_COMBAT_H
#define PROGRAMMING_PARADIGM_FINAL_PROJECT_COMBAT_H

#include <string>
#include "cocos2d.h"
#include "Building/Building.h"
#include "Soldier/Soldier.h"
#include "Map/Map.h"


class SoldierInCombat;
class BuildingInCombat;
class Combat;

class SoldierInCombat : public cocos2d::Sprite{
public:
    ~SoldierInCombat();
    cocos2d::Vec2 location_;
    bool is_alive_;

    SoldierInCombat* Create(Soldier* soldier, const cocos2d::Vec2& spawn_pos);
    // 初始化函数
    bool Init(Soldier* soldier_template, const cocos2d::Vec2& spawn_pos);

private:
    Soldier* soldier_template_;
    int current_health_;
    BuildingInCombat* current_target_;

    void SetTarget(BuildingInCombat* target);
    void Die();
    void MoveToTarget();
    void StartAttack();
    void DealDamageToTarget();
    void CheckTargetAlive();
    BuildingInCombat* FindNextTarget();

    // -------------------------- 动画资源（静态共享） --------------------------
    void LoadAnimationFrames();
    static cocos2d::Vector<cocos2d::SpriteFrame*> move_frames_;   // 移动动画帧
    static cocos2d::Vector<cocos2d::SpriteFrame*> attack_frames_; // 攻击动画帧
    static cocos2d::Vector<cocos2d::SpriteFrame*> die_frames_;    // 死亡动画帧
    static bool is_animation_loaded_; // 标记动画资源是否已加载（避免重复加载）
};


class BuildingInCombat : public cocos2d::Sprite{
public:
    // 构造函数
    BuildingInCombat();
    // 析构函数
    virtual ~BuildingInCombat();

    // 初始化函数
    virtual bool Init(const std::string& filename);

    // 被攻击函数
    void TakeDamage(int damage);

    //使用时应确保建筑类型为防御类建筑，函数参数有待确定，暂时考虑通过轮询方式搜索攻击目标，如果有更好的想法再做优化
    void Attack();

    // 判断是否存活
    bool IsAlive() const;

    void Destroyed();

private:
    Building* building_template;
    int current_health;
    bool is_alive_;
};

//负责管理战斗中部署士兵的交互实现，以及其他与动画相关的细节，等具体开始实现Combat时填充内容
class CombatScene : public cocos2d::Scene{

};

//负责统筹管理整个战斗过程，仅包含最基本的需求
class Combat{
private:
    std::vector<Soldier*> soldiers_;
    Map map_;
    CombatScene scene_;
    int destroy_degree_;

public:
    Combat();
    ~Combat();

    //初始化战场中的建筑，返回初始化结果
    bool Init();

    //在接收到交互指令后，将士兵加入到战斗中；
    void SendSoldier();

    //对于依赖轮询的对象进行更新，确认战斗是否可以结束
    void Update();

    //退出战斗，返回破坏度（destroy_degree_）
    int EndCombat();
};

#endif //PROGRAMMING_PARADIGM_FINAL_PROJECT_COMBAT_H
