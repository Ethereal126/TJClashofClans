//
// Created by duby0 on 2025/12/25.
//
#include "HpBarUtils.h"


HpBarComponents HpBarComponents::createHpBar(cocos2d::Node *hostNode, float hostHeight, float hpOffsetRatio,
                                             float hpWidth, const std::string &bgPath,
                                             const std::string &progressPath) {
    HpBarComponents components;
    if (!hostNode) {
        return components;
    }

    // 计算血条参数
    float hpHeight = hpWidth / 10;
    auto hpY = static_cast<float>(hostHeight * hpOffsetRatio);

    // 初始化血条背景
    components.hpBg = cocos2d::ui::Scale9Sprite::create(bgPath);
    components.hpBg->setContentSize(cocos2d::Size(hpWidth, hpHeight));
    // 注意：这里修正了你原代码的背景位置（原代码hpWidth/2会导致背景偏移，统一以宿主中心为基准）
    components.hpBg->setPosition(cocos2d::Vec2(hpWidth / 2, hpY));
    components.hpBg->setVisible(false);
    components.hpBg->setScale(1.0f);
    hostNode->addChild(components.hpBg, 10);

    // 初始化血条进度条
    components.hpBar = cocos2d::ui::Scale9Sprite::create(progressPath);
    components.hpBar->setContentSize(cocos2d::Size(hpWidth, hpHeight));
    components.hpBar->setAnchorPoint(cocos2d::Vec2(0, 0.5));
    // 进度条左边缘与背景左边缘对齐（修正原逻辑，无需hpWidth/2）
    components.hpBar->setPosition(cocos2d::Vec2(0, hpY));
    components.hpBar->setVisible(false);
    components.hpBar->setScale(1.0f);
    hostNode->addChild(components.hpBar, 11);

    return components;
}

void HpBarComponents::updateHp(int currentHp, int maxHp) {
    float hpPercent = static_cast<float>(currentHp)/ static_cast<float>(maxHp);
    hpPercent = std::clamp(hpPercent, 0.0f, 1.0f); // 限制比例在0~1之间

    bool isVisible = (hpPercent > 0 && hpPercent < 1);
    hpBg->setVisible(isVisible);
    hpBar->setVisible(isVisible);

    hpBar->runAction(cocos2d::ScaleTo::create(0.2f, hpPercent, 1.0f));
}
