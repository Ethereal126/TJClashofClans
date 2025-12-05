//
// Created by napper-d on 2025/12/1.
//

#ifndef PROGRAMMING_PARADIGM_FINAL_PROJECT_COMBAT_H
#define PROGRAMMING_PARADIGM_FINAL_PROJECT_COMBAT_H

#include <string>
#include <vector>
#include <unordered_set>
#include "cocos2d.h"
#include "Building/Building.h"
#include "Soldier/Soldier.h"
#include "Map/Map.h"



enum class SoldierStatus : int{
    kMoving,
    kFighting,
    kDead
};

class SoldierInCombat : public Soldier{
public:
    cocos2d::Vec2 location;

    int status;

    //通过schedule功能为士兵安排一条移动向某个建筑附近的路径，并订阅目标建筑
    void PlanPath();

    // 攻击函数
    void Attack(cocos2d::Node* target);

    // 被攻击函数
    void BeAttacked(int damage);

    // 移动到指定位置
    void MoveTo(const cocos2d::Vec2& position);
};


class BuildingInCombat : public Building{
public:
    std::unordered_set<Soldier*> subscribers;

    // 构造函数
    BuildingInCombat();
    // 析构函数
    virtual ~BuildingInCombat();

    // 初始化函数
    virtual bool Init(const std::string& filename);

    // 被攻击函数
    void BeAttacked(int damage);

    //使用时应确保建筑类型为防御类建筑，函数参数有待确定，暂时考虑通过轮询方式搜索攻击目标，如果有更好的想法再做优化
    void Attack();

    // 判断是否存活
    bool IsAlive() const;

    //设置is_alive_，修改动画展示，向subscribers广播信息，为其重新规划路径
    void Destroyed();

private:
    bool is_alive_;
};

//负责管理战斗中部署士兵的交互实现，以及其他与动画相关的细节，等具体开始实现Combat时填充内容
class CombatScene : public cocos2d::Scene{

};

//负责统筹管理整个战斗过程，仅包含最基本的需求
class Combat{
private:
    std::vector<Soldier*> soldiers_;
    Map map_;
    CombatScene scene_;
    int destroy_degree_;

public:
    Combat();
    ~Combat();

    //初始化战场中的建筑，返回初始化结果
    bool Init();

    //在接收到交互指令后，将士兵加入到战斗中；
    void SendSoldier();

    //对于依赖轮询的对象进行更新，确认战斗是否可以结束
    void Update();

    //退出战斗，返回破坏度（destroy_degree_）
    int EndCombat();
};

#endif //PROGRAMMING_PARADIGM_FINAL_PROJECT_COMBAT_H
