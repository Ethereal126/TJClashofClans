//
// TownHall.cpp
//

#include "TownHall.h"
#include <cmath>

USING_NS_CC;

// ==================== TownHall 实现 ====================

// ==================== 单例管理相关 ====================

// 在文件开头添加静态成员初始化
TownHall* TownHall::instance_ = nullptr;

TownHall* TownHall::GetInstance() {
    return instance_;
}

bool TownHall::InitializeInstance(const std::string& name, int base,
    cocos2d::Vec2 position, const std::string& texture) {
    if (instance_ != nullptr) {
        cocos2d::log("大本营单例已经初始化");
        return false;
    }

    // 创建大本营实例
    instance_ = new TownHall(name, base, position, texture);
    if (instance_) {
        instance_->is_initialized_ = true;
        cocos2d::log("大本营单例初始化成功");
        return true;
    }

    cocos2d::log("大本营单例初始化失败");
    return false;
}

void TownHall::DestroyInstance() {
    if (instance_ != nullptr) {
        delete instance_;
        instance_ = nullptr;
        cocos2d::log("大本营单例已销毁");
    }
}

bool TownHall::IsInstanceInitialized() {
    return instance_ != nullptr && instance_->is_initialized_;
}

TownHall* TownHall::GetCurrentInstance() const {
    return instance_;
}

void TownHall::ResetTownHall() {
    if (!IsInstanceInitialized()) {
        cocos2d::log("大本营未初始化，无法重置");
        return;
    }

    // 重置资源
    gold_ = 0;
    elixir_ = 0;

    // 清空管理列表
    gold_storages_.clear();
    elixir_storages_.clear();

    // 重置容量
    max_gold_capacity_ = 0;
    max_elixir_capacity_ = 0;

    // 重置初始化标记
    is_initialized_ = false;

    cocos2d::log("大本营状态已重置");
}


TownHall::TownHall(std::string name, int base, cocos2d::Vec2 position, std::string texture)
    : Building(name, 1, base * 100, base * 5, base * 60, base * 200, 4, 4,
        std::make_pair(static_cast<int>(position.x), static_cast<int>(position.y)))
    , is_initialized_(false)
    , gold_storage_capacity_(base * 2)
    , elixir_storage_capacity_(base * 2)
    , gold_mine_capacity_(base * 3)
    , elixir_collector_capacity_(base * 3)
    , barrack_capacity_(base)
    , army_capacity_(base * 10)
    , gold_(0)
    , elixir_(0)
    , current_army_count_(0)
    , max_gold_capacity_(0)
    , max_elixir_capacity_(0)
    , flag_sprite_(nullptr)
    , level_label_(nullptr) {

    // 设置大本营纹理
    this->setTexture(texture);
    this->setPosition(position);

    // 初始化UI组件
    UpdateLevelLabel();

    // 构造函数执行完成后标记为已初始化
    is_initialized_ = true;
}

TownHall::~TownHall() {
    // 销毁的是单例实例，重置静态指针
    if (this == instance_) {
        instance_ = nullptr;
    }

    // 清理UI组件
    flag_sprite_ = nullptr;
    level_label_ = nullptr;

    // 清空管理列表
    gold_storages_.clear();
    elixir_storages_.clear();
}

void TownHall::Upgrade() {
    // 调用基类升级
    Building::Upgrade();

    // 提升大本营特有属性
    level_ += 1;
    gold_storage_capacity_ += 1;           // 每级增加1个资源池容量
    elixir_storage_capacity_ += 1;
    gold_mine_capacity_ += 1;              // 每级增加1个金矿容量
    elixir_collector_capacity_ += 1;       // 每级增加1个圣水收集器容量
    barrack_capacity_ += 1;                // 每级增加1个兵营容量
    army_capacity_ += 5;                   // 每级增加5个军队容量

    // 更新资源持有上限
    UpdateAllResourceCapacities();

    // 更新等级显示
    UpdateLevelLabel();

    // 播放升级特效
    PlayUpgradeEffect();

    // 更新纹理（假设纹理命名规则为 "townhall_levelX.png"）
    std::string new_texture = "TownHall/townhall_level" + std::to_string(level_) + ".png";
    this->setTexture(new_texture);

    cocos2d::log("%s 升级到等级 %d，金币池上限: %d，圣水池上限: %d，金矿上限: %d，圣水收集器上限: %d，训练营上限: %d，军队容量: %d",
        name_.c_str(), level_, gold_storage_capacity_, elixir_storage_capacity_,
        gold_mine_capacity_, elixir_collector_capacity_, barrack_capacity_, army_capacity_);
}

// ==================== 金矿管理 ====================
void TownHall::AddGoldMine(SourceBuilding* gold_mine) {
    if (!gold_mine || gold_mines_.size() >= gold_mine_capacity_) {
        cocos2d::log("已达到金矿上限，无法添加更多金矿");
        return;
    }

    auto it = std::find(gold_mines_.begin(), gold_mines_.end(), gold_mine);
    if (it == gold_mines_.end()) {
        gold_mines_.push_back(gold_mine);
        cocos2d::log("添加金矿，当前数量: %zu/%d", gold_mines_.size(), gold_mine_capacity_);
    }
}

void TownHall::RemoveGoldMine(SourceBuilding* gold_mine) {
    auto it = std::find(gold_mines_.begin(), gold_mines_.end(), gold_mine);
    if (it != gold_mines_.end()) {
        gold_mines_.erase(it);
        cocos2d::log("移除金矿，剩余数量: %zu", gold_mines_.size());
    }
}

int TownHall::GetTotalGoldMineCount() const {
    return static_cast<int>(gold_mines_.size());
}

int TownHall::GetTotalGoldProduction() const {
    int total = 0;
    for (const auto* mine : gold_mines_) {
        if (mine && mine->IsActive()) {
            total += mine->GetProductionRate();
        }
    }
    return total;
}

// ==================== 圣水收集器管理 ====================
void TownHall::AddElixirCollector(SourceBuilding* elixir_collector) {
    if (!elixir_collector || elixir_collectors_.size() >= elixir_collector_capacity_) {
        cocos2d::log("已达到圣水收集器上限，无法添加更多圣水收集器");
        return;
    }

    auto it = std::find(elixir_collectors_.begin(), elixir_collectors_.end(), elixir_collector);
    if (it == elixir_collectors_.end()) {
        elixir_collectors_.push_back(elixir_collector);
        cocos2d::log("添加圣水收集器，当前数量: %zu/%d", elixir_collectors_.size(), elixir_collector_capacity_);
    }
}

void TownHall::RemoveElixirCollector(SourceBuilding* elixir_collector) {
    auto it = std::find(elixir_collectors_.begin(), elixir_collectors_.end(), elixir_collector);
    if (it != elixir_collectors_.end()) {
        elixir_collectors_.erase(it);
        cocos2d::log("移除圣水收集器，剩余数量: %zu", elixir_collectors_.size());
    }
}

int TownHall::GetTotalElixirCollectorCount() const {
    return static_cast<int>(elixir_collectors_.size());
}

int TownHall::GetTotalElixirProduction() const {
    int total = 0;
    for (const auto* collector : elixir_collectors_) {
        if (collector && collector->IsActive()) {
            total += collector->GetProductionRate();
        }
    }
    return total;
}

// ==================== 训练营管理 ====================

void TownHall::AddTrainingBuilding(TrainingBuilding* training_building) {
    if (!training_building || barracks_.size() >= barrack_capacity_) {
        cocos2d::log("已达到训练营上限，无法添加更多训练营");
        return;
    }

    auto it = std::find(barracks_.begin(), barracks_.end(), training_building);
    if (it == barracks_.end()) {
        barracks_.push_back(training_building);
        cocos2d::log("添加训练营，当前数量: %zu/%d", barracks_.size(), barrack_capacity_);
    }
}

void TownHall::RemoveTrainingBuilding(TrainingBuilding* training_building) {
    auto it = std::find(barracks_.begin(), barracks_.end(), training_building);
    if (it != barracks_.end()) {
        barracks_.erase(it);
        cocos2d::log("移除训练营，剩余数量: %zu", barracks_.size());
    }
}

int TownHall::GetTotalTrainingBuildingCount() const {
    return static_cast<int>(barracks_.size());
}

// ==================== 军营管理（特指Barracks） ====================

void TownHall::AddBarracks(Barracks* barracks) {
    if (!barracks) return;

    auto it = std::find(all_barracks_.begin(), all_barracks_.end(), barracks);
    if (it == all_barracks_.end()) {
        all_barracks_.push_back(barracks);

        // 更新军队容量（每个军营提供基础容量）
        UpdateArmyCapacityFromBarracks();

        cocos2d::log("添加军营，当前数量: %zu", all_barracks_.size());
    }
}

void TownHall::RemoveBarracks(Barracks* barracks) {
    auto it = std::find(all_barracks_.begin(), all_barracks_.end(), barracks);
    if (it != all_barracks_.end()) {
        all_barracks_.erase(it);

        // 更新军队容量
        UpdateArmyCapacityFromBarracks();

        cocos2d::log("移除军营，剩余数量: %zu", all_barracks_.size());
    }
}

int TownHall::GetTotalBarracksCount() const {
    return static_cast<int>(all_barracks_.size());
}

int TownHall::GetTotalArmyCapacityFromBarracks() const {
    int total_capacity = 0;
    for (const auto* barracks : all_barracks_) {
        if (barracks && barracks->IsActive()) {
            // 假设Barracks有一个GetMaxTroopCapacity()函数
            total_capacity += barracks->GetTrainingCapacity();
        }
    }
    return total_capacity;
}

void TownHall::UpdateArmyCapacityFromBarracks() {
    int total_barracks_capacity = 0;

    // 遍历所有军营，计算总容量
    for (const auto* barracks : all_barracks_) {
        if (barracks && barracks->IsActive()) {
            // 获取军营的训练容量
            // 注意：这里假设Barracks有一个GetTrainingCapacity()方法
            // 如果Barracks类中没有这个方法，需要在Barracks类中添加
            total_barracks_capacity += barracks->GetTrainingCapacity();
        }
    }

    // 计算军队总容量 = 大本营基础容量 + 所有军营容量
    // 大本营基础容量：每级5个单位
    int base_capacity = level_ * 5;
    army_capacity_ = base_capacity + total_barracks_capacity;

    // 如果当前军队人数超过新容量，调整人数
    if (current_army_count_ > army_capacity_) {
        current_army_count_ = army_capacity_;
    }

    cocos2d::log("军队容量更新: 基础%d + 军营%d = %d",
        base_capacity, total_barracks_capacity, army_capacity_);
}

// ==================== 军队人数更新 ====================

void TownHall::UpdateArmyCount(int delta) {
    int new_count = current_army_count_ + delta;

    // 限制在0到容量之间
    if (new_count < 0) new_count = 0;
    if (new_count > army_capacity_) new_count = army_capacity_;

    int old_count = current_army_count_;
    current_army_count_ = new_count;

    cocos2d::log("军队人数更新: %d -> %d/%d", old_count, current_army_count_, army_capacity_);

    // 如果军队人数变化，可以触发一些事件
    if (current_army_count_ >= army_capacity_) {
        cocos2d::log("军队已满，无法训练更多士兵");
        // 可以在这里触发UI提示
    }
}

// ==================== 军队容量相关 ====================

int TownHall::GetArmyCapacity() const {
    return army_capacity_;
}

int TownHall::GetCurrentArmyCount() const {
    return current_army_count_;
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
    cocos2d::log("金矿数量: %zu/%d", gold_mines_.size(), gold_mine_capacity_);
    cocos2d::log("圣水收集器数量: %zu/%d", elixir_collectors_.size(), elixir_collector_capacity_);
    cocos2d::log("训练营数量: %zu/%d", barracks_.size(), barrack_capacity_);
    cocos2d::log("军营数量: %zu", all_barracks_.size());
    cocos2d::log("大本营金币: %d/%d", gold_, max_gold_capacity_);
    cocos2d::log("大本营圣水: %d/%d", elixir_, max_elixir_capacity_);
    cocos2d::log("总金币: %d/%d (%.1f%%)",
        gold_ + GetTotalGoldFromStorages(),
        GetTotalGoldCapacity(),
        GetTotalGoldCapacity() > 0 ?
        (gold_ + GetTotalGoldFromStorages()) * 100.0f / GetTotalGoldCapacity() : 0);
    cocos2d::log("总圣水: %d/%d (%.1f%%)",
        elixir_ + GetTotalElixirFromStorages(),
        GetTotalElixirCapacity(),
        GetTotalElixirCapacity() > 0 ?
        (elixir_ + GetTotalElixirFromStorages()) * 100.0f / GetTotalElixirCapacity() : 0);
    cocos2d::log("军队容量: %d/%d (%.1f%%)",
        current_army_count_,
        army_capacity_,
        army_capacity_ > 0 ? current_army_count_ * 100.0f / army_capacity_ : 0);
    cocos2d::log("金币已满: %s", IsGoldFull() ? "是" : "否");
    cocos2d::log("圣水已满: %s", IsElixirFull() ? "是" : "否");

    // 显示生产信息
    if (GetTotalGoldMineCount() > 0) {
        cocos2d::log("金矿总产量: %d/小时", GetTotalGoldProduction());
    }
    if (GetTotalElixirCollectorCount() > 0) {
        cocos2d::log("圣水收集器总产量: %d/小时", GetTotalElixirProduction());
    }
}

std::vector<TownHall::BuildingTemplate> TownHall::GetAllBuildingTemplates() {
    std::vector<BuildingTemplate> templates;

    // 金矿
    templates.emplace_back(
        "Gold Mine",
        "ui/icons/gold_mine_icon.png",
        150,  // 成本
        3,    // 宽度
        3,    // 长度
        []() -> Building* {
            return SourceBuilding::Create("Gold Mine", 1, { 0, 0 }, "textures/gold_mine.png", "Gold");
        }
    );

    // 圣水收集器
    templates.emplace_back(
        "Elixir Collector",
        "ui/icons/elixir_collector_icon.png",
        150,
        3,
        3,
        []() -> Building* {
            return SourceBuilding::Create("Elixir Collector", 1, { 0, 0 }, "textures/elixir_collector.png", "Elixir");
        }
    );

    // 金币储罐
    templates.emplace_back(
        "Gold Storage",
        "ui/icons/gold_storage_icon.png",
        300,
        3,
        3,
        []() -> Building* {
            return GoldStorage::Create("Gold Storage", 1, { 0, 0 });
        }
    );

    // 圣水储罐
    templates.emplace_back(
        "Elixir Storage",
        "ui/icons/elixir_storage_icon.png",
        300,
        3,
        3,
        []() -> Building* {
            return ElixirStorage::Create("Elixir Storage", 1, { 0, 0 });
        }
    );

    // 军营
    templates.emplace_back(
        "Barracks",
        "ui/icons/barracks_icon.png",
        200,
        3,
        3,
        []() -> Building* {
            return Barracks::Create("Barracks", 1, { 0, 0 });
        }
    );

    // 训练营（更高级的军营）
    templates.emplace_back(
        "Training Camp",
        "ui/icons/training_camp_icon.png",
        500,
        4,
        4,
        []() -> Building* {
            // 调用 TrainingBuilding 的 Create 函数
            return TrainingBuilding::Create("Training Camp", 2, { 0, 0 },
                "textures/training_camp.png", 10, 20);
        }
    );

    // 防御建筑
    templates.emplace_back(
        "Cannon",
        "ui/icons/cannon_icon.png",
        250,
        2,
        2,
        []() -> Building* {
            return AttackBuilding::Create("Cannon", 1, { 0, 0 }, "textures/cannon.png", 5);
        }
    );

    templates.emplace_back(
        "Archer Tower",
        "ui/icons/archer_tower_icon.png",
        350,
        2,
        2,
        []() -> Building* {
            return AttackBuilding::Create("Archer Tower", 1, { 0, 0 }, "textures/archer_tower.png", 7);
        }
    );

    return templates;
}

std::vector<TownHall::SoldierTemplate> TownHall::GetSoldierCategory() {
    std::vector<SoldierTemplate> soldiers;

    // 假设有一个Soldier基类和对应的创建函数
    // 这里需要你实现具体的Soldier类和创建函数

    soldiers.emplace_back(
        "Barbarian",
        "ui/icons/barbarian_icon.png",
        1,    // 人口消耗
        20,   // 训练时间（秒）
        25,   // 训练费用（金币）
        []() -> Soldier* {
            // 实现野蛮人创建函数
            // return new Barbarian();
            return nullptr; // 暂时返回空
        }
    );

    soldiers.emplace_back(
        "Archer",
        "ui/icons/archer_icon.png",
        1,
        25,
        50,
        []() -> Soldier* {
            // 实现弓箭手创建函数
            // return new Archer();
            return nullptr; // 暂时返回空
        }
    );

    soldiers.emplace_back(
        "Giant",
        "ui/icons/giant_icon.png",
        5,
        120,
        500,
        []() -> Soldier* {
            // 实现巨人创建函数
            // return new Giant();
            return nullptr; // 暂时返回空
        }
    );

    soldiers.emplace_back(
        "Goblin",
        "ui/icons/goblin_icon.png",
        1,
        30,
        25,
        []() -> Soldier* {
            // 实现哥布林创建函数
            // return new Goblin();
            return nullptr; // 暂时返回空
        }
    );

    soldiers.emplace_back(
        "Wall Breaker",
        "ui/icons/wall_breaker_icon.png",
        2,
        60,
        1000,
        []() -> Soldier* {
            // 实现炸弹人创建函数
            // return new WallBreaker();
            return nullptr; // 暂时返回空
        }
    );

    soldiers.emplace_back(
        "Balloon",
        "ui/icons/balloon_icon.png",
        5,
        180,
        2000,
        []() -> Soldier* {
            // 实现气球兵创建函数
            // return new Balloon();
            return nullptr; // 暂时返回空
        }
    );

    soldiers.emplace_back(
        "Wizard",
        "ui/icons/wizard_icon.png",
        4,
        150,
        1500,
        []() -> Soldier* {
            // 实现法师创建函数
            // return new Wizard();
            return nullptr; // 暂时返回空
        }
    );

    soldiers.emplace_back(
        "Healer",
        "ui/icons/healer_icon.png",
        14,
        900,
        6000,
        []() -> Soldier* {
            // 实现治疗师创建函数
            // return new Healer();
            return nullptr; // 暂时返回空
        }
    );

    soldiers.emplace_back(
        "Dragon",
        "ui/icons/dragon_icon.png",
        20,
        1800,
        25000,
        []() -> Soldier* {
            // 实现飞龙创建函数
            // return new Dragon();
            return nullptr; // 暂时返回空
        }
    );

    soldiers.emplace_back(
        "P.E.K.K.A",
        "ui/icons/pekka_icon.png",
        25,
        2400,
        30000,
        []() -> Soldier* {
            // 实现皮卡超人创建函数
            // return new PEKKA();
            return nullptr; // 暂时返回空
        }
    );

    return soldiers;
}