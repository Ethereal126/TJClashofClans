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
#include "Soldier/Soldier.h"
#include "ResourceStorage/ResourceStorage.h"

/**
 * @brief TownHall类
 * 表示玩家村庄中的“大本营”建筑，是整个村庄的核心。
 * 继承自 Building 基类，额外维护资源建筑容量和兵营容量等信息。
 */
class TownHall : public Building {
private:
    /**
     * @brief 私有构造函数
     * 使用基准数值 base 初始化大本营的生命、防御、建造时间与成本等属性。
     * @param name 大本营名称，一般为 "Town Hall" 或本地化后的名称。
     * @param base 基准数值，不同等级或不同难度下可采用不同的基准系数。
     * @param position 场景中的网格坐标 (x, y)。
     * @param texture 大本营当前等级使用的贴图路径。
     */
    TownHall(std::string name, int base, cocos2d::Vec2 position, std::string texture);
    
    /**
     * @brief 私有析构函数
     * 清理UI组件
     */
    ~TownHall();

protected:
    // ==================== 单例相关 ====================
    static TownHall* instance_;            // 单例实例指针
    bool is_initialized_;                  // 标记是否已初始化

    // ==================== 资源与容量相关属性 ====================
    int gold_storage_capacity_;          // 当前等级下可存储的最大金币池数量
    int elixir_storage_capacity_;        // 当前等级下可存储的最大圣水池数量
    int gold_mine_capacity_;             // 当前等级下可解锁的最大金矿上限
    int elixir_collector_capacity_;      // 当前等级下可解锁的最大圣水收集器上限
    int barrack_capacity_;               // 当前等级下可解锁的最大训练营上限
    int army_capacity_;                  // 当前等级下军队总容量（所有军营最大承载人数）
    int gold_;                           // 当前已存储的金币数量
    int elixir_;                         // 当前已存储的圣水数量
    int max_gold_capacity_;              // 最大可持有金币上限（所有金币池总容量）
    int max_elixir_capacity_;            // 最大可持有圣水上限（所有圣水池总容量）
    int current_army_count_;             // 当前军队总人数

    // ==================== 城墙管理相关属性 ====================
    int wall_capacity_;                        // 当前等级下可建造的最大城墙数量
    std::vector<WallBuilding*> walls_;         // 管理的城墙列表

    // ==================== 资源池管理相关属性 ====================
    std::vector<GoldStorage*> gold_storages_;           // 管理的金币池列表
    std::vector<ElixirStorage*> elixir_storages_;       // 管理的圣水池列表
    std::vector<SourceBuilding*> gold_mines_;           // 管理的金矿列表
    std::vector<SourceBuilding*> elixir_collectors_;    // 管理的圣水收集器列表
    std::vector<TrainingBuilding*> barracks_;           // 管理的训练营列表
    std::vector<Barracks*> all_barracks_;               // 管理的军营列表（特指Barracks）

    // ==================== 与 Cocos2d 渲染相关的属性 ====================
    cocos2d::Sprite* flag_sprite_;    // 可选：显示部落旗帜的子节点
    cocos2d::Label* level_label_;    // 显示大本营等级的文本标签

	// ==================== 内部辅助函数 ====================
    void UpdateArmyCapacityFromBarracks();
    int GetTotalArmyCapacityFromBarracks() const;

public:
    /**
     * @brief 获取大本营单例实例
     * @return 返回TownHall单例指针，如果未初始化则返回nullptr
     */
    static TownHall* GetInstance();

    /**
     * @brief 初始化大本营单例
     * @param name 大本营名称
     * @param base 基准数值
     * @param position 场景中的网格坐标
     * @param texture 贴图路径
     * @return 初始化成功返回true，如果已经初始化则返回false
     */
    static bool InitializeInstance(const std::string& name, int base,
        cocos2d::Vec2 position, const std::string& texture);

    /**
     * @brief 销毁大本营单例
     * 清理单例实例并释放资源
     */
    static void DestroyInstance();

    /**
     * @brief 检查单例是否已初始化
     * @return 已初始化返回true，否则返回false
     */
    static bool IsInstanceInitialized();

    /**
     * @brief 获取当前实例（非静态版本）
     * 用于从非静态上下文获取单例实例
     * @return 返回当前实例指针
     */
    TownHall* GetCurrentInstance() const;

    /**
     * @brief 重置大本营状态
     * 重置所有资源和管理列表，恢复到初始状态
     */
    void ResetTownHall();

    /**
     * @brief 重写升级接口
     * 在基类 Building::Upgrade 的基础上，额外提升资源容量、军队容量等属性，
     * 并根据等级更新大本营的贴图与 UI 显示效果。
     */
    virtual void Upgrade() override;

    // ==================== 资源操作接口 ====================

    // 金矿管理
    void AddGoldMine(SourceBuilding* gold_mine);
    void RemoveGoldMine(SourceBuilding* gold_mine);
    int GetTotalGoldMineCount() const;
    int GetTotalGoldProduction() const;

    // 圣水收集器管理
    void AddElixirCollector(SourceBuilding* elixir_collector);
    void RemoveElixirCollector(SourceBuilding* elixir_collector);
    int GetTotalElixirCollectorCount() const;
    int GetTotalElixirProduction() const;

    // 训练营管理
    void AddTrainingBuilding(TrainingBuilding* training_building);
    void RemoveTrainingBuilding(TrainingBuilding* training_building);
    int GetTotalTrainingBuildingCount() const;

    // 军营管理（特指Barracks）
    void AddBarracks(Barracks* barracks);
    void RemoveBarracks(Barracks* barracks);
    int GetTotalBarracksCount() const;

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

    // ==================== 军队容量相关接口 ====================

    /**
     * @brief 获取所有军营最大承载人数
     * @return 军队总容量
     */
    int GetArmyCapacity() const;

    /**
     * @brief 获取当前军队人数
     * @return 当前军队总人数
     */
    int GetCurrentArmyCount() const;

    /**
     * @brief 更新当前军队人数
     * 当训练完成或士兵死亡时调用
     * @param delta 人数变化量（正数为增加，负数为减少）
     */
    void UpdateArmyCount(int delta);

    /**
     * @brief 获取训练营上限
     * @return 当前等级可支持的最大训练营数。
     */
    int GetTrainingCampCapacity() const { return barrack_capacity_; }

    // ==================== 城墙管理接口 ====================

    /**
     * @brief 添加城墙到管理列表
     * @param wall 城墙指针
     */
    void AddWall(WallBuilding* wall);

    /**
     * @brief 从管理列表中移除城墙
     * @param wall 城墙指针
     */
    void RemoveWall(WallBuilding* wall);

    /**
     * @brief 获取当前城墙数量
     * @return 当前管理的城墙数量
     */
    int GetCurrentWallCount() const;

    /**
     * @brief 获取城墙容量上限
     * @return 当前等级允许的最大城墙数量
     */
    int GetWallCapacity() const { return wall_capacity_; }

    /**
     * @brief 检查是否可以添加更多城墙
     * @return true 表示已达到容量上限，无法添加更多城墙
     */
    bool IsWallCapacityFull() const;

    /**
     * @brief 检查是否有城墙需要修复
     * @return true 表示有城墙生命值不满
     */
    bool HasDamagedWalls() const;

    /**
     * @brief 检查是否有城墙可以升级
     * @return true 表示有未达到最大等级且未在升级中的城墙
     */
    bool HasUpgradableWalls() const;

    /**
     * @brief 批量升级所有可升级的城墙
     * 尝试升级所有未达到最大等级且未在升级中的城墙
     * @return 成功开始升级的城墙数量
     */
    int UpgradeAllWalls();

    /**
     * @brief 批量修复所有受损的城墙
     * 修复所有生命值不满的城墙
     * @return 成功修复的城墙数量
     */
    int RepairAllDamagedWalls();

    /**
     * @brief 获取城墙统计信息
     * @param[out] active_count 活跃城墙数量
     * @param[out] damaged_count 受损城墙数量
     * @param[out] upgrading_count 正在升级的城墙数量
     */
    void GetWallStats(int& active_count, int& damaged_count, int& upgrading_count) const;

    /**
     * @brief 清空所有城墙
     * 移除所有城墙（用于重置游戏或清理场景）
     */
    void ClearAllWalls();

    /**
     * @brief 按等级筛选城墙
     * @param min_level 最小等级
     * @param max_level 最大等级
     * @return 符合等级范围的城墙数量
     */
    int CountWallsByLevel(int min_level, int max_level) const;


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
    int GetBarrackCapacity() const { return barrack_capacity_; }

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

    // ==================== 士兵管理相关 ====================
    std::vector<Soldier*> GetAllTrainedSoldiers() const;
    int GetTotalTrainedSoldierCount() const;
    int GetTotalSoldierHousingSpace() const;
    bool CanTrainMoreSoldiers() const;
    bool IsSoldierTypeAvailable(const std::string& soldierType) const;

    // ==================== 士兵管理相关接口 ====================

   /**
    * @brief 添加训练完成的士兵到军队
    * @param soldier 训练完成的士兵指针
    * @return 添加成功返回true，军队已满返回false
    */
    bool AddTrainedSoldier(Soldier* soldier);

    /**
     * @brief 检查是否可以添加指定人口的士兵
     * @param housing_space 需要的人口空间
     * @return 如果可以添加返回true，军队已满返回false
     */
    bool CanAddSoldier(int housing_space) const;

    /**
     * @brief 获取士兵模板列表
     * @return 所有士兵模板的引用
     */
    static const std::vector<SoldierTemplate>& GetSoldierTemplates();

    /**
     * @brief 根据士兵类型获取士兵模板
     * @param type 士兵类型
     * @return 士兵模板指针，未找到返回nullptr
     */
    static const ::SoldierTemplate* GetSoldierTemplate(SoldierType type);

    /**
     * @brief 根据士兵名称获取士兵模板
     * @param name 士兵名称
     * @return 士兵模板指针，未找到返回nullptr
     */
    static const ::SoldierTemplate* GetSoldierTemplate(const std::string& name);

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
     * @brief 播放资源满特效
     * 在资源大于资源总容量时触发，例如变红特效、抖动镜头等。
     */
    void PlayFullCapacityAnimation();

    /**
     * @brief 播放资源不足特效
     * 在资源小于资源需求时触发，例如变红特效、抖动镜头等。
     */
    void PlayNotEnoughAnimation();

    /**
     * @brief 显示大本营详细信息
     * 在基类 Building::ShowInfo 的基础上，额外输出资源容量、当前资源与军队容量。
     */
    virtual void ShowInfo() const override;

    
    struct BuildingTemplate {
        std::string name_;
        std::string icon_path_;
        int cost_;
        int width_;
        int length_;
        std::function<Building* ()> createFunc;

        BuildingTemplate(const std::string& n, const std::string& i, int c,
            int w, int l, std::function<Building* ()> func)
            : name_(n), icon_path_(i), cost_(c), width_(w), length_(l), createFunc(func) {
        }
    };

    /**
     * @brief 获取所有建筑模板列表
     * @return 包含所有建筑模板的向量
     */
    static std::vector<BuildingTemplate> GetAllBuildingTemplates();

    /**
     * @brief 获取所有士兵模板列表
     * @return 包含所有士兵模板的向量
     */
    static std::vector<::SoldierTemplate> GetSoldierCategory();
};


#endif // __TOWN_HALL_H__
