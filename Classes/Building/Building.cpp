//
// Created by Faith_Oriest on 2025/12/7.
//

#include <iostream>
#include "Building/Building.h"

// ==================== Building 基类函数的实现 ====================

/**
 * @brief Building 构造函数
 * 初始化建筑名称、等级、生命、防御、建造时间与建造成本，并设置在场景中的坐标。
 */
Building::Building(std::string name, int level, int health, int defense,
    int buildtime, int build_cost, std::pair<int, int> position)
    : name_(std::move(name)), level_(level), health_(health), defense_(defense),
    build_time_(buildtime), build_cost_(build_cost),
    position_(cocos2d::Vec2(static_cast<float>(position.first), static_cast<float>(position.second))) {
    this->setPosition(position_);
}

/**
 * @brief 建筑升级
 * 提升建筑等级，同时调整建造时间、建造成本、防御并将生命值回满。
 */
void Building::Upgrade() {
    level_ = GetNextLevel();
    build_time_ = GetNextBuildTime(); // 升级时间增加
    build_cost_ = GetNextBuildCost(); // 升级成本增加
    health_ = GetNextHealth(); // 血量回满
    defense_ = GetNextDefense(); // 防御提升
}

/**
 * @brief 建筑受到伤害
 * 根据传入伤害计算实际伤害值并扣减生命，至少造成 1 点伤害。
 */
void Building::TakeDamage(int damage) {
    int actualDamage = damage - defense_;
    if (actualDamage < 1) actualDamage = 1; // 至少造成1点伤害
    health_ -= actualDamage;
    if (health_ < 0) health_ = 0;
}

/**
 * @brief 修复建筑
 * 将当前生命值恢复为最大生命值。
 */
void Building::Repair() {
    health_ = GetMaxHealth();
}

/**
 * @brief 检查建筑是否被摧毁
 * @return true 表示生命值小于等于 0，建筑已被摧毁；否则为 false。
 */
bool Building::IsDamaged() const {
    return health_ <= 0;
}

/**
 * @brief 输出建筑信息
 * 使用 cocos2d::log 打印建筑的名称、等级、生命、防御、建造时间、成本和坐标。
 */
void Building::ShowInfo() const {
    cocos2d::log("=== 建筑信息 ===");
    cocos2d::log("名称: %s", name_.c_str());
    cocos2d::log("等级: %d", level_);
    cocos2d::log("血量: %d/%d", health_, GetMaxHealth());
    cocos2d::log("防御: %d", defense_);
    cocos2d::log("建造时间: %d秒", build_time_);
    cocos2d::log("建造成本: %d金币", build_cost_);
    cocos2d::log("位置: (%.1f, %.1f)", position_.x, position_.y);
}

//各类Get函数

/**
 * @brief 获取最大生命值
 * 最大生命值按“防御值的八倍”进行计算。
 */
int Building::GetMaxHealth() const {
    return static_cast<int>(defense_ * 8);//最大血量设定为为八倍的防御值
}

/**
 * @brief 获取建筑名称
 * @return 建筑名称字符串引用。
 */
const std::string& Building::GetName() const {
    return name_;
}

/**
 * @brief 获取建筑等级
 * @return 当前等级数值。
 */
int Building::GetLevel() const {
    return level_;
}

/**
 * @brief 获取当前生命值
 * @return 当前生命值数值。
 */
int Building::GetHealth() const {
    return health_;
}

/**
 * @brief 获取防御值
 * @return 当前防御数值。
 */
int Building::GetDefense() const {
    return defense_;
}

/**
 * @brief 获取建造时间
 * @return 建造时间（秒）。
 */
int Building::GetBuildTime() const {
    return build_time_;
}

/**
 * @brief 获取建造成本
 * @return 建造所需金币数量。
 */
int Building::GetBuildCost() const {
    return build_cost_;
}

/**
 * @brief 获取建筑坐标
 * @return 以整数对形式表示的位置坐标。
 */
std::pair<int, int> Building::GetPosition() const {
    return std::make_pair(static_cast<int>(position_.x), static_cast<int>(position_.y));
}

// ==================== SourceBuilding 成员函数的实现 ====================

/**
 * @brief SourceBuilding 构造函数
 * 按给定基数 base 初始化资源建筑的生命、防御、建造时间和成本，并设置纹理。
 */
SourceBuilding::SourceBuilding(std::string name, int base, std::pair<int, int> position, std::string texture)
    : Building(name, 1, 16 * base, 2 * base,
        base, base * 500, position),
    production_rate_(base * 50) {
    // 设置资源建筑的纹理
    this->setTexture(texture);
}

/**
 * @brief 生产资源
 * @return 本次生产的资源数量（与 production_rate_ 相同）。
 */
int SourceBuilding::ProduceResource() {
    int resource = production_rate_;
    return resource;
}

/**
 * @brief 输出 SourceBuilding 详细信息
 * 调用基类 ShowInfo 再额外输出资源生产速率。
 */
void SourceBuilding::ShowInfo() const {
    Building::ShowInfo();
    cocos2d::log("资源生产速率: %d 点/秒", production_rate_);
}

// ==================== AttackBuilding 成员函数的实现 ====================

/**
 * @brief AttackBuilding 构造函数
 * 初始化攻击建筑的生命、防御、建造时间和成本，并设置攻击范围与纹理。
 */
AttackBuilding::AttackBuilding(std::string name, int base, std::pair<int, int> position, std::string texture, int range)
    : Building(name, 1, 6 * base, base,
        base * 2, base * 500, position),
    Range_(range) {
    this->setTexture(texture);
}

/**
 * @brief 输出 AttackBuilding 详细信息
 * 调用基类 ShowInfo 再额外输出攻击范围信息。
 */
void AttackBuilding::ShowInfo() const {
    Building::ShowInfo();
    cocos2d::log("攻击范围: %d 格", Range_);
}

// ==================== TrainingBuilding 成员函数的实现 ====================

/**
 * @brief TrainingBuilding 构造函数
 * 初始化训练营的生命、防御、建造时间和成本，并设置训练属性与纹理。
 */
TrainingBuilding::TrainingBuilding(std::string name, int base, std::pair<int, int> position,
    std::string texture, int capacity, int speed)
    : Building(name, 1, 8 * base, 2 * base,
        base * 3, base * 400, position),
    training_capacity_(capacity),
    training_speed_(speed) {

    // 初始化可训练单位列表（根据等级解锁不同单位）
    available_units_ = { "Barbarian", "Archer" };

    // 设置训练营的纹理
    this->setTexture(texture);
}

/**
 * @brief 开始训练士兵
 */
bool TrainingBuilding::StartTraining(const std::string& unit_type, int count) {
    // 检查单位类型是否可训练
    if (std::find(available_units_.begin(), available_units_.end(), unit_type) == available_units_.end()) {
        cocos2d::log("错误: 无法训练单位类型 %s", unit_type.c_str());
        return false;
    }

    // 检查训练容量
    int current_training = 0;
    for (const auto& unit : available_units_) {
        // 这里需要访问训练队列，假设有一个私有成员 training_queue_
        // current_training += training_queue_[unit];
    }

    if (current_training + count > training_capacity_) {
        cocos2d::log("错误: 训练容量不足 (当前: %d, 需要: %d, 容量: %d)",
            current_training, count, training_capacity_);
        return false;
    }

    // 开始训练逻辑
    cocos2d::log("开始训练 %d 个 %s", count, unit_type.c_str());
    return true;
}

/**
 * @brief 取消训练
 */
void TrainingBuilding::CancelTraining(const std::string& unit_type, int count) {
    cocos2d::log("取消训练 %d 个 %s", count, unit_type.c_str());
    // 实际实现需要更新训练队列
}

/**
 * @brief 获取当前训练队列信息
 */
std::map<std::string, int> TrainingBuilding::GetTrainingQueue() const {
    std::map<std::string, int> queue;
    // 返回训练队列的拷贝
    // 实际实现需要返回 training_queue_ 的拷贝
    return queue;
}

/**
 * @brief 检查是否有训练完成
 */
std::pair<std::string, int> TrainingBuilding::CheckCompletedTraining() {
    // 检查训练计时器，返回完成的单位
    // 实际实现需要检查训练时间是否达到
    return std::make_pair("", 0);
}

/**
 * @brief 升级训练营
 * 重写基类升级方法，提升训练容量和速度
 */
void TrainingBuilding::Upgrade() {
    // 先调用基类的升级
    Building::Upgrade();

    // 训练营特有的升级逻辑
    training_capacity_ += 5;  // 每级增加5个训练容量
    training_speed_ = static_cast<int>(training_speed_ * 0.9);  // 训练速度提升10%

    // 根据等级解锁新单位
    if (level_ >= 3 && std::find(available_units_.begin(), available_units_.end(), "Giant") == available_units_.end()) {
        available_units_.push_back("Giant");
        cocos2d::log("解锁新单位: Giant");
    }
    if (level_ >= 5 && std::find(available_units_.begin(), available_units_.end(), "Wizard") == available_units_.end()) {
        available_units_.push_back("Wizard");
        cocos2d::log("解锁新单位: Wizard");
    }

    cocos2d::log("训练营升级到 %d 级", level_);
    cocos2d::log("训练容量: %d, 训练速度: %d 秒/每兵", training_capacity_, training_speed_);
}

/**
 * @brief 输出 TrainingBuilding 详细信息
 * 调用基类 ShowInfo 再额外输出训练相关属性。
 */
void TrainingBuilding::ShowInfo() const {
    Building::ShowInfo();
    cocos2d::log("训练容量: %d 个单位", training_capacity_);
    cocos2d::log("训练速度: %d 秒/每兵", training_speed_);
    cocos2d::log("可训练单位: ");
    for (const auto& unit : available_units_) {
        cocos2d::log("  - %s", unit.c_str());
    }
}

/**
 * @brief 获取训练容量
 */
int TrainingBuilding::GetTrainingCapacity() const {
    return training_capacity_;
}

/**
 * @brief 获取训练速度
 */
int TrainingBuilding::GetTrainingSpeed() const {
    return training_speed_;
}

/**
 * @brief 获取可训练单位列表
 */
const std::vector<std::string>& TrainingBuilding::GetAvailableUnits() const {
    return available_units_;
}