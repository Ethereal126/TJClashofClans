//
// Created by duby0 on 2025/12/7.
//
#include "Combat.h"

CombatManager* CombatManager::instance_ = nullptr;
// Combat类的实现
CombatManager* CombatManager::Create(MapManager* map) {
    // 若已创建，直接返回现有实例（避免重复初始化）
    if (instance_) {
        CCLOG("Combat instance already created! Return existing one.");
        return instance_;
    }

    // 与你 SoldierInCombat 一致的创建逻辑：new → Init → autorelease
    auto combat = new (std::nothrow) CombatManager();
    if (combat && combat->Init(map)) {
        combat->autorelease(); // 接入 Cocos 自动内存管理
        combat->retain();
        instance_ = combat;
        CCLOG("Combat singleton created successfully!");
        return instance_;
    }

    // 初始化失败，释放内存
    CC_SAFE_DELETE(combat);
    instance_ = nullptr;
    CCLOG("Combat singleton create failed!");
    return nullptr;
}

CombatManager* CombatManager::GetInstance() {
    if (!instance_) {
        CCLOG("Combat instance not created yet! Call Create first.");
        return nullptr;
    }
    return instance_;
}

CombatManager::~CombatManager(){
    if (instance_) {
        instance_->EndCombat(); // 强制结束战斗
        instance_->map_ = nullptr;
        instance_->release(); // 释放 Cocos 引用计数
        instance_ = nullptr;
        CCLOG("Combat singleton destroyed!");
    }
}


//初始化战场中的建筑，返回初始化结果
bool CombatManager::Init(MapManager* map){
    if(map== nullptr) return false;
    this->map_=map;
    combat_root_node_ = cocos2d::Node::create();
    combat_root_node_->retain(); // 手动管理内存，防止被自动释放

    for(auto building:map_->getAllBuildings()){
        auto b = BuildingInCombat::Create(building,building->GetPosition(),map_);
        combat_root_node_->addChild(b);
        buildings_.push_back(b);
        live_buildings++;
    }
    destroy_degree_ = 0;
    state_ = CombatState::kReady;
    return true;
}

// 开始战斗：启动帧循环，标记战斗状态
void CombatManager::StartCombat() {
    if (state_ != CombatState::kReady) {
        CCLOG("CombatManager is not in Ready state, cannot start!");
        return;
    }

    state_ = CombatState::kFighting;
    combat_time_ = 0.0f;
    this->scheduleUpdate();
    CCLOG("CombatManager started!");
}

// 暂停战斗：暂停帧更新
void CombatManager::PauseCombat() {
    if (state_ != CombatState::kFighting){
        CCLOG("unexpected pause command");
        return;
    }
    this->pause();
    CCLOG("CombatManager paused!");
}

// 恢复战斗：恢复帧更新
void CombatManager::ResumeCombat() {
    if (state_ != CombatState::kFighting) {
        CCLOG("unexpected resume command");
        return;
    }
    this->resume();
    CCLOG("CombatManager resumed!");
}

// 结束战斗：停止帧更新，触发回调
void CombatManager::EndCombat() {
    if (state_ == CombatState::kEnded){
        CCLOG("use of EndCombat() after combat ended");
        return;
    }

    state_ = CombatState::kEnded;
    this->unscheduleUpdate(); // 停止帧检测
}

// 每帧更新：核心检测逻辑（Cocos 帧循环驱动）
void CombatManager::Update(float dt) {
    if (state_ != CombatState::kFighting){
        CCLOG("Update() when not fighting");
        return;
    }

    combat_time_ += dt;
    if (combat_time_ >= kMaxCombatTime) {
        EndCombat();
        winner_ = WinnerState::kDefence;
        return;
    }

    //TODO:轮询逻辑应当并非必须，在soldier和building相关实现处进行check
    if (IsCombatEnd()) {
        return; // 已结束，无需后续逻辑
    }
}

//在接收到交互指令后，将士兵加入到战斗中；
void CombatManager::SendSoldier(Soldier* soldier_template, cocos2d::Vec2 spawn_pos) {
    if (state_ != CombatState::kFighting){
        CCLOG("SendSoldier() when not fighting");
        return;
    }
    auto soldier = SoldierInCombat::Create(soldier_template,spawn_pos,map_);
    if (!soldier) { // 创建失败则返回
        std::string name = soldier_template->GetName();
        CCLOG("创建士兵失败，类型：%s",name.c_str());
        return;
    }

    combat_root_node_->addChild(soldier);
    live_soldiers_++;
}

bool CombatManager::IsCombatEnd() {
    bool all_soldiers_die = (live_soldiers_==0);
    for(auto it:soldier_to_use){
        if(it.second!=0){
            all_soldiers_die = false;
            break;
        }
    }
    if(all_soldiers_die) {
        winner_ = WinnerState::kDefence;
        return true;
    }

    bool all_buildings_die = (live_buildings==0);
    if(all_buildings_die) {
        winner_ = WinnerState::kOffence;
        return true;
    }

    return false;
}