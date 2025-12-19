//
// Created by duby0 on 2025/12/7.
//
#include "SoldierInCombat.h"
#include "BuildingInCombat.h"
#include "Combat.h"
#include <unordered_set>


bool SoldierInCombat::is_animation_loaded_ = false;

// -------------------------- 工厂方法实现 --------------------------
SoldierInCombat* SoldierInCombat::Create(Soldier* soldier_template, const cocos2d::Vec2& spawn_pos,MapManager* map) {
    auto soldier = new (std::nothrow) SoldierInCombat();
    if (soldier && soldier->Init(soldier_template, spawn_pos,map)) {
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
bool SoldierInCombat::Init(Soldier* soldier_template, const cocos2d::Vec2& spawn_pos,MapManager* map) {
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
    map_ = map;
    // 只有在map_不为nullptr时才调用addChild
    if (map_ != nullptr) {
        map_->addChild(this);
    }
    this->setScale(1.5f);  // 调整大小（根据实际资源修改）

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
    cocos2d::Vec2 current_map_pos = map_->worldToVec(this->getPosition());
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
    float anim_duration = move_anim ? move_anim->getDuration() : 0.1f; // 兜底值
    // 计算动画需要循环的次数（移动总时长 / 单段动画时长）
    int anim_repeat_count = static_cast<int>(move_time / anim_duration);
    anim_repeat_count = std::max(anim_repeat_count, 1); // 至少循环1次

    // 用Repeat替代RepeatForever，确保动画和移动同步结束
    auto animate = cocos2d::Repeat::create(cocos2d::Animate::create(move_anim), anim_repeat_count);

    // 4. 构建“延迟+检测”的循环动作（每0.1秒检测一次）
    auto check_target = cocos2d::CallFunc::create([this]() {
        this->CheckTargetAlive();
    });
    float check_interval = 0.1f;
    // 单轮检测：延迟0.1秒 → 执行检测
    auto single_check_loop = cocos2d::Sequence::create(
            cocos2d::DelayTime::create(check_interval),
            check_target,
            nullptr
    );
    // 循环次数：移动总时长 / 检测间隔（确保移动过程中持续检测）
    int check_repeat_count = static_cast<int>(move_time / check_interval);
    // 避免除数为0（移动时间极短时）
    check_repeat_count = std::max(check_repeat_count, 1);
    // 循环执行检测
    auto repeat_check = cocos2d::Repeat::create(single_check_loop, check_repeat_count);
    return cocos2d::Spawn::create(move_to,animate,repeat_check,nullptr);
}

// -------------------------- 死亡动画实现 --------------------------
void SoldierInCombat::Die() {
    if (!is_alive_) return;

    is_alive_ = false;
    this->stopAllActions();  // 停止所有当前动作

    auto death_anim = cocos2d::AnimationCache::getInstance()->getAnimation("Soldier_Death");
    auto animate = cocos2d::Animate::create(death_anim);
    auto remove_self = cocos2d::CallFunc::create([this]() {
        this->removeFromParent();
        auto manager = CombatManager::GetInstance();
        manager->live_soldiers_--;
        if(manager->IsCombatEnd()){
            manager->EndCombat();
        }
    });
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
    this->location_=map_->worldToVec(this->getPosition());
    if (!current_target_ || !current_target_->IsAlive()) {
        this->stopAllActions();  // 目标死亡，停止当前攻击动作
        BuildingInCombat* next_target = GetNextTarget();  // 寻找下一个目标
        if (next_target) {
            current_target_ = next_target;
            MoveToTarget();  // 移动到新目标继续攻击
        }
    }
}

struct AStarNode {
    cocos2d::Vec2 tile_pos;   // 格子坐标
    float g_cost{};              // 起点到当前的实际代价
    float h_cost{};              // 当前到终点的启发代价
    float f_cost() const { return g_cost + h_cost; }  // 总代价
    cocos2d::Vec2 parent_pos;         // 父节点（用于回溯路径）

    AStarNode() = default;
    // 构造函数
    explicit AStarNode(const cocos2d::Vec2& pos) : tile_pos(pos), g_cost(0), h_cost(0), parent_pos({-1,-1}) {}
    // 比较函数（用于优先队列的排序：F值小的在前）
    bool operator>(const AStarNode& other) const { return f_cost() > other.f_cost(); }
};

struct Vec2Hash {
    size_t operator()(const cocos2d::Vec2& v) const {
        return std::hash<float>()(v.x) ^ (std::hash<float>()(v.y) << 1);
    }
};

class PathFinder {
public:
    // A*寻路入口：返回从start到end的格子路径（若失败则返回空）
    std::vector<cocos2d::Vec2> FindPath(const cocos2d::Vec2& start_tile,const cocos2d::Vec2& end_tile,float soldier_range_);

private:
    // 生成8个方向的邻居格子
    constexpr static const float kDestroyCost = 10.0;
    static float ManhattanDistance(const cocos2d::Vec2& a, const::cocos2d::Vec2& b);
    MapManager* map_;
};

static const cocos2d::Vec2 kNeighborDirs[] = {
        cocos2d::Vec2(1, 0), cocos2d::Vec2(0, 1), cocos2d::Vec2(-1, 0), cocos2d::Vec2(0, -1)  // 4方向
};

float PathFinder::ManhattanDistance(const cocos2d::Vec2& a, const::cocos2d::Vec2& b){
    return abs(a.x-b.x) + abs(a.y-b.y);
}

// -------------------------- A*寻路入口 --------------------------
std::vector<cocos2d::Vec2> PathFinder::FindPath(const cocos2d::Vec2& start_tile, const cocos2d::Vec2& end_tile, const float soldier_range) {
    // 1. 边界检查：起点/终点不可通行则直接返回失败
    if (!map_->IsGridAvailable(start_tile) || !map_->IsGridAvailable(end_tile)) {
        return {};
    }

    // 2. 初始化OpenList（小根堆，按F值排序）、ClosedList（哈希表，避免重复）
    std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<>> open_list;
    std::unordered_set<cocos2d::Vec2, Vec2Hash> closed_list;
    // node_map：存储所有节点的详细信息（坐标→节点，避免指针拷贝）
    std::unordered_map<cocos2d::Vec2, AStarNode, Vec2Hash> node_map;

    // 3. 起点入队
    AStarNode start_node(start_tile);
    start_node.h_cost = ManhattanDistance(start_tile, end_tile);
    open_list.push(start_node);
    node_map[start_tile] = start_node;

    // 4. 核心寻路循环
    while (!open_list.empty()) {
        // 4.1 取出OpenList中F值最小的节点
        AStarNode current = open_list.top();
        open_list.pop();

        // 4.2 若到达终点，回溯路径
        if (current.tile_pos.distance(end_tile)<=soldier_range) {
            std::vector<cocos2d::Vec2> path;
            cocos2d::Vec2 temp_pos = end_tile;
            // 回溯父节点直到起点
            while (temp_pos != start_tile) {
                path.push_back(temp_pos);
                temp_pos = node_map[temp_pos].parent_pos;
            }
            std::reverse(path.begin(), path.end());  // 反转路径为起点→终点
            return path;
        }

        // 4.3 标记当前节点为已考察
        if(closed_list.count(current.tile_pos)) continue;
        closed_list.insert(current.tile_pos);

        // 4.4 遍历所有邻居节点
        for (const auto& dir : kNeighborDirs) {
            cocos2d::Vec2 neighbor_tile = current.tile_pos + dir;

            // 邻居不可通行或已在ClosedList，跳过
            if (!map_->isValidGrid(neighbor_tile) || closed_list.count(neighbor_tile)) {
                continue;
            }
            // 4.5 计算邻居的G/H/F代价
            float new_g = current.g_cost + 1;
            float new_h = ManhattanDistance(current.tile_pos,end_tile);  // 启发代价为对角线距离
            if(!map_->IsGridAvailable(neighbor_tile)){
                new_g += kDestroyCost;
            }

            bool is_neighbor_in_open = (node_map.count(neighbor_tile) > 0);
            // 若邻居不在OpenList，或新路径代价更低 → 更新
            if (!is_neighbor_in_open || new_g < node_map[neighbor_tile].g_cost) {
                AStarNode neighbor_node(neighbor_tile);
                neighbor_node.g_cost = new_g;
                neighbor_node.h_cost = new_h;
                neighbor_node.parent_pos = current.tile_pos; // 关联父节点坐标（核心）
                // 更新node_map并加入OpenList
                node_map[neighbor_tile] = neighbor_node;
                open_list.push(neighbor_node);
            }
        }
    }

    // 5. 寻路失败，返回空
    return {};
}

void SoldierInCombat::RedirectPath(std::vector<cocos2d::Vec2>& path){
    for(auto ptr = path.begin();ptr != path.end();ptr++){
        if(!map_->IsGridAvailable(*ptr)){
            auto new_target = *ptr;
            while(ptr->distance(new_target)<=this->soldier_template_->GetAttackRange()){
                --ptr;
            }//ptr指向超出攻击范围的第一个点
            ++ptr;//ptr指向目标到达的新的点
            path.erase(++ptr,path.end());//删除ptr后的路径
            break;
        }
    }
}

void SoldierInCombat::SimplifyPath(std::vector<cocos2d::Vec2>& path){
    if(path.size()<=2){
        CCLOG("invalid path found when simplified");
        return;
    }
    cocos2d::Vec2 former_dir = path[1]-path[0];
    std::vector<int> erased;
    //去除同方向直线上的中间点
    for(int i=2;i<path.size();i++){
        if(path[i]-path[i-1]==former_dir){
            erased.push_back(i-1);
        }
        else{
            former_dir = path[i]-path[i-1];
        }
    }
    //从后向前不影响顺序的情况下完成删除
    for(auto it = erased.rbegin(); it != erased.rend(); ++it){
        path.erase(path.begin() + *it);
    }
}

BuildingInCombat* SoldierInCombat::GetNextTarget() const {
    auto buildings = CombatManager::GetInstance()->buildings_;
    BuildingInCombat* target = *std::min_element(buildings.begin(),buildings.end(),
                                                 [&](BuildingInCombat* a,BuildingInCombat* b){
        return this->location_.distance(a->getPosition())<this->location_.distance(b->getPosition());
    });
    return target;
}

void SoldierInCombat::MoveToTarget() {
    PathFinder pf{};
    auto path = pf.FindPath(this->location_,current_target_->getPosition(),
                            this->soldier_template_->GetAttackRange());
    RedirectPath(path);
    SimplifyPath(path);
    cocos2d::Vector<cocos2d::FiniteTimeAction*> moves;
    for(int i=1;i<path.size();i++){
        moves.pushBack(CreateStraightMoveAction(path[i]));
    }
    cocos2d::Sequence* seq = cocos2d::Sequence::create(moves);
    if(seq) this->runAction(seq);
}
