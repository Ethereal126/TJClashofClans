#pragma once
#ifndef __MAP_MANAGER_H__
#define __MAP_MANAGER_H__

#include "cocos2d.h"
#include "../Building/Building.h"
#include "ui/CocosGUI.h"
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

// 地图管理类
class MapManager : public cocos2d::Node {
public:
    // 创建地图（参数：地图大小（格子数），格子像素大小，地图类型）
    static MapManager* create(int width, int length, int gridSize, TerrainType terrainType = TerrainType::Home);

    virtual bool init(int width, int length, int gridSize, TerrainType terrainType);

    // 获取地图尺寸（宽×高 格子数）
    std::pair<int, int> getMapSize() const;

    // 获取格子大小（像素）
    int getGridSize() const;

    // 获取地图类型
    TerrainType getTerrainType() const;

    // ========== 格子验证功能 ==========
    // 检查指定格子坐标是否在地图范围内
    virtual bool isValidGrid(int gridX, int gridY) const;
    virtual bool isValidGrid(const cocos2d::Vec2& grid_pos) const;

    // 检查指定格子位置是否可放置建筑
    // building参数用于获取建筑占用的格子大小，如果为nullptr则默认检查1x1
    bool isPositionAvailable(int gridX, int gridY, const Building* building = nullptr) const;

	// 检查建筑是否可以放置在指定范围（从gridX开始，到gridX+width；从gridY开始，到gridY+length）
    virtual bool isRangeAvailable(int gridX, int gridY, int width, int length) const;
    virtual bool IsGridAvailable(const cocos2d::Vec2& pos) const;

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

    // 用于Mock的方法需要声明为virtual
    virtual cocos2d::Vec2 vecToWorld(cocos2d::Vec2 vecPos) const;
    virtual cocos2d::Vec2 worldToVec(cocos2d::Vec2 worldPos) const;

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

    // ========== 建筑放置模式 ==========
    // 进入放置模式
    // building: 待放置的建筑实例（由外部通过工厂函数创建）
    // cost: 建造费用（确认放置时扣除）
    void enterPlacementMode(Building* building, int cost);

    // 退出放置模式（取消放置）
    void exitPlacementMode();

    // 确认放置建筑
    // 返回是否成功放置
    bool confirmPlacement();

    //napper:辅助测试使用
    void PushBuilding(Building* b){_buildings.push_back(b);}

protected:
    MapManager();
    virtual ~MapManager();

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

    // 批处理地面tile节点（单一纹理的批次渲染）
    cocos2d::SpriteBatchNode* _groundBatch = nullptr;

    // 地图上的建筑引用（二维数组：[x][y]），用于快速查找
    std::vector<std::vector<Building*>> _gridBuildings;

	// 地图上的障碍物引用（二维数组：[x][y]），用于显示和管理
    std::vector<std::vector<cocos2d::Sprite*>> _gridObstacles;

	// 士兵部署禁区标记（二维数组：[x][y]），true表示不可部署
    std::vector<std::vector<bool>> _noDeploy;

    // 维护一个所有建筑的列表，方便遍历
    std::vector<Building*> _buildings;

    // ========== 放置模式相关成员 ==========
    bool _isPlacementMode = false;              // 是否处于放置模式
    Building* _pendingBuilding = nullptr;       // 待放置的建筑实例
    int _pendingBuildingCost = 0;               // 待放置建筑的费用
    cocos2d::DrawNode* _placementHighlight = nullptr;   // 格子高亮绘制节点
    cocos2d::Node* _placementUINode = nullptr;          // 确认/取消按钮容器
    cocos2d::ui::Button* _confirmBtn = nullptr;         // 确认按钮
    cocos2d::ui::Button* _cancelBtn = nullptr;          // 取消按钮

    int _placementGridX = 0;                    // 当前放置格子X
    int _placementGridY = 0;                    // 当前放置格子Y
    bool _canPlaceAtCurrentPos = false;         // 当前位置是否可放置

    cocos2d::EventListenerTouchOneByOne* _placementTouchListener = nullptr;  // 放置模式触摸监听器

    // 放置模式内部方法
    void setupPlacementTouchListener();         // 设置触摸监听
    void removePlacementTouchListener();        // 移除触摸监听
    void updatePlacementPreview(const cocos2d::Vec2& touchPos);  // 更新预览位置
    void drawPlacementHighlight();              // 绘制格子高亮
    void createPlacementUI();                   // 创建确认/取消按钮
    void removePlacementUI();                   // 移除UI
    void updateConfirmButtonState();            // 更新确认按钮状态

};

#endif // __MAP_MANAGER_H__