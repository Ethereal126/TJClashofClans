#pragma once
#ifndef __BATTLE_SCENE_H__
#define __BATTLE_SCENE_H__

#include "cocos2d.h"
#include "MapManager/MapManager.h"
#include "UIManager/UIManager.h"
#include "Combat/Combat.h"
#include "MainScene.h"

class BattleScene : public cocos2d::Scene {
public:
    static BattleScene* createScene(int levelId) {
        auto scene = BattleScene::create();
        if (!scene) {
            CCLOG("scene creation error");
            return nullptr;
        }
        else {
            CCLOG("scene creation success");
        }
        // 创建战斗地图 (TerrainType::Battle)
        auto map = MapManager::create(30,30,-1, TerrainType::Battle);
        if (!map) {
            CCLOG("map error");
        }
        else {
            CCLOG("map success");
        }
        // 根据 levelId 加载对应的建筑布局数据
        std::string levelPath = "archived/battle_field" + std::to_string(levelId) + ".json";
        
        // 2. 只给地图传数据，不影响 TownHall
        map->loadMapData(levelPath);

        scene->addChild(map, 0);
		scene->_map = map;

        auto combatMgr = CombatManager::Create(map);
        scene->addChild(combatMgr); // 挂载到场景中以触发 Update

        // 初始化战斗 UI
        if (UIManager::getInstance()->init(scene)) {
            UIManager::getInstance()->enterBattleMode(map);
        }
        
        return scene;
    }

    MapManager* getMap() const { return _map; }

    CREATE_FUNC(BattleScene);
    
private:
    MapManager* _map = nullptr;
};


#endif // __BATTLE_SCENE_H__
