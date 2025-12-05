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
    Empty,        // 空位置
    HasBuilding,  // 有建筑
    Obstacle,     // 障碍物
};

// 地图类型枚举
enum class TerrainType {
    Home,         // 主村庄
    Battle,       // 战斗地图
};

// 地图类
class Map : public cocos2d::Node {
public:
    // 创建地图（参数：地图大小（格子数），格子像素大小，地图类型）
    static Map* create(int width, int length, int gridSize, TerrainType terrainType = TerrainType::Home);

    virtual bool init(int width, int length, int gridSize, TerrainType terrainType);

    // 获取地图尺寸（宽×高 格子数）
    std::pair<int, int> getMapSize() const;

    // 获取格子大小（像素）
    int getGridSize() const;

    // 获取地图类型
    TerrainType getTerrainType() const;

    // ========== 格子验证功能 ==========
    // 检查指定格子坐标是否在地图范围内
    bool isValidGrid(int gridX, int gridY) const;

    // 检查指定格子位置是否可放置建筑
    // building参数用于获取建筑占用的格子大小，如果为nullptr则默认检查1x1
    bool isPositionAvailable(int gridX, int gridY, const Building* building = nullptr) const;

	// 检查建筑是否可以放置在指定范围（从gridX开始，到gridX+width；从gridY开始，到gridY+length）
    bool isRangeAvailable(int gridX, int gridY, int width, int length) const;

    // ========== 建筑管理功能 ==========
    // 将建筑放置到指定格子位置  返回是否成功
    bool placeBuilding(Building* building, int gridX, int gridY);

    // 从指定格子位置移除建筑
    bool removeBuilding(int gridX, int gridY);

    // 移动建筑到新位置,玩家在编辑模式下拖动建筑时
    bool moveBuilding(int fromX, int fromY, int toX, int toY);

    // 将格子坐标转换为Map节点的本地坐标（如果要获得世界坐标需调用 convertToWorldSpace）
    cocos2d::Vec2 gridToWorld(int gridX, int gridY) const;

    // 将Map节点的本地坐标转换为格子坐标
    // 如果传入的是世界坐标，外部请先调用 convertToNodeSpace；此处保持接口不做转换
    std::pair<int, int> worldToGrid(const cocos2d::Vec2& worldPos) const;

    // 获取指定位置的建筑
    Building* getBuildingAt(int gridX, int gridY) const;

    // 获取所有建筑列表
    const std::vector<Building*>& getAllBuildings() const;

    // ===== 士兵部署禁区 =====
    // 标记该格为不可部署士兵
    void markNoDeploy(int gridX, int gridY);

    // 取消不可部署标记
    void unmarkNoDeploy(int gridX, int gridY);

    // 该格是否允许部署士兵
    bool isDeployAllowedGrid(int gridX, int gridY) const;

    // 该像素位置是否允许部署士兵
    // 如果传的是世界坐标，建议调用方先 convertToNodeSpace
    bool isDeployAllowedPixel(const cocos2d::Vec2& worldPos) const;

    // ========== 障碍物管理 ==========
    // 放置障碍物
    void placeObstacle(int gridX, int gridY);

    // 移除障碍物
    void removeObstacle(int gridX, int gridY);

    // ========== 地图数据读取与保存 ==========
    // 从配置文件加载地图数据
    bool loadMapData(const std::string& filePath);

    // 保存地图数据到配置文件
    bool saveMapData(const std::string& filePath) const;

protected:
    Map();
    virtual ~Map();

    // 初始化地图格子状态
    void initGrids();

    // 设置格子状态
    void setGridState(int gridX, int gridY, GridState state);

    // 获取格子状态
    GridState getGridState(int gridX, int gridY) const;

    // 更新建筑占用的格子状态
    void updateBuildingGrids(Building* building, int gridX, int gridY, bool occupy);

private:
    int _width;               // 地图宽度（格子数）
    int _length;              // 地图高度（格子数）
    int _gridSize;            // 每个格子的像素大小
    TerrainType _terrainType; // 地形类型（主村庄/战斗地图）

    // 地图格子状态（二维数组：[x][y]）
    std::vector<std::vector<GridState>> _gridStates;

    // 地图上的建筑引用（二维数组：[x][y]），用于快速查找
    std::vector<std::vector<Building*>> _gridBuildings;

	// 地图上的障碍物引用（二维数组：[x][y]），用于显示和管理
    std::vector<std::vector<cocos2d::Sprite*>> _gridObstacles;

	// 士兵部署禁区标记（二维数组：[x][y]），true表示不可部署
    std::vector<std::vector<bool>> _noDeploy;

    // 维护一个所有建筑的列表，方便遍历
    std::vector<Building*> _buildings;
};

#endif // __MAP_H__