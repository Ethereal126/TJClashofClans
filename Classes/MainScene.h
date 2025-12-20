#pragma once
#ifndef __HOME_SCENE_H__
#define __HOME_SCENE_H__

#include "cocos2d.h"
#include "MapManager/MapManager.h"
#include "UIManager/UIManager.h"
#include "TownHall/TownHall.h"
#include "BattleScene.h"


class MainScene : public cocos2d::Scene {
public:
    static MainScene* createScene() {
        auto scene = MainScene::create();
        if (!scene) {
            CCLOG("scene creation error");
            return nullptr;
        }
        else {
            CCLOG("scene creation success");
        }
        // 创建地图（主村庄）
        auto map = MapManager::create(30, 30, -1, TerrainType::Home);
        if (!map) {
            CCLOG("map error");
        }
        else {
            CCLOG("map success");
        }
        // 1. 获取本地存档路径
        std::string path = "archived/player_save.json"; 
    
        // 2. 如果存档存在，则分发数据
        if (cocos2d::FileUtils::getInstance()->isFileExist(path)) {
            map->loadMapData(path);
        }
        scene->addChild(map, 0);
		scene->_map = map;

        if (UIManager::getInstance()->init(scene)) {
            UIManager::getInstance()->showPanel(UIPanelType::GameHUD, UILayer::HUD);

            // 注册 UI 回调事件
            // 1. 购买建筑后的放置模式
            UIManager::getInstance()->setUICallback("OnEnterPlacementMode", [map]() {
                auto building = UIManager::getInstance()->getPendingPlacementBuilding();
                auto cost = UIManager::getInstance()->getPendingPlacementCost();
                if (building && map) {
                    map->enterPlacementMode(building, cost);
                }
            });

            // 2. 地图选择进入战斗 (Map 1)
            UIManager::getInstance()->setUICallback("OnBattleStart_Map1", []() {
                CCLOG("Switching to Battle Map 1...");
                auto scene = BattleScene::createScene(1);
                cocos2d::Director::getInstance()->replaceScene(cocos2d::TransitionFade::create(0.5, scene));
            });

            // 3. 地图选择进入战斗 (Map 2)
            UIManager::getInstance()->setUICallback("OnBattleStart_Map2", []() {
                CCLOG("Switching to Battle Map 2...");
                auto scene = BattleScene::createScene(2);
                cocos2d::Director::getInstance()->replaceScene(cocos2d::TransitionFade::create(0.5, scene));
            });

            UIManager::getInstance()->setUICallback("OnUpgradeStarted", [map]() {
                if (map) {
                    map->saveMapData("archived/player_save.json");
                }
            });

            UIManager::getInstance()->setUICallback("OnUpgradeComplete", [map]() {
                if (map) {
                    map->saveMapData("archived/player_save.json");
                }
            });

            UIManager::getInstance()->setUICallback("OnArmyConfigSaved", []() {
                CCLOG("Army config saved");
            });
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