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

//负责统筹管理整个战斗过程，仅包含最基本的需求
class Combat {
private:
    // ========== 单例核心：私有构造/拷贝/赋值 ==========
    // 1. 私有构造函数（禁止外部创建实例）
    Combat();
    // 2. 私有析构函数（禁止外部delete）
    ~Combat();


    MapManager* map_;
    cocos2d::Node* combat_root_node_ = nullptr;
    int destroy_degree_;
    bool is_inited_ = false;

    static Combat& GetInstanceInternal();

public:
    Combat(const Combat&) = delete;
    Combat& operator=(const Combat&) = delete;

    std::vector<SoldierInCombat*> soldiers_;
    std::vector<BuildingInCombat*> buildings_;
    // ========== 单例核心：对外暴露的获取实例接口 ==========
    // 外部唯一获取实例的方式（全局可访问）
    static Combat& GetInstance();
    //初始化战场中的建筑，返回初始化结果
    bool Init(MapManager* map);
    void Reset();
    //在接收到交互指令后，将士兵加入到战斗中；
    void SendSoldier(Soldier* soldier_template,cocos2d::Vec2 spawn_pos);


    //退出战斗，返回破坏度（destroy_degree_）
    int EndCombat();
};

#endif //PROGRAMMING_PARADIGM_FINAL_PROJECT_COMBAT_H
