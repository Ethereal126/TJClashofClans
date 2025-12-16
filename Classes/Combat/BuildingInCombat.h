//
// Created by duby0 on 2025/12/7.
//

#ifndef PROGRAMMING_PARADIGM_FINAL_PROJECT_BUILDINGINCOMBAT_H
#define PROGRAMMING_PARADIGM_FINAL_PROJECT_BUILDINGINCOMBAT_H

#include "cocos2d.h"
#include "Building/Building.h"
#include "Map/MapManager.h"
#include "Combat.h"

class BuildingInCombat : public cocos2d::Sprite{
public:
    // 构造函数
    static BuildingInCombat* Create(Building* building_template,const cocos2d::Vec2& spawn_pos,MapManager* map);
    // 析构函数
    virtual ~BuildingInCombat();

    // 初始化函数
    virtual bool Init(const Building* building_template,const cocos2d::Vec2& spawn_pos,MapManager* map);

    // 被攻击函数
    virtual void TakeDamage(int damage);

    //使用时应确保建筑类型为防御类建筑，函数参数有待确定，暂时考虑通过轮询方式搜索攻击目标，如果有更好的想法再做优化
    void Attack();

    // 判断是否存活
    virtual bool IsAlive() const;

    void Die();

private:
    Building* building_template_;
    int current_health;
    bool is_alive_;
    MapManager* map_;
};

#endif //PROGRAMMING_PARADIGM_FINAL_PROJECT_BUILDINGINCOMBAT_H
