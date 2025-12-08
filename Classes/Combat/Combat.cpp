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
        LoadAnimationFrames();
        is_animation_loaded_ = true;
    }

    // 4. 设置初始状态
    this->setPosition(spawn_pos);
    this->setScale(0.5f);  // 调整大小（根据实际资源修改）

    return true;
}

// -------------------------- 动画资源加载 --------------------------
void SoldierInCombat::LoadAnimationFrames() {
    auto frame_cache = cocos2d::SpriteFrameCache::getInstance();
    // 加载动画帧缓存文件（需提前将soldier_anim.plist/soldier_anim.png放入Resources目录）
    frame_cache->addSpriteFramesWithFile("soldier_anim.plist", "soldier_anim.png");

    std::string cpp_string_soldier_name = soldier_template_->GetName();
    auto soldier_name = cpp_string_soldier_name.c_str();
    // 加载移动动画帧（帧名称示例：barbarian_move_1.png ~ barbarian_move_4.png）
    for (int i = 1; i <= 4; ++i) {
        auto frame_name = cocos2d::StringUtils::format("%s_move_%d.png",soldier_name, i);
        auto frame = frame_cache->getSpriteFrameByName(frame_name);
        if (frame) move_frames_.pushBack(frame);
    }

    // 加载攻击动画帧（示例：barbarian_attack_1.png ~ barbarian_attack_3.png）
    for (int i = 1; i <= 3; ++i) {
        auto frame_name = cocos2d::StringUtils::format("%s_attack_%d.png",soldier_name, i);
        auto frame = frame_cache->getSpriteFrameByName(frame_name);
        if (frame) attack_frames_.pushBack(frame);
    }

    // 加载死亡动画帧（示例：barbarian_die_1.png ~ barbarian_die_5.png）
    for (int i = 1; i <= 5; ++i) {
        auto frame_name = cocos2d::StringUtils::format("%s_die_%d.png",soldier_name, i);
        auto frame = frame_cache->getSpriteFrameByName(frame_name);
        if (frame) die_frames_.pushBack(frame);
    }
}

// -------------------------- 对外接口实现 --------------------------
void SoldierInCombat::SetTarget(BuildingInCombat* target) {
    if (!is_alive_ || !target || !target->IsAlive()) return;

    // 核心：停止当前所有Action（避免旧行为与新行为冲突）
    this->stopAllActions();
    current_target_ = target;
    MoveToTarget();  // 开始新行为：移动到目标
}

void SoldierInCombat::Die() {
    if (!is_alive_) return;

    is_alive_ = false;
    this->stopAllActions();  // 停止所有当前动作

    // 死亡动画序列：播放死亡帧动画 → 动画结束后自动移除节点
    auto die_animation = cocos2d::Animation::createWithSpriteFrames(die_frames_, 0.2f);
    auto die_animate = cocos2d::Animate::create(die_animation);
    auto remove_self = cocos2d::CallFunc::create(std::bind(&SoldierInCombat::removeFromParent, this));
    auto die_sequence = cocos2d::Sequence::create(die_animate, remove_self, nullptr);
    this->runAction(die_sequence);  // Action自动更新，无需手动管理
}


// -------------------------- 行为逻辑实现 --------------------------
void SoldierInCombat::MoveToTarget() {
    if (!current_target_ || !current_target_->IsAlive()) return;

    // 计算移动时间：距离 ÷ 速度（保证移动速度一致）
    auto target_pos = current_target_->getPosition();
    auto distance = location_.cocos2d::Vec2::distance(target_pos);
    auto move_time = distance / soldier_template_->GetMoveSpeed();

    // 移动Action组合：
    // 1. MoveTo：自动更新位置到目标点
    // 2. Animate：自动播放移动帧动画
    // 3. CallFunc：移动结束后触发攻击
    auto move_to = cocos2d::MoveTo::create(move_time, target_pos);
    auto move_animation = cocos2d::Animation::createWithSpriteFrames(move_frames_, 0.1f);
    auto move_animate = cocos2d::RepeatForever::create(cocos2d::Animate::create(move_animation));
    auto move_complete = cocos2d::CallFunc::create(std::bind(&SoldierInCombat::StartAttack, this));
    auto move_sequence = cocos2d::Sequence::create(
            cocos2d::Spawn::create(move_to, move_animate, nullptr),  // 并行执行移动和帧动画
            move_complete,                                             // 移动结束后回调
            nullptr
    );
    this->runAction(move_sequence);  // 启动自动更新
}

void SoldierInCombat::StartAttack() {
    if (!is_alive_ || !current_target_ || !current_target_->IsAlive()) return;

    // 攻击Action序列（无限循环，直到目标死亡或士兵死亡）：
    // 1. 播放攻击帧动画 → 2. 攻击间隔延迟 → 3. 造成伤害 → 4. 检查目标存活
    auto attack_animation = cocos2d::Animation::createWithSpriteFrames(attack_frames_, 0.15f);
    auto attack_animate = cocos2d::Animate::create(attack_animation);
    auto attack_delay = cocos2d::DelayTime::create(soldier_template_->GetAttackDelay());
    auto deal_damage = cocos2d::CallFunc::create(std::bind(&SoldierInCombat::DealDamageToTarget, this));
    auto check_target = cocos2d::CallFunc::create(std::bind(&SoldierInCombat::CheckTargetAlive, this));
    auto attack_sequence = cocos2d::RepeatForever::create(
            cocos2d::Sequence::create(attack_animate, attack_delay, deal_damage, check_target, nullptr)
    );
    this->runAction(attack_sequence);  // 启动自动攻击循环
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
    return nullptr;
}