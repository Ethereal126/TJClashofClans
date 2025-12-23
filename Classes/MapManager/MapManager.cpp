#include "MapManager.h"
#include "TownHall/TownHall.h"
#include "UIManager/UIManager.h"
#include <algorithm>
#include <cmath>

MapManager::MapManager():_width(0),_length(0),_gridSize(0),_terrainType(TerrainType::Home){}

MapManager::~MapManager() {

    // 移除输入监听
    if (_inputListener) {
        _eventDispatcher->removeEventListener(_inputListener);
        _inputListener = nullptr;
    }
    // 先移除批处理地面节点
    if (_groundBatch) {
        _groundBatch->removeFromParent();
        _groundBatch = nullptr;
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

    // 改为中心锚点（按你之前的做法）
    this->setAnchorPoint(cocos2d::Vec2(0.5f, 0.5f));

    // 放置位置：保持你之前偏下的视觉效果（如果你想改为顶部/完全居中可调整）
    cocos2d::Director* director = cocos2d::Director::getInstance();
    if (director) {
        cocos2d::Size visibleSize = director->getVisibleSize();
        cocos2d::Vec2 origin = director->getVisibleOrigin();
        float windowCenterX = origin.x + visibleSize.width / 2.0f;
        float windowCenterY = origin.y + visibleSize.height / 7.0f;
        this->setPosition(windowCenterX, windowCenterY);
    }
    // 初始化网格数据并创建地面 tile（initGrids 会调用 rebuildGroundTiles）
    initGrids();
    // 设置输入监听
    setupInputListener();
    return true;
}

void MapManager::initGrids() {
	// 初始化格子状态为 Empty、建筑指针为 nullptr
    _gridStates.assign(_width, std::vector<GridState>(_length, GridState::Empty));
    _gridBuildings.assign(_width, std::vector<Building*>(_length, nullptr));
    _gridObstacles.assign(_width, std::vector<cocos2d::Sprite*>(_length, nullptr));
    _noDeploy.assign(_width, std::vector<bool>(_length, false));
    _buildings.clear();

    // 创建/重建地面批处理节点
    if (_groundBatch) {
        _groundBatch->removeFromParent();
        _groundBatch = nullptr;
    }

    const std::string groundTilePath = "tile/ground_iso.png"; 
    if (cocos2d::FileUtils::getInstance()->isFileExist(groundTilePath)) {
        CCLOG("file found");
        auto tempSprite = cocos2d::Sprite::create(groundTilePath);
        if (tempSprite) {
            auto texture = tempSprite->getTexture();
            _groundBatch = cocos2d::SpriteBatchNode::createWithTexture(texture);
            // 地面层置于最底层
            this->addChild(_groundBatch, -10);

            for (int x = 0; x < _width; ++x) {
                for (int y = 0; y < _length; ++y) {
                    auto tile = cocos2d::Sprite::createWithTexture(texture);
                    tile->setAnchorPoint(cocos2d::Vec2(0.5f, 0.0f));
                    tile->setPosition(gridToWorld(x, y));
                    // 如需与格子宽度对齐，可按需缩放（根据资源决定是否启用）
                    // float targetWidth = static_cast<float>(_gridSize);
                    // float scaleX = targetWidth / tile->getContentSize().width;
                    // tile->setScale(scaleX);
                    _groundBatch->addChild(tile);
                }
            }
        }
    }
    else{
        CCLOG("file unfound");
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
    return gridX >= 0 && gridY >= 0 && gridX < _width && gridY < _length;
}

bool MapManager::isValidGrid(const cocos2d::Vec2& grid_pos) const {
    return isValidGrid(std::floor(grid_pos.x),std::floor(grid_pos.y));
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
    // 边界检查
    if (gridX < 0 || gridY < 0 || gridX + width > _width || gridY + length > _length) return false;
    for (int x = gridX; x < gridX + width; ++x) {
        for (int y = gridY; y < gridY + length; ++y) {
            const GridState st = _gridStates[x][y];
            if (st == GridState::HasBuilding || st == GridState::Obstacle) {
                CCLOG("(%d,%d) in path not available because of state %d",x,y,int(st));
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
}


bool MapManager::placeBuilding(Building* building, int gridX, int gridY) {
    if (!building) return false;
    if (!isPositionAvailable(gridX, gridY, building)) return false;

    building->setPosition(gridToWorld(gridX, gridY));
    this->addChild(building);

    updateBuildingGrids(building, gridX, gridY, true);
    _buildings.push_back(building);
    return true;
}

bool MapManager::removeBuilding(int gridX, int gridY) {
    if (!isValidGrid(gridX, gridY)) return false;
    Building* b = _gridBuildings[gridX][gridY];
    if (!b) return false;

    updateBuildingGrids(b, gridX, gridY, false);
    b->removeFromParent();

    auto it = std::find(_buildings.begin(), _buildings.end(), b);
    if (it != _buildings.end()) _buildings.erase(it);
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

    b->setPosition(gridToWorld(toX, toY));
    return true;
}


cocos2d::Vec2 MapManager::gridToWorld(int gridX, int gridY) const {
    auto grid_pos = cocos2d::Vec2(static_cast<float>(gridX),static_cast<float>(gridY));

    return vecToWorld(grid_pos);
}

std::pair<int, int> MapManager::worldToGrid(const cocos2d::Vec2& worldPos) const {
    auto vec_pos = worldToVec(worldPos);

    const int ix = static_cast<int>(std::floor(vec_pos.x));
    const int iy = static_cast<int>(std::floor(vec_pos.y));
    return { ix, iy };
}

cocos2d::Vec2 MapManager::vecToWorld(cocos2d::Vec2 vec) const{
    const float halfW = static_cast<float>(_gridSize) * 0.5f;
    const float quarterH = static_cast<float>(_gridSize) * 0.25f;

    const float sx = (vec.x - vec.y) * halfW;
    const float sy = (vec.x + vec.y) * quarterH;

    return cocos2d::Vec2(sx, sy);
}

cocos2d::Vec2 MapManager::worldToVec(cocos2d::Vec2 worldPos) const{
    const float halfW = static_cast<float>(_gridSize) * 0.5f;
    const float quarterH = static_cast<float>(_gridSize) * 0.25f;

    const float gx = (worldPos.y / quarterH + worldPos.x / halfW) * 0.5f;
    const float gy = (worldPos.y / quarterH - worldPos.x / halfW) * 0.5f;
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

            sprite->setAnchorPoint(cocos2d::Vec2(0.5f, 0.0f));
            sprite->setPosition(gridToWorld(gridX, gridY));
            _gridObstacles[gridX][gridY] = sprite;

            this->addChild(sprite, 1);
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
    _pendingBuilding->setAnchorPoint(cocos2d::Vec2(0.5f, 0.0f));
    this->addChild(_pendingBuilding, 1000);

    // 创建格子高亮绘制节点
    _placementHighlight = cocos2d::DrawNode::create();
    this->addChild(_placementHighlight, 999);

    // 初始位置放在地图中央
    int buildingWidth = _pendingBuilding->GetWidth();
    int buildingHeight = _pendingBuilding->GetLength();
    _placementGridX = _width / 2 - buildingWidth / 2;
    _placementGridY = _length / 2 - buildingHeight / 2;

    // 更新预览显示
    cocos2d::Vec2 worldPos = gridToWorld(_placementGridX, _placementGridY);
    _pendingBuilding->setPosition(worldPos);
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

    // 注册到格子系统
    updateBuildingGrids(_pendingBuilding, _placementGridX, _placementGridY, true);
    _buildings.push_back(_pendingBuilding);

    //TownHall* townHall = TownHall::GetInstance();
    //townHall->SpendGold(_pendingBuildingCost);
    CCLOG("Building placed! Cost: %d gold (TODO: deduct from resources)", _pendingBuildingCost);

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

    // 将触摸位置转换为地图本地坐标
    cocos2d::Vec2 localPos = this->convertToNodeSpace(touchPos);

    // 转换为格子坐标
    auto gridPos = worldToGrid(localPos);
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

    // 更新建筑位置
    cocos2d::Vec2 worldPos = gridToWorld(_placementGridX, _placementGridY);
    _pendingBuilding->setPosition(worldPos);

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
        cocos2d::Color4F(0.0f, 1.0f, 0.0f, 0.3f) :  // 绿色半透明
        cocos2d::Color4F(1.0f, 0.0f, 0.0f, 0.3f);   // 红色半透明

    cocos2d::Color4F borderColor = _canPlaceAtCurrentPos ?
        cocos2d::Color4F(0.0f, 1.0f, 0.0f, 0.8f) :  // 绿色边框
        cocos2d::Color4F(1.0f, 0.0f, 0.0f, 0.8f);   // 红色边框

    // 绘制每个格子的等角菱形
    for (int x = _placementGridX; x < _placementGridX + buildingWidth; ++x) {
        for (int y = _placementGridY; y < _placementGridY + buildingHeight; ++y) {
            cocos2d::Vec2 center = gridToWorld(x, y);
            float halfW = _gridSize * 0.5f;
            float quarterH = _gridSize * 0.25f;

            cocos2d::Vec2 top(center.x, center.y + quarterH);
            cocos2d::Vec2 right(center.x + halfW, center.y);
            cocos2d::Vec2 bottom(center.x, center.y - quarterH);
            cocos2d::Vec2 left(center.x - halfW, center.y);

            cocos2d::Vec2 verts[] = { top, right, bottom, left };
            _placementHighlight->drawPolygon(verts, 4, fillColor, 2.0f, borderColor);
        }
    }
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

    // 更新按钮位置跟随建筑
    cocos2d::Vec2 buildingPos = _pendingBuilding->getPosition();
    float previewHeight = _pendingBuilding->GetLength() * _gridSize * 0.25f;

    float buttonY = buildingPos.y + previewHeight + 60.0f;
    float btnSpacing = 40.0f;

    _confirmBtn->setPosition(cocos2d::Vec2(buildingPos.x - btnSpacing, buttonY));
    _cancelBtn->setPosition(cocos2d::Vec2(buildingPos.x + btnSpacing, buttonY));
}

void MapManager::createPlacementUI() {
    removePlacementUI();
    if (!_isPlacementMode || !_pendingBuilding) return;

    _placementUINode = cocos2d::Node::create();
    this->addChild(_placementUINode, 1200);

    // 确认按钮
    _confirmBtn = cocos2d::ui::Button::create("UI/btn_confirm.png", "UI/btn_confirm_pressed.png");
    if (!_confirmBtn || !_confirmBtn->getVirtualRenderer()) {
        _confirmBtn = cocos2d::ui::Button::create();
        _confirmBtn->setScale9Enabled(true);
        _confirmBtn->setContentSize(cocos2d::Size(140, 54));
        _confirmBtn->setTitleText("Confirm");
        _confirmBtn->setTitleFontSize(20);
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
        _cancelBtn->setContentSize(cocos2d::Size(140, 54));
        _cancelBtn->setTitleText("Cancel");
        _cancelBtn->setTitleFontSize(20);
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
    // 只有在主村庄地图才设置输入监听器
    if (_inputListener || _terrainType != TerrainType::Home) return;

    _inputListener = cocos2d::EventListenerTouchOneByOne::create();
    _inputListener->setSwallowTouches(false); // 不吞噬触摸，允许穿透

    _inputListener->onTouchBegan = [this](cocos2d::Touch* touch, cocos2d::Event*) -> bool {
        if (_isPlacementMode) return false;

        cocos2d::Vec2 nodePos = this->convertToNodeSpace(touch->getLocation());
        auto gridIdx = worldToGrid(nodePos);

        if (isValidGrid(gridIdx.first, gridIdx.second)) {
            Building* b = getBuildingAt(gridIdx.first, gridIdx.second);
            if (b) {
                // 确定建筑类型
                BuildingCategory cat = BuildingCategory::Normal;
                std::string name = b->GetName();
                if (name.find("Town Hall") != std::string::npos) cat = BuildingCategory::Normal;
                else if (name.find("Gold") != std::string::npos || name.find("Elixir") != std::string::npos) cat = BuildingCategory::Resource;
                else if (name.find("Barrack") != std::string::npos || name.find("Camp") != std::string::npos) cat = BuildingCategory::Military;
                else if (name.find("Cannon") != std::string::npos || name.find("Tower") != std::string::npos || name.find("Wall") != std::string::npos) cat = BuildingCategory::Defense;

                // 显示操作面板（转换位置到屏幕坐标）
                cocos2d::Vec2 screenPos = this->convertToWorldSpace(b->getPosition());
                UIManager::getInstance()->showBuildingOptions(screenPos, cat, b);
                return true;
            }
            else {
                // 点击空地，隐藏面板
                UIManager::getInstance()->hidePanel(UIPanelType::BuildingOptions);
                UIManager::getInstance()->hidePanel(UIPanelType::BuildingInfo);
                UIManager::getInstance()->hidePanel(UIPanelType::BuildingUpgrade);
                UIManager::getInstance()->hidePanel(UIPanelType::ArmyTraining);
            }
        }
        return false;
    };

    _eventDispatcher->addEventListenerWithSceneGraphPriority(_inputListener, this);
}

void MapManager::removePlacementTouchListener() {
    if (_placementTouchListener) {
        cocos2d::Director::getInstance()->getEventDispatcher()
            ->removeEventListener(_placementTouchListener);
        _placementTouchListener = nullptr;
    }
}


void MapManager::clearMap() {
    // Remove all buildings
    auto buildingsCopy = _buildings;
    for (auto b : buildingsCopy) {
        if (b) {
            updateBuildingGrids(b, (int)b->getPosition().x, (int)b->getPosition().y, false); // This is wrong, need grid pos
            // Better: just clear the vectors and remove children
            b->removeFromParent();
        }
    }
    _buildings.clear();

    // Remove all obstacles
    for (int x = 0; x < _width; ++x) {
        for (int y = 0; y < _length; ++y) {
            if (_gridObstacles[x][y]) {
                _gridObstacles[x][y]->removeFromParent();
                _gridObstacles[x][y] = nullptr;
            }
            _gridBuildings[x][y] = nullptr;
            _gridStates[x][y] = GridState::Empty;
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

            // Find matching template
            for (const auto& t : templates) {
                if (t.name_ == type) {
                    Building* building = t.createFunc();
                    if (building) {
                        // Set level (assuming UpgradeToLevel exists or calling Upgrade multiple times)
                        for(int l = 1; l < level; ++l) building->Upgrade();
                        
                        if (!placeBuilding(building, gx, gy)) {
                            building->release(); // Failed to place
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

    return true;
}

bool MapManager::loadMapData(const std::string& filePath) {
    std::string fullPath = cocos2d::FileUtils::getInstance()->fullPathForFilename(filePath);
    if (!cocos2d::FileUtils::getInstance()->isFileExist(fullPath)) return false;

    std::string content = cocos2d::FileUtils::getInstance()->getStringFromFile(fullPath);
    rapidjson::Document doc;
    doc.Parse(content.c_str());

    if (doc.HasParseError()) return false;

    // If it's the full save file, the map layout is under "map_layout"
    if (doc.HasMember("map_layout")) {
        return loadFromJSONObject(doc["map_layout"]);
    }
    
    return loadFromJSONObject(doc);
}

bool MapManager::saveMapData(const std::string& filePath) const {
    rapidjson::Document doc;
    
    const std::string fullPath = cocos2d::FileUtils::getInstance()->fullPathForFilename(filePath);
    if (cocos2d::FileUtils::getInstance()->isFileExist(fullPath)) {
        const std::string content = cocos2d::FileUtils::getInstance()->getStringFromFile(fullPath);
        doc.Parse(content.c_str());
        if (doc.HasParseError() || !doc.IsObject()) {
            doc.SetObject();
        }
    } else {
        doc.SetObject();
    }

    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

    rapidjson::Value mapLayout(rapidjson::kObjectType);
    mapLayout.AddMember("width", _width, allocator);
    mapLayout.AddMember("length", _length, allocator);
    mapLayout.AddMember("grid_size", _gridSize, allocator);

    rapidjson::Value buildingsArray(rapidjson::kArrayType);
    for (auto b : _buildings) {
        rapidjson::Value bObj(rapidjson::kObjectType);
        bObj.AddMember("type", rapidjson::Value(b->GetName().c_str(), allocator).Move(), allocator);

        auto gridPos = worldToGrid(b->getPosition());
        bObj.AddMember("x", gridPos.first, allocator);
        bObj.AddMember("y", gridPos.second, allocator);
        bObj.AddMember("level", b->GetLevel(), allocator);

        buildingsArray.PushBack(bObj, allocator);
    }
    mapLayout.AddMember("buildings", buildingsArray, allocator);

    rapidjson::Value obstaclesArray(rapidjson::kArrayType);
    for (int x = 0; x < _width; ++x) {
        for (int y = 0; y < _length; ++y) {
            if (_gridStates[x][y] == GridState::Obstacle) {
                rapidjson::Value oObj(rapidjson::kObjectType);
                oObj.AddMember("type", rapidjson::Value("Obstacle", allocator).Move(), allocator);
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

    return cocos2d::FileUtils::getInstance()->writeStringToFile(buffer.GetString(), filePath);

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
