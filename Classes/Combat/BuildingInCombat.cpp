//
// Created by duby0 on 2025/12/7.
//
#include "BuildingInCombat.h"

// -------------------------- 工厂方法实现 --------------------------
BuildingInCombat* BuildingInCombat::Create(Building* soldier_template, const cocos2d::Vec2& spawn_pos,MapManager* map) {
    auto soldier = new (std::nothrow) BuildingInCombat();
    if (soldier && soldier->Init(soldier_template, spawn_pos,map)) {
        soldier->autorelease();  // Cocos2d-x自动内存管理
        return soldier;
    }
    CC_SAFE_DELETE(soldier);
    return nullptr;
}

BuildingInCombat::~BuildingInCombat() {
    building_template_ = nullptr; // 清空模板指针
}

bool BuildingInCombat::Init(const Building* building_template,const cocos2d::Vec2& spawn_pos,MapManager* map) {
// 1. 调用父类Sprite::init()确保渲染节点初始化
    if (!cocos2d::Sprite::init()) {
        CCLOG("BuildingInCombat Init Failed: Sprite Init Error");
        return false;
    }
    // 2. 验证兵种模板的有效性
    if (!building_template) {
        CCLOG("BuildingInCombat init failed: Invalid building template!");
        return false;
    }

    is_alive_ = true;
    current_health = building_template->GetHealth();

    if(!this->initWithFile(building_template->texture_)){
        CCLOG("BuildingInCombat init failed: init texture failure!");
        return false;
    }

    this->setPosition(spawn_pos);
    map_ = map;
    // 只有在map_不为nullptr时才调用addChild
    if (map_ != nullptr) {
        map_->addChild(this);
    }
    this->setScale(1.5f);  // 调整大小（根据实际资源修改）

    CCLOG("Building init success");
    return true;
}

void BuildingInCombat::TakeDamage(int damage) {
    current_health -= damage;
    if (current_health < 0) {
        current_health = 0;
        Die();
    }
}

void BuildingInCombat::Attack() {
    // TODO: 实现建筑攻击逻辑
    // 目前为空，需要根据实际游戏设计来实现
}

bool BuildingInCombat::IsAlive() const {
    return is_alive_;
}

void BuildingInCombat::Die() {
    if (!is_alive_) return;

    is_alive_ = false;

    this->removeFromParent();
    auto manager = CombatManager::GetInstance();
    manager->live_buildings--;
    if(manager->IsCombatEnd()){
        manager->EndCombat();
    }
}
