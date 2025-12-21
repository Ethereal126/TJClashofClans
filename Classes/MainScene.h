#pragma once
#ifndef __HOME_SCENE_H__
#define __HOME_SCENE_H__

#include "cocos2d.h"

// 前向声明
class MapManager;

class MainScene : public cocos2d::Scene {
public:
    static MainScene* createScene();
    virtual bool init() override;
    
    // 获取地图引用
    MapManager* getMap() const { return _map; }

    CREATE_FUNC(MainScene);

private:
    MapManager* _map = nullptr;
};

#endif // __HOME_SCENE_H__