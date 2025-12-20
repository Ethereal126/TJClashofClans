#pragma once
#ifndef __HOME_SCENE_H__
#define __HOME_SCENE_H__

#include "cocos2d.h"
#include "MapManager/MapManager.h"
#include "UIManager/UIManager.h"

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
        scene->addChild(map, 0);
		scene->_map = map;

        if (UIManager::getInstance()->init(scene)) {
            UIManager::getInstance()->showPanel(UIPanelType::GameHUD, UILayer::HUD);
            UIManager::getInstance()->showPanel(UIPanelType::ResourceBar, UILayer::HUD);
        }

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