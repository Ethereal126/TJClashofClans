//
// Created by duby0 on 2025/12/7.
//
#include "BuildingInCombat.h"

BuildingInCombat::~BuildingInCombat() {
    building_template = nullptr; // 清空模板指针
}

bool BuildingInCombat::Init(const std::string& filename) {
    if (!cocos2d::Sprite::initWithFile(filename)) {
        CCLOG("BuildingInCombat Init Failed: Sprite Init Error");
        return false;
    }
    return true;
}

void BuildingInCombat::TakeDamage(int damage) {
    current_health -= damage;
    if (current_health < 0) {
        current_health = 0;
        Destroyed();
    }
}

void BuildingInCombat::Attack() {
    // TODO: 实现建筑攻击逻辑
    // 目前为空，需要根据实际游戏设计来实现
}

bool BuildingInCombat::IsAlive() const {
    return is_alive_;
}

void BuildingInCombat::Destroyed() {
    if (!is_alive_) return;
    
    is_alive_ = false;
    // TODO: 添加建筑销毁动画和逻辑
    this->removeFromParent();
}
