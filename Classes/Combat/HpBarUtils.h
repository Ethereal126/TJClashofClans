//
// Created by duby0 on 2025/12/25.
//
#ifndef PROGRAMMING_PARADIGM_FINAL_PROJECT_HPBARUTILS_H
#define PROGRAMMING_PARADIGM_FINAL_PROJECT_HPBARUTILS_H

#include "cocos2d.h"
#include "ui/CocosGUI.h"

class HpBarComponents {
private:
    cocos2d::ui::Scale9Sprite* hpBg = nullptr;
    cocos2d::ui::Scale9Sprite* hpBar = nullptr;
public:
    static HpBarComponents createHpBar(
            cocos2d::Node* hostNode,        // 宿主节点（士兵/建筑，血条会添加为它的子节点）
            float hostHeight,               // 宿主高度（用于计算血条Y坐标）
            float hpOffsetRatio = 1.2f,     // 血条偏移比例
            float hpWidth = 20.0f,          // 血条宽度
            const std::string& bgPath = "UI/slider_bg.png",      // 血条背景路径
            const std::string& progressPath = "UI/slider_progress.png" // 血条进度条路径
    );
    void updateHp(int currentHp, int maxHp);
};

#endif //PROGRAMMING_PARADIGM_FINAL_PROJECT_HPBARUTILS_H
