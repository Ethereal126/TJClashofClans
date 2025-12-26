#pragma once
#ifndef __TOWN_HALL_IN_COMBAT_H__
#define __TOWN_HALL_IN_COMBAT_H__
#include "Building/Building.h"

/**
 * @brief TownHallInCombat类
 * 表示战斗中的大本营建筑，继承自Building类。
 * 该类用于战斗场景中，具有基本的建筑属性和功能。
 */
 //napper: 本类本身并没有实现任何与战斗相关的逻辑，命名包含InCombat与Combat相关实现的规范不符，容易引发歧义，改名为TownHallTemplate
class TownHallTemplate : public Building {
private:
	std::string texture_;
public:
	TownHallTemplate(int level, int base, int width, int length, cocos2d::Vec2 position);

	~TownHallTemplate();

	static TownHallTemplate* Create(int level, cocos2d::Vec2 position);

	virtual void Upgrade() override;
};
#endif // __TOWN_HALL_IN_COMBAT_H__