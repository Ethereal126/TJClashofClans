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
    _gridOffsetX = 0.0f;
    _gridOffsetY = -210.0f;
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
    // 预加载所有可能的士兵动画，防止队友代码因找不到动画而崩溃
    //reloadAllSoldierAnimations();
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

void MapManager::preloadAllSoldierAnimations() {
    // 1. 定义需要预加载的士兵及其模板数据（这里需要你确保这些数据与游戏初始化时一致）
    // 注意：这里我们模拟队友的加载过程，但不触碰他的静态变量，
    // 因为 AnimationCache::addAnimation 如果发现同名动画已存在，会自动跳过或覆盖，是安全的。
    
    struct PreloadData {
        std::string name;
        int walkFrames;
        int attackFrames;
    };
    std::vector<PreloadData> soldiers = {
        {"Barbarian", 8, 8}, 
        {"Archer", 8, 8},
        {"Giant", 8, 8},
        {"Bomber", 8, 8}
    };

    auto frameCache = cocos2d::SpriteFrameCache::getInstance();
    auto animCache = cocos2d::AnimationCache::getInstance();
    std::vector<std::string> directions = {"up", "down", "left", "right"};

    for (const auto& s : soldiers) {
        std::string plist = "Soldiers/" + s.name + "/anims.plist";
        std::string png = "Soldiers/" + s.name + "/anims.png";
        
        if (cocos2d::FileUtils::getInstance()->isFileExist(plist)) {
            frameCache->addSpriteFramesWithFile(plist, png);
            
            // 预加载行走动画 (完全匹配队友的命名：name + "walk" + dir)
            for (const auto& dir : directions) {
                std::string animName = s.name + "walk" + dir;
                if (!animCache->getAnimation(animName)) {
                    cocos2d::Vector<cocos2d::SpriteFrame*> frames;
                    for (int i = 1; i <= s.walkFrames; ++i) {
                        std::string frameName = animName + std::to_string(i) + ".png";
                        auto frame = frameCache->getSpriteFrameByName(frameName);
                        if (frame) frames.pushBack(frame);
                    }
                    if (!frames.empty()) {
                        auto anim = cocos2d::Animation::createWithSpriteFrames(frames, 0.1f);
                        animCache->addAnimation(anim, animName);
                    }
                }
            }

            // 预加载攻击动画 (完全匹配队友的命名：name + "attack" + dir)
            for (const auto& dir : directions) {
                std::string animName = s.name + "attack" + dir;
                if (!animCache->getAnimation(animName)) {
                    cocos2d::Vector<cocos2d::SpriteFrame*> frames;
                    for (int i = 1; i <= s.attackFrames; ++i) {
                        std::string frameName = animName + std::to_string(i) + ".png";
                        auto frame = frameCache->getSpriteFrameByName(frameName);
                        if (frame) frames.pushBack(frame);
                    }
                    if (!frames.empty()) {
                        auto anim = cocos2d::Animation::createWithSpriteFrames(frames, 0.1f);
                        animCache->addAnimation(anim, animName);
                    }
                }
            }
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

    // 1. 设置位置（相对 worldNode）
    node->setPosition(gridToWorld(gridX, gridY));
    node->setAnchorPoint(cocos2d::Vec2(0.5f, 0.0f));

    // 2. 设置缩放适配格子
    CCLOG("width:%d,length:%d",width,length);
    auto sprite = dynamic_cast<cocos2d::Sprite*>(node);
    if (sprite && sprite->getContentSize().width > 0) {
        // 考虑网格的缩放系数 _gridScaleX
        float visualWidth = (width + length) * (_gridSize * 0.5f * _gridScaleX);
        // 考虑网格的缩放系数 _gridScaleY
        float visualHeight = (width + length) * (_gridSize * 0.5f * _gridScaleY);
        float scale = std::min(visualWidth / sprite->getContentSize().width,
                               visualHeight / sprite->getContentSize().height);
        node->setScale(scale);
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
    _worldNode->addChild(_pendingBuilding, 1000);

    // 创建格子高亮绘制节点
    _placementHighlight = cocos2d::DrawNode::create();
    _worldNode->addChild(_placementHighlight, 999);

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
                        
//                        if (!placeBuilding(building, gx, gy)) {
//                            building->release(); // Failed to place
//                        }
                        PushBuilding(building);
                        building->setVisible(false);
                        this->addChild(building);
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
    if (!cocos2d::FileUtils::getInstance()->isFileExist(fullPath)){
        CCLOG("file not found");
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

        // 使用 worldToVec 获取本地坐标对应的格子，因为 b->getPosition() 是相对 _worldNode 的
        auto vecPos = worldToVec(b->getPosition());
        bObj.AddMember("x", (int)std::floor(vecPos.x), allocator);
        bObj.AddMember("y", (int)std::floor(vecPos.y), allocator);
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
