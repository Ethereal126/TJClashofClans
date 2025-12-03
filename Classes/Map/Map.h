#pragma once
#ifndef __MAP_H__
#define __MAP_H__

#include "cocos2d.h"
#include "../Building/Building.h"
#include <vector>
#include <string>
#include <utility>

// 地图格子状态枚举
enum class GridState {
    Empty,        // 空位置（可放置）
    HasBuilding,  // 有建筑（不可放置）
    Obstacle,     // 障碍物（不可放置，如山、水）
    Resource      // 资源点（特殊位置，矿场/金币场/圣水场）
};

// 地形类型枚举（用于多地图切换）
enum class TerrainType {
    Home,         // 主村庄（主基地）
    Battle,       // 战斗地图（战斗场景）
};

// 地图类：负责管理地图格子、建筑放置和地形信息
class Map : public cocos2d::Node {
public:
    // 创建地图（参数：地图尺寸-宽×高，格子大小，地形类型）
    static Map* create(int width, int height, int gridSize, TerrainType terrainType = TerrainType::Home);

    virtual bool init(int width, int height, int gridSize, TerrainType terrainType);

    // 获取地图尺寸（宽×高，单位：格子数）
    std::pair<int, int> getMapSize() const;

    // 获取格子大小（像素）
    int getGridSize() const;

    // 获取地形类型
    TerrainType getTerrainType() const;

    // ========== 格子验证功能 ==========
    // 检查指定格子坐标是否有效（在地图范围内）
    bool isValidGrid(int gridX, int gridY) const;

    // 检查指定格子位置是否可放置建筑
    // building参数用于获取建筑占用的格子大小，如果为nullptr则默认检查1x1
    bool isPositionAvailable(int gridX, int gridY, const Building* building = nullptr) const;

    // 检查建筑是否可以放置在指定范围（用于多格建筑，如3x3的大本营）
    bool isRangeAvailable(int gridX, int gridY, int width, int height) const;

    // ========== 建筑管理功能 ==========
    // 将建筑放置到指定格子位置
    // 返回值：成功返回true，失败（位置不可用）返回false
    bool placeBuilding(Building* building, int gridX, int gridY);

    // 从指定格子位置移除建筑
    bool removeBuilding(int gridX, int gridY);

    // 移动建筑到新位置（编辑模式使用）
    bool moveBuilding(int fromX, int fromY, int toX, int toY);

    // 根据格子坐标获取世界坐标（格子中心点）
    cocos2d::Vec2 gridToWorld(int gridX, int gridY) const;

    // 根据世界坐标获取格子坐标
    std::pair<int, int> worldToGrid(const cocos2d::Vec2& worldPos) const;

    // 获取指定位置的建筑
    Building* getBuildingAt(int gridX, int gridY) const;

    // 获取所有建筑列表
    const std::vector<Building*>& getAllBuildings() const;

    // ========== 资源点管理（资源建筑） ==========
    // 标记资源点位置（金币场、圣水场）
    void markResourcePoint(int gridX, int gridY);

    // 检查是否为资源点
    bool isResourcePoint(int gridX, int gridY) const;

    // ========== 障碍物管理 ==========
    // 放置障碍物（树木、石头等）
    void placeObstacle(int gridX, int gridY);

    // 移除障碍物（铲除功能）
    void removeObstacle(int gridX, int gridY);

    // ========== 地图数据持久化 ==========
    // 加载地图数据（从配置文件）
    // 调用时机：进入游戏时、切换地图时
    bool loadMapData(const std::string& filePath);

    // 保存地图数据（到配置文件）
    // 调用时机：退出游戏时、自动保存时
    bool saveMapData(const std::string& filePath) const;

protected:
    Map();
    virtual ~Map();

    // 初始化地图格子状态
    void initGrids();

    // 设置格子状态（内部使用）
    void setGridState(int gridX, int gridY, GridState state);

    // 获取格子状态
    GridState getGridState(int gridX, int gridY) const;

    // 更新建筑占用的格子状态（用于多格建筑）
    void updateBuildingGrids(Building* building, int gridX, int gridY, bool occupy);

private:
    int _width;               // 地图宽度（格子数）
    int _height;              // 地图高度（格子数）
    int _gridSize;            // 每个格子的像素大小
    TerrainType _terrainType; // 地形类型（主村庄/战斗地图）

    // 地图格子状态（二维数组：[x][y]）
    std::vector<std::vector<GridState>> _gridStates;

    // 地图上的建筑引用（二维数组：[x][y]），用于快速查找
    std::vector<std::vector<Building*>> _gridBuildings;

    // 维护一个所有建筑的列表，方便遍历
    std::vector<Building*> _buildings;
};

#endif // __MAP_H__