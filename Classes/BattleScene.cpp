#include "BattleScene.h"
#include "MainScene.h"
#include "MapManager/MapManager.h"
#include "UIManager/UIManager.h"
#include "Combat/Combat.h"
#include "AudioManager/AudioManager.h"

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
        ui->enterBattleMode(map);
        combatMgr->StartCombat();
        ui->setUICallback("OnRequestEndBattle", []() {
            AudioManager::getInstance()->playMusic(false);
            UIManager::getInstance()->exitBattleMode();
            auto homeScene = MainScene::createScene();
            Director::getInstance()->replaceScene(TransitionFade::create(0.5f, homeScene));
        });

        //test soldier behavior
        auto giant = TownHall::GetSoldierTemplate(SoldierType::kGiant)->Create();
        auto bomber = TownHall::GetSoldierTemplate(SoldierType::kBomber)->Create();
        auto archer = TownHall::GetSoldierTemplate(SoldierType::kArcher)->Create();
        auto barbarian = TownHall::GetSoldierTemplate(SoldierType::kBarbarian)->Create();

        combatMgr->addChild(giant);
        combatMgr->addChild(bomber);
        combatMgr->addChild(archer);
        combatMgr->addChild(barbarian);

        combatMgr->SendSoldier(bomber,cocos2d::Vec2(1,2));
        combatMgr->SendSoldier(giant,cocos2d::Vec2(20,20));
        combatMgr->SendSoldier(archer,cocos2d::Vec2(2,2));
        combatMgr->SendSoldier(barbarian,cocos2d::Vec2(18,18));
    }
    
    return scene;
}