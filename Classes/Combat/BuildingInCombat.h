//
// Created by duby0 on 2025/12/7.
//

#ifndef PROGRAMMING_PARADIGM_FINAL_PROJECT_BUILDINGINCOMBAT_H
#define PROGRAMMING_PARADIGM_FINAL_PROJECT_BUILDINGINCOMBAT_H

#include "cocos2d.h"
#include "Building/Building.h"

class BuildingInCombat : public cocos2d::Sprite{
public:
    // 构造函数
    BuildingInCombat(Building* b): building_template(b){
        current_health = building_template->GetHealth();
        is_alive_ = true;
    };
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

#endif //PROGRAMMING_PARADIGM_FINAL_PROJECT_BUILDINGINCOMBAT_H
