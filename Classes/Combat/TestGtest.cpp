#include "HelloWorldScene.h"
// 引入测试头文件
#include "gtest/gtest.h"

USING_NS_CC;

TEST(HelloWorldTest, Init) {
    auto scene = HelloWorld::createScene();
    EXPECT_TRUE(scene != nullptr);
}