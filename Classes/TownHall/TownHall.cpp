//
// TownHall.cpp
//

#include "TownHall.h"
#include <cmath>

USING_NS_CC;

// ==================== TownHall 实现 ====================

TownHall::TownHall(std::string name, int base, cocos2d::Vec2 position, std::string texture)
    : Building(name, 1, base * 100, base * 5, base * 60, base * 200,
        std::make_pair(static_cast<int>(position.x), static_cast<int>(position.y)))
    , gold_storage_capacity_(base * 2)
    , elixir_storage_capacity_(base * 2)
    , gold_(0)
    , elixir_(0)
    , barrack_capacity_(base)
    , max_gold_capacity_(0)
    , max_elixir_capacity_(0)
    , flag_sprite_(nullptr)
    , level_label_(nullptr) {

    // 设置大本营纹理
    this->setTexture(texture);
    this->setPosition(position);

    // 初始化UI组件
    UpdateLevelLabel();
}

TownHall::~TownHall() {
    // 清理UI组件
    flag_sprite_ = nullptr;
    level_label_ = nullptr;
}

void TownHall::Upgrade() {
    // 调用基类升级
    Building::Upgrade();

    // 提升大本营特有属性
    level_ += 1;
    gold_storage_capacity_ += 1;      // 每级增加1个资源池容量
    elixir_storage_capacity_ += 1;
    barrack_capacity_ += 1;           // 每级增加1个兵营容量

    // 更新资源持有上限
    UpdateAllResourceCapacities();

    // 更新等级显示
    UpdateLevelLabel();

    // 播放升级特效
    PlayUpgradeEffect();

    // 更新纹理（假设纹理命名规则为 "townhall_levelX.png"）
    std::string new_texture = "TownHall/townhall_level" + std::to_string(level_) + ".png";
    this->setTexture(new_texture);

    cocos2d::log("%s 升级到等级 %d，金币池上限: %d，圣水池上限: %d，兵营上限: %d",
        name_.c_str(), level_, gold_storage_capacity_, elixir_storage_capacity_, barrack_capacity_);
}

int TownHall::AddGold(int amount) {
    if (amount <= 0) {
        return 0;
    }

    // 获取总金币容量（金币池容量总和）
    int max_capacity = GetTotalGoldCapacity();
    int current_total = GetTotalGoldFromStorages();

    if (current_total >= max_capacity) {
        cocos2d::log("金币池已达上限，无法存入更多金币");
        PlayFullCapacityAnimation();
        return 0;
    }

    // 计算实际可存入的数量
    int available_space = max_capacity - current_total;
    int actual_add = std::min(amount, available_space);

    // 存入金币池
    int remaining = actual_add;
    for (auto* storage : gold_storages_) {
        if (remaining <= 0) break;

        int space_in_storage = storage->GetCapacity() - storage->GetCurrentAmount();
        if (space_in_storage > 0) {
            int add_to_storage = std::min(remaining, space_in_storage);
            storage->AddResource(add_to_storage);
            remaining -= add_to_storage;
        }
    }

    //返回存入金币数
    cocos2d::log("存入金币: %d，剩余可存入: %d", actual_add, remaining);
    return actual_add;
}


bool TownHall::SpendGold(int amount) {
    if (amount <= 0) {
        return false;
    }

    // 获取总金币
    int total_gold = GetTotalGoldFromStorages();
    if (total_gold < amount) {
        cocos2d::log("金币不足，需要: %d，现有: %d", amount, total_gold);
        PlayNotEnoughAnimation();
        return false;
    }

    // 从金币池消耗
    int remaining = amount;
    for (auto* storage : gold_storages_) {
        if (remaining <= 0) break;

        int gold_in_storage = storage->GetCurrentAmount();
        if (gold_in_storage > 0) {
            int deduct_from_storage = std::min(remaining, gold_in_storage);
            storage->UseResource(deduct_from_storage);
            remaining -= deduct_from_storage;
        }
    }

    cocos2d::log("消耗金币: %d，剩余: %d", amount, total_gold - amount);
    return true;
}


int TownHall::AddElixir(int amount) {
    if (amount <= 0) {
        return 0;
    }

    // 获取总圣水容量（圣水池容量总和）
    int max_capacity = GetTotalElixirCapacity();
    int current_total = GetTotalElixirFromStorages();

    if (current_total >= max_capacity) {
        cocos2d::log("圣水池已达上限，无法存入更多圣水");
        PlayFullCapacityAnimation();
        return 0;
    }

    // 计算实际可存入的数量
    int available_space = max_capacity - current_total;
    int actual_add = std::min(amount, available_space);

    // 存入圣水池
    int remaining = actual_add;
    for (auto* storage : elixir_storages_) {
        if (remaining <= 0) break;

        int space_in_storage = storage->GetCapacity() - storage->GetCurrentAmount();
        if (space_in_storage > 0) {
            int add_to_storage = std::min(remaining, space_in_storage);
            storage->AddResource(add_to_storage);
            remaining -= add_to_storage;
        }
    }

    cocos2d::log("存入圣水: %d，剩余可存入: %d", actual_add, remaining);
    return actual_add;
}

bool TownHall::SpendElixir(int amount) {
    if (amount <= 0) {
        return false;
    }

    // 获取总圣水
    int total_elixir = GetTotalElixirFromStorages();
    if (total_elixir < amount) {
        cocos2d::log("圣水不足，需要: %d，现有: %d", amount, total_elixir);
        PlayNotEnoughAnimation();
        return false;
    }

    // 从圣水池消耗
    int remaining = amount;
    for (auto* storage : elixir_storages_) {
        if (remaining <= 0) break;

        int elixir_in_storage = storage->GetCurrentAmount();
        if (elixir_in_storage > 0) {
            int deduct_from_storage = std::min(remaining, elixir_in_storage);
            storage->UseResource(deduct_from_storage);
            remaining -= deduct_from_storage;
        }
    }

    cocos2d::log("消耗圣水: %d，剩余: %d", amount, total_elixir - amount);
    return true;
}

void TownHall::AddGoldStorage(GoldStorage* gold_storage) {
    if (!gold_storage) {
        return;
    }

    // 检查是否已达到上限
    if (static_cast<int>(gold_storages_.size()) >= gold_storage_capacity_) {
        cocos2d::log("已达到金币池上限，无法添加更多金币池");
        return;
    }

    // 检查是否已存在
    auto it = std::find(gold_storages_.begin(), gold_storages_.end(), gold_storage);
    if (it == gold_storages_.end()) {
        gold_storages_.push_back(gold_storage);
        UpdateMaxGoldCapacity();
        cocos2d::log("添加金币池，当前数量: %zu", gold_storages_.size());
    }
}

void TownHall::RemoveGoldStorage(GoldStorage* gold_storage) {
    if (!gold_storage) {
        return;
    }

    auto it = std::find(gold_storages_.begin(), gold_storages_.end(), gold_storage);
    if (it != gold_storages_.end()) {
        gold_storages_.erase(it);
        UpdateMaxGoldCapacity();
        cocos2d::log("移除金币池，剩余数量: %zu", gold_storages_.size());
    }
}

void TownHall::AddElixirStorage(ElixirStorage* elixir_storage) {
    if (!elixir_storage) {
        return;
    }

    // 检查是否已达到上限
    if (static_cast<int>(elixir_storages_.size()) >= elixir_storage_capacity_) {
        cocos2d::log("已达到圣水池上限，无法添加更多圣水池");
        return;
    }

    // 检查是否已存在
    auto it = std::find(elixir_storages_.begin(), elixir_storages_.end(), elixir_storage);
    if (it == elixir_storages_.end()) {
        elixir_storages_.push_back(elixir_storage);
        UpdateMaxElixirCapacity();
        cocos2d::log("添加圣水池，当前数量: %zu", elixir_storages_.size());
    }
}

void TownHall::RemoveElixirStorage(ElixirStorage* elixir_storage) {
    if (!elixir_storage) {
        return;
    }

    auto it = std::find(elixir_storages_.begin(), elixir_storages_.end(), elixir_storage);
    if (it != elixir_storages_.end()) {
        elixir_storages_.erase(it);
        UpdateMaxElixirCapacity();
        cocos2d::log("移除圣水池，剩余数量: %zu", elixir_storages_.size());
    }
}

int TownHall::GetTotalGoldFromStorages() const {
    int total = 0;
    for (const auto* storage : gold_storages_) {
        if (storage && storage->IsActive()) {
            total += storage->GetCurrentAmount();
        }
    }
    return total;
}

int TownHall::GetTotalElixirFromStorages() const {
    int total = 0;
    for (const auto* storage : elixir_storages_) {
        if (storage && storage->IsActive()) {
            total += storage->GetCurrentAmount();
        }
    }
    return total;
}

int TownHall::GetTotalGoldCapacity() const {
    // 计算金币池的总容量
    int total = 0;
    for (const auto* storage : gold_storages_) {
        if (storage && storage->IsActive()) {
            total += storage->GetCapacity();
        }
    }
    return total;
}

int TownHall::GetTotalElixirCapacity() const {
    // 计算圣水池的总容量
    int total = 0;
    for (const auto* storage : elixir_storages_) {
        if (storage && storage->IsActive()) {
            total += storage->GetCapacity();
        }
    }
    return total;
}

void TownHall::UpdateMaxGoldCapacity() {
    // 计算金币池的总容量并更新大本营的金币容量
    max_gold_capacity_ = GetTotalGoldCapacity();

    // 同时更新UI
    if (level_label_) {
        UpdateLevelLabel();
    }

    cocos2d::log("更新金币持有上限: %d", max_gold_capacity_);
}

void TownHall::UpdateMaxElixirCapacity() {
    // 计算圣水池的总容量并更新大本营的圣水容量
    max_elixir_capacity_ = GetTotalElixirCapacity();

    // 同时更新UI
    if (level_label_) {
        UpdateLevelLabel();
    }

    cocos2d::log("更新圣水持有上限: %d", max_elixir_capacity_);
}

void TownHall::UpdateAllResourceCapacities() {
    UpdateMaxGoldCapacity();
    UpdateMaxElixirCapacity();
}

bool TownHall::IsGoldFull() const {
    int total_gold = gold_ + GetTotalGoldFromStorages();
    int total_capacity = GetTotalGoldCapacity();
    return total_gold >= total_capacity;
}

bool TownHall::IsElixirFull() const {
    int total_elixir = elixir_ + GetTotalElixirFromStorages();
    int total_capacity = GetTotalElixirCapacity();
    return total_elixir >= total_capacity;
}

void TownHall::UpdateLevelLabel() {
    // 移除旧的标签
    if (level_label_) {
        level_label_->removeFromParent();
    }

    // 创建新的等级标签
    level_label_ = Label::createWithTTF("Lv." + std::to_string(level_), "fonts/Marker Felt.ttf", 20);
    if (level_label_) {
        level_label_->setTextColor(Color4B::YELLOW);
        level_label_->enableOutline(Color4B::BLACK, 2);
        level_label_->setPosition(Vec2(0, this->getContentSize().height / 2 + 10));
        this->addChild(level_label_, 10);
    }

    // 如果有旗帜精灵，也更新它的位置
    if (flag_sprite_) {
        flag_sprite_->setPosition(Vec2(0, this->getContentSize().height / 2 + 40));
    }
}

void TownHall::PlayUpgradeEffect() {
    // 播放粒子特效
    auto particles = ParticleSystemQuad::create("particles/upgrade.plist");
    if (particles) {
        particles->setPosition(Vec2::ZERO);
        particles->setAutoRemoveOnFinish(true);
        this->addChild(particles);
    }

    // 播放缩放动画
    auto scale_up = ScaleTo::create(0.3f, 1.2f);
    auto scale_down = ScaleTo::create(0.3f, 1.0f);
    auto sequence = Sequence::create(scale_up, scale_down, nullptr);
    this->runAction(sequence);

    // 播放颜色闪烁
    auto tint_gold = TintTo::create(0.2f, 255, 215, 0);  // 金色
    auto tint_purple = TintTo::create(0.2f, 147, 112, 219);  // 紫色
    auto tint_normal = TintTo::create(0.2f, 255, 255, 255);  // 白色
    auto tint_sequence = Sequence::create(tint_gold, tint_purple, tint_normal, nullptr);
    this->runAction(tint_sequence);

    // 播放音效(如果有的话)
    // CocosDenshion::SimpleAudioEngine::getInstance()->playEffect("sounds/upgrade.wav");

    cocos2d::log("%s 升级特效播放完成", name_.c_str());
}

void TownHall::PlayDestroyedEffect() {
    // 播放爆炸粒子特效
    auto explosion = ParticleSystemQuad::create("particles/explosion.plist");
    if (explosion) {
        explosion->setPosition(Vec2::ZERO);
        explosion->setAutoRemoveOnFinish(true);
        this->addChild(explosion);
    }

    // 播放烟雾特效
    auto smoke = ParticleSystemQuad::create("particles/smoke.plist");
    if (smoke) {
        smoke->setPosition(Vec2::ZERO);
        smoke->setAutoRemoveOnFinish(true);
        this->addChild(smoke);
    }

    // 播放震动效果
    auto shake_right = MoveBy::create(0.05f, Vec2(10, 0));
    auto shake_left = MoveBy::create(0.05f, Vec2(-20, 0));
    auto shake_back = MoveBy::create(0.05f, Vec2(10, 0));
    auto shake_sequence = Sequence::create(shake_right, shake_left, shake_back, nullptr);
    this->runAction(shake_sequence);

    // 播放渐隐效果
    auto fade_out = FadeOut::create(1.0f);
    this->runAction(fade_out);

    // 播放音效(如果有的话)
    // CocosDenshion::SimpleAudioEngine::getInstance()->playEffect("sounds/destroyed.wav");

    cocos2d::log("%s 被摧毁，游戏结束", name_.c_str());
}

void TownHall::PlayFullCapacityAnimation() {
    // 播放满容量提示动画
    auto label = Label::createWithTTF("已满!", "fonts/Marker Felt.ttf", 24);
    label->setColor(Color3B::RED);
    label->setPosition(Vec2(0, this->getContentSize().height / 2 + 50));
    this->addChild(label);

    auto fade_in = FadeIn::create(0.3f);
    auto fade_out = FadeOut::create(0.7f);
    auto remove = RemoveSelf::create();
    auto sequence = Sequence::create(fade_in, fade_out, remove, nullptr);
    label->runAction(sequence);

    // 播放闪烁效果
    auto blink = Blink::create(1.0f, 3);
    this->runAction(blink);
}

void TownHall::PlayNotEnoughAnimation() {
    // 播放资源不足提示动画
    auto label = Label::createWithTTF("资源不足!", "fonts/Marker Felt.ttf", 24);
    label->setColor(Color3B::RED);
    label->setPosition(Vec2(0, this->getContentSize().height / 2 + 50));
    this->addChild(label);

    auto fade_in = FadeIn::create(0.2f);
    auto fade_out = FadeOut::create(0.8f);
    auto remove = RemoveSelf::create();
    auto sequence = Sequence::create(fade_in, fade_out, remove, nullptr);
    label->runAction(sequence);

    // 播放抖动效果
    auto shake = MoveBy::create(0.05f, Vec2(5, 0));
    auto shake_back = MoveBy::create(0.05f, Vec2(-10, 0));
    auto shake_again = MoveBy::create(0.05f, Vec2(5, 0));
    auto shake_sequence = Sequence::create(shake, shake_back, shake_again, nullptr);
    this->runAction(shake_sequence);
}

void TownHall::ShowInfo() const {
    // 调用基类显示基本信息
    Building::ShowInfo();

    // 显示大本营特有信息
    cocos2d::log("=== 大本营信息 ===");
    cocos2d::log("金币池数量: %zu/%d", gold_storages_.size(), gold_storage_capacity_);
    cocos2d::log("圣水池数量: %zu/%d", elixir_storages_.size(), elixir_storage_capacity_);
    cocos2d::log("大本营金币: %d/%d", gold_, max_gold_capacity_);
    cocos2d::log("大本营圣水: %d/%d", elixir_, max_elixir_capacity_);
    cocos2d::log("总金币: %d/%d (%.1f%%)",
        gold_ + GetTotalGoldFromStorages(),
        GetTotalGoldCapacity());
    cocos2d::log("总圣水: %d/%d (%.1f%%)",
        elixir_ + GetTotalElixirFromStorages(),
        GetTotalElixirCapacity());
    cocos2d::log("兵营上限: %d", barrack_capacity_);
    cocos2d::log("金币已满: %s", IsGoldFull() ? "是" : "否");
    cocos2d::log("圣水已满: %s", IsElixirFull() ? "是" : "否");
}