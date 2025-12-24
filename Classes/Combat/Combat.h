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
#include "MapManager/MapManager.h"
#include "SoldierInCombat.h"
#include "BuildingInCombat.h"

enum class CombatState {
    kWrongInit,//初始化失败
    kReady,   // 战斗准备（未开始）
    kFighting,// 战斗进行中
    kEnded    // 战斗结束
};


//负责统筹管理整个战斗过程，仅包含最基本的需求
class CombatManager :public cocos2d::Node {
public:
    std::vector<std::pair<Soldier*,int>> soldier_to_use_;
    std::vector<BuildingInCombat*> live_buildings_;
    std::vector<SoldierInCombat*> live_soldiers;
    int num_of_live_soldiers_ = 0,num_of_live_buildings = 0;
    int stars = 0,destroy_degree_ = 0,buildings_should_count=0,buildings_should_count_destroyed=0;

    static CombatManager* InitializeInstance(MapManager* map); // 初始化单例（仅第一次调用有效）
    static void DestroyInstance();
    static CombatManager* GetInstance();// 获取单例（已初始化则直接返回）

    //在接收到交互指令后，将士兵加入到战斗中；
    void SendSoldier(const Soldier* soldier_template,cocos2d::Vec2 spawn_pos);

    bool IsCombatEnd();
    void StartCombat();
    void PauseCombat();
    void ResumeCombat();
    void EndCombat();

protected:
    // 禁止外部直接构造/析构，仅通过 InitializeInstance/Destroy 管理
    CombatManager() = default;
    ~CombatManager() override;
    //初始化战场中的建筑，返回初始化结果
    bool Init(MapManager* map);

private:
    static CombatManager* instance_;
    MapManager* map_ = nullptr;
    CombatState state_ = CombatState::kWrongInit;
    float combat_time_ = 0.0f;
    const float kMaxCombatTime = 300.0f;

    void Update(float dt);
};

#endif //PROGRAMMING_PARADIGM_FINAL_PROJECT_COMBAT_H
