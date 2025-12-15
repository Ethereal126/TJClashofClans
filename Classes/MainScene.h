#pragma once
#ifndef __HOME_SCENE_H__
#define __HOME_SCENE_H__

#include "cocos2d.h"
#include "Map/Map.h"

class MainScene : public cocos2d::Scene {
public:
    static MainScene* createScene(int mapWidth, int mapLength, int gridSize) {
        auto scene = MainScene::create();
        if (!scene) return nullptr;

        // 创建地图（主村庄）
        auto map = ::Map::create(mapWidth, mapLength, gridSize, TerrainType::Home);
        // 将地图摆放到屏幕中心偏下（根据你的需求调整）
        map->setPosition(cocos2d::Vec2(0.f, 0.f));
        scene->addChild(map, 0);

        // TODO: 如果有 HUD/UIManager，这里可调用 UIManager 显示 HUD 和 ResourceBar

        return scene;
    }

    virtual bool init() override {
        if (!cocos2d::Scene::init()) return false;
        return true;
    }

    CREATE_FUNC(MainScene);
};

#endif // __HOME_SCENE_H__