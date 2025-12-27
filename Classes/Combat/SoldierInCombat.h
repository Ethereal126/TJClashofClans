//
// Created by duby0 on 2025/12/7.
//

#ifndef PROGRAMMING_PARADIGM_FINAL_PROJECT_SOLDIERINCOMBAT_H
#define PROGRAMMING_PARADIGM_FINAL_PROJECT_SOLDIERINCOMBAT_H

#include "cocos2d.h"
#include "cocos-ext.h"
#include "Soldier/Soldier.h"
#include "MapManager/MapManager.h"
#include "HpBarUtils.h"
#include "AudioManager/AudioManager.h"

class BuildingInCombat;

class SoldierInCombat : public cocos2d::Sprite{
public:
    cocos2d::Vec2 position_;
    const Soldier* soldier_template_;
    MapManager* map_;

    static SoldierInCombat* Create(const Soldier* soldier_template, const cocos2d::Vec2& spawn_pos,MapManager* map);
    // 初始化函数
    bool Init(const Soldier* soldier_template, const cocos2d::Vec2& spawn_pos,MapManager* map);
    // 被攻击函数
    void TakeDamage(int damage);

    void DoAllMyActions();

    BuildingInCombat* current_target_;
    void Die();

    int GetCurrentHealth() const{return current_health_;};
protected:
    int current_health_;
    static const std::string direction_names[4];
    HpBarComponents hp_bar_;

    ~SoldierInCombat() override;
    void MoveToTargetAndStartAttack();
    void Attack(const cocos2d::Vec2& pos);
    void BomberAttack(const cocos2d::Vec2& pos);
    void DealDamageToBuilding(BuildingInCombat* target) const;
    void DealSplashDamage(const cocos2d::Vec2& pos);
    void UpdatePosition();
    BuildingInCombat* GetNextTarget();
    void NotifyManagerDie();

    cocos2d::Spawn* CreateStraightMoveAction(const cocos2d::Vec2& start_map_pos,const cocos2d::Vec2& target_map_pos);
    void RedirectPath(std::vector<cocos2d::Vec2>& path);
    static void SimplifyPath(std::vector<cocos2d::Vec2>& path);
    void LogPath(const std::vector<cocos2d::Vec2>& path) const;


    // -------------------------- 动画资源（静态共享） --------------------------
    void LoadSoldierAnimations() const;
    static bool is_animation_loaded_[static_cast<int>(SoldierType::kSoldierTypes)]; // 标记动画资源是否已加载（避免重复加载）
};

#endif //PROGRAMMING_PARADIGM_FINAL_PROJECT_SOLDIERINCOMBAT_H
