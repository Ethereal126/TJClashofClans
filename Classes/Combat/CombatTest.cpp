//
// Created by duby0 on 2025/12/11.
//
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "cocos2d.h"
#include "SoldierInCombat.h"
#include "Soldier/Soldier.h"
#include "Map/Map.h"
#include "Building/Building.h"
#include "BuildingInCombat.h"

// 前向声明
class MockSoldier;
class MockMap;
class MockBuildingInCombat;

// Mock Soldier类
class MockSoldier : public Soldier {
public:
    // 提供默认构造函数，调用基类构造函数
    MockSoldier() : Soldier(SoldierType::kBarbarian, 100, 20, 100.0f, 1.0f, 1.0) {}
    
    MOCK_METHOD(int, GetHealth, (), (const, override));
    MOCK_METHOD(float, GetMoveSpeed, (), (const, override));
    MOCK_METHOD(int, GetDamage, (), (const, override));
    MOCK_METHOD(float, GetAttackRange, (), (const, override));
    MOCK_METHOD(float, GetAttackDelay, (), (const, override));
    MOCK_METHOD(void, SetHealth, (int health), (override));
    MOCK_METHOD(void, SetDamage, (int damage), (override));
    MOCK_METHOD(void, SetAttackRange, (double range), (override));
    MOCK_METHOD(void, SetAttackDelay, (double delay), (override));
    MOCK_METHOD(void, SetMoveSpeed, (double speed), (override));
    MOCK_METHOD(std::string, GetName, (), (const, override));
    MOCK_METHOD(void, SetName, (std::string name), (override));
};

// Mock Map类
class MockMap : public Map {
public:
    // 公开构造函数
    MockMap() : Map() {}
    
    MOCK_METHOD(cocos2d::Vec2, worldToVec, (cocos2d::Vec2 worldPos), (const, override));
    MOCK_METHOD(cocos2d::Vec2, vecToWorld, (cocos2d::Vec2 vecPos), (const, override));
    MOCK_METHOD(bool, IsGridAvailable, (const cocos2d::Vec2& pos), (const, override));
    MOCK_METHOD(bool, isValidGrid, (const cocos2d::Vec2& pos), (const, override));
    MOCK_METHOD(void, addChild, (cocos2d::Node* child, int zOrder), (override));
};

// Mock BuildingInCombat类
class MockBuildingInCombat : public BuildingInCombat {
public:
    MockBuildingInCombat(Building* b) : BuildingInCombat(b), tempBuilding(b) {}
    
    MOCK_METHOD(bool, IsAlive, (), (const, override));
    MOCK_METHOD(void, TakeDamage, (int damage), (override));
    MOCK_METHOD(const cocos2d::Vec2&, getPosition, (), (const, override));
    
    // 用于测试的公共方法，返回临时Building指针
    Building* GetTempBuilding() const { return tempBuilding; }
private:
    Building* tempBuilding; // 保存临时Building指针以便清理
};

// 使测试类成为SoldierInCombat的友元，以便测试private方法
class SoldierInCombatTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建mock对象
        mockSoldier = new MockSoldier();
        mockMap = new MockMap();
    
        // 创建一个临时Building对象用于BuildingInCombat构造函数
        Building* tempBuilding = new Building(
            "TestBuilding",     // 名称
            1,                   // 等级
            100,                 // 生命值
            10,                  // 防御值
            300,                 // 建造时间
            1000,                // 建造成本
            std::make_pair(1, 1) // 位置
        );
        mockBuilding = new MockBuildingInCombat(tempBuilding);
        
        // 设置mock行为
        ON_CALL(*mockSoldier, GetHealth()).WillByDefault(::testing::Return(100));
        ON_CALL(*mockSoldier, GetMoveSpeed()).WillByDefault(::testing::Return(100.0f));
        ON_CALL(*mockSoldier, GetDamage()).WillByDefault(::testing::Return(20));
        ON_CALL(*mockSoldier, GetAttackRange()).WillByDefault(::testing::Return(1.0f));
        
        // 地图坐标转换模拟
        ON_CALL(*mockMap, worldToVec(::testing::_)).WillByDefault(::testing::ReturnArg<0>());
        ON_CALL(*mockMap, vecToWorld(::testing::_)).WillByDefault(::testing::ReturnArg<0>());
        
        ON_CALL(*mockMap, IsGridAvailable(::testing::_)).WillByDefault(::testing::Return(true));
        ON_CALL(*mockMap, isValidGrid(::testing::_)).WillByDefault(::testing::Return(true));
        
        ON_CALL(*mockBuilding, IsAlive()).WillByDefault(::testing::Return(true));
        ON_CALL(*mockBuilding, getPosition()).WillByDefault(::testing::ReturnRefOfCopy(cocos2d::Vec2(100, 100)));

        auto director = cocos2d::Director::getInstance();
        if (!director->getOpenGLView()) {
            // Cocos2d-x 4.0 中 GLViewImpl::createWithRect 写法不变，但需确保引入头文件
            auto glview = cocos2d::GLViewImpl::createWithRect("CombatTests", cocos2d::Rect(0, 0, 800, 600));
            director->setOpenGLView(glview);
        }
        // Cocos2d-x 4.0 中 setAnimationInterval 仍有效
        director->setAnimationInterval(0.0f);
        director->setDisplayStats(false);
    }
    
    void TearDown() override {
        // 清理资源
        delete mockSoldier;
        delete mockMap;
        
        // 手动删除Building对象，然后再删除BuildingInCombat对象
        delete mockBuilding->GetTempBuilding();
        delete mockBuilding;

        cocos2d::Director::getInstance()->end();
    }
    
    MockSoldier* mockSoldier;
    MockMap* mockMap;
    MockBuildingInCombat* mockBuilding;
};

// Soldier创建测试
TEST_F(SoldierInCombatTest, SoldierCreateTest) {
    // 测试正常创建
    cocos2d::Vec2 spawnPos(0, 0);
    
    // 创建士兵对象
    auto soldier = SoldierInCombat::Create(mockSoldier, spawnPos);
    
    // 设置地图指针（友元类可以访问私有成员）
    if (soldier) {
        soldier->map_ = mockMap;
    }
    
    // 验证创建是否成功
    EXPECT_NE(soldier, nullptr);
    if (soldier) {
        EXPECT_TRUE(soldier->is_alive_);
        EXPECT_EQ(soldier->location_, spawnPos);
        EXPECT_EQ(soldier->map_, mockMap);
    }
}

// Soldier移动逻辑测试
TEST_F(SoldierInCombatTest, CreateStraightMoveActionTest) {
    // 创建士兵
    cocos2d::Vec2 spawnPos(0, 0);
    auto soldier = SoldierInCombat::Create(mockSoldier, spawnPos);
    
    // 设置地图指针（友元类可以访问私有成员）
    if (soldier) {
        soldier->map_ = mockMap;
    }
    
    // 测试向上移动
    cocos2d::Vec2 targetPos(0, 100);
    
    // 由于CreateStraightMoveAction是private，我们需要通过反射或友元访问
    // 这里我们已经在测试类中声明为友元，所以可以直接访问
    auto moveAction = soldier ? soldier->CreateStraightMoveAction(targetPos) : nullptr;
    
    // 验证动作是否正确创建
    EXPECT_NE(moveAction, nullptr);
    
    // 验证动作类型
    EXPECT_TRUE(dynamic_cast<cocos2d::Spawn*>(moveAction) != nullptr);
    
    // 验证moveAction不为nullptr（创建成功）
    EXPECT_NE(moveAction, nullptr);
}

// 多位置组合测试
TEST_F(SoldierInCombatTest, MultiPositionMoveTest) {
    // 创建士兵
    cocos2d::Vec2 spawnPos(0, 0);
    auto soldier = SoldierInCombat::Create(mockSoldier, spawnPos);
    
    // 设置地图指针（友元类可以访问私有成员）
    if (soldier) {
        soldier->map_ = mockMap;
    }
    
    // 定义多个测试目标位置
    std::vector<cocos2d::Vec2> targetPositions = {
        cocos2d::Vec2(100, 0),   // 向右移动
        cocos2d::Vec2(0, 100),   // 向上移动
        cocos2d::Vec2(-100, 0),  // 向左移动
        cocos2d::Vec2(0, -100),  // 向下移动
        cocos2d::Vec2(100, 100)  // 右上移动
    };
    
    for (const auto& targetPos : targetPositions) {
        auto moveAction = soldier ? soldier->CreateStraightMoveAction(targetPos) : nullptr;
        EXPECT_NE(moveAction, nullptr);
        EXPECT_TRUE(dynamic_cast<cocos2d::Spawn*>(moveAction) != nullptr);
    }
}

// 移动速度测试
TEST_F(SoldierInCombatTest, MoveSpeedTest) {
    // 创建士兵
    cocos2d::Vec2 spawnPos(0, 0);
    auto soldier = SoldierInCombat::Create(mockSoldier, spawnPos);
    
    // 设置地图指针（友元类可以访问私有成员）
    if (soldier) {
        soldier->map_ = mockMap;
    }
    
    // 测试不同移动速度
    std::vector<float> speeds = {50.0f, 100.0f, 200.0f};
    cocos2d::Vec2 targetPos(100, 100);
    cocos2d::Vec2 spawnPosVec(0, 0); // 与soldier创建时的spawnPos对应
    
    for (float speed : speeds) {
        // 设置不同的移动速度
        ON_CALL(*mockSoldier, GetMoveSpeed()).WillByDefault(::testing::Return(speed));
        
        // 计算预期的移动时间
        float move_distance = (targetPos - spawnPosVec).length();
        float expectedTime = move_distance / speed;
        
        auto moveAction = soldier ? soldier->CreateStraightMoveAction(targetPos) : nullptr;
        EXPECT_NE(moveAction, nullptr);
        
        // 验证moveAction不为nullptr且动作持续时间正确
        EXPECT_NE(moveAction, nullptr);
        if (moveAction) {
            // 验证Spawn动作的总持续时间
            EXPECT_FLOAT_EQ(moveAction->getDuration(), expectedTime);
        }
    }
}

int main(int argc, char** argv) {
    // 1. 初始化 GTest
    ::testing::InitGoogleTest(&argc, argv);
    
    // 2. 初始化 Cocos2d-x 引擎（极简版，仅核心模块）
    auto app = cocos2d::Application::getInstance();

    // 3. 运行所有 GTest 测试用例
    int testResult = RUN_ALL_TESTS();

    return testResult;
}

