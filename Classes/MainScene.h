#pragma once
#ifndef __HOME_SCENE_H__
#define __HOME_SCENE_H__

#include "cocos2d.h"
#include "MapManager/MapManager.h"

class MainScene : public cocos2d::Scene {
public:
    static MainScene* createScene(int mapWidth, int mapLength, int gridSize) {
        auto scene = MainScene::create();
        if (!scene) {
            CCLOG("scene creation error");
            return nullptr;
        }
        else {
            CCLOG("scene creation success");
        }
        // 创建地图（主村庄）
        auto map = MapManager::create(mapWidth, mapLength, gridSize, TerrainType::Home);
        if (!map) {
            CCLOG("map error");
        }
        else {
            CCLOG("map success");
        }
        // 将地图摆放到屏幕中心偏下
        map->setPosition(cocos2d::Vec2(0.f, 0.f));
        scene->addChild(map, 0);
		scene->_map = map;

        // TODO: 如果有 HUD/UIManager，这里可调用 UIManager 显示 HUD 和 ResourceBar

        return scene;
    }

    virtual bool init() override {
        if (!cocos2d::Scene::init()) return false;
        return true;
    }

    // 获取地图引用
    MapManager* getMap() const { return _map; }


    CREATE_FUNC(MainScene);

private:
    MapManager* _map = nullptr;
};

#endif // __HOME_SCENE_H__