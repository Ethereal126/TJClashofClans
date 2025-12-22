#include "MainScene.h"
#include "BattleScene.h"
#include "MapManager/MapManager.h"
#include "UIManager/UIManager.h"
#include "AudioManager/AudioManager.h"

using namespace cocos2d;

MainScene* MainScene::createScene() {
    auto scene = MainScene::create();
    if (!scene) return nullptr;

    auto map = MapManager::create(30, 30, -1, TerrainType::Home);
    if (map) {
        std::string path = "archived/player_save.json"; 
        if (FileUtils::getInstance()->isFileExist(path)) {
            map->loadMapData(path);
        }
        scene->addChild(map, 0);
        scene->_map = map;
    }

    auto ui = UIManager::getInstance();
    if (ui->init(scene)) {
        ui->showPanel(UIPanelType::GameHUD, UILayer::HUD);

        ui->setUICallback("OnEnterPlacementMode", [map]() {
            auto building = UIManager::getInstance()->getPendingPlacementBuilding();
            auto cost = UIManager::getInstance()->getPendingPlacementCost();
            if (building && map) {
                map->enterPlacementMode(building, cost);
            }
        });

        ui->setUICallback("OnBattleStart_Map1", []() {
            auto scene = BattleScene::createScene(1);
            Director::getInstance()->replaceScene(TransitionFade::create(0.5, scene));
        });

        ui->setUICallback("OnBattleStart_Map2", []() {
            auto scene = BattleScene::createScene(2);
            Director::getInstance()->replaceScene(TransitionFade::create(0.5, scene));
        });
    }

    return scene;
}

bool MainScene::init() {
    if (!Scene::init()) return false;
    return true;
}