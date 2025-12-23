//
// Created by Faith_Oriest on 2025/12/7.
//

#include <iostream>
#include "Building/Building.h"
#include "TownHall/TownHall.h"

// ==================== Building 基类函数的实现 ====================

/**
 * @brief Building 构造函数
 * 初始化建筑名称、等级、生命、防御、建造时间与建造成本，并设置在场景中的坐标。
 */
Building::Building(std::string name, int level, int health, int defense,
    int buildtime, int build_cost, int width, int length, cocos2d::Vec2 position)
    : name_(std::move(name)), level_(level), health_(health), defense_(defense),
    build_time_(buildtime), build_cost_(build_cost), width_(width), length_(length),
    is_upgrading_(false), upgrade_remaining_time_(0.0f),
    position_(position) {
    this->setPosition(position_);
}

/**
 * @brief 建筑升级
 * 提升建筑等级，同时调整建造时间、建造成本、防御并将生命值回满。
 */
void Building::Upgrade() {
    if (is_upgrading_) {
        cocos2d::log("建筑 %s 正在升级中，请等待升级完成", name_.c_str());
        return;
    }
    level_ = GetNextLevel();
    build_time_ = GetNextBuildTime(); // 升级时间增加
    build_cost_ = GetNextBuildCost(); // 升级成本增加
    health_ = GetNextHealth(); // 血量回满
    defense_ = GetNextDefense(); // 防御提升
}

/**
 * @brief 开始升级
 * 启动升级过程，在指定时间内无法再次升级
 */
void Building::StartUpgrade(float upgrade_time) {
    if (is_upgrading_) {
        cocos2d::log("建筑 %s 正在升级中，无法再次升级", name_.c_str());
        return;
    }

    is_upgrading_ = true;
    upgrade_remaining_time_ = upgrade_time;

    cocos2d::log("建筑 %s 开始升级，需要 %.1f 秒", name_.c_str(), upgrade_time);

    // 启动升级计时器
    // 注意：这里假设 Building 继承自 cocos2d::Sprite，可以使用 schedule
    this->schedule([this](float dt) {
        upgrade_remaining_time_ -= dt;
        if (upgrade_remaining_time_ <= 0.0f) {
            upgrade_remaining_time_ = 0.0f;
            is_upgrading_ = false;

            // 升级完成，执行真正的升级逻辑
            this->Upgrade();

            // 停止计时器
            this->unschedule("upgrade_timer");

            cocos2d::log("建筑 %s 升级完成！当前等级：%d",
                this->GetName().c_str(), this->GetLevel());
        }
        }, "upgrade_timer");
}

/**
 * @brief 检查是否允许升级
 * @return true 表示允许升级，false 表示正在升级中
 */
bool Building::IsAllowedUpgrade() const {
    return !is_upgrading_;
}

/**
 * @brief 获取建筑图片路径
 * @return 建筑图片路径字符串
 */
std::string Building::GetBuildingImagePath() const {
    // 可以根据等级或其他条件返回不同的图片路径
    // 例如：return "buildings/" + name_ + std::to_string(level_) + ".png";
    return "buildings/" + name_ + ".png";
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
 * @return 以cocos2d::Vec2形式表示的位置坐标。
 */
cocos2d::Vec2 Building::GetPosition() const {
    return position_;
}

// ==================== SourceBuilding 成员函数的实现 ====================

/**
 * @brief SourceBuilding 构造函数
 * 按给定基数 base 初始化资源建筑的生命、防御、建造时间和成本，并设置纹理。
 */
SourceBuilding::SourceBuilding(std::string name, int base, cocos2d::Vec2 position, std::string texture)
    : Building(name, 1, 16 * base, 2 * base,
        base, base * 500, 3, 3, position),
    production_rate_(base * 50), creationTime_(0.0) {
    // 初始化创建时间为当前系统时间
    creationTime_ = cocos2d::utils::gettime();
    // 设置资源建筑的纹理
    this->setTexture(texture);
}

/**
 * @brief 静态创建函数
 * 用于创建资源建筑实例的静态工厂方法。
 */
SourceBuilding* SourceBuilding::Create(const std::string& name, int base,
    cocos2d::Vec2 position,
    const std::string& texture,
    const std::string& resourceType) {
    // 使用nothrow避免分配失败时抛出异常
    auto building = new (std::nothrow) SourceBuilding(name, base, position, texture);

    if (building) {
        // 设置资源类型（如果构造函数没有设置的话）
        // 注意：这里假设SourceBuilding类有一个SetResourceType方法
        // 如果没有，可以在构造函数中直接设置
        if (building->initWithFile(texture)) {
            // 标记为自动释放（Cocos2d-x的内存管理机制）
            building->autorelease();

            // 设置一些默认属性
            if (resourceType == "Gold") {
                building->setColor(cocos2d::Color3B::YELLOW);
            }
            else if (resourceType == "Elixir") {
                building->setColor(cocos2d::Color3B(200, 100, 255)); // 紫色
            }

            cocos2d::log("创建资源建筑: %s (类型: %s, 位置: %f,%f)",
                name.c_str(), resourceType.c_str(),
                position.x, position.y);
            return building;
        }

        // 初始化失败，删除对象
        delete building;
    }

    cocos2d::log("创建资源建筑失败: %s", name.c_str());
    return nullptr;
}

double SourceBuilding::CalculateTimeProductionProduct() {
    // 获取当前系统时间
    double currentTime = cocos2d::utils::gettime();

    // 计算时间差（秒）
    double timeDifference = currentTime - creationTime_;

    // 计算时间差与生产速率的乘积
    double result = timeDifference * production_rate_;

    // 更新记录的时间为当前时间
    creationTime_ = currentTime;

    // 记录日志
    cocos2d::log("计算时间与生产速率乘积: 时间差=%.2f秒, 生产速率=%.2f, 结果=%.2f",
        timeDifference, production_rate_, result);

    return result;
}

/**
 * @brief 输出 SourceBuilding 详细信息
 * 调用基类 ShowInfo 再额外输出资源生产速率。
 */
void SourceBuilding::ShowInfo() const {
    Building::ShowInfo();
    cocos2d::log("资源生产速率: %d 点/秒", production_rate_);
}

// ==================== WallBuilding 成员函数的实现 ====================

/**
 * @brief WallBuilding 构造函数
 * 初始化墙体的生命、防御、建造时间和成本
 */
WallBuilding::WallBuilding(std::string name, int base, cocos2d::Vec2 position,
    std::string texture)
    : Building(name, 1, 15 * base, 2 * base,  // 墙体生命值较高，防御值较高
        base * 2, base * 150, 1, 1, position) {  // 墙体占用1x1格子

    this->setTexture(texture);
    this->setColor(cocos2d::Color3B(180, 180, 180)); // 灰色墙体
}

/**
 * @brief 创建墙体实例
 */
WallBuilding* WallBuilding::Create(const std::string& name, int base,
    cocos2d::Vec2 position,
    const std::string& texture) {
    auto wall = new (std::nothrow) WallBuilding(name, base, position, texture);

    if (wall) {
        if (wall->initWithFile(texture)) {
            wall->autorelease();

            // 设置墙体锚点为底部中心，方便放置
            wall->setAnchorPoint(cocos2d::Vec2(0.5f, 0.0f));

            cocos2d::log("创建城墙: %s (位置: %.1f,%.1f)",
                name.c_str(), position.x, position.y);
            return wall;
        }
        delete wall;
    }

    cocos2d::log("创建城墙失败: %s", name.c_str());
    return nullptr;
}

/**
 * @brief 墙体升级
 * 墙体升级时更新纹理和属性
 */
void WallBuilding::WallBuilding::Upgrade() {
    if (is_upgrading_) {
        cocos2d::log("城墙 %s 正在升级中，请等待升级完成", name_.c_str());
        return;
    }

    // 调用基类升级
    Building::Upgrade();

    // 更新墙体纹理（假设命名规则为 "wallX.png"）
    std::string new_texture = "buildings/wall" + std::to_string(level_) + ".png";
    this->setTexture(new_texture);

    // 根据等级改变颜色（可选视觉效果）
    if (level_ >= 8) {
        this->setColor(cocos2d::Color3B(255, 215, 0)); // 金色
    }
    else if (level_ >= 5) {
        this->setColor(cocos2d::Color3B(192, 192, 192)); // 银色
    }

    cocos2d::log("城墙升级到等级 %d，血量: %d/%d，防御: %d",
        level_, health_, GetMaxHealth(), defense_);
}

/**
 * @brief 显示墙体信息
 */
void WallBuilding::ShowInfo() const {
    Building::ShowInfo();
    cocos2d::log("墙体升级进度: %d", level_);
}

// ==================== AttackBuilding 成员函数的实现 ====================

/**
 * @brief AttackBuilding 构造函数
 * 初始化攻击建筑的生命、防御、建造时间和成本，并设置攻击范围与纹理。
 */
AttackBuilding::AttackBuilding(std::string name, int base, cocos2d::Vec2 position, std::string texture,
                               float attack_interval,int attack_damage,float attack_range)
    : Building(name, 1, 6 * base, base,
        base * 2, base * 500, 2, 2, position),
    attack_interval_(attack_interval),attack_damage_(attack_damage),attack_range_(attack_range) {
    this->setTexture(texture);
}

/**
 * @brief 静态创建函数
 * 用于创建攻击建筑实例的静态工厂方法。
 */
AttackBuilding* AttackBuilding::Create(const std::string& name, int base,
    cocos2d::Vec2 position,
    const std::string& texture, float attack_interval,int attack_damage,float attack_range) {
    // 使用nothrow避免分配失败时抛出异常
    auto building = new (std::nothrow) AttackBuilding(name, base, position, texture, attack_interval,attack_damage,attack_range);

    if (building) {
        if (building->initWithFile(texture)) {
            // 标记为自动释放
            building->autorelease();

            // 设置防御建筑特有属性
            building->setScale(0.9f); // 稍微缩小一点，看起来更像防御建筑

            // 根据建筑类型设置不同颜色
            if (name.find("Cannon") != std::string::npos) {
                building->setColor(cocos2d::Color3B(180, 180, 180)); // 灰色
            }
            else if (name.find("Archer Tower") != std::string::npos) {
                building->setColor(cocos2d::Color3B(200, 150, 100)); // 棕色
            }
            else if (name.find("Mortar") != std::string::npos) {
                building->setColor(cocos2d::Color3B(150, 150, 150)); // 深灰色
            }
            else if (name.find("Wizard Tower") != std::string::npos) {
                building->setColor(cocos2d::Color3B(100, 100, 200)); // 蓝色
            }
            else if (name.find("Air Defense") != std::string::npos) {
                building->setColor(cocos2d::Color3B(100, 200, 200)); // 青色
            }
            else if (name.find("Tesla") != std::string::npos) {
                building->setColor(cocos2d::Color3B(200, 200, 100)); // 淡黄色
            }
            else if (name.find("X-Bow") != std::string::npos) {
                building->setColor(cocos2d::Color3B(150, 100, 200)); // 紫色
            }
            else if (name.find("Inferno Tower") != std::string::npos) {
                building->setColor(cocos2d::Color3B(200, 100, 100)); // 红色
            }
            else if (name.find("Eagle Artillery") != std::string::npos) {
                building->setColor(cocos2d::Color3B(150, 150, 200)); // 淡蓝色
            }

            cocos2d::log("创建攻击建筑: %s (范围: %d, 位置: %.1f,%.1f)",
                name.c_str(), attack_range, position.x, position.y);
            return building;
        }

        // 初始化失败，删除对象
        delete building;
    }

    cocos2d::log("创建攻击建筑失败: %s", name.c_str());
    return nullptr;
}

/**
 * @brief 输出 AttackBuilding 详细信息
 * 调用基类 ShowInfo 再额外输出攻击范围信息。
 */
void AttackBuilding::ShowInfo() const {
    Building::ShowInfo();
    cocos2d::log("攻击范围: %f 格", attack_range_);
}

// ==================== TrainingBuilding 成员函数的实现 ====================

/**
 * @brief TrainingBuilding 构造函数
 * 初始化训练营的生命、防御、建造时间和成本，并设置训练属性与纹理。
 */
TrainingBuilding::TrainingBuilding(std::string name, int base, cocos2d::Vec2 position,
    std::string texture, int capacity, int speed)
    : Building(name, 1, 8 * base, 2 * base,
        base * 3, base * 400, 3, 3, position),
    training_capacity_(capacity),
    training_speed_(speed),
    training_timer_(0.0f) {

    // 初始化默认可训练士兵类型
    available_soldier_types_ = { SoldierType::kBarbarian, SoldierType::kArcher };

    // 设置训练营的纹理
    this->setTexture(texture);
}

/**
 * @brief 初始化
 */
bool TrainingBuilding::Init() {
    // 基类初始化
    if (!Building::initWithFile("buildings/" + name_ + ".png")) {
        return false;
    }

    // 设置建筑尺寸和位置
    setContentSize(cocos2d::Size(width_ * 32, length_ * 32));
    setAnchorPoint(cocos2d::Vec2(0.5f, 0.5f));

    return true;
}

/**
 * @brief 创建训练营实例
 */
TrainingBuilding* TrainingBuilding::Create(const std::string& name, int base,
    cocos2d::Vec2 position,
    const std::string& texture, int capacity, int speed) {
    auto building = new (std::nothrow) TrainingBuilding(name, base, position, texture, capacity, speed);

    if (building) {
        if (building->Init()) {
            // 标记为自动释放
            building->autorelease();

            cocos2d::log("创建训练营: %s (容量: %d人口, 速度: %d, 位置: %.1f,%.1f)",
                name.c_str(), capacity, speed, position.x, position.y);
            return building;
        }

        // 初始化失败，删除对象
        delete building;
    }

    cocos2d::log("创建训练营失败: %s", name.c_str());
    return nullptr;
}

/**
 * @brief 开始训练士兵
 */
bool TrainingBuilding::StartTraining(SoldierType soldier_type, int count) {
    if (!IsActive()) {
        cocos2d::log("错误: 训练营已被摧毁，无法训练");
        return false;
    }

    // 检查士兵类型是否可训练
    if (!CanTrainSoldierType(soldier_type)) {
        cocos2d::log("错误: 无法训练士兵类型 %d", static_cast<int>(soldier_type));
        return false;
    }

    // 获取士兵模板
    const SoldierTemplate* tmpl = TownHall::GetSoldierTemplate(soldier_type);
    if (!tmpl) {
        cocos2d::log("错误: 未找到士兵模板");
        return false;
    }

    // 计算所需人口
    int required_population = tmpl->housing_space_ * count;

    // 检查训练队列容量
    if (GetTrainingQueuePopulation() + required_population > training_capacity_) {
        cocos2d::log("错误: 训练容量不足 (当前: %d, 需要: %d, 容量: %d)",
            GetTrainingQueuePopulation(), required_population, training_capacity_);
        return false;
    }

    // 检查军队容量（通过TownHall）
    TownHall* town_hall = TownHall::GetInstance();
    if (!town_hall || !town_hall->CanAddSoldier(required_population)) {
        cocos2d::log("错误: 军队容量不足");
        return false;
    }

    // 计算训练所需资源
    auto [gold_cost, elixir_cost] = CalculateTrainingCost(soldier_type, count);

    // 检查资源是否足够
    if (!town_hall->SpendGold(gold_cost) || !town_hall->SpendElixir(elixir_cost)) {
        cocos2d::log("错误: 资源不足，需要金币: %d, 圣水: %d", gold_cost, elixir_cost);
        return false;
    }

    // 计算训练时间
    int training_time = CalculateTrainingTime(soldier_type, count);

    // 添加到训练队列
    training_queue_.emplace_back(soldier_type, count, training_time);

    cocos2d::log("开始训练 %d 个 %s，需要 %d 秒，消耗金币: %d，圣水: %d",
        count, tmpl->name_.c_str(), training_time, gold_cost, elixir_cost);

    return true;
}

/**
 * @brief 更新训练进度
 */
void TrainingBuilding::UpdateTrainingProgress(float delta_time) {
    if (!IsActive() || training_queue_.empty()) {
        return;
    }

    training_timer_ += delta_time;

    // 每秒更新一次训练队列
    if (training_timer_ >= 1.0f) {
        training_timer_ = 0.0f;

        // 减少每个训练项目的剩余时间
        for (auto& item : training_queue_) {
            if (item.remaining_time > 0) {
                item.remaining_time--;
            }
        }

        // 处理训练完成的士兵
        ProcessCompletedTraining();
    }
}

/**
 * @brief 处理训练完成的士兵
 */
int TrainingBuilding::ProcessCompletedTraining() {
    int completed_count = 0;

    // 遍历训练队列，检查是否有训练完成的
    for (auto it = training_queue_.begin(); it != training_queue_.end(); ) {
        if (it->remaining_time <= 0) {
            // 训练完成，创建士兵并添加到TownHall
            const SoldierTemplate* tmpl = TownHall::GetSoldierTemplate(it->soldier_type);
            if (tmpl) {
                for (int i = 0; i < it->count; ++i) {
                    // 创建士兵
                    Soldier* soldier = tmpl->Create();

                    // 添加到TownHall
                    TownHall* town_hall = TownHall::GetInstance();
                    if (town_hall) {
                        town_hall->AddTrainedSoldier(soldier);
                    }
                    else {
                        delete soldier; // TownHall不存在，删除士兵
                    }
                }

                completed_count += it->count;
                cocos2d::log("训练完成: %d 个 %s", it->count, tmpl->name_.c_str());
            }

            // 从队列中移除
            it = training_queue_.erase(it);
        }
        else {
            ++it;
        }
    }

    return completed_count;
}

/**
 * @brief 计算训练时间
 */
int TrainingBuilding::CalculateTrainingTime(SoldierType soldier_type, int count) const {
    const SoldierTemplate* tmpl = TownHall::GetSoldierTemplate(soldier_type);
    if (!tmpl) return 0;

    // 训练时间 = 单个士兵训练时间 × 数量 / 训练速度
    // 注意：这里假设训练速度是倍数，实际可能需要调整公式
    return (tmpl->training_time_ * count) / training_speed_;
}

/**
 * @brief 计算训练所需资源
 */
std::pair<int, int> TrainingBuilding::CalculateTrainingCost(SoldierType soldier_type, int count) const {
    const SoldierTemplate* tmpl = TownHall::GetSoldierTemplate(soldier_type);
    if (!tmpl) return { 0, 0 };

    // 这里简单处理：所有士兵都用金币训练
    // 实际上部落冲突中不同士兵消耗不同资源，你可以根据需要扩展
    return { tmpl->training_cost_ * count, 0 };
}

/**
 * @brief 检查训练队列是否已满
 */
bool TrainingBuilding::IsTrainingQueueFull() const {
    return GetTrainingQueuePopulation() >= training_capacity_;
}

/**
 * @brief 获取训练队列当前占用的人口
 */
int TrainingBuilding::GetTrainingQueuePopulation() const {
    int population = 0;
    for (const auto& item : training_queue_) {
        const SoldierTemplate* tmpl = TownHall::GetSoldierTemplate(item.soldier_type);
        if (tmpl) {
            population += tmpl->housing_space_ * item.count;
        }
    }
    return population;
}

/**
 * @brief 检查是否可训练指定类型的士兵
 */
bool TrainingBuilding::CanTrainSoldierType(SoldierType soldier_type) const {
    for (SoldierType type : available_soldier_types_) {
        if (type == soldier_type) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 升级训练营
 */
void TrainingBuilding::Upgrade() {
    if (!IsActive()) {
        cocos2d::log("训练营已被摧毁，无法升级");
        return;
    }

    // 先调用基类的升级
    Building::Upgrade();

    // 训练营特有的升级逻辑
    training_capacity_ += 5;  // 每级增加5个人口容量
    training_speed_ = static_cast<int>(training_speed_ * 0.9);  // 训练速度提升10%

    // 根据等级解锁新兵种
    if (level_ >= 3 && !CanTrainSoldierType(SoldierType::kGiant)) {
        available_soldier_types_.push_back(SoldierType::kGiant);
        cocos2d::log("解锁新兵种: 巨人(Giant)");
    }
    if (level_ >= 5 && !CanTrainSoldierType(SoldierType::kBomber)) {
        available_soldier_types_.push_back(SoldierType::kBomber);
        cocos2d::log("解锁新兵种: 炸弹人(Bomber)");
    }

    cocos2d::log("训练营升级到 %d 级", level_);
    cocos2d::log("训练容量: %d 人口, 训练速度: %d 秒/每人口", training_capacity_, training_speed_);
}

/**
 * @brief 显示训练营信息
 */
void TrainingBuilding::ShowInfo() const {
    Building::ShowInfo();
    cocos2d::log("训练容量: %d 人口", training_capacity_);
    cocos2d::log("训练速度: %d 秒/每人口", training_speed_);
    cocos2d::log("可训练兵种: ");
    for (const auto& type : available_soldier_types_) {
        const SoldierTemplate* tmpl = TownHall::GetSoldierTemplate(type);
        if (tmpl) {
            cocos2d::log("  - %s (人口: %d, 训练时间: %d秒, 费用: %d金币)",
                tmpl->name_.c_str(), tmpl->housing_space_, tmpl->training_time_, tmpl->training_cost_);
        }
    }
    cocos2d::log("训练队列: %d个项目，占用人口: %d/%d",
        static_cast<int>(training_queue_.size()), GetTrainingQueuePopulation(), training_capacity_);
}

// ... 其他辅助函数的实现 ...