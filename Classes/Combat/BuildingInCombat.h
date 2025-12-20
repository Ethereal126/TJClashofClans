//
// Created by duby0 on 2025/12/7.
//

#ifndef PROGRAMMING_PARADIGM_FINAL_PROJECT_BUILDINGINCOMBAT_H
#define PROGRAMMING_PARADIGM_FINAL_PROJECT_BUILDINGINCOMBAT_H

#include "cocos2d.h"
#include "Building/Building.h"
#include "MapManager/MapManager.h"
#include "Combat.h"

class BuildingInCombat : public cocos2d::Sprite{
public:
    cocos2d::Vec2 position_;
    std::vector<SoldierInCombat*> subscribers;
    const Building* building_template_;
    // 构造函数
    static BuildingInCombat* Create(Building* building_template,MapManager* map);
    // 析构函数
    ~BuildingInCombat() override;

    // 初始化函数
    virtual bool Init(const Building* building_template,MapManager* map);

    // 被攻击函数
    virtual void TakeDamage(int damage);

    void Die();

    int GetCurrentHealth() const{return current_health_;};
private:
    int current_health_;
    MapManager* map_;
};

class AttackBuildingInCombat : public BuildingInCombat{
public:
    static AttackBuildingInCombat* Create(const Building* building_template, MapManager* map);
    bool Init(const Building* building_template,MapManager* map) override;
    void StartAttack();
private:
    SoldierInCombat* current_target_;
    int attack_damage_;
    float attack_range_,attack_interval_;

    void DealDamageToTarget();
    void ChooseTarget();
};

#endif //PROGRAMMING_PARADIGM_FINAL_PROJECT_BUILDINGINCOMBAT_H
