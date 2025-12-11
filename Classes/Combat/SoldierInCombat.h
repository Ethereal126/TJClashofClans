//
// Created by duby0 on 2025/12/7.
//

#ifndef PROGRAMMING_PARADIGM_FINAL_PROJECT_SOLDIERINCOMBAT_H
#define PROGRAMMING_PARADIGM_FINAL_PROJECT_SOLDIERINCOMBAT_H

#include "cocos2d.h"
#include "cocos-ext.h"
#include "Soldier/Soldier.h"
#include "Map/Map.h"

class BuildingInCombat;

class SoldierInCombat : public cocos2d::Sprite{
public:
    ~SoldierInCombat();
    cocos2d::Vec2 location_;
    bool is_alive_;

    static SoldierInCombat* Create(Soldier* soldier_template, const cocos2d::Vec2& spawn_pos);
    // 初始化函数
    bool Init(Soldier* soldier_template, const cocos2d::Vec2& spawn_pos);
    // 被攻击函数
    void TakeDamage(int damage);

private:
    Soldier* soldier_template_;
    int current_health_;
    BuildingInCombat* current_target_;
    Map* map_;

    void SetTarget(BuildingInCombat* target);
    void Die();
    void MoveToTarget();
    void StartAttack();
    void DealDamageToTarget();
    void CheckTargetAlive();
    BuildingInCombat* GetNextTarget() const;
    cocos2d::Spawn* CreateStraightMoveAction(const cocos2d::Vec2& target_map_pos);
    void RedirectPath(std::vector<cocos2d::Vec2>& path);
    static void SimplifyPath(std::vector<cocos2d::Vec2>& path);
    // -------------------------- 动画资源（静态共享） --------------------------
    void LoadSoldierAnimations();
    static bool is_animation_loaded_; // 标记动画资源是否已加载（避免重复加载）
};

#endif //PROGRAMMING_PARADIGM_FINAL_PROJECT_SOLDIERINCOMBAT_H
