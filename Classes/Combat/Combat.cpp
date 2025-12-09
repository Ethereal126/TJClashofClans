//
// Created by duby0 on 2025/12/7.
//
#include "Combat.h"
#include "Soldier/Soldier.h"

cocos2d::Vector<cocos2d::SpriteFrame*> SoldierInCombat::move_frames_;
cocos2d::Vector<cocos2d::SpriteFrame*> SoldierInCombat::attack_frames_;
cocos2d::Vector<cocos2d::SpriteFrame*> SoldierInCombat::die_frames_;

bool SoldierInCombat::is_animation_loaded_ = false;

// -------------------------- 工厂方法实现 --------------------------
SoldierInCombat* SoldierInCombat::Create(Soldier* soldier_template, const cocos2d::Vec2& spawn_pos) {
    auto soldier = new (std::nothrow) SoldierInCombat();
    if (soldier && soldier->Init(soldier_template, spawn_pos)) {
        soldier->autorelease();  // Cocos2d-x自动内存管理
        return soldier;
    }
    CC_SAFE_DELETE(soldier);
    return nullptr;
}

SoldierInCombat::~SoldierInCombat() {
    current_target_ = nullptr;  // 清空目标指针，避免悬空
}

// -------------------------- 初始化实现 --------------------------
bool SoldierInCombat::Init(Soldier* soldier_template, const cocos2d::Vec2& spawn_pos) {
    // 1. 调用父类Sprite::init()确保渲染节点初始化
    if (!cocos2d::Sprite::init()) {
        CCLOG("SoldierInCombat Init Failed: Sprite Init Error");
        return false;
    }
    // 2. 验证兵种模板的有效性
    if (!soldier_template) {
        CCLOG("SoldierInCombat init failed: Invalid soldier template!");
        return false;
    }

    // 3.设置兵种属性
    is_alive_ = true;
    soldier_template_ = soldier_template;
    location_ = spawn_pos;
    current_health_ = soldier_template->GetHealth();
    current_target_ = nullptr;

    if (!is_animation_loaded_) {
        LoadSoldierAnimations();
        is_animation_loaded_ = true;
    }

    // 4. 设置初始状态
    this->setPosition(spawn_pos);
    map_->addChild(this);
    this->setScale(0.5f);  // 调整大小（根据实际资源修改）

    return true;
}

void SoldierInCombat::TakeDamage(int damage) {
    current_health_ -= damage;
    if (current_health_ < 0) current_health_ = 0;
}


// -------------------------- 动画加载（4方向6帧+死亡7帧） --------------------------
void SoldierInCombat::LoadSoldierAnimations() {
    auto frame_cache = cocos2d::SpriteFrameCache::getInstance();
    // 1. 加载动画资源plist（需将所有士兵动画帧打包为soldier_anim.plist+png，放在Resources目录）
    frame_cache->addSpriteFramesWithFile("soldier_anim.plist", "soldier_anim.png");

    // 2. 4方向移动动画（每方向6帧）
    const std::string direction_names[] = {"up", "down", "left", "right"};
    for (const auto& dir_name : direction_names) {
        cocos2d::Vector<cocos2d::SpriteFrame*> move_frames;
        for (int i = 0; i <= 5; ++i) {
            std::string frame_name =  "Walk_"+ dir_name + "-" +std::to_string(i) + ".png";
            auto frame = frame_cache->getSpriteFrameByName(frame_name);
            if (frame) move_frames.pushBack(frame);
        }
        // 动画缓存：自动释放，全游戏共享
        auto move_anim = cocos2d::Animation::createWithSpriteFrames(move_frames, 0.1f); // 0.1秒/帧
        cocos2d::AnimationCache::getInstance()->addAnimation(move_anim, "Soldier_Move_" + dir_name);
    }

    // 3. 死亡动画（7帧）
    cocos2d::Vector<cocos2d::SpriteFrame*> death_frames;
    for (int i = 0; i <= 6; ++i) { // 严格按要求：死亡6帧
        std::string frame_name =  "Death-"+ std::to_string(i) +".png";
        auto frame = frame_cache->getSpriteFrameByName(frame_name);
        if (frame) death_frames.pushBack(frame);
    }
    auto death_anim = cocos2d::Animation::createWithSpriteFrames(death_frames, 0.15f);
    cocos2d::AnimationCache::getInstance()->addAnimation(death_anim, "Soldier_Death");
}



// -------------------------- 4方向判断（核心：根据移动向量） --------------------------
enum class MoveDirection :int{
    UP = 0,
    DOWN = 1,
    LEFT = 2,
    RIGHT = 3
};
MoveDirection GetMoveDirection(const cocos2d::Vec2& move_delta){
    if (move_delta.isZero()) return MoveDirection::DOWN; // 默认方向
    float abs_x = abs(move_delta.x);
    float abs_y = abs(move_delta.y);
    // x分量占比大→左右方向；y分量占比大→上下方向
    if (abs_x > abs_y) {
        return move_delta.x > 0 ? MoveDirection::RIGHT : MoveDirection::LEFT;
    } else {
        return move_delta.y > 0 ? MoveDirection::UP : MoveDirection::DOWN;
    }
}

// -------------------------- 核心：封装单段直线移动Action（Spawn同步MoveTo+动画） --------------------------
cocos2d::Spawn* SoldierInCombat::CreateStraightMoveAction(const cocos2d::Vec2& target_map_pos) {
    // 1. 计算移动参数
    cocos2d::Vec2 current_map_pos = this->getPosition(); // 若需直接用地图坐标，可外部保存map_pos变量
    cocos2d::Vec2 move_delta = target_map_pos - current_map_pos;
    float move_distance = move_delta.length();
    float move_time = move_distance / soldier_template_->GetMoveSpeed(); // 基于数据类的移动速度

    // 2. 创建MoveTo（目标为地图坐标→屏幕坐标）
    cocos2d::Vec2 target_screen_pos = map_->vecToWorld(target_map_pos);
    auto move_to = cocos2d::MoveTo::create(move_time, target_screen_pos);

    // 3. 获取对应方向的动画
    MoveDirection dir = GetMoveDirection(move_delta);
    const std::string direction_names[] = {"up", "down", "left", "right"};
    std::string dir_name = direction_names[static_cast<int>(dir)];
    auto move_anim = cocos2d::AnimationCache::getInstance()->getAnimation( "Soldier_Move_" + dir_name);
    auto animate = cocos2d::RepeatForever::create(cocos2d::Animate::create(move_anim)); // 移动期间循环播放动画

    // 4. Spawn同步两个Action（Cocos自动更新，无需手动调度）
    return cocos2d::Spawn::create(move_to, animate, nullptr);
}

// -------------------------- 死亡动画实现 --------------------------
void SoldierInCombat::Die() {
    if (!is_alive_) return;

    is_alive_ = false;
    this->stopAllActions();  // 停止所有当前动作

    auto death_anim = cocos2d::AnimationCache::getInstance()->getAnimation("Soldier_Death");
    auto animate = cocos2d::Animate::create(death_anim);
    auto remove_self = cocos2d::CallFunc::create([this]() { this->removeFromParent(); });
    auto death_sequence = cocos2d::Sequence::create(animate, remove_self, nullptr);
    this->runAction(death_sequence);
}


// -------------------------- 对外接口实现 --------------------------
void SoldierInCombat::SetTarget(BuildingInCombat* target) {
    if (!is_alive_ || !target || !target->IsAlive()) return;

    // 核心：停止当前所有Action（避免旧行为与新行为冲突）
    this->stopAllActions();
    current_target_ = target;
    MoveToTarget();  // 开始新行为：移动到目标
}

void SoldierInCombat::DealDamageToTarget() {
    if (current_target_ && current_target_->IsAlive()) {
        current_target_->TakeDamage(soldier_template_->GetDamage());  // 调用建筑的受伤害方法
    }
}

void SoldierInCombat::CheckTargetAlive() {
    if (!current_target_ || !current_target_->IsAlive()) {
        this->stopAllActions();  // 目标死亡，停止当前攻击动作
        BuildingInCombat* next_target = FindNextTarget();  // 寻找下一个目标
        if (next_target) {
            current_target_ = next_target;
            MoveToTarget();  // 移动到新目标继续攻击
        }
    }
}

BuildingInCombat* SoldierInCombat::FindNextTarget() {
    auto buildings = map_->getAllBuildings();
    for(auto building : buildings){

    }
    return nullptr;
}

