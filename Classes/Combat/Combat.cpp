//
// Created by duby0 on 2025/12/7.
//
#include "Combat.h"

CombatManager* CombatManager::instance_ = nullptr;
// Combat类的实现
CombatManager* CombatManager::InitializeInstance(MapManager* map) {
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
        CCLOG("Combat instance not created yet! Call InitializeInstance first.");
        return nullptr;
    }
    return instance_;
}

CombatManager::~CombatManager(){
    DestroyInstance();
}

void CombatManager::DestroyInstance(){
    if (instance_) {
        instance_->map_ = nullptr;
        instance_ = nullptr;
        CCLOG("Combat singleton destroyed!");
    }
}

//初始化战场中的建筑，返回初始化结果
bool CombatManager::Init(MapManager* map){
    this->map_=map;
    if(map_ == nullptr){
        CCLOG("manager init failure : map is null");
        return false;
    }

    if(map->getAllBuildings().empty()){
        CCLOG("no building available");
    }
    for(auto building:map_->getAllBuildings()){
        if(typeid(*building)==typeid(AttackBuilding)){
            auto attack_b = AttackBuildingInCombat::Create(building,map_);
            attack_b->StartAttack();
            live_buildings_.push_back(attack_b);
        }
        else {
            auto b = BuildingInCombat::Create(building, map_);
            live_buildings_.push_back(b);
        }
        if(BuildingInCombat::IsBuildingShouldCount(building)){
            buildings_should_count_++;
        }
        num_of_live_buildings_++;
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

    for(auto it:live_soldiers_){
        it->stopAllActions();
        it->removeFromParent();
    }
    for(auto it:live_buildings_){
        it->stopAllActions();
        it->removeFromParent();
    }
    this->removeFromParent();
    UIManager::getInstance()->endBattle(stars_, destroy_degree_);
    DestroyInstance();
    CCLOG("CombatManager EndCombat() finished");
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
        return;
    }

    //TODO:轮询逻辑应当并非必须，在soldier和building相关实现处进行check
    if (IsCombatEnd()) {
        return; // 已结束，无需后续逻辑
    }
}

//在接收到交互指令后，将士兵加入到战斗中；
void CombatManager::SendSoldier(const Soldier* soldier_template, cocos2d::Vec2 spawn_pos) {
    if (state_ != CombatState::kFighting){
        CCLOG("SendSoldier() when not fighting");
        return;
    }
    auto soldier = SoldierInCombat::Create(soldier_template,spawn_pos,this->map_);
    if (!soldier) { // 创建失败则返回
        std::string name = soldier_template->GetName();
        CCLOG("创建士兵失败，类型：%s",name.c_str());
        return;
    }

    live_soldiers_.push_back(soldier);
    num_of_live_soldiers_++;
}

bool CombatManager::IsCombatEnd() {
//    bool all_soldiers_die = (num_of_live_soldiers_ == 0);
//    for(auto it:soldier_to_use_){
//        if(it.second!=0){
//            all_soldiers_die = false;
//            break;
//        }
//    }
    if(/*all_soldiers_die ||*/ destroy_degree_==100) {
        return true;
    }
    return false;
}