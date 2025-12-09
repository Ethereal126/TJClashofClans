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

//// -------------------------- 坐标映射（核心：地图→屏幕） --------------------------
//// 示例：斜45°俯视角映射规则（若需其他俯视角可修改）
//Vec2 SoldierInCombat::MapPosToScreenPos(const Vec2& map_pos) const {
//    // 斜45°规则：screen_x = 原点x + map_x*瓦片宽/2 - map_y*瓦片宽/2
//    //            screen_y = 原点y + map_x*瓦片高/4 + map_y*瓦片高/4
//    // 原点默认设为屏幕中心（可外部调整map_origin_修改）
//    if (map_origin_.isZero()) {
//        map_origin_ = Vec2(Director::getInstance()->getVisibleSize().width / 2,
//                           Director::getInstance()->getVisibleSize().height / 2);
//    }
//    float screen_x = map_origin_.x + (map_pos.x * tile_width_ / 2) - (map_pos.y * tile_width_ / 2);
//    float screen_y = map_origin_.y + (map_pos.x * tile_height_ / 4) + (map_pos.y * tile_height_ / 4);
//    return Vec2(screen_x, screen_y);
//}

// -------------------------- 动画加载（4方向5帧+死亡6帧） --------------------------
void SoldierInCombat::LoadSoldierAnimations() {
    auto frame_cache = cocos2d::SpriteFrameCache::getInstance();
    // 1. 加载动画资源plist（需将所有士兵动画帧打包为soldier_anim.plist+png，放在Resources目录）
    frame_cache->addSpriteFramesWithFile("soldier_anim.plist", "soldier_anim.png");

    // 2. 4方向移动动画（每方向5帧）
    const std::string direction_names[] = {"up", "down", "left", "right"};
    for (const auto& dir_name : direction_names) {
        cocos2d::Vector<cocos2d::SpriteFrame*> move_frames;
        for (int i = 1; i <= 5; ++i) { // 严格按要求：每个方向5帧
            std::string frame_name = soldier_template_->GetName() + "move_" + dir_name + "_" + std::to_string(i) + ".png";
            auto frame = frame_cache->getSpriteFrameByName(frame_name);
            if (frame) move_frames.pushBack(frame);
        }
        // 动画缓存：自动释放，全游戏共享
        auto move_anim = cocos2d::Animation::createWithSpriteFrames(move_frames, 0.1f); // 0.1秒/帧
        cocos2d::AnimationCache::getInstance()->addAnimation(move_anim, soldier_template_->GetName() + "move_" + dir_name);
    }

    // 3. 死亡动画（6帧）
    cocos2d::Vector<cocos2d::SpriteFrame*> death_frames;
    for (int i = 1; i <= 6; ++i) { // 严格按要求：死亡6帧
        std::string frame_name = soldier_template_->GetName() + "death_" + std::to_string(i) + ".png";
        auto frame = frame_cache->getSpriteFrameByName(frame_name);
        if (frame) death_frames.pushBack(frame);
    }
    auto death_anim = cocos2d::Animation::createWithSpriteFrames(death_frames, 0.15f);
    cocos2d::AnimationCache::getInstance()->addAnimation(death_anim, soldier_template_->GetName() + "death");
}

enum class MoveDirection {
    UP = 0,
    DOWN = 1,
    LEFT = 2,
    RIGHT = 3
};
// -------------------------- 4方向判断（核心：根据移动向量） --------------------------
MoveDirection SoldierInCombat::GetMoveDirection(const cocos2d::Vec2& move_delta) const {
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
    float move_time = move_distance / soldier_template_->move_speed; // 基于数据类的移动速度

    // 2. 创建MoveTo（目标为地图坐标→屏幕坐标）
    cocos2d::Vec2 target_screen_pos = MapPosToScreenPos(target_map_pos);
    auto move_to = cocos2d::MoveTo::create(move_time, target_screen_pos);

    // 3. 获取对应方向的动画
    MoveDirection dir = GetMoveDirection(move_delta);
    const std::string direction_names[] = {"up", "down", "left", "right"};
    std::string dir_name = direction_names[static_cast<int>(dir)];
    auto move_anim = cocos2d::AnimationCache::getInstance()->getAnimation(soldier_template_->name + "move_" + dir_name);
    auto animate = cocos2d::RepeatForever::create(cocos2d::Animate::create(move_anim)); // 移动期间循环播放动画

    // 4. Spawn同步两个Action（Cocos自动更新，无需手动调度）
    return cocos2d::Spawn::create(move_to, animate, nullptr);
}

// -------------------------- 死亡动画实现 --------------------------
void SoldierInCombat::PlayDeathAnim() {
    auto death_anim = cocos2d::AnimationCache::getInstance()->getAnimation("Death");
    auto animate = cocos2d::Animate::create(death_anim);
    auto remove_self = cocos2d::CallFunc::create([this]() { this->removeFromParent(); });
    auto death_sequence = cocos2d::Sequence::create(animate, remove_self, nullptr);
    this->runAction(death_sequence);
}