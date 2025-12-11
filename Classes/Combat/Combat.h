//
// Created by napper-d on 2025/12/1.
//

#ifndef PROGRAMMING_PARADIGM_FINAL_PROJECT_COMBAT_H
#define PROGRAMMING_PARADIGM_FINAL_PROJECT_COMBAT_H

#include <string>
#include "cocos2d.h"
#include "cocos-ext.h"
#include "Building/Building.h"
#include "Soldier/Soldier.h"
#include "Map/Map.h"



class SoldierInCombat;
class BuildingInCombat;
class Combat;

class SoldierInCombat : public cocos2d::Sprite{
public:
    ~SoldierInCombat();
    cocos2d::Vec2 location_;
    bool is_alive_;

    SoldierInCombat* Create(Soldier* soldier_template, const cocos2d::Vec2& spawn_pos);
    // 初始化函数
    bool Init(Soldier* soldier_template, const cocos2d::Vec2& spawn_pos);
    // 被攻击函数
    void TakeDamage(int damage);

private:
    Soldier* soldier_template_;
    int current_health_;
    BuildingInCombat* current_target_;
    Map* map_;

    void SetTarget(BuildingInCombat* target);
    void Die();
    void MoveToTarget();
    void StartAttack();
    void DealDamageToTarget();
    void CheckTargetAlive();
    BuildingInCombat* GetNextTarget();
    cocos2d::Spawn* CreateStraightMoveAction(const cocos2d::Vec2& target_map_pos);
    void RedirectPath(std::vector<cocos2d::Vec2>& path);
    void SimplifyPath(std::vector<cocos2d::Vec2>& path);
    // -------------------------- 动画资源（静态共享） --------------------------
    void LoadSoldierAnimations();
    static cocos2d::Vector<cocos2d::SpriteFrame*> move_frames_;   // 移动动画帧
    static cocos2d::Vector<cocos2d::SpriteFrame*> attack_frames_; // 攻击动画帧
    static cocos2d::Vector<cocos2d::SpriteFrame*> die_frames_;    // 死亡动画帧
    static bool is_animation_loaded_; // 标记动画资源是否已加载（避免重复加载）
};


class BuildingInCombat : public cocos2d::Sprite{
public:
    // 构造函数
    BuildingInCombat(Building* b): building_template(b){
        current_health = building_template->GetHealth();
        is_alive_ = true;
    };
    // 析构函数
    virtual ~BuildingInCombat();

    // 初始化函数
    virtual bool Init(const std::string& filename);

    // 被攻击函数
    void TakeDamage(int damage);

    //使用时应确保建筑类型为防御类建筑，函数参数有待确定，暂时考虑通过轮询方式搜索攻击目标，如果有更好的想法再做优化
    void Attack();

    // 判断是否存活
    bool IsAlive() const;

    void Destroyed();

private:
    Building* building_template;
    int current_health;
    bool is_alive_;
};

class CombatScene : public cocos2d::Layer {
public:
    // Cocos2d-x标准场景创建接口
    static cocos2d::Scene* CreateScene();

protected:
    CombatScene() = default;  // 构造函数保护，仅允许工厂方法创建
    ~CombatScene() override;

    // 重写初始化函数（核心逻辑在这里）
    bool init() override;

    // -------------------------- 伪等轴场景核心工具函数 --------------------------
    // 1. 瓦片坐标→屏幕坐标转换（Cocos2d-x原生支持，封装后更易用）
    cocos2d::Vec2 TileToScreenPos(const cocos2d::Vec2& tile_pos) const;
    // 2. 屏幕坐标→瓦片坐标转换
    cocos2d::Vec2 ScreenToTilePos(const cocos2d::Vec2& screen_pos) const;
    // 3. 自动深度排序（根据Y坐标设置Z轴，解决伪等轴遮挡）
    void AutoSortDepth(cocos2d::Node* node) const;

private:
    // -------------------------- 成员变量 --------------------------
    cocos2d::TMXTiledMap* map_ = nullptr;  // 伪等轴地图实例
    float map_min_y_ = 0;  // 地图Y轴最小边界（用于士兵透视缩放）
    float map_max_y_ = 0;  // 地图Y轴最大边界（用于士兵透视缩放）

    // 宏定义：注册场景为Cocos2d-x自动创建对象
    CREATE_FUNC(CombatScene);
};

//负责统筹管理整个战斗过程，仅包含最基本的需求
class Combat{
private:
    // ========== 单例核心：私有构造/拷贝/赋值 ==========
    // 1. 私有构造函数（禁止外部创建实例）
    Combat();
    // 2. 私有析构函数（禁止外部delete）
    ~Combat();
    // 3. 禁用拷贝构造和赋值运算符（确保唯一实例）
    Combat(const Combat&) = delete;
    Combat& operator=(const Combat&) = delete;


    Map* map_;
    CombatScene scene_;
    int destroy_degree_;

    static Combat& GetInstanceInternal() {
        static Combat instance; // 第一次调用时初始化，仅初始化一次
        return instance;
    }

public:
    std::vector<SoldierInCombat*> soldiers_;
    std::vector<BuildingInCombat*> buildings_;
    // ========== 单例核心：对外暴露的获取实例接口 ==========
    // 外部唯一获取实例的方式（全局可访问）
    static Combat& GetInstance() {
        return GetInstanceInternal();
    }
    //初始化战场中的建筑，返回初始化结果
    bool Init(Map* map){
        if(map== nullptr) return false;
        this->map_=map;
        for(auto building:map_->getAllBuildings()){
            auto b = new BuildingInCombat(building);
            buildings_.push_back(b);
        }
        destroy_degree_ = 0;
        return true;
    };

    //在接收到交互指令后，将士兵加入到战斗中；
    void SendSoldier();

    //对于依赖轮询的对象进行更新，确认战斗是否可以结束
    void Update();

    //退出战斗，返回破坏度（destroy_degree_）
    int EndCombat();
};


#endif //PROGRAMMING_PARADIGM_FINAL_PROJECT_COMBAT_H
