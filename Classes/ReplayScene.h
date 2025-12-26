#ifndef __REPLAY_SCENE_H__
#define __REPLAY_SCENE_H__

#include "cocos2d.h"
#include "UIManager/UIManager.h"

class ReplayScene : public cocos2d::Scene {
public:
    // 创建场景，传入地图ID和操作剧本
    static ReplayScene* createScene(int levelId, const std::vector<ReplayStep>& steps);
    
    virtual bool init() override;
    CREATE_FUNC(ReplayScene);
};

#endif