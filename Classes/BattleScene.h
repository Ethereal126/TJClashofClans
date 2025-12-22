#pragma once
#ifndef __BATTLE_SCENE_H__
#define __BATTLE_SCENE_H__

#include "cocos2d.h"

// 前向声明
class MapManager;

class BattleScene : public cocos2d::Scene {
public:
    static BattleScene* createScene(int levelId);
    
    MapManager* getMap() const { return _map; }

    CREATE_FUNC(BattleScene);
    
private:
    MapManager* _map = nullptr;
};

#endif // __BATTLE_SCENE_H__