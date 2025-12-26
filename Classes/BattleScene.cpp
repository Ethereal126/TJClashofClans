#include "BattleScene.h"
#include "MainScene.h"
#include "MapManager/MapManager.h"
#include "UIManager/UIManager.h"
#include "Combat/Combat.h"
#include "AudioManager/AudioManager.h"
#include "ReplayScene.h"

using namespace cocos2d;

BattleScene* BattleScene::createScene(int levelId) {
    CCLOG("BattleScene::createScene() started");
    auto scene = BattleScene::create();
    if (!scene) return nullptr;

    auto map = MapManager::create(30, 30, -1, TerrainType::Battle);
    if (map) {
        std::string levelPath = "archived/battle_field" + std::to_string(levelId) + ".json";
        if(!map->loadMapData(levelPath)) CCLOG("BattleScene::createScene() map loading failure");
        else CCLOG("BattleScene::createScene() map loaded");
        scene->addChild(map, 0);
        scene->_map = map;
    }

    auto combatMgr = CombatManager::InitializeInstance(map);
    scene->addChild(combatMgr);

    auto ui = UIManager::getInstance();
    if (ui->init(scene)) {
        ui->setCurrentLevelId(levelId); 
        ui->enterBattleMode(map);
        AudioManager::getInstance()->playMusic(true);
        combatMgr->StartCombat();
        ui->setUICallback("OnRequestEndBattle", [combatMgr]() {
            AudioManager::getInstance()->playMusic(false);
            UIManager::getInstance()->exitBattleMode();
            combatMgr->EndCombat();
            auto homeScene = MainScene::createScene();
            Director::getInstance()->replaceScene(TransitionFade::create(0.5f, homeScene));
        });
        ui->setUICallback("OnRequestReplay", []() {
            auto ui = UIManager::getInstance();
            int levelId = ui->getCurrentLevelId();
            auto steps = ui->getRecordedSteps();
            
            // 销毁当前战斗实例，准备重播
            CombatManager::DestroyInstance();
            
            auto replayScene = ReplayScene::createScene(levelId, steps);
            Director::getInstance()->replaceScene(TransitionFade::create(0.5f, replayScene));
        });
    }
    
    return scene;
}