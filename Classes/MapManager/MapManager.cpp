#include "MapManager.h"
#include "TownHall/TownHall.h"
#include "UIManager/UIManager.h"
#include "Combat/Combat.h"
#include <algorithm>
#include <cmath>

MapManager::MapManager():_width(0),_length(0),_gridSize(0),_terrainType(TerrainType::Home){}

MapManager::~MapManager() {

    // 移除输入监听
    if (_inputListener) {
        _eventDispatcher->removeEventListener(_inputListener);
        _inputListener = nullptr;
    }

    for (int x = 0; x < _width; ++x) {
        for (int y = 0; y < _length; ++y) {
            cocos2d::Sprite* spr = (x < (int)_gridObstacles.size() && y < (int)_gridObstacles[x].size())
                ? _gridObstacles[x][y] : nullptr;
            if (spr) spr->removeFromParent();
        }
    }
    _gridObstacles.clear();
    _noDeploy.clear();
    _buildings.clear();
    _gridBuildings.clear();
    _gridStates.clear();
}

MapManager* MapManager::create(int width, int length, int gridSize, TerrainType terrainType) {
    MapManager* ret = new (std::nothrow) MapManager();
    if (ret && ret->init(width, length, gridSize, terrainType)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool MapManager::init(int width, int length, int gridSize, TerrainType terrainType) {
    if (!cocos2d::Node::init()) return false;

    // 校准后的参数
    _gridOffsetX = -2.5f;
    _gridOffsetY = -219.0f;
    _gridScaleX = 2.3f;
    _gridScaleY = 2.33f;

    _width = width;
    _length = length;
    // 如果外部传入 gridSize <= 0，则在 init 中自动计算合适的 gridSize（按窗口宽度占比）
    _gridSize = gridSize;
    _terrainType = terrainType;

    // 设默认占比（可调整）
    const float defaultMapWidthRatio = 0.90f;

    // 如果未传有效 gridSize，则按照窗口计算
    if (_gridSize <= 0) {
        cocos2d::Director* director = cocos2d::Director::getInstance();
        if (director) {
            cocos2d::Size visibleSize = director->getVisibleSize();
            // 期望地图像素宽度
            float desiredMapPixelWidth = visibleSize.width * defaultMapWidthRatio;
            // tileWidth = desiredMapPixelWidth * 2 / (width + length)
            float tileWidth = desiredMapPixelWidth * 2.0f / static_cast<float>(_width + _length);
            _gridSize = std::max(1, static_cast<int>(std::round(tileWidth)));
            _gridSize = _gridSize * 0.95f;
        }
        else {
            // fallback
            _gridSize = 64;
        }
    }

    // 改为中心锚点
    this->setAnchorPoint(cocos2d::Vec2(0.5f, 0.5f));

    // 创建世界容器节点，所有地图内容都放在这里
    _worldNode = cocos2d::Node::create();
    _worldNode->setAnchorPoint(cocos2d::Vec2(0.5f, 0.5f));
    this->addChild(_worldNode);

    // 放置位置：保持你之前偏下的视觉效果（如果你想改为顶部/完全居中可调整）
    cocos2d::Director* director = cocos2d::Director::getInstance();
    if (director) {
        cocos2d::Size visibleSize = director->getVisibleSize();
        cocos2d::Vec2 origin = director->getVisibleOrigin();
        this->setPosition(origin.x + visibleSize.width / 2.0f, origin.y + visibleSize.height / 2.0f);
    }
    // 初始化网格数据
    initGrids();
    // 设置输入监听
    setupInputListener();
    return true;
}

void MapManager::initGrids() {
    // 确保容器大小正确
    if (_gridStates.size() != _width) _gridStates.assign(_width, std::vector<GridState>(_length, GridState::Empty));
    if (_gridBuildings.size() != _width) _gridBuildings.assign(_width, std::vector<Building*>(_length, nullptr));
    if (_gridObstacles.size() != _width) _gridObstacles.assign(_width, std::vector<cocos2d::Sprite*>(_length, nullptr));
    if (_noDeploy.size() != _width) _noDeploy.assign(_width, std::vector<bool>(_length, false));
    _buildings.clear();

    // 创建禁区可视化节点 (仅在战斗地图使用)
    if (_terrainType == TerrainType::Battle) {
        if (!_noDeployVisual) {
            _noDeployVisual = cocos2d::DrawNode::create();
            _worldNode->addChild(_noDeployVisual, -100); // 放在背景之上，建筑之下
        }
        _noDeployVisual->clear();
    }

    // 加载大的地图背景图 (map_bg.png 现在包含平地和森林)
    const std::string bgPath = "tile/map_bg.png";
    if (!_bgSprite) {
        _bgSprite = cocos2d::Sprite::create(bgPath);
        if (_bgSprite) {
            _bgSprite->setAnchorPoint(cocos2d::Vec2(0.5f, 0.5f));
            _bgSprite->setPosition(cocos2d::Vec2::ZERO);
            _worldNode->addChild(_bgSprite, -200);
        }
    }
}


std::pair<int, int> MapManager::getMapSize() const {
    return { _width, _length };
}

int MapManager::getGridSize() const {
    return _gridSize;
}

TerrainType MapManager::getTerrainType() const {
    return _terrainType;
}

bool MapManager::isValidGrid(int gridX, int gridY) const {
    // 基础边界检查
    return (gridX >= 0 && gridX < _width && gridY >= 0 && gridY < _length);
}

bool MapManager::isValidGrid(const cocos2d::Vec2& grid_pos) const {
    // 增加精度补偿的浮点数判定，防止边缘浮点误差导致的索引溢出
    return (grid_pos.x >= -0.001f && grid_pos.x < static_cast<float>(_width) - 0.999f &&
            grid_pos.y >= -0.001f && grid_pos.y < static_cast<float>(_length) - 0.999f);
}

GridState MapManager::getGridState(int gridX, int gridY) const {
    if (!isValidGrid(gridX, gridY)) {
        return GridState::Obstacle;
    }
    return _gridStates[gridX][gridY];
}

void MapManager::setGridState(int gridX, int gridY, GridState state) {
    if (!isValidGrid(gridX, gridY)) return;
    _gridStates[gridX][gridY] = state;
}


bool MapManager::isRangeAvailable(int gridX, int gridY, int width, int length) const {
    // 1. 范围合法性首层保护：严格边界检查
    if (gridX < 0 || gridY < 0 || (gridX + width) > _width || (gridY + length) > _length) {
        return false;
    }
    for (int x = gridX; x < gridX + width; ++x) {
        for (int y = gridY; y < gridY + length; ++y) {
            const GridState st = _gridStates[x][y];
            if (st == GridState::HasBuilding || st == GridState::Obstacle) {
                return false;
            }
        }
    }
    return true;
}

//对于士兵类型的特别重载
bool MapManager::IsGridAvailable(const cocos2d::Vec2& pos) const{
    return isRangeAvailable(std::floor(pos.x),std::floor(pos.y),1,1);
}

bool MapManager::isPositionAvailable(int gridX, int gridY, const Building* building ) const {
	const int bw = building ? building->GetWidth() : 1;
	const int bh = building ? building->GetLength() : 1;
    return isRangeAvailable(gridX, gridY, bw, bh);

}

void MapManager::updateBuildingGrids(Building* building, int gridX, int gridY, bool occupy) {
    
	const int bw = building ? building->GetWidth() : 1;
	const int bh = building ? building->GetLength() : 1;

    // 1. 更新建筑占位和逻辑状态
    for (int x = gridX; x < gridX + bw; ++x) {
        for (int y = gridY; y < gridY + bh; ++y) {
            if (!isValidGrid(x, y)) continue;
            if (occupy) {
                _gridStates[x][y] = GridState::HasBuilding;
                _gridBuildings[x][y] = building;
            }
            else {
                _gridStates[x][y] = GridState::Empty;
                _gridBuildings[x][y] = nullptr;
            }
        }
    }

    // 2. 更新士兵部署禁区 (仅在战斗地图生效)
    if (_terrainType == TerrainType::Battle) {
        // 范围：建筑占据的格子 + 周围一圈 (缓冲区)
        for (int x = gridX - 1; x <= gridX + bw; ++x) {
            for (int y = gridY - 1; y <= gridY + bh; ++y) {
                if (!isValidGrid(x, y)) continue;
                
                if (occupy) {
                    _noDeploy[x][y] = true;
                } else {
                    // 暂时简单处理：移除建筑时取消禁区
                    // 注意：如果多个建筑禁区重叠，这里可能会误删邻近建筑的禁区
                    // 但战斗地图建筑通常是静态加载的，不会在运行时移动
                    _noDeploy[x][y] = false;
                }
            }
        }
    }
}

//由于combat相关实现中Building*具有const标签，对这一函数做重新实现
void MapManager::updateEmptyBuildingGrids(const Building* building){
    const int gridX = floor(building->GetPosition().x);
    const int gridY = floor(building->GetPosition().y);

    const int bw = building ? building->GetWidth() : 1;
    const int bh = building ? building->GetLength() : 1;

    for (int x = gridX; x < gridX + bw; ++x) {
        for (int y = gridY; y < gridY + bh; ++y) {
            if (!isValidGrid(x, y)) continue;
            _gridStates[x][y] = GridState::Empty;
            _gridBuildings[x][y] = nullptr;
        }
    }
}


void MapManager::addToWorld(cocos2d::Node* node, int zOrder) {
    if (_worldNode && node) {
        _worldNode->addChild(node, zOrder);
    }
    else{
        CCLOG("node addToWorld() failed");
    }
}


bool MapManager::placeBuilding(Building* building, int gridX, int gridY) {
    if (!building) return false;
    if (!isPositionAvailable(gridX, gridY, building)) return false;

    setupNodeOnMap(building, gridX, gridY, building->GetWidth(), building->GetLength());
    _worldNode->addChild(building);

    updateBuildingGrids(building, gridX, gridY, true);
    _buildings.push_back(building);
    return true;
}


void MapManager::setupNodeOnMap(cocos2d::Node* node, int gridX, int gridY, int width, int length) {
    if (!node) return;

    // 1. 设置位置（相对 worldNode）和锚点
    node->setPosition(vecToWorld(cocos2d::Vec2{gridX+0.5f*width, gridY+0.5f*length}));
    node->setAnchorPoint(cocos2d::Vec2(0.5f, 0.4f));

    // 2. 设置缩放适配格子
    auto sprite = dynamic_cast<cocos2d::Sprite*>(node);
    if (sprite && sprite->getContentSize().width > 0) {
        // 核心逻辑：如果传入了有效的占地尺寸 (width > 0)，则执行网格适配缩放
        // 这样可以同时兼顾 Building 和普通障碍物
        if (width > 0 && length > 0) {
            float gridHalfWidth = _gridSize * 0.5f * _gridScaleX;
            float visualWidthOnMap = (width + length) * gridHalfWidth;
            
            // 核心调整：确保主场景 Building 和战斗场景 BuildingInCombat 使用相同的缩放系数
            // 我们通过检查节点名称或尝试转换来识别它们
            bool isBuilding = dynamic_cast<Building*>(node) != nullptr || 
                              (node->getName() == "BuildingInCombat") ||
                              (dynamic_cast<BuildingInCombat*>(node) != nullptr);

            float fitFactor = isBuilding ? 0.8f : 1.0f;
            float targetWidth = visualWidthOnMap * fitFactor;
            
            float scale = targetWidth / sprite->getContentSize().width;
            node->setScale(scale);
        }
        // 如果没有传入尺寸（比如士兵调用时传 0），则不在此处处理缩放，保持其原始大小
    }

    // 3. 遮挡关系处理 (Y-Sorting)
    updateYOrder(node);
}

void MapManager::updateYOrder(cocos2d::Node* node) {
    if (!node) return;
    // Y 坐标越小（越靠下），ZOrder 越高
    // 我们给一个基础偏移 1000 保证在背景之上
    node->setLocalZOrder(1000 - (int)node->getPositionY());
}

bool MapManager::removeBuilding(int gridX, int gridY) {
    if (!isValidGrid(gridX, gridY)) return false;
    Building* b = _gridBuildings[gridX][gridY];
    if (!b) return false;

    updateBuildingGrids(b, gridX, gridY, false);
    b->removeFromParent();

    auto it = std::find(_buildings.begin(), _buildings.end(), b);
    if (it != _buildings.end()) _buildings.erase(it);

    // 自动保存
    if (!_currentSavePath.empty() && _terrainType == TerrainType::Home) {
        saveMapData(_currentSavePath);
    }
    return true;
}

bool MapManager::moveBuilding(int fromX, int fromY, int toX, int toY) {
    if (!isValidGrid(fromX, fromY) || !isValidGrid(toX, toY)) return false;
    Building* b = _gridBuildings[fromX][fromY];
    if (!b) return false;

	const int bw = b->GetWidth();
	const int bh = b->GetLength();
    if (!isRangeAvailable(toX, toY, bw, bh)) return false;

    updateBuildingGrids(b, fromX, fromY, false);
    updateBuildingGrids(b, toX, toY, true);

    setupNodeOnMap(b, toX, toY, bw, bh);

    // 自动保存
    if (!_currentSavePath.empty() && _terrainType == TerrainType::Home) {
        saveMapData(_currentSavePath);
    }
    return true;
}


cocos2d::Vec2 MapManager::gridToWorld(int gridX, int gridY) const {
    auto grid_pos = cocos2d::Vec2(static_cast<float>(gridX),static_cast<float>(gridY));

    return vecToWorld(grid_pos);
}

std::pair<int, int> MapManager::worldToGrid(const cocos2d::Vec2& worldPos) const {
    // 关键点：将世界坐标转换为 worldNode 的本地坐标，自动处理拖拽和缩放
    cocos2d::Vec2 localPos = _worldNode->convertToNodeSpace(worldPos);
    auto vec_pos = worldToVec(localPos);
    // 精度补偿并强制钳位保护：确保返回的索引永远在合法网格范围内
    int ix = static_cast<int>(std::floor(vec_pos.x + 0.001f));
    int iy = static_cast<int>(std::floor(vec_pos.y + 0.001f));

    ix = std::max(0, std::min(ix, _width - 1));
    iy = std::max(0, std::min(iy, _length - 1));
    return { ix, iy };
}

cocos2d::Vec2 MapManager::vecToWorld(cocos2d::Vec2 vec) const{
    const float halfW = static_cast<float>(_gridSize) * 0.5f * _gridScaleX;
    const float quarterH = static_cast<float>(_gridSize) * 0.25f * _gridScaleY;

    const float sx = (vec.x - vec.y) * halfW + _gridOffsetX;
    const float sy = (vec.x + vec.y) * quarterH + _gridOffsetY;

    return cocos2d::Vec2(sx, sy);
}

cocos2d::Vec2 MapManager::worldToVec(cocos2d::Vec2 worldPos) const{
    const float halfW = static_cast<float>(_gridSize) * 0.5f * _gridScaleX;
    const float quarterH = static_cast<float>(_gridSize) * 0.25f * _gridScaleY;

    float dx = worldPos.x - _gridOffsetX;
    float dy = worldPos.y - _gridOffsetY;

    const float gx = (dy / quarterH + dx / halfW) * 0.5f;
    const float gy = (dy / quarterH - dx / halfW) * 0.5f;
    return cocos2d::Vec2(gx,gy);
}

Building* MapManager::getBuildingAt(int gridX, int gridY) const {
    if (!isValidGrid(gridX, gridY)) return nullptr;
    return _gridBuildings[gridX][gridY];
}

const std::vector<Building*>& MapManager::getAllBuildings() const {
    return _buildings;
}

void MapManager::placeObstacle(int gridX, int gridY) {
    if (!isValidGrid(gridX, gridY)) return;
    const GridState st = _gridStates[gridX][gridY];
    if (st == GridState::Empty) {
        _gridStates[gridX][gridY] = GridState::Obstacle;
        if (_gridObstacles[gridX][gridY]) {
            _gridObstacles[gridX][gridY]->removeFromParent();
            _gridObstacles[gridX][gridY] = nullptr;
        }
        const std::string obstaclePath = "obstacles/rock.png";
        cocos2d::Sprite* sprite = nullptr;
        if (cocos2d::FileUtils::getInstance()->isFileExist(obstaclePath)) {
            sprite = cocos2d::Sprite::create(obstaclePath);
            setupNodeOnMap(sprite, gridX, gridY, 1, 1);
            _gridObstacles[gridX][gridY] = sprite;
            _worldNode->addChild(sprite, 1);
        }		
    }
}

void MapManager::removeObstacle(int gridX, int gridY) {
    if (!isValidGrid(gridX, gridY)) return;
    if (_gridStates[gridX][gridY] == GridState::Obstacle) {
        _gridStates[gridX][gridY] = GridState::Empty;
        cocos2d::Sprite* sprite = _gridObstacles[gridX][gridY];
        if (sprite) {
            sprite->removeFromParent();
            _gridObstacles[gridX][gridY] = nullptr;
        }
        // 自动保存
        if (!_currentSavePath.empty() && _terrainType == TerrainType::Home) {
            saveMapData(_currentSavePath);
        }
    }
}

void MapManager::markNoDeploy(int gridX, int gridY) {
    if (!isValidGrid(gridX, gridY)) return;
    _noDeploy[gridX][gridY] = true;
}

void MapManager::unmarkNoDeploy(int gridX, int gridY) {
    if (!isValidGrid(gridX, gridY)) return;
    _noDeploy[gridX][gridY] = false;
}

bool MapManager::isDeployAllowedGrid(int gridX, int gridY) const {
    if (!isValidGrid(gridX, gridY)) return false;
    if (_noDeploy[gridX][gridY]) return false;
    const GridState st = _gridStates[gridX][gridY];
    if (st == GridState::HasBuilding || st == GridState::Obstacle) return false;
    return true;
}


bool MapManager::isDeployAllowedPixel(const cocos2d::Vec2& worldPos) const {   
    auto grid = worldToGrid(worldPos);
    return isDeployAllowedGrid(grid.first, grid.second);
}

// ==================== 建筑放置模式 ====================

void MapManager::enterPlacementMode(Building* building, int cost) {
    if (!building) return;

    // 如果已在放置模式，先退出
    if (_isPlacementMode) {
        exitPlacementMode();
    }

    _isPlacementMode = true;
    _pendingBuilding = building;
    _pendingBuildingCost = cost;

    // 设置建筑为半透明预览状态
    _pendingBuilding->setOpacity(180);

    // 初始位置放在地图中央
    int buildingWidth = _pendingBuilding->GetWidth();
    int buildingHeight = _pendingBuilding->GetLength();
    _placementGridX = _width / 2 - buildingWidth / 2;
    _placementGridY = _length / 2 - buildingHeight / 2;

    // 使用统一接口设置初始位置、缩放和遮挡
    setupNodeOnMap(_pendingBuilding, _placementGridX, _placementGridY, buildingWidth, buildingHeight);
    _worldNode->addChild(_pendingBuilding, 1000); // 预览时给予较高的初始 ZOrder

    // 创建格子高亮绘制节点
    _placementHighlight = cocos2d::DrawNode::create();
    _worldNode->addChild(_placementHighlight, 999);

    // 检查是否可放置
    _canPlaceAtCurrentPos = isRangeAvailable(_placementGridX, _placementGridY,
        buildingWidth, buildingHeight);
    drawPlacementHighlight();

    // 创建确认/取消UI
    createPlacementUI();

    // 设置触摸监听
    setupPlacementTouchListener();

    CCLOG("Entered placement mode for building (size: %dx%d, cost: %d)",
        buildingWidth, buildingHeight, cost);
}

void MapManager::exitPlacementMode() {
    if (!_isPlacementMode) return;

    _isPlacementMode = false;

    // 移除待放置的建筑（如果取消放置）
    if (_pendingBuilding) {
        _pendingBuilding->removeFromParent();
        _pendingBuilding = nullptr;
    }

    // 移除高亮
    if (_placementHighlight) {
        _placementHighlight->removeFromParent();
        _placementHighlight = nullptr;
    }

    // 移除UI
    removePlacementUI();

    // 移除触摸监听
    removePlacementTouchListener();

    _pendingBuildingCost = 0;

    CCLOG("Exited placement mode");
}

bool MapManager::confirmPlacement() {
    if (!_isPlacementMode || !_canPlaceAtCurrentPos || !_pendingBuilding) {
        return false;
    }

    // 恢复建筑不透明
    _pendingBuilding->setOpacity(255);

    // 确保最终位置和缩放正确
    setupNodeOnMap(_pendingBuilding, _placementGridX, _placementGridY, _pendingBuilding->GetWidth(), _pendingBuilding->GetLength());
    _pendingBuilding->SetPosition({_placementGridX, _placementGridY});

    // 注册到格子系统
    updateBuildingGrids(_pendingBuilding, _placementGridX, _placementGridY, true);
    _buildings.push_back(_pendingBuilding);

    TownHall* townHall = TownHall::GetInstance();
    if (townHall->SpendGold(_pendingBuildingCost)) {
        // 更新资源栏显示
        UIManager::getInstance()->updateResourceDisplay(ResourceType::Gold, townHall->GetGold());
    }    
    CCLOG("Building placed! Cost: %d gold", _pendingBuildingCost);

    // 如果设置了保存路径且是主场景，则自动保存
    if (!_currentSavePath.empty() && _terrainType == TerrainType::Home) {
        saveMapData(_currentSavePath);
        CCLOG("Auto-saved map to: %s", _currentSavePath.c_str());
    }

    // 清理放置模式状态（不删除建筑，因为已放置成功）
    Building* placedBuilding = _pendingBuilding;
    _pendingBuilding = nullptr;  // 防止 exitPlacementMode 删除它
    _isPlacementMode = false;
    _pendingBuildingCost = 0;

    // 移除高亮和UI
    if (_placementHighlight) {
        _placementHighlight->removeFromParent();
        _placementHighlight = nullptr;
    }
    removePlacementUI();
    removePlacementTouchListener();

    return true;
}

void MapManager::updatePlacementPreview(const cocos2d::Vec2& touchPos) {
    if (!_isPlacementMode || !_pendingBuilding) return;

    int buildingWidth = _pendingBuilding->GetWidth();
    int buildingHeight = _pendingBuilding->GetLength();

    // 直接使用 worldToGrid，它内部会处理 worldNode 的转换
    auto gridPos = worldToGrid(touchPos);
    int newGridX = gridPos.first;
    int newGridY = gridPos.second;

    // 限制在地图范围内
    newGridX = std::max(0, std::min(newGridX, _width - buildingWidth));
    newGridY = std::max(0, std::min(newGridY, _length - buildingHeight));

    // 如果位置没变，不需要更新
    if (newGridX == _placementGridX && newGridY == _placementGridY) {
        return;
    }

    _placementGridX = newGridX;
    _placementGridY = newGridY;

    // 使用统一接口更新建筑位置、缩放和 Y-Sorting
    setupNodeOnMap(_pendingBuilding, _placementGridX, _placementGridY, buildingWidth, buildingHeight);
    // 预览过程中可以保持较高的 ZOrder 避免被已有建筑完全遮挡，或者跟随 Y-Sorting
    // 这里 setupNodeOnMap 内部会调用 updateYOrder

    // 检查是否可放置
    _canPlaceAtCurrentPos = isRangeAvailable(_placementGridX, _placementGridY,
        buildingWidth, buildingHeight);

    // 更新高亮颜色
    drawPlacementHighlight();

    // 更新确认按钮状态
    updateConfirmButtonState();
}

void MapManager::drawPlacementHighlight() {
    if (!_placementHighlight || !_pendingBuilding) return;

    _placementHighlight->clear();

    int buildingWidth = _pendingBuilding->GetWidth();
    int buildingHeight = _pendingBuilding->GetLength();

    // 根据是否可放置选择颜色
    cocos2d::Color4F fillColor = _canPlaceAtCurrentPos ?
        cocos2d::Color4F(0.0f, 1.0f, 0.0f, 0.35f) :  // 绿色半透明
        cocos2d::Color4F(1.0f, 0.0f, 0.0f, 0.35f);   // 红色半透明

    cocos2d::Color4F borderColor = _canPlaceAtCurrentPos ?
        cocos2d::Color4F(0.0f, 1.0f, 0.0f, 0.8f) :  // 绿色边框
        cocos2d::Color4F(1.0f, 0.0f, 0.0f, 0.8f);   // 红色边框

    // 计算建筑占地的四个顶点坐标
    cocos2d::Vec2 origin = gridToWorld(_placementGridX, _placementGridY);
    cocos2d::Vec2 rightEdge = gridToWorld(_placementGridX + buildingWidth, _placementGridY);
    cocos2d::Vec2 topEdge = gridToWorld(_placementGridX + buildingWidth, _placementGridY + buildingHeight);
    cocos2d::Vec2 leftEdge = gridToWorld(_placementGridX, _placementGridY + buildingHeight);
    cocos2d::Vec2 verts[] = { origin, rightEdge, topEdge, leftEdge };
    
    // 绘制一个覆盖整个建筑区域的闭合多边形
    _placementHighlight->drawPolygon(verts, 4, fillColor, 2.0f, borderColor);
}

void MapManager::updateConfirmButtonState() {
    if (!_confirmBtn || !_cancelBtn || !_pendingBuilding) return;

    // 更新确认按钮状态
    if (_canPlaceAtCurrentPos) {
        _confirmBtn->setEnabled(true);
        _confirmBtn->setOpacity(255);
        _confirmBtn->setTitleColor(cocos2d::Color3B::WHITE);
    }
    else {
        _confirmBtn->setEnabled(false);
        _confirmBtn->setOpacity(128);
        _confirmBtn->setTitleColor(cocos2d::Color3B::GRAY);
    }

    // 更新按钮位置跟随建筑（使用本地坐标，因为 UI 现在是 _worldNode 的子节点）
    cocos2d::Vec2 buildingPos = _pendingBuilding->getPosition();
    
    // 按钮放在建筑上方一点
    float buttonY = buildingPos.y + 70.0f; 
    float btnSpacing = 40.0f;

    _confirmBtn->setPosition(cocos2d::Vec2(buildingPos.x - btnSpacing, buttonY));
    _cancelBtn->setPosition(cocos2d::Vec2(buildingPos.x + btnSpacing, buttonY));
}

void MapManager::createPlacementUI() {
    removePlacementUI();
    if (!_isPlacementMode || !_pendingBuilding) return;

    _placementUINode = cocos2d::Node::create();
    _worldNode->addChild(_placementUINode, 1200);

    // 确认按钮
    _confirmBtn = cocos2d::ui::Button::create("UI/btn_confirm.png", "UI/btn_confirm_pressed.png");
    if (!_confirmBtn || !_confirmBtn->getVirtualRenderer()) {
        _confirmBtn = cocos2d::ui::Button::create();
        _confirmBtn->setScale9Enabled(true);
        _confirmBtn->setContentSize(cocos2d::Size(80, 36));
        _confirmBtn->setTitleText("Confirm");
        _confirmBtn->setTitleFontSize(14);
    }
    else{
        _confirmBtn->setScale(0.2f);
    }
    _confirmBtn->addClickEventListener([this](cocos2d::Ref*) {
        confirmPlacement();
        });
    _placementUINode->addChild(_confirmBtn);

    // 取消按钮
    _cancelBtn = cocos2d::ui::Button::create("UI/btn_cancel.png", "UI/btn_cancel_pressed.png");
    if (!_cancelBtn || !_cancelBtn->getVirtualRenderer()) {
        _cancelBtn = cocos2d::ui::Button::create();
        _cancelBtn->setScale9Enabled(true);
        _cancelBtn->setContentSize(cocos2d::Size(80, 36));
        _cancelBtn->setTitleText("Cancel");
        _cancelBtn->setTitleFontSize(14);
    }
    else{
        _cancelBtn->setScale(0.2f);
    }
    _cancelBtn->addClickEventListener([this](cocos2d::Ref*) {
        exitPlacementMode();
        });
    _placementUINode->addChild(_cancelBtn);

    updateConfirmButtonState();
}

void MapManager::removePlacementUI() {
    if (_placementUINode) {
        _placementUINode->removeFromParent();
        _placementUINode = nullptr;
    }
    _confirmBtn = nullptr;
    _cancelBtn = nullptr;
}

void MapManager::setupPlacementTouchListener() {
    if (_placementTouchListener || !_isPlacementMode) return;

    _placementTouchListener = cocos2d::EventListenerTouchOneByOne::create();
    _placementTouchListener->setSwallowTouches(true);

    _placementTouchListener->onTouchBegan = [this](cocos2d::Touch* touch, cocos2d::Event*) -> bool {
        if (!_isPlacementMode || !_pendingBuilding) return false;
        updatePlacementPreview(touch->getLocation());
        return true;
        };
    _placementTouchListener->onTouchMoved = [this](cocos2d::Touch* touch, cocos2d::Event*) {
        if (!_isPlacementMode || !_pendingBuilding) return;
        updatePlacementPreview(touch->getLocation());
        };
    _placementTouchListener->onTouchEnded = [this](cocos2d::Touch* touch, cocos2d::Event*) {
        if (!_isPlacementMode || !_pendingBuilding) return;
        updatePlacementPreview(touch->getLocation());
        };

    cocos2d::Director::getInstance()->getEventDispatcher()
        ->addEventListenerWithSceneGraphPriority(_placementTouchListener, this);
}

void MapManager::setupInputListener() {
    // 设置输入监听器
    if (_inputListener || !_worldNode) return;

    // 1. 触摸监听 (用于拖拽地图)
    _inputListener = cocos2d::EventListenerTouchOneByOne::create();
    _inputListener->setSwallowTouches(false); // 不吞噬触摸，允许穿透到子节点（如建筑）

    _inputListener->onTouchBegan = [this](cocos2d::Touch* touch, cocos2d::Event*) -> bool {
        if (_isPlacementMode) return false;

        _lastTouchPos = touch->getLocation();
        _isMapDragging = false;
        return true;
    };

    _inputListener->onTouchMoved = [this](cocos2d::Touch* touch, cocos2d::Event*) {
        if (_isPlacementMode) return;
        cocos2d::Vec2 currentPos = touch->getLocation();
        float dist = currentPos.distance(_lastTouchPos);

        if (!_isMapDragging && dist > _dragThreshold) { 
            _isMapDragging = true;
        }

        if (_isMapDragging) {
            cocos2d::Vec2 delta = currentPos - _lastTouchPos;
            cocos2d::Vec2 newPos = _worldNode->getPosition() + delta;

            // 防止黑边：限制 _worldNode 的移动范围
            if (_bgSprite) {
                cocos2d::Size visibleSize = cocos2d::Director::getInstance()->getVisibleSize();
                cocos2d::Size bgSize = _bgSprite->getContentSize() * _worldNode->getScale();
                
                // 允许的移动边界 (相对于中心点 0,0)
                float maxX = (bgSize.width - visibleSize.width) / 2.0f;
                float maxY = (bgSize.height - visibleSize.height) / 2.0f;

                // 如果背景图比屏幕小，则固定在中心
                if (maxX < 0) maxX = 0;
                if (maxY < 0) maxY = 0;

                newPos.x = std::max(-maxX, std::min(maxX, newPos.x));
                newPos.y = std::max(-maxY, std::min(maxY, newPos.y));
            }

            _worldNode->setPosition(newPos);
            _lastTouchPos = currentPos;
        }
    };

    _inputListener->onTouchEnded = [this](cocos2d::Touch* touch, cocos2d::Event*) {
        if (_isPlacementMode) return;
        if (!_isMapDragging) {
            // 如果是战斗模式，禁止点击建筑弹出面板
            if (UIManager::getInstance()->isInBattleMode()) {
                return;
            }
            auto gridIdx = worldToGrid(touch->getLocation());

            if (isValidGrid(gridIdx.first, gridIdx.second)) {
                Building* b = getBuildingAt(gridIdx.first, gridIdx.second);
                if (b) {
                    BuildingCategory cat = BuildingCategory::Normal;
                    std::string name = b->GetName();
                    if (name.find("Town Hall") != std::string::npos) cat = BuildingCategory::Normal;
                    else if (name.find("Gold") != std::string::npos || name.find("Elixir") != std::string::npos) cat = BuildingCategory::Resource;
                    else if (name.find("Barrack") != std::string::npos || name.find("Camp") != std::string::npos) cat = BuildingCategory::Military;
                    else if (name.find("Cannon") != std::string::npos || name.find("Tower") != std::string::npos || name.find("Wall") != std::string::npos) cat = BuildingCategory::Defense;

                    // 关键改动：将 _worldNode 作为父节点传入，实现 UI 随地图移动
                    // 传入建筑的本地坐标 b->getPosition()
                    UIManager::getInstance()->showBuildingOptions(b->getPosition(), cat, b, _worldNode);
                }
                else {
                    UIManager::getInstance()->hidePanel(UIPanelType::BuildingOptions);
                    UIManager::getInstance()->hidePanel(UIPanelType::BuildingInfo);
                    UIManager::getInstance()->hidePanel(UIPanelType::BuildingUpgrade);
                    UIManager::getInstance()->hidePanel(UIPanelType::ArmyTraining);
                }
            }
        }
        _isMapDragging = false;        
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(_inputListener, this);

    // 2. 鼠标滚轮监听 (用于缩放地图 - PC版特有)
    auto mouseListener = cocos2d::EventListenerMouse::create();
    mouseListener->onMouseScroll = [this](cocos2d::EventMouse* event) {
        float scrollY = event->getScrollY();
        float zoomSpeed = 0.05f;
        float newScale = _currentScale + (scrollY > 0 ? zoomSpeed : -zoomSpeed);
        
        // 限制缩放范围
        newScale = std::max(_minScale, std::min(_maxScale, newScale));
        
        if (newScale != _currentScale) {
            _currentScale = newScale;
            _worldNode->setScale(_currentScale);
            CCLOG("Map Zoom Scale: %.2f", _currentScale);
        }
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(mouseListener, this);
}

void MapManager::removePlacementTouchListener() {
    if (_placementTouchListener) {
        cocos2d::Director::getInstance()->getEventDispatcher()
            ->removeEventListener(_placementTouchListener);
        _placementTouchListener = nullptr;
    }
}


void MapManager::clearMap() {
    // 移除所有建筑
    for (int x = 0; x < _width; ++x) {
        for (int y = 0; y < _length; ++y) {
            Building* b = _gridBuildings[x][y];
            if (b) {
                // 找到建筑的左下角起始点（因为一个建筑占用多个格子，只需处理一次）
                // 这里简单处理：直接通过 gridBuildings 遍历并移除
                b->removeFromParent();
                _gridBuildings[x][y] = nullptr;
            }
            _gridStates[x][y] = GridState::Empty;
        }
    }
    _buildings.clear();

    // 移除所有障碍物
    for (int x = 0; x < _width; ++x) {
        for (int y = 0; y < _length; ++y) {
            if (_gridObstacles[x][y]) {
                _gridObstacles[x][y]->removeFromParent();
                _gridObstacles[x][y] = nullptr;
            }
        }
    }
}

bool MapManager::loadFromJSONObject(const rapidjson::Value& mapData) {
    if (!mapData.IsObject()) return false;

    clearMap();

    // 1. Load Buildings
    if (mapData.HasMember("buildings") && mapData["buildings"].IsArray()) {
        const auto& buildingsJson = mapData["buildings"];
        auto templates = TownHall::GetAllBuildingTemplates();

        for (rapidjson::SizeType i = 0; i < buildingsJson.Size(); i++) {
            const auto& bJson = buildingsJson[i];
            if (!bJson.HasMember("type") || !bJson.HasMember("x") || !bJson.HasMember("y")) continue;

            std::string type = bJson["type"].GetString();
            int gx = bJson["x"].GetInt();
            int gy = bJson["y"].GetInt();
            int level = bJson.HasMember("level") ? bJson["level"].GetInt() : 1;
            CCLOG("load building '%s' : (%d,%d),level%d",type.c_str(),gx,gy,level);

            // Find matching template
            for (const auto& t : templates) {
                if (t.name_ == type) {
                    CCLOG("template founded when loading : %s",type.c_str());
                    Building* building = t.createFunc();
                    if (building) {
                        building->SetMapPosition({static_cast<float>(gx),static_cast<float>(gy)});
                        // Set level (assuming UpgradeToLevel exists or calling Upgrade multiple times)
                        for(int l = 1; l < level; ++l) building->Upgrade();
                        
                        // 关键改动：如果不是主场景（战斗模式），我们只记录建筑数据，不进行实际的渲染和网格占用
                        // 战斗中的建筑渲染由 CombatManager 负责创建对应的 BuildingInCombat 节点
                        if (_terrainType == TerrainType::Home) {
                            placeBuilding(building, gx, gy);
                        } else {
                            // 战斗模式下，只需将建筑添加到列表供 CombatManager 读取
                            _buildings.push_back(building);
                            // 依然需要更新网格数据以便战斗逻辑查询
                            updateBuildingGrids(building, gx, gy, true);
                            // 建筑本身不需要显示，因为 Combat 会创建新的 Sprite
                            building->setVisible(false);
                            this->addChild(building);
                        }
                    }
                    break;
                }
            }
        }
    }

    // 2. Load Obstacles
    if (mapData.HasMember("obstacles") && mapData["obstacles"].IsArray()) {
        const auto& obstaclesJson = mapData["obstacles"];
        for (rapidjson::SizeType i = 0; i < obstaclesJson.Size(); i++) {
            const auto& oJson = obstaclesJson[i];
            if (oJson.HasMember("x") && oJson.HasMember("y")) {
                placeObstacle(oJson["x"].GetInt(), oJson["y"].GetInt());
            }
        }
    }

    // 地图加载完成后，更新一次禁区可视化
    if (_terrainType == TerrainType::Battle) {
        updateNoDeployVisual();
    }

    return true;
}

void MapManager::updateNoDeployVisual() {
    if (!_noDeployVisual || _terrainType != TerrainType::Battle) return;

    _noDeployVisual->clear();

    float halfW = _gridSize * 0.5f * _gridScaleX;
    float quarterH = _gridSize * 0.25f * _gridScaleY;

    // 遍历所有格子，绘制禁区
    for (int x = 0; x < _width; ++x) {
        for (int y = 0; y < _length; ++y) {
            if (_noDeploy[x][y]) {
                cocos2d::Vec2 center = gridToWorld(x, y);
                
                // 绘制菱形的四个顶点
                cocos2d::Vec2 vertices[4] = {
                    cocos2d::Vec2(center.x, center.y + quarterH),   // 上
                    cocos2d::Vec2(center.x + halfW, center.y),      // 右
                    cocos2d::Vec2(center.x, center.y - quarterH),   // 下
                    cocos2d::Vec2(center.x - halfW, center.y)       // 左
                };

                // 使用非常透明的红色 (Alpha = 60/255)
                _noDeployVisual->drawSolidPoly(vertices, 4, cocos2d::Color4F(1.0f, 0.0f, 0.0f, 0.2f));
            }
        }
    }
}

bool MapManager::loadMapData(const std::string& filePath) {
    _currentSavePath = filePath; // 记录保存路径以便后续自动保存
    
    // 1. 构造可写目录下的绝对路径
    std::string writablePath = cocos2d::FileUtils::getInstance()->getWritablePath();
    std::string fullPath = writablePath + filePath;
    // 确保路径末尾有斜杠
    if (!writablePath.empty() && writablePath.back() != '/' && writablePath.back() != '\\') {
        writablePath += "/";
    }
    
    // 2. 检查可写目录是否存在该存档
    bool existsInWritable = cocos2d::FileUtils::getInstance()->isFileExist(fullPath);
    
    // 3. 如果可写目录没有，才去资源目录找初始地图
    if (!existsInWritable) {
        fullPath = cocos2d::FileUtils::getInstance()->fullPathForFilename(filePath);
    }
    
    CCLOG("MapManager: Loading map from absolute path: %s (%s)", fullPath.c_str(), existsInWritable ? "Player Progress" : "Initial Template");

    if (!cocos2d::FileUtils::getInstance()->isFileExist(fullPath)){
        CCLOG("MapManager: Load file not found: %s", fullPath.c_str());
        return false;
    }

    std::string content = cocos2d::FileUtils::getInstance()->getStringFromFile(fullPath);
    rapidjson::Document doc;
    doc.Parse(content.c_str());

    if (doc.HasParseError()){
        CCLOG("parse error");
        return false;
    }

    // If it's the full save file, the map layout is under "map_layout"
    if (doc.HasMember("level_info") && doc["level_info"].IsObject()) {
        const auto& info = doc["level_info"];
        if (info.HasMember("reward_gold")) _baseGoldReward = info["reward_gold"].GetInt();
        if (info.HasMember("reward_elixir")) _baseElixirReward = info["reward_elixir"].GetInt();
    }
    if (doc.HasMember("map_layout")) {
        return loadFromJSONObject(doc["map_layout"]);
    }
    
    return loadFromJSONObject(doc);
}

bool MapManager::saveMapData(const std::string& filePath) const {
    rapidjson::Document doc;
    
    // 优先从可写目录读取现有存档以保留 player_stats
    std::string writablePath = cocos2d::FileUtils::getInstance()->getWritablePath();
    // 确保路径末尾有斜杠
    if (!writablePath.empty() && writablePath.back() != '/' && writablePath.back() != '\\') {
        writablePath += "/";
    }
    std::string fullPath = writablePath + filePath;
    
    // 自动创建子目录 (例如 archived/)
    size_t lastSlash = fullPath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        std::string dirPath = fullPath.substr(0, lastSlash);
        if (!cocos2d::FileUtils::getInstance()->isDirectoryExist(dirPath)) {
            cocos2d::FileUtils::getInstance()->createDirectory(dirPath);
            CCLOG("MapManager: Created directory %s", dirPath.c_str());
        }
    }
    
    CCLOG("MapManager: Attempting to save to absolute path: %s", fullPath.c_str());
    
    // 如果可写目录没文件，尝试从资源目录读
    std::string readPath = fullPath;
    if (!cocos2d::FileUtils::getInstance()->isFileExist(readPath)) {
        readPath = cocos2d::FileUtils::getInstance()->fullPathForFilename(filePath);
    }

    if (cocos2d::FileUtils::getInstance()->isFileExist(readPath)) {
        const std::string content = cocos2d::FileUtils::getInstance()->getStringFromFile(readPath);
        doc.Parse(content.c_str());
        if (doc.HasParseError() || !doc.IsObject()) {
            doc.SetObject();
        }
    } else {
        doc.SetObject();
    }

    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

    rapidjson::Value mapLayout(rapidjson::kObjectType);

    rapidjson::Value buildingsArray(rapidjson::kArrayType);
    for (auto b : _buildings) {
        rapidjson::Value bObj(rapidjson::kObjectType);
        
        rapidjson::Value nameVal;
        nameVal.SetString(b->GetName().c_str(), allocator);
        bObj.AddMember("type", nameVal, allocator);

        auto pos = b->GetPosition(); 
        bObj.AddMember("x", (int)pos.x, allocator);
        bObj.AddMember("y", (int)pos.y, allocator);
        bObj.AddMember("level", b->GetLevel(), allocator);

        buildingsArray.PushBack(bObj, allocator);
    }
    mapLayout.AddMember("buildings", buildingsArray, allocator);

    rapidjson::Value obstaclesArray(rapidjson::kArrayType);
    for (int x = 0; x < _width; ++x) {
        for (int y = 0; y < _length; ++y) {
            if (_gridStates[x][y] == GridState::Obstacle) {
                rapidjson::Value oObj(rapidjson::kObjectType);
                oObj.AddMember("x", x, allocator);
                oObj.AddMember("y", y, allocator);
                obstaclesArray.PushBack(oObj, allocator);
            }
        }
    }
    mapLayout.AddMember("obstacles", obstaclesArray, allocator);

    if (doc.HasMember("map_layout")) {
        doc.RemoveMember("map_layout");
    }
    doc.AddMember("map_layout", mapLayout, allocator);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    // 写入到可写路径（保证有权限）
    bool success = cocos2d::FileUtils::getInstance()->writeStringToFile(buffer.GetString(), fullPath);
    if (success) {
        CCLOG("MapManager: Auto-saved to %s", fullPath.c_str());
    } else {
        CCLOG("MapManager: ERROR failed to save to %s", fullPath.c_str());
    }
    return success;
}

std::vector<cocos2d::Vec2> MapManager::GetSurroundings(const cocos2d::Vec2& pos) const{
    std::vector<cocos2d::Vec2> v;
    static std::array<cocos2d::Vec2,8> dir = {
            cocos2d::Vec2(-1,1),
            cocos2d::Vec2(0,1),
            cocos2d::Vec2(1,1),
            cocos2d::Vec2(-1,0),
            cocos2d::Vec2(1,0),
            cocos2d::Vec2(-1,-1),
            cocos2d::Vec2(0,-1),
            cocos2d::Vec2(1,-1)
    };
    for(auto it:dir){
        auto target_pos = pos+it;
        if(isValidGrid(target_pos)){
            v.push_back(target_pos);
        }
    }
    return v;
}
