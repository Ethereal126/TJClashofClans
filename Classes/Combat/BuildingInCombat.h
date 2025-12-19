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
    // 构造函数
    static BuildingInCombat* Create(Building* building_template,MapManager* map);
    // 析构函数
    ~BuildingInCombat() override;

    // 初始化函数
    virtual bool Init(const Building* building_template,MapManager* map);

    // 被攻击函数
    virtual void TakeDamage(int damage);

    // 判断是否存活
    virtual bool IsAlive() const;

    void Die();

    int GetCurrentHealth() const{return current_health_;};
private:
    int current_health_;
    Building* building_template_;
    bool is_alive_;
    MapManager* map_;
};

class AttackBuildingInCombat : public BuildingInCombat{
public:
    void StartAttack();
private:
    AttackBuilding* building_template_;
    SoldierInCombat* current_target_;

    void DealDamageToTarget();
    void ChooseTarget();
};

#endif //PROGRAMMING_PARADIGM_FINAL_PROJECT_BUILDINGINCOMBAT_H
