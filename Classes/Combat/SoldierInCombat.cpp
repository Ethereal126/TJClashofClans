//
// Created by duby0 on 2025/12/7.
//
#include "SoldierInCombat.h"
#include "BuildingInCombat.h"
#include "Combat.h"
#include <unordered_set>


bool SoldierInCombat::is_animation_loaded_[];

const std::string SoldierInCombat::direction_names[4] = {"up", "down", "left", "right"};
// -------------------------- 工厂方法实现 --------------------------
SoldierInCombat* SoldierInCombat::Create(const Soldier* soldier_template, const cocos2d::Vec2& spawn_pos,MapManager* map) {
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

bool SoldierInCombat::Init(const Soldier* soldier_template, const cocos2d::Vec2& spawn_pos,MapManager* map) {
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
    soldier_template_ = soldier_template;
    position_ = spawn_pos;
    current_health_ = soldier_template->GetHealth();
    current_target_ = nullptr;

    if (!is_animation_loaded_[static_cast<int>(this->soldier_template_->GetSoldierType())]) {
        LoadSoldierAnimations();
    }
    auto soldierAnim = cocos2d::AnimationCache::getInstance()->getAnimation(this->soldier_template_->GetName()+"walkdown");
    auto firstFrame = soldierAnim->getFrames().at(0)->getSpriteFrame();
    this->setSpriteFrame(firstFrame);

    // 4. 设置初始状态
    map_ = map;
    // 只有在map_不为nullptr时才调用addChild
    if (map_ == nullptr) {
        CCLOG("SoldierInCombat init failed: Invalid map!");
        return false;
    }
    map_->setupNodeOnMap(this,floor(spawn_pos.x),floor(spawn_pos.y),1,1);
//    this->setPosition(map_->vecToWorld(spawn_pos));
//    this->setAnchorPoint(cocos2d::Vec2(0.5f, 0.4f));
    // 根据地图缩放系数调整士兵大小，保持视觉比例
    this->setScale(0.5f * map_->getGridScaleFactor());
    map_->addToWorld(this);
    map_->updateYOrder(this);

    auto soldier_size = this->getContentSize();
    hp_bar_ = HpBarComponents::createHpBar(this, soldier_size.height);

    this->DoAllMyActions();

    return true;
}

void SoldierInCombat::TakeDamage(int damage) {
    current_health_ -= damage;
    if (current_health_ < 0){
        current_health_ = 0;
    }
    hp_bar_.updateHp(current_health_,soldier_template_->GetHealth());
    if(current_health_==0) Die();
}


void SoldierInCombat::LoadSoldierAnimations() const {
    auto name = this->soldier_template_->GetName();
    auto frame_cache = cocos2d::SpriteFrameCache::getInstance();
    // 1. 加载动画资源plist（需将所有士兵动画帧打包为soldier_anim.plist+png，放在Resources目录）
    std::string plist_name = "Soldiers/"+name+"/anims.plist";
    std::string png_name = "Soldiers/"+name+"/anims.png";
    frame_cache->addSpriteFramesWithFile(plist_name, png_name);
    CCLOG("call load soldier animations of %s",name.c_str());

    // 2. 4方向移动动画（每方向8帧）
    for (const auto& dir_name : direction_names) {
        cocos2d::Vector<cocos2d::SpriteFrame*> move_frames;
        std::string anim_name = name+"walk" ;
        anim_name += dir_name;
        for (int i = 1; i <= this->soldier_template_->walk_frame_num; ++i) {
            std::string frame_name =  anim_name + std::to_string(i) + ".png";
            auto frame = frame_cache->getSpriteFrameByName(frame_name);
            if (frame) move_frames.pushBack(frame);
        }
        if(!move_frames.empty()) {
            auto move_anim = cocos2d::Animation::createWithSpriteFrames(move_frames, 0.1f); // 0.1秒/帧
            cocos2d::AnimationCache::getInstance()->addAnimation(move_anim, anim_name);
        }
    }

    // 3. 攻击动画（8帧）
    for (const auto& dir_name : direction_names) {
        cocos2d::Vector<cocos2d::SpriteFrame*> attack_frames;
        std::string anim_name = name+"attack" ;
        anim_name += dir_name;
        for (int i = 1; i <= this->soldier_template_->attack_frame_num; ++i) {
            std::string frame_name = anim_name + std::to_string(i) + ".png";
            auto frame = frame_cache->getSpriteFrameByName(frame_name);
            if (frame) attack_frames.pushBack(frame);
        }
        if(!attack_frames.empty()) {
            auto attack_anim = cocos2d::Animation::createWithSpriteFrames(attack_frames, 0.1f);
            cocos2d::AnimationCache::getInstance()->addAnimation(attack_anim, anim_name);
        }
    }

    is_animation_loaded_[static_cast<int>(this->soldier_template_->GetSoldierType())] = true;
}

// -------------------------- 4方向判断（核心：根据移动向量） --------------------------
enum class Direction : int{
    UP = 0,
    DOWN = 1,
    LEFT = 2,
    RIGHT = 3
};

void SetDirection(SoldierInCombat* s,const cocos2d::Vec2& delta){
    float abs_x = abs(delta.x),abs_y = abs(delta.y);
    if (abs_x > abs_y) {
        if(delta.x > 0) {
            s->setFlippedX(false);
        }
        else{
            s->setFlippedX(true);
        }
    } else {
        if(delta.y>0){
            s->setFlippedX(true);
        }
        else{
            s->setFlippedX(false);
        }
    }
}

Direction GetDirection(const cocos2d::Vec2& delta){
    if(delta.x>0 || delta.y>0) return Direction::UP;
    else return Direction::DOWN;
}

const float kCheckInterval = 0.1f;
// -------------------------- 核心：封装单段直线移动Action（Spawn同步MoveTo+动画） --------------------------
cocos2d::Spawn* SoldierInCombat::CreateStraightMoveAction(const cocos2d::Vec2& start_map_pos,const cocos2d::Vec2& target_map_pos) {
    // 1. 计算移动参数
    cocos2d::Vec2 move_delta = target_map_pos - start_map_pos;
    float move_distance = move_delta.length();
    float move_time = move_distance / soldier_template_->GetMoveSpeed(); // 基于数据类的移动速度

    // 2. 创建MoveTo（目标为地图坐标→屏幕坐标）
    cocos2d::Vec2 target_screen_pos = map_->vecToWorld(target_map_pos);
    auto move_to = cocos2d::MoveTo::create(move_time, target_screen_pos);


    // 3. 获取对应方向的动画
    Direction dir = GetDirection(move_delta);
    auto set_dir =cocos2d::CallFunc::create([this,dir,move_delta]() {
        SetDirection(this,move_delta);
    });
    std::string dir_name = direction_names[static_cast<int>(dir)],soldier_name = this->soldier_template_->GetName();
    auto move_anim = cocos2d::AnimationCache::getInstance()->getAnimation( soldier_name +"walk" + dir_name);

    float anim_duration = move_anim ? move_anim->getDuration() : 0.1f; // 兜底值
    // 计算动画需要循环的次数（移动总时长 / 单段动画时长）
    int anim_repeat_count = static_cast<int>(move_time / anim_duration);
    anim_repeat_count = std::max(anim_repeat_count, 1); // 至少循环1次

    // 用Repeat替代RepeatForever，确保动画和移动同步结束
    auto animate = cocos2d::Repeat::create(cocos2d::Animate::create(move_anim), anim_repeat_count);
    auto whole_animate = cocos2d::Sequence::create(set_dir,animate, nullptr);

    // 4. 构建“延迟+检测”的循环动作（每0.1秒检测一次）
    auto check_target = cocos2d::CallFunc::create([this]() {
        this->UpdatePosition();
    });
    // 单轮检测：延迟0.1秒 → 执行检测
    auto single_check_loop = cocos2d::Sequence::create(
            cocos2d::DelayTime::create(kCheckInterval),
            check_target,
            nullptr
    );
    // 循环次数：移动总时长 / 检测间隔（确保移动过程中持续检测）
    int check_repeat_count = static_cast<int>(move_time / kCheckInterval);
    // 避免除数为0（移动时间极短时）
    check_repeat_count = std::max(check_repeat_count, 1);
    // 循环执行检测
    auto repeat_check = cocos2d::Repeat::create(single_check_loop, check_repeat_count);
    return cocos2d::Spawn::create(move_to,whole_animate,repeat_check,nullptr);
}

void SoldierInCombat::DoAllMyActions(){
    BuildingInCombat* next_target = GetNextTarget();  // 寻找下一个目标
    if (next_target) {
        current_target_ = next_target;
        MoveToTargetAndStartAttack();  // 移动到新目标继续攻击
    }
}

// -------------------------- 死亡实现 --------------------------
void SoldierInCombat::NotifyManagerDie(){
    auto manager = CombatManager::GetInstance();
    if (manager) {
        manager->num_of_live_soldiers_--;
        for(auto it = manager->live_soldiers_.begin(); it != manager->live_soldiers_.end(); ++it){
            if(*it == this){
                manager->live_soldiers_.erase(it);
                break;
            }
        }

        for(auto it:manager->live_buildings_){
            if(typeid(*it)== typeid(AttackBuildingInCombat)){
                auto b = dynamic_cast<AttackBuildingInCombat*>(it);
                if(b->current_target_==this){
                    b->current_target_ = nullptr;
                }
            }
        }

        if(manager->IsCombatEnd()){
            CCLOG("call EndCombat() from soldier");
            manager->EndCombat();
        }
    }
}

void SoldierInCombat::Die() {
    CCLOG("soldier die started");
    this->stopAllActions();  // 停止所有当前动作

    auto remove_self = cocos2d::CallFunc::create([this]() {
        if (current_target_){
            auto it = std::find(current_target_->subscribers.begin(),
                                current_target_->subscribers.end(),this);
            if (it != current_target_->subscribers.end()) current_target_->subscribers.erase(it);
        }
        NotifyManagerDie();
        AudioManager::getInstance()->playDie();
        this->removeFromParent();
    });
    this->runAction(remove_self);
}

void SoldierInCombat::Attack(const cocos2d::Vec2& pos) {
    if(this->soldier_template_->GetSoldierType()==SoldierType::kBomber){
        BomberAttack(pos);
        return;
    }
    auto delta = current_target_->position_-pos;
    Direction dir = GetDirection(delta);
    auto set_dir =cocos2d::CallFunc::create([this,delta]() {
        SetDirection(this,delta);
    });
    std::string dir_name = direction_names[static_cast<int>(dir)],soldier_name = this->soldier_template_->GetName();
    auto attack_anim = cocos2d::AnimationCache::getInstance()->getAnimation( soldier_name+"attack"+dir_name);

    auto animate = cocos2d::Animate::create(attack_anim);
    auto single_attack = cocos2d::CallFunc::create([this]() {
        this->DealDamageToBuilding(current_target_);
        AudioManager::getInstance()->playSoldierAttack(this->soldier_template_->GetSoldierType());
    });
    auto anim_and_delay = cocos2d::Sequence::create(
            set_dir,
            animate,
            single_attack,
            cocos2d::DelayTime::create(this->soldier_template_->GetAttackDelay()-animate->getDuration()),
            nullptr
    );
    auto repeat_attack = cocos2d::RepeatForever::create(anim_and_delay);
    this->runAction(repeat_attack);
}

void SoldierInCombat::BomberAttack(const cocos2d::Vec2& pos) {
    auto delta = current_target_->position_ - pos;
    auto animate = cocos2d::CallFunc::create([this, delta,pos]() {
        cocos2d::DelayTime::create(this->soldier_template_->GetAttackDelay());
        DealSplashDamage(pos);
        AudioManager::getInstance()->playSoldierAttack(this->soldier_template_->GetSoldierType());
        Die();
    });
    this->runAction(animate);
}
void SoldierInCombat::DealDamageToBuilding(BuildingInCombat* target) const {
    bool ret=false;
    if (target) {
        if(this->soldier_template_->GetSoldierType()==SoldierType::kBomber&&
                typeid(*(target->building_template_))==typeid(WallBuilding)){
            ret = target->TakeDamage(soldier_template_->GetDamage()*40);  // 调用建筑的受伤害方法
        }
        else{
            ret = target->TakeDamage(soldier_template_->GetDamage());  // 调用建筑的受伤害方法
        }
    }
    if(ret){
        auto name = target->building_template_->GetName();
        CCLOG("%s health:%d",name.c_str(),target->GetCurrentHealth());
    }
}

void SoldierInCombat::DealSplashDamage(const cocos2d::Vec2& pos){
    auto surroundings = map_->GetSurroundings(pos);
    std::unordered_set<Building*> visited;
    auto buildings_in_combat = CombatManager::GetInstance()->live_buildings_;
    for(auto it:surroundings){
        auto building = map_->getBuildingAt(static_cast<int>(it.x),static_cast<int>(it.y));
        if(building && (visited.find(building)==visited.end())){
            CCLOG("splash");
            auto building_in_combat = find_if(buildings_in_combat.begin(),buildings_in_combat.end(),[building](BuildingInCombat* a){
                return a->building_template_ == building;
            });
            if(building_in_combat!=buildings_in_combat.end()) DealDamageToBuilding(*building_in_combat);
        }
    }
}

void SoldierInCombat::UpdatePosition() {
    this->position_=map_->worldToVec(this->getPosition());
    map_->updateYOrder(this);
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
    explicit PathFinder(MapManager* map):map_(map){};
    // A*寻路入口：返回从start到end的格子路径（若失败则返回空）
    std::vector<cocos2d::Vec2> FindPath(cocos2d::Vec2 start_tile,cocos2d::Vec2 end_tile,
                                        float soldier_range_,int building_width,int building_length);

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
std::vector<cocos2d::Vec2> PathFinder::FindPath(cocos2d::Vec2 start_tile,cocos2d::Vec2 end_tile, float soldier_range,
                                                int building_width,int building_length) {
    // 1. 初始化OpenList（小根堆，按F值排序）、ClosedList（哈希表，避免重复）
    std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<>> open_list;
    std::unordered_set<cocos2d::Vec2, Vec2Hash> closed_list;
    // node_map：存储所有节点的详细信息（坐标→节点，避免指针拷贝）
    std::unordered_map<cocos2d::Vec2, AStarNode, Vec2Hash> node_map;
    start_tile.x = floor(start_tile.x);
    start_tile.y = floor(start_tile.y);
    end_tile.x = floor(end_tile.x);
    end_tile.y = floor(end_tile.y);


    if(building_width!=1 || building_length!=1){
        std::vector<cocos2d::Vec2> candidates;
        int x_min = floor(end_tile.x),y_min = floor(end_tile.y),
            x_max = x_min+building_width-1,y_max = y_min+building_length-1;
        // 1. 下边
        for (int x = x_min; x <= x_max; x += 1) candidates.emplace_back(x, y_min);
        // 2. 上边
        for (int x = x_min; x <= x_max; x += 1) candidates.emplace_back(x, y_max);
        // 3. 右边
        for (int y = y_min + 1; y < y_max; y += 1) candidates.emplace_back(x_max, y);
        // 4. 左边
        for (int y = y_min + 1; y < y_max; y += 1) candidates.emplace_back(x_min, y);
        end_tile = *std::min_element(candidates.begin(),candidates.end(),[start_tile](auto a,auto b){
            return start_tile.distance(a)<start_tile.distance(b);
        });
    }

    // 2. 起点入队
    AStarNode start_node(start_tile);
    start_node.h_cost = ManhattanDistance(start_tile, end_tile);
    open_list.push(start_node);
    node_map[start_tile] = start_node;

    // 3. 核心寻路循环
    while (!open_list.empty()) {
        // 3.1 取出OpenList中F值最小的节点
        AStarNode current = open_list.top();
        open_list.pop();

        // 3.2 若到达终点，回溯路径
        if (current.tile_pos.distance(end_tile)<=soldier_range) {
            CCLOG("end tile:(%f,%f),soldier_range:%f",end_tile.x,end_tile.y,soldier_range);
            std::vector<cocos2d::Vec2> path;
            cocos2d::Vec2 temp_pos = current.tile_pos;
            // 回溯父节点直到起点
            while (temp_pos != start_tile) {
                path.push_back(temp_pos);
                temp_pos = node_map[temp_pos].parent_pos;
            }
            std::reverse(path.begin(), path.end());  // 反转路径为起点→终点
            return path;
        }

        // 3.3 标记当前节点为已考察
        if(closed_list.count(current.tile_pos)) continue;
        closed_list.insert(current.tile_pos);

        // 3.4 遍历所有邻居节点
        for (const auto& dir : kNeighborDirs) {
            cocos2d::Vec2 neighbor_tile = current.tile_pos + dir;

            // 邻居不可通行或已在ClosedList，跳过
            if (!map_->isValidGrid(neighbor_tile) || closed_list.count(neighbor_tile)) {
                continue;
            }
            // 3.5 计算邻居的G/H/F代价
            float new_g = current.g_cost + 1;
            float new_h = ManhattanDistance(neighbor_tile,end_tile);  // 启发代价为对角线距离
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

    CCLOG("no path");
    // 4. 寻路失败，返回空
    return {};
}

void SoldierInCombat::RedirectPath(std::vector<cocos2d::Vec2>& path){
    if(path.empty()){
        CCLOG("empty path");
    }
    LogPath(path);
    for(auto ptr = path.begin();ptr != path.end();ptr++){
        if(!map_->IsGridAvailable(*ptr)){
            //CCLOG("(%f,%f) in path not available",ptr->x,ptr->y);
            auto new_target = *ptr;
            while(ptr != path.begin() && ptr->distance(new_target) <= this->soldier_template_->GetAttackRange()){
                --ptr;
            }//ptr指向超出攻击范围的第一个点
            // 此时 ptr 指向的是我们要保留的倒数第二个点
            // 删除 ptr 后一点之后的所有点
            auto last_to_keep = std::next(ptr);
            if (last_to_keep == path.end()) break;
            auto start_erase = std::next(last_to_keep);
            if (start_erase != path.end()) {
                path.erase(start_erase, path.end());
            }
            break;
        }
    }
}

void SoldierInCombat::SimplifyPath(std::vector<cocos2d::Vec2>& path){
    if(path.size()<2){
        if(path.empty()){
            CCLOG("empty path");
        }
        for(auto it:path){
            CCLOG("(%f,%f)",it.x,it.y);
        }
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

void LogVec2(const cocos2d::Vec2& pos,std::string& s){
    s +="(";
    s += std::to_string(pos.x);
    s += ",";
    s += std::to_string(pos.y);
    s +=")";
}
void SoldierInCombat::LogPath(const std::vector<cocos2d::Vec2>& path) const{
    std::string s = soldier_template_->GetName() + " find path:";
    for(auto i : path){
        LogVec2(i,s);
    }
    s+=" target:";
    auto target = current_target_->position_;
    LogVec2(target,s);
    auto up_right = target+cocos2d::Vec2(current_target_->building_template_->GetWidth()-1,current_target_->building_template_->GetLength()-1);
    LogVec2(up_right,s);
    CCLOG("%s",s.c_str());
}
BuildingInCombat* SoldierInCombat::GetNextTarget() {
    auto buildings = CombatManager::GetInstance()->live_buildings_;
    if(buildings.empty()) return nullptr;
    else{
        auto b = buildings.front();
        CCLOG("building name:%s",b->getResourceName().c_str());
    }

    BuildingInCombat* target = *std::min_element(buildings.begin(),buildings.end(),
                                                 [&](BuildingInCombat* a,BuildingInCombat* b){
        if(this->soldier_template_->building_preference_.has_value()) {
            std::type_index ta = typeid(*a->building_template_),tb = typeid(*b->building_template_);
            std::type_index preference = this->soldier_template_->building_preference_.value();
            bool aIsTarget = (ta == preference), bIsTarget = (tb == preference);
            CCLOG("a:%s,b:%s,preference:%s", ta.name(), tb.name(), preference.name());
            if (aIsTarget != bIsTarget) {
                return aIsTarget;
            }
        }

        return this->position_.distance(a->position_) < this->position_.distance(b->position_);
    });
    target->subscribers.push_back(this);
    return target;
}

void SoldierInCombat::MoveToTargetAndStartAttack() {
    PathFinder pf(map_);
    auto width = current_target_->building_template_->GetWidth(),length = current_target_->building_template_->GetLength();
    auto path = pf.FindPath(this->position_, current_target_->position_,
                            this->soldier_template_->GetAttackRange(),width,length);
    RedirectPath(path);
    SimplifyPath(path);
    LogPath(path);
    cocos2d::Vector<cocos2d::FiniteTimeAction*> moves;
    for(int i=1;i<path.size();i++){
        moves.pushBack(CreateStraightMoveAction(path[i-1],path[i]));
    }
    auto start_attack = cocos2d::CallFunc::create([this,path]() {
        if (path.empty()) this->Attack(this->position_);
        else this->Attack(path.back());
    });
    moves.pushBack(start_attack);
    cocos2d::Sequence* seq = cocos2d::Sequence::create(moves);
    if(seq) this->runAction(seq);
}
