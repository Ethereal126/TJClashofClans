#include "ReplayScene.h"
#include "MainScene.h"
#include "MapManager/MapManager.h"
#include "UIManager/UIManager.h"
#include "Combat/Combat.h"
#include "AudioManager/AudioManager.h"

using namespace cocos2d;

ReplayScene* ReplayScene::createScene(int levelId, const std::vector<ReplayStep>& steps) {
    auto scene = ReplayScene::create();
    if (!scene) return nullptr;

    // 1. 加载相同的战斗地图
    auto map = MapManager::create(30, 30, -1, TerrainType::Battle);
    if (map) {
        std::string levelPath = "archived/battle_field" + std::to_string(levelId) + ".json";
        if(!map->loadMapData(levelPath)) CCLOG("ReplayScene: Map loading failure");
        scene->addChild(map, 0);
    }

    // 2. 初始化战斗管理器
    auto combatMgr = CombatManager::InitializeInstance(map);
    scene->addChild(combatMgr);

    // 3. 配置 UI 进入回放模式
    auto ui = UIManager::getInstance();
    if (ui->init(scene)) {
        ui->enterReplayMode(map, steps);
        AudioManager::getInstance()->playMusic(true);
        
        // 4. 启动战斗逻辑
        combatMgr->StartCombat();
        
        // 5. 设置回调：处理回放中的交互
        ui->setUICallback("OnRequestReplay", [levelId]() {
            auto ui = UIManager::getInstance();
            auto currentSteps = ui->getPlaybackSteps(); // 拿当前正在播的这份
            
            CCLOG("Re-requesting Replay. Steps count: %d", (int)currentSteps.size());
            
            CombatManager::DestroyInstance();
            auto replayScene = ReplayScene::createScene(levelId, currentSteps);
            Director::getInstance()->replaceScene(TransitionFade::create(0.5f, replayScene));
        });
        ui->setUICallback("OnRequestExitReplay", []() {
            AudioManager::getInstance()->playMusic(false);
            UIManager::getInstance()->exitReplayMode();
            CombatManager::DestroyInstance(); // 必须销毁回放中的战斗单例
            auto homeScene = MainScene::createScene();
            Director::getInstance()->replaceScene(TransitionFade::create(0.5f, homeScene));
        });
    }    
    return scene;
}

bool ReplayScene::init() {
    if (!Scene::init()) return false;
    return true;
}