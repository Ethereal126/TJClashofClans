#include "TownHallinCombat/TownHallinCombat.h"
#include <string>
#include <stdexcept>

USING_NS_CC;

/**
 * @brief TownHallinCombat构造函数
 * 初始化战斗中的大本营建筑，继承自Building基类
 * @param name 建筑名称
 * @param level 建筑等级
 * @param base 基础数值（用于计算建筑属性）
 * @param width 建筑宽度
 * @param length 建筑长度
 * @param position 建筑位置坐标
 */
TownHallInCombat::TownHallInCombat(int level, int base = 20, int width = 4, int length = 4, cocos2d::Vec2 position = {0, 0})
    : Building("TownHall", level, 8 * base, base, base / 20, base * 20, width, length, position)
{
    if (level <= 0) {
        throw std::invalid_argument("建筑等级必须大于0");
    }

    if (base <= 0) {
        throw std::invalid_argument("基础数值必须大于0");
    }

    if (width <= 0 || length <= 0) {
        throw std::invalid_argument("建筑宽度和长度必须大于0");
    }

    if(level<=9)
	    texture_ = "buildings/TownHall" + std::to_string(level) + ".png";
    else
        texture_ = "buildings/TownHall9.png";
}

/**
 * @brief TownHallinCombat析构函数
 * 清理战斗中的大本营资源
 */
TownHallInCombat::~TownHallInCombat()
{
    // 当前无需特殊清理操作
}

/**
 * @brief 创建TownHallinCombat实例的工厂方法
 * 提供统一的创建接口，包含完整的错误处理
 * @param name 建筑名称
 * @param level 建筑等级
 * @param base 基础等级
 * @param width 建筑宽度
 * @param length 建筑长度
 * @param position 建筑位置坐标
 * @return 创建的TownHallinCombat实例指针，失败返回nullptr
 */
TownHallInCombat* TownHallInCombat::Create(int level, cocos2d::Vec2 position)
{
    try {
        if (level <= 0) {
            cocos2d::log("错误: 创建TownHallinCombat时等级无效: %d", level);
            return nullptr;
        }

        // 创建实例
        TownHallInCombat* instance = new (std::nothrow) TownHallInCombat(level);
        instance->autorelease();

        if (!instance) {
            cocos2d::log("错误: 创建TownHallinCombat实例失败");
            return nullptr;
        }

        // 初始化精灵纹理
        if (!instance->initWithFile(instance->texture_)) {
            cocos2d::log("错误: 加载TownHallinCombat纹理失败: %s", instance->texture_.c_str());
            delete instance;
            return nullptr;
        }

        // 设置建筑位置和锚点
        instance->setPosition(position);
        instance->setAnchorPoint(cocos2d::Vec2(0.5f, 0.5f));

        cocos2d::log("成功创建TownHallinCombat: (等级%d)", level);
        return instance;

    }
    catch (const std::exception& e) {
        cocos2d::log("创建TownHallinCombat时发生异常: %s", e.what());
        return nullptr;
    }
    catch (...) {
        cocos2d::log("创建TownHallinCombat时发生未知异常");
        return nullptr;
    }
}

void TownHallInCombat::Upgrade() {
	// 调用基类升级
	Building::Upgrade();
	// 更新大本营纹理（假设命名规则为 "TownHallX.png"）
	std::string new_texture;
	if (level_ <= 9)
		new_texture = "buildings/TownHall" + std::to_string(level_) + ".png";
	else
		new_texture = "buildings/TownHall9.png";
	this->setTexture(new_texture);
	cocos2d::log("大本营升级到等级 %d，血量: %d/%d，防御: %d",
		level_, health_, GetMaxHealth(), defense_);
}