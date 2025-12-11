//
// Created by duby0 on 2025/12/7.
//
#include "Combat.h"

// Combat类的实现
Combat::Combat() {
    // 初始化Combat类的成员变量
    combat_root_node_ = cocos2d::Node::create();
    combat_root_node_->retain(); // 手动管理内存，防止被自动释放
}

Combat::~Combat() {
    if (combat_root_node_) {
        combat_root_node_->release();
        combat_root_node_ = nullptr;
    }
}

Combat& Combat::GetInstanceInternal() {
    static Combat instance; // 第一次调用时初始化，仅初始化一次
    return instance;
}

Combat& Combat::GetInstance() {
    return GetInstanceInternal();
}

//初始化战场中的建筑，返回初始化结果
bool Combat::Init(Map* map){
    if(map== nullptr) return false;
    this->map_=map;
    for(auto building:map_->getAllBuildings()){
        auto b = new BuildingInCombat(building);
        combat_root_node_->addChild(b);
        buildings_.push_back(b);
    }
    destroy_degree_ = 0;
    is_inited_ = true;
    return true;
}

void Combat::Reset() {
    // 1. 清理可视化节点（Cocos2d-x节点树）
    if (combat_root_node_) {
        combat_root_node_->removeAllChildrenWithCleanup(true);
        combat_root_node_ = nullptr;
    }

    // 2. 释放士兵/建筑内存（纯C++手动管理）
    for (auto soldier : soldiers_) {
        delete soldier;
    }
    soldiers_.clear();

    for (auto building : buildings_) {
        delete building;
    }
    buildings_.clear();

    // 3. 重置状态
    map_ = nullptr;
    destroy_degree_ = 0;
    is_inited_ = false;
}

//在接收到交互指令后，将士兵加入到战斗中；
void Combat::SendSoldier(Soldier* soldier_template,cocos2d::Vec2 spawn_pos) {
    if (!is_inited_) return;
    auto soldier = SoldierInCombat::Create(soldier_template,spawn_pos);
    if (!soldier) { // 创建失败则返回
        std::string name = soldier_template->GetName();
        CCLOG("创建士兵失败，类型：%s",name.c_str());
        return;
    }
    combat_root_node_->addChild(soldier);
    soldiers_.push_back(soldier);
}

//退出战斗，返回破坏度（destroy_degree_）
int Combat::EndCombat(){
    int result = destroy_degree_;
    Reset(); // 战斗结束自动清理
    return result;
}
