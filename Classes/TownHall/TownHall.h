//
// Created by Faith_Oriest on 2025/12/9.
//

#pragma once
#ifndef __TOWN_HALL_H__
#define __TOWN_HALL_H__

#include <string>
#include <utility>
#include <vector>
#include "cocos2d.h"
#include "Building/Building.h"
#include "ResourceManager/ResourceManager.h"

/**
 * @brief TownHall类
 * 表示玩家村庄中的“大本营”建筑，是整个村庄的核心。
 * 继承自 Building 基类，额外维护资源建筑容量和兵营容量等信息。
 */
class TownHall : public Building {
protected:
    // ==================== 资源与容量相关属性 ====================
    int gold_storage_capacity_;          // 当前等级下可存储的最大金币池数量
    int elixir_storage_capacity_;        // 当前等级下可存储的最大圣水池数量
    int gold_;                   // 当前已存储的金币数量
    int elixir_;                 // 当前已存储的圣水数量
    int barrack_capacity_;          // 当前等级下可解锁的最大军营上限
    int max_gold_capacity_;      // 最大可持有金币上限（所有金币池总容量）
    int max_elixir_capacity_;    // 最大可持有圣水上限（所有圣水池总容量）

    // ==================== 资源池管理相关属性 ====================
    std::vector<GoldStorage*> gold_storages_;      // 管理的金币池列表
    std::vector<ElixirStorage*> elixir_storages_;  // 管理的圣水池列表

    // ==================== 与 Cocos2d 渲染相关的属性 ====================
    cocos2d::Sprite* flag_sprite_;    // 可选：显示部落旗帜的子节点
    cocos2d::Label* level_label_;    // 显示大本营等级的文本标签

public:
    /**
     * @brief 构造函数
     * 使用基准数值 base 初始化大本营的生命、防御、建造时间与成本等属性。
     * @param name 大本营名称，一般为 "Town Hall" 或本地化后的名称。
     * @param base 基准数值，不同等级或不同难度下可采用不同的基准系数。
     * @param position 场景中的网格坐标 (x, y)。
     * @param texture 大本营当前等级使用的贴图路径。
     */
    TownHall(std::string name, int base, cocos2d::Vec2 position, std::string texture);

    /**
     * @brief 重写升级接口
     * 在基类 Building::Upgrade 的基础上，额外提升资源容量、军队容量等属性，
     * 并根据等级更新大本营的贴图与 UI 显示效果。
     */
    virtual void Upgrade() override;

    // ==================== 资源操作接口 ====================
    /**
     * @brief 向大本营中存入金币
     * @param amount 要增加的金币数量。
     * @return 实际成功存入的金币数量（可能受容量限制小于 amount）。
     */
    int AddGold(int amount);

    /**
     * @brief 消耗大本营中的金币
     * @param amount 需要消耗的金币数量。
     * @return 若金币足够并成功扣除返回 true，否则返回 false 且不扣除。
     */
    bool SpendGold(int amount);

    /**
     * @brief 向大本营中存入圣水
     * @param amount 要增加的圣水数量。
     * @return 实际成功存入的圣水数量。
     */
    int AddElixir(int amount);

    /**
     * @brief 消耗大本营中的圣水
     * @param amount 需要消耗的圣水数量。
     * @return 若圣水足够并成功扣除返回 true，否则返回 false。
     */
    bool SpendElixir(int amount);

    /**
     * @brief 添加金币池到管理列表
     * @param gold_storage 金币池指针
     */
    void AddGoldStorage(GoldStorage* gold_storage);

    /**
     * @brief 从管理列表中移除金币池
     * @param gold_storage 金币池指针
     */
    void RemoveGoldStorage(GoldStorage* gold_storage);

    /**
     * @brief 添加圣水池到管理列表
     * @param elixir_storage 圣水池指针
     */
    void AddElixirStorage(ElixirStorage* elixir_storage);

    /**
     * @brief 从管理列表中移除圣水池
     * @param elixir_storage 圣水池指针
     */
    void RemoveElixirStorage(ElixirStorage* elixir_storage);

    /**
     * @brief 计算所有金币池中的金币总数
     * 遍历所有管理的金币池，汇总当前存储的金币数量。
     * @return 所有金币池中的金币总数。
     */
    int GetTotalGoldFromStorages() const;

    /**
     * @brief 计算所有圣水池中的圣水总数
     * 遍历所有管理的圣水池，汇总当前存储的圣水数量。
     * @return 所有圣水池中的圣水总数。
     */
    int GetTotalElixirFromStorages() const;

    /**
     * @brief 计算所有金币池的总容量
     * 遍历所有管理的金币池，汇总它们的存储容量。
     * @return 所有金币池的总容量。
     */
    int GetTotalGoldCapacity() const;

    /**
     * @brief 计算所有圣水池的总容量
     * 遍历所有管理的圣水池，汇总它们的存储容量。
     * @return 所有圣水池的总容量。
     */
    int GetTotalElixirCapacity() const;

    /**
     * @brief 更新金币持有上限
     * 重新计算所有金币池的总容量并更新上限值。
     */
    void UpdateMaxGoldCapacity();

    /**
     * @brief 更新圣水持有上限
     * 重新计算所有圣水池的总容量并更新上限值。
     */
    void UpdateMaxElixirCapacity();

    /**
     * @brief 更新所有资源持有上限
     * 同时更新金币和圣水的持有上限。
     */
    void UpdateAllResourceCapacities();

    /**
     * @brief 获取最大可持有金币上限
     * 返回所有金币池的总容量。
     * @return 最大可持有金币数量。
     */
    int GetMaxGoldCapacity() const { return max_gold_capacity_; }

    /**
     * @brief 获取最大可持有圣水上限
     * 返回所有圣水池的总容量。
     * @return 最大可持有圣水数量。
     */
    int GetMaxElixirCapacity() const { return max_elixir_capacity_; }

    /**
     * @brief 获取当前金币存储比例
     * 计算当前金币存储量占总容量的比例。
     * @return 金币存储比例（0.0-1.0）。
     */
    float GetGoldStorageRatio() const;

    /**
     * @brief 获取当前圣水存储比例
     * 计算当前圣水存储量占总容量的比例。
     * @return 圣水存储比例（0.0-1.0）。
     */
    float GetElixirStorageRatio() const;

    /**
     * @brief 检查金币是否已达上限
     * 判断当前金币存储量是否已达到最大容量。
     * @return 如果金币已满返回 true，否则返回 false。
     */
    bool IsGoldFull() const;

    /**
     * @brief 检查圣水是否已达上限
     * 判断当前圣水存储量是否已达到最大容量。
     * @return 如果圣水已满返回 true，否则返回 false。
     */
    bool IsElixirFull() const;

    // ==================== 资源与容量的 Get 接口 ====================
    /**
     * @brief 获取金币池上限
     * @return 当前等级的金币池上限。
     */
    int GetGoldCapacity() const { return gold_storage_capacity_; }

    /**
     * @brief 获取圣水池上限
     * @return 当前等级的圣水池上限。
     */
    int GetElixirCapacity() const { return elixir_storage_capacity_; }

    /**
     * @brief 获取兵营上限
     * @return 当前等级可支持的最大兵营数。
     */
    int GetArmyCapacity() const { return barrack_capacity_; }

    /**
    * @brief 获取当前金币数量
    * @return 当前存储的金币数量。
    */
    int GetGold() const { return gold_; }

    /**
     * @brief 获取当前圣水数量
     * @return 当前存储的圣水数量。
     */
    int GetElixir() const { return elixir_; }

    // ==================== Cocos2d 显示相关接口 ====================

    /**
     * @brief 刷新大本营等级显示
     * 在升级或初始化时，更新 level_label_ 的文本内容。
     */
    void UpdateLevelLabel();

    /**
     * @brief 播放升级特效
     * 可在此接口中触发粒子、缩放动画、音效等，用于表现大本营升级。
     */
    void PlayUpgradeEffect();

    /**
     * @brief 播放被摧毁特效
     * 在大本营生命值降为 0 时触发，例如爆炸特效、震动镜头等。
     */
    void PlayDestroyedEffect();

    /**
     * @brief 显示大本营详细信息
     * 在基类 Building::ShowInfo 的基础上，额外输出资源容量、当前资源与军队容量。
     */
    virtual void ShowInfo() const override;
};

#endif // __TOWN_HALL_H__
