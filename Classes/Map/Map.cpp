
#include "Map.h"
#include <algorithm>
#include <cmath>

Map::Map():_width(0),_length(0),_gridSize(0),_terrainType(TerrainType::Home){}

Map::~Map() {
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

Map* Map::create(int width, int length, int gridSize, TerrainType terrainType) {
    Map* ret = new (std::nothrow) Map();
    if (ret && ret->init(width, length, gridSize, terrainType)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool Map::init(int width, int length, int gridSize, TerrainType terrainType) {
    if (!cocos2d::Node::init()) return false;

    _width = width;
    _length = length;
    _gridSize = gridSize;
    _terrainType = terrainType;

    // 地图以左下角为原点
    this->setAnchorPoint(cocos2d::Vec2::ANCHOR_BOTTOM_LEFT);

    initGrids();
    return true;
}

void Map::initGrids() {
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

    const std::string groundTilePath = "....Resources/tiles/ground_iso.png"; 
    if (cocos2d::FileUtils::getInstance()->isFileExist(groundTilePath)) {
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
}

std::pair<int, int> Map::getMapSize() const {
    return { _width, _length };
}

int Map::getGridSize() const {
    return _gridSize;
}

TerrainType Map::getTerrainType() const {
    return _terrainType;
}

bool Map::isValidGrid(int gridX, int gridY) const {
    return gridX >= 0 && gridY >= 0 && gridX < _width && gridY < _length;
}

GridState Map::getGridState(int gridX, int gridY) const {
    if (!isValidGrid(gridX, gridY)) {
        return GridState::Obstacle;
    }
    return _gridStates[gridX][gridY];
}

void Map::setGridState(int gridX, int gridY, GridState state) {
    if (!isValidGrid(gridX, gridY)) return;
    _gridStates[gridX][gridY] = state;
}

bool Map::isRangeAvailable(int gridX, int gridY, int width, int length) const {
    // 边界检查
    if (gridX < 0 || gridY < 0 || gridX + width > _width || gridY + length > _length) return false;
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

bool Map::isPositionAvailable(int gridX, int gridY, const Building* building ) const {
	const int bw = building ? building->GetWidth() : 1;
	const int bh = building ? building->GetLength() : 1;
    return isRangeAvailable(gridX, gridY, bw, bh);

}

void Map::updateBuildingGrids(Building* building, int gridX, int gridY, bool occupy) {
    
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


bool Map::placeBuilding(Building* building, int gridX, int gridY) {
    if (!building) return false;
    if (!isPositionAvailable(gridX, gridY, building)) return false;

    building->setPosition(gridToWorld(gridX, gridY));
    this->addChild(building);

    updateBuildingGrids(building, gridX, gridY, true);
    _buildings.push_back(building);
    return true;
}

bool Map::removeBuilding(int gridX, int gridY) {
    if (!isValidGrid(gridX, gridY)) return false;
    Building* b = _gridBuildings[gridX][gridY];
    if (!b) return false;

    updateBuildingGrids(b, gridX, gridY, false);
    b->removeFromParent();

    auto it = std::find(_buildings.begin(), _buildings.end(), b);
    if (it != _buildings.end()) _buildings.erase(it);

    return true;
}

bool Map::moveBuilding(int fromX, int fromY, int toX, int toY) {
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


cocos2d::Vec2 Map::gridToWorld(int gridX, int gridY) const {
    const float halfW = static_cast<float>(_gridSize) * 0.5f;
    const float quarterH = static_cast<float>(_gridSize) * 0.25f;

    const float sx = (static_cast<float>(gridX) - static_cast<float>(gridY)) * halfW;
    const float sy = (static_cast<float>(gridX) + static_cast<float>(gridY)) * quarterH;

    return cocos2d::Vec2(sx, sy);
}


std::pair<int, int> Map::worldToGrid(const cocos2d::Vec2& worldPos) const {
    
    const float halfW = static_cast<float>(_gridSize) * 0.5f;
    const float quarterH = static_cast<float>(_gridSize) * 0.25f;

    const float gx = (worldPos.y / quarterH + worldPos.x / halfW) * 0.5f;
    const float gy = (worldPos.y / quarterH - worldPos.x / halfW) * 0.5f;

    const int ix = static_cast<int>(std::floor(gx));
    const int iy = static_cast<int>(std::floor(gy));
    return { ix, iy };
}

Building* Map::getBuildingAt(int gridX, int gridY) const {
    if (!isValidGrid(gridX, gridY)) return nullptr;
    return _gridBuildings[gridX][gridY];
}

const std::vector<Building*>& Map::getAllBuildings() const {
    return _buildings;
}

void Map::placeObstacle(int gridX, int gridY) {
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

void Map::removeObstacle(int gridX, int gridY) {
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

void Map::markNoDeploy(int gridX, int gridY) {
    if (!isValidGrid(gridX, gridY)) return;
    _noDeploy[gridX][gridY] = true;
}

void Map::unmarkNoDeploy(int gridX, int gridY) {
    if (!isValidGrid(gridX, gridY)) return;
    _noDeploy[gridX][gridY] = false;
}

bool Map::isDeployAllowedGrid(int gridX, int gridY) const {
    if (!isValidGrid(gridX, gridY)) return false;
    if (_noDeploy[gridX][gridY]) return false;
    const GridState st = _gridStates[gridX][gridY];
    if (st == GridState::HasBuilding || st == GridState::Obstacle) return false;
    return true;
}


bool Map::isDeployAllowedPixel(const cocos2d::Vec2& worldPos) const {   
    auto grid = worldToGrid(worldPos);
    return isDeployAllowedGrid(grid.first, grid.second);
}


bool Map::saveMapData(const std::string& filePath) const {
    // 占位实现：后续确定 JSON/二进制协议后填充
    cocos2d::log("Map::saveMapData(%s) - stub", filePath.c_str());
    return true;
}

bool Map::loadMapData(const std::string& filePath) {
    // 占位实现：清空并重置
    cocos2d::log("Map::loadMapData(%s) - stub", filePath.c_str());
    initGrids();
    // TODO: 解析文件并重建状态与建筑
    return true;
}