//
// Created by duby0 on 2025/12/7.
//
#include "BuildingInCombat.h"

// -------------------------- 工厂方法实现 --------------------------
BuildingInCombat* BuildingInCombat::Create(Building* soldier_template,MapManager* map) {
    auto soldier = new (std::nothrow) BuildingInCombat();
    if (soldier && soldier->Init(soldier_template,map)) {
        soldier->autorelease();  // Cocos2d-x自动内存管理
        return soldier;
    }
    CC_SAFE_DELETE(soldier);
    return nullptr;
}

BuildingInCombat::~BuildingInCombat() {
    building_template_ = nullptr; // 清空模板指针
}

bool BuildingInCombat::Init(const Building* building_template,MapManager* map) {
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

    this->building_template_ = building_template;
    current_health_ = building_template->GetHealth();

    if(!this->initWithTexture(building_template->getTexture())){
        CCLOG("BuildingInCombat init failed: init texture failure!");
        return false;
    }

    map_ = map;
    // 只有在map_不为nullptr时才调用addChild
    if (map_ != nullptr) {
        map_->addChild(this,0);
    }
    else{
        CCLOG("BuildingInCombat init failed: get map failure!");
        return false;
    }

    this->position_ = building_template->GetPosition();
    this->setPosition(map->vecToWorld(position_));
    this->setScale(0.2f);  // 调整大小（根据实际资源修改）

    CCLOG("Building init success");
    return true;
}

//返回值说明建筑收到伤害后是否仍然存活
bool BuildingInCombat::TakeDamage(int damage) {
    current_health_ -= damage;
    if (current_health_ <= 0) {
        current_health_ = 0;
        Die();
        return false;
    }
    return true;
}

AttackBuildingInCombat* AttackBuildingInCombat::Create(const Building* building_template, MapManager* map) {
    auto soldier = new (std::nothrow) AttackBuildingInCombat();
    if (soldier && soldier->Init(building_template, map)) {
        soldier->autorelease();  // Cocos2d-x自动内存管理
        return soldier;
    }
    CC_SAFE_DELETE(soldier);
    return nullptr;
}

bool AttackBuildingInCombat::Init(const Building *building_template, MapManager *map) {
    if(!BuildingInCombat::Init(building_template,map)){
        CCLOG("attack building father init failure");
        return false;
    }

    auto attack_building_template = dynamic_cast<const AttackBuilding*>(building_template);
    if(!attack_building_template){
        CCLOG("AttackBuildingInCombat Init Failure : unexpected invalid template");
        return false;
    }
    else{
        this->attack_damage_ = attack_building_template->attack_damage_;
        this->attack_interval_ = attack_building_template->attack_interval_;
        this->attack_range_ = attack_building_template->attack_range_;
    }

    return true;
}

void AttackBuildingInCombat::DealDamageToTarget() {
    if (current_target_ && current_target_->is_alive_) {
        current_target_->TakeDamage(attack_damage_);  // 调用建筑的受伤害方法
    }
    CCLOG("current soldier health:%d",current_target_->GetCurrentHealth());
}

void AttackBuildingInCombat::StartAttack() {
    auto single_attack = cocos2d::CallFunc::create([this]() {
        this->ChooseTarget();
        if(current_target_) this->DealDamageToTarget();
    });
    // 单轮检测：延迟0.1秒 → 执行检测
    auto single_check_loop = cocos2d::Sequence::create(
            cocos2d::DelayTime::create(attack_interval_),
            single_attack,
            nullptr
    );
    auto repeat_attack = cocos2d::RepeatForever::create(single_check_loop);
    this->runAction(repeat_attack);
}

void AttackBuildingInCombat::ChooseTarget(){
    if(current_target_!= nullptr && current_target_->is_alive_) return;
    auto soldiers = CombatManager::GetInstance()->live_soldiers;
    if(soldiers.empty()) CCLOG("no target for building");
    SoldierInCombat* target = *std::min_element(soldiers.begin(),soldiers.end(),
                                                 [&](SoldierInCombat* a,SoldierInCombat* b){
                                                     return this->position_.distance(a->position_)<this->position_.distance(b->position_);
                                                 });
    if(this->position_.distance(target->position_)<=attack_range_){
        current_target_ = target;
    }
    else{
        current_target_ = nullptr;
    }
}

void BuildingInCombat::Die() {
    for(auto s:subscribers){
        s->stopAllActions();  // 目标死亡，停止当前攻击动作
        s->current_target_ = nullptr;
    }

    auto manager = CombatManager::GetInstance();
    manager->num_of_live_buildings--;
    if(IsBuildingShouldCount(building_template_)) manager->buildings_should_count_destroyed++;
    manager->destroy_degree_ = 100*manager->buildings_should_count_destroyed/manager->buildings_should_count;
    UIManager::getInstance()->updateDestructionPercent(manager->destroy_degree_);
    if(typeid(*building_template_)==typeid(TownHall)) manager->stars++;

    CCLOG("live buildings:%d",manager->num_of_live_buildings);
    if(manager->IsCombatEnd()){
        CCLOG("call EndCombat() from building");
        manager->EndCombat();
        return;
    }

    auto it = find(manager->live_buildings_.begin(),manager->live_buildings_.end(),this);
    if(it!=manager->live_buildings_.end()){
        CCLOG("dead building found,erase it");
        manager->live_buildings_.erase(it);
    }
    else{
        CCLOG("dead building unfound");
    }
    for(auto s:subscribers){
        s->DoAllMyActions();
    }

    this->removeFromParent();
    CCLOG("building finish Die()");
}

bool BuildingInCombat::IsBuildingShouldCount(const Building* b) {
    return typeid(*b)!=typeid(WallBuilding);
}
