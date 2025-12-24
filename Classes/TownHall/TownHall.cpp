//
// TownHall.cpp
//

#include "TownHall.h"
#include <cmath>
#include <fstream>
#include <sstream>
USING_NS_CC;

static std::vector<SoldierTemplate> soldier_templates = {
    SoldierTemplate(SoldierType::kBarbarian, "Barbarian","Soldiers/Barbarian/Barbarianwalkright1.png",
                    50, 12, 0.75f, 1.0f, 1.0f, 1, 25, 20),
    SoldierTemplate(SoldierType::kArcher, "Archer","Soldiers/Archer/Archerattackdown1.png",
                    25, 10, 1.0f, 3.5f, 1.0f, 1, 50, 25),
    SoldierTemplate(SoldierType::kBomber, "Bomber","Soldiers/Giant/Giantattackdown1.png",
                    20, 10, 1.0f, 1.0f, 1.0f, 2, 1000, 60),
    SoldierTemplate(SoldierType::kGiant, "Giant","Soldiers/Bomber/Bomberwalkdown1.png",
                    500, 30, 0.5f, 1.0f, 2.0f, 5, 500, 120)
};

// ==================== JSON数据读取函数 ====================

/**
 * @brief 从JSON文件中读取玩家数据
 * @param file_path JSON文件路径
 * @param gold 输出参数，读取到的金币数量
 * @param elixir 输出参数，读取到的圣水数量
 * @param level 输出参数，读取到的大本营等级
 * @return 读取成功返回true，失败返回false
 */
static bool LoadPlayerDataFromJSON(const std::string& file_path,
    int& gold, int& elixir, int& level) {
    // 参数验证
    if (file_path.empty()) {
        cocos2d::log("错误: JSON文件路径为空");
        return false;
    }

    // 使用Cocos2d-x的FileUtils处理资源文件路径
    std::string fullPath = cocos2d::FileUtils::getInstance()->fullPathForFilename(file_path);
    cocos2d::log("尝试加载JSON文件: %s -> %s", file_path.c_str(), fullPath.c_str());

    // 读取并显示文件内容（仅用于调试）
    if (cocos2d::FileUtils::getInstance()->isFileExist(fullPath)) {
        std::string content = cocos2d::FileUtils::getInstance()->getStringFromFile(fullPath);
        cocos2d::log("JSON文件内容: %s", content.c_str());
    }

    // 使用局部变量存储实际使用的路径
    std::string actualFilePath = file_path;

    // 如果主路径找不到文件，尝试备用路径
    if (!cocos2d::FileUtils::getInstance()->isFileExist(fullPath)) {
        cocos2d::log("主路径未找到文件，尝试备用路径...");

        // 尝试的备用路径列表
        std::vector<std::string> alternativePaths = {
            "Resources/archived/player_save.json",
            "Resources/archived/player_save.json",
            "../Resources/archived/player_save.json",
            "../../Resources/archived/player_save.json"
        };

        bool found = false;
        for (const auto& altPath : alternativePaths) {
            std::string altFullPath = cocos2d::FileUtils::getInstance()->fullPathForFilename(altPath);
            if (cocos2d::FileUtils::getInstance()->isFileExist(altFullPath)) {
                fullPath = altFullPath;
                actualFilePath = altPath;  // 修改局部变量而不是const参数
                found = true;
                cocos2d::log("使用备用路径找到文件: %s", altFullPath.c_str());
                break;
            }
        }

        if (!found) {
            cocos2d::log("错误: 无法找到JSON文件: %s", fullPath.c_str());
            cocos2d::log("搜索路径列表:");
            const auto& searchPaths = cocos2d::FileUtils::getInstance()->getSearchPaths();
            for (const auto& path : searchPaths) {
                cocos2d::log("  - %s", path.c_str());
            }
            return false;
        }
    }

    // 记录实际使用的文件路径
    cocos2d::log("实际使用的文件路径: %s", actualFilePath.c_str());

    // 读取文件内容
    std::string content = cocos2d::FileUtils::getInstance()->getStringFromFile(fullPath);
    if (content.empty()) {
        cocos2d::log("错误: JSON文件为空或读取失败: %s", fullPath.c_str());
        return false;
    }

    cocos2d::log("成功读取JSON文件，内容长度: %zu 字节", content.length());

    // 解析JSON数据
    rapidjson::Document document;
    if (document.Parse(content.c_str()).HasParseError()) {
        size_t error_offset = document.GetErrorOffset();
        cocos2d::log("JSON解析错误，错误位置: %zu", error_offset);

        // 输出错误位置前后的内容用于调试
        if (error_offset < content.length()) {
            std::string context = content.substr(std::max(0, (int)error_offset - 20),
                std::min(40, (int)content.length() - (int)error_offset));
            cocos2d::log("错误上下文: ...%s", context.c_str());
        }
        return false;
    }

    // 检查根对象
    if (!document.IsObject()) {
        cocos2d::log("JSON根对象不是有效的对象类型");
        return false;
    }

    try {
        // 读取玩家统计数据
        if (!document.HasMember("player_stats")) {
            cocos2d::log("JSON中缺少player_stats字段");
            return false;
        }

        if (!document["player_stats"].IsObject()) {
            cocos2d::log("player_stats不是有效的对象类型");
            return false;
        }

        const auto& player_stats = document["player_stats"];

        // 读取金币数据
        if (player_stats.HasMember("gold")) {
            if (player_stats["gold"].IsInt()) {
                gold = player_stats["gold"].GetInt();
                // 验证金币数值范围
                if (gold < 0) {
                    cocos2d::log("警告: 金币数值为负数，已修正为0");
                    gold = 0;
                }
            }
            else if (player_stats["gold"].IsString()) {
                // 如果是字符串，尝试转换
                try {
                    gold = std::stoi(player_stats["gold"].GetString());
                    if (gold < 0) {
                        cocos2d::log("警告: 金币数值为负数，已修正为0");
                        gold = 0;
                    }
                }
                catch (...) {
                    cocos2d::log("无法将金币字符串转换为整数，使用默认值0");
                    gold = 0;
                }
            }
            else {
                cocos2d::log("金币字段类型无效，使用默认值0");
                gold = 0;
            }
        }
        else {
            cocos2d::log("缺少gold字段，使用默认值0");
            gold = 0;
        }

        // 读取圣水数据
        if (player_stats.HasMember("elixir")) {
            if (player_stats["elixir"].IsInt()) {
                elixir = player_stats["elixir"].GetInt();
                // 验证圣水数值范围
                if (elixir < 0) {
                    cocos2d::log("警告: 圣水数值为负数，已修正为0");
                    elixir = 0;
                }
            }
            else if (player_stats["elixir"].IsString()) {
                // 如果是字符串，尝试转换
                try {
                    elixir = std::stoi(player_stats["elixir"].GetString());
                    if (elixir < 0) {
                        cocos2d::log("警告: 圣水数值为负数，已修正为0");
                        elixir = 0;
                    }
                }
                catch (...) {
                    cocos2d::log("无法将圣水字符串转换为整数，使用默认值0");
                    elixir = 0;
                }
            }
            else {
                cocos2d::log("圣水字段类型无效，使用默认值0");
                elixir = 0;
            }
        }
        else {
            cocos2d::log("缺少elixir字段，使用默认值0");
            elixir = 0;
        }

        // 读取大本营等级数据
        if (player_stats.HasMember("level")) {
            if (player_stats["level"].IsInt()) {
                level = player_stats["level"].GetInt();
                // 验证等级数值范围
                if (level <= 0) {
                    cocos2d::log("警告: 大本营等级小于等于0，已修正为1");
                    level = 1;
                }
                else if (level > 10) {
                    cocos2d::log("警告: 大本营等级超过最大值10，已修正为10");
                    level = 10;
                }
            }
            else if (player_stats["level"].IsString()) {
                // 如果是字符串，尝试转换
                try {
                    level = std::stoi(player_stats["level"].GetString());
                    if (level <= 0) {
                        cocos2d::log("警告: 大本营等级小于等于0，已修正为1");
                        level = 1;
                    }
                    else if (level > 10) {
                        cocos2d::log("警告: 大本营等级超过最大值10，已修正为10");
                        level = 10;
                    }
                }
                catch (...) {
                    cocos2d::log("无法将等级字符串转换为整数，使用默认值1");
                    level = 1;
                }
            }
            else {
                cocos2d::log("等级字段类型无效，使用默认值1");
                level = 1;
            }
        }
        else {
            cocos2d::log("缺少level字段，使用默认值1");
            level = 1;
        }

        // 也尝试读取town_hall_level字段（向后兼容）
        if (player_stats.HasMember("town_hall_level") && player_stats["town_hall_level"].IsInt()) {
            level = player_stats["town_hall_level"].GetInt();
            // 验证等级数值范围
            if (level <= 0) {
                cocos2d::log("警告: 大本营等级小于等于0，已修正为1");
                level = 1;
            }
            else if (level > 10) {
                cocos2d::log("警告: 大本营等级超过最大值10，已修正为10");
                level = 10;
            }
            cocos2d::log("从town_hall_level字段读取大本营等级: %d", level);
        }

        cocos2d::log("成功解析JSON数据: 金币=%d, 圣水=%d, 等级=%d", gold, elixir, level);
        return true;
    }
    catch (const std::exception& e) {
        cocos2d::log("解析JSON数据时发生异常: %s", e.what());
        return false;
    }
    catch (...) {
        cocos2d::log("解析JSON数据时发生未知异常");
        return false;
    }
}

bool TownHall::SavePlayerDataToJSON(const std::string& file_path,
    int gold, int elixir, int level) {
    // 参数验证
    if (file_path.empty()) {
        cocos2d::log("错误: JSON文件路径为空");
        return false;
    }

    // 使用Cocos2d-x的FileUtils处理资源文件路径
    std::string fullPath = cocos2d::FileUtils::getInstance()->fullPathForFilename(file_path);
    cocos2d::log("尝试保存JSON文件: %s -> %s", file_path.c_str(), fullPath.c_str());

    // 使用局部变量存储实际使用的路径
    std::string actualFilePath = file_path;

    // 如果主路径找不到文件，尝试备用路径
    if (!cocos2d::FileUtils::getInstance()->isFileExist(fullPath)) {
        cocos2d::log("主路径未找到文件，尝试备用路径...");

        // 尝试的备用路径列表
        std::vector<std::string> alternativePaths = {
            "Resources/archived/player_save.json",
            "archived/player_save.json",
            "../Resources/archived/player_save.json",
            "../../Resources/archived/player_save.json"
        };

        bool found = false;
        for (const auto& altPath : alternativePaths) {
            std::string altFullPath = cocos2d::FileUtils::getInstance()->fullPathForFilename(altPath);
            if (cocos2d::FileUtils::getInstance()->isFileExist(altFullPath)) {
                fullPath = altFullPath;
                actualFilePath = altPath;
                found = true;
                cocos2d::log("使用备用路径找到文件: %s", altFullPath.c_str());
                break;
            }
        }

        if (!found) {
            cocos2d::log("错误: 无法找到JSON文件，将创建新文件: %s", fullPath.c_str());
            // 如果找不到文件，使用可写路径
            fullPath = cocos2d::FileUtils::getInstance()->getWritablePath() + "player_save.json";
            actualFilePath = "player_save.json";
        }
    }

    // 读取现有JSON文件内容（如果存在）
    rapidjson::Document doc;
    std::string content = "";

    if (cocos2d::FileUtils::getInstance()->isFileExist(fullPath)) {
        content = cocos2d::FileUtils::getInstance()->getStringFromFile(fullPath);
        if (!content.empty()) {
            doc.Parse(content.c_str());
            if (doc.HasParseError()) {
                cocos2d::log("警告: JSON解析失败，将创建新文档，错误代码: %d", doc.GetParseError());
                doc.SetObject();
            }
        }
        else {
            doc.SetObject();
        }
    }
    else {
        doc.SetObject();
    }

    // 确保文档是对象类型
    if (!doc.IsObject()) {
        doc.SetObject();
    }

    // 创建或更新player_stats对象
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

    if (!doc.HasMember("player_stats")) {
        rapidjson::Value player_stats(rapidjson::kObjectType);
        doc.AddMember("player_stats", player_stats, allocator);
    }

    rapidjson::Value& player_stats = doc["player_stats"];
    if (!player_stats.IsObject()) {
        player_stats.SetObject();
    }

    // 检查是否存在player_stats对象
    if (!doc.HasMember("player_stats")) {
        // 如果不存在，创建player_stats对象
        rapidjson::Value playerStats(rapidjson::kObjectType);
        doc.AddMember("player_stats", playerStats, doc.GetAllocator());
    }

    // 确保player_stats是对象类型
    if (!doc["player_stats"].IsObject()) {
        doc["player_stats"].SetObject();
    }

    // 更新数据
    if (doc["player_stats"].HasMember("gold")) {
        doc["player_stats"]["gold"].SetInt(gold);
    }
    else {
        doc["player_stats"].AddMember("gold", gold, allocator);
    }

    if (doc["player_stats"].HasMember("elixir")) {
        doc["player_stats"]["elixir"].SetInt(elixir);
    }
    else {
        doc["player_stats"].AddMember("elixir", elixir, allocator);
    }

    if (doc["player_stats"].HasMember("town_hall_level")) {
        doc["player_stats"]["town_hall_level"].SetInt(level);
    }
    else {
        doc["player_stats"].AddMember("town_hall_level", level, allocator);
    }

    // 将JSON转换为字符串
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    std::string jsonContent = buffer.GetString();

    // 写入文件
    std::ofstream outputFile(fullPath);
    if (!outputFile.is_open()) {
        cocos2d::log("错误: 无法打开文件进行写入: %s", fullPath.c_str());
        return false;
    }

    outputFile << jsonContent;
    outputFile.close();

    cocos2d::log("成功保存玩家数据到JSON文件: 金币=%d, 圣水=%d, 大本营等级=%d", gold, elixir, level);
    return true;
}

bool TownHall::UpdatePlayerDataField(const std::string& file_path,
    const std::string& field_name, int value) {
    // 参数验证
    if (file_path.empty()) {
        cocos2d::log("错误: JSON文件路径为空");
        return false;
    }

    if (field_name != "gold" && field_name != "elixir" && field_name != "town_hall_level") {
        cocos2d::log("错误: 不支持的字段名: %s", field_name.c_str());
        return false;
    }

    // 如果传入的是完整路径，直接使用；否则尝试从Resources目录查找
    std::string fullPath = file_path;

    // 检查是否是绝对路径（包含可写路径）
    if (file_path.find(cocos2d::FileUtils::getInstance()->getWritablePath()) == std::string::npos) {
        // 如果不是绝对路径，尝试从Resources目录查找
        fullPath = cocos2d::FileUtils::getInstance()->fullPathForFilename(file_path);

        // 如果Resources目录找不到，使用可写路径
        if (!cocos2d::FileUtils::getInstance()->isFileExist(fullPath)) {
            fullPath = cocos2d::FileUtils::getInstance()->getWritablePath() + "player_save.json";
            cocos2d::log("Resources目录未找到文件，使用可写路径: %s", fullPath.c_str());
        }
    }

    cocos2d::log("尝试更新JSON字段: %s -> %s", file_path.c_str(), fullPath.c_str());

    // 使用局部变量存储实际使用的路径
    std::string actualFilePath = file_path;

    // 读取现有JSON文件内容（如果存在）
    rapidjson::Document doc;
    std::string content = "";

    if (cocos2d::FileUtils::getInstance()->isFileExist(fullPath)) {
        content = cocos2d::FileUtils::getInstance()->getStringFromFile(fullPath);
        if (!content.empty()) {
            doc.Parse(content.c_str());
            if (doc.HasParseError()) {
                cocos2d::log("警告: JSON解析失败，将创建新文档，错误代码: %d", doc.GetParseError());
                doc.SetObject();
            }
        }
        else {
            doc.SetObject();
        }
    }
    else {
        doc.SetObject();
    }

    // 确保文档是对象类型
    if (!doc.IsObject()) {
        doc.SetObject();
    }

    // 创建或更新player_stats对象
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

    if (!doc.HasMember("player_stats")) {
        rapidjson::Value player_stats(rapidjson::kObjectType);
        doc.AddMember("player_stats", player_stats, allocator);
    }

    rapidjson::Value& player_stats = doc["player_stats"];
    if (!player_stats.IsObject()) {
        player_stats.SetObject();
    }

    // 检查是否存在player_stats对象
    if (!doc.HasMember("player_stats")) {
        // 如果不存在，创建player_stats对象
        rapidjson::Value playerStats(rapidjson::kObjectType);
        doc.AddMember("player_stats", playerStats, doc.GetAllocator());
    }

    // 确保player_stats是对象类型
    if (!doc["player_stats"].IsObject()) {
        doc["player_stats"].SetObject();
    }

    // 更新player_stats对象内的指定字段
    rapidjson::Value fieldName(field_name.c_str(), allocator);

    if (doc["player_stats"].HasMember(fieldName)) {
        doc["player_stats"][fieldName].SetInt(value);
    }
    else {
        doc["player_stats"].AddMember(fieldName, value, allocator);
    }

    // 将JSON转换为字符串
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    std::string jsonContent = buffer.GetString();

    // 写入文件
    std::ofstream outputFile(fullPath);
    if (!outputFile.is_open()) {
        cocos2d::log("错误: 无法打开文件进行写入: %s", fullPath.c_str());
        return false;
    }

    outputFile << jsonContent;
    outputFile.close();

    cocos2d::log("成功更新JSON字段: %s = %d", field_name.c_str(), value);
    return true;
}

// ==================== TownHall 实现 ====================
// ==================== 单例管理相关 ====================

// 在文件开头添加静态成员初始化
TownHall* TownHall::instance_ = nullptr;

TownHall* TownHall::Create(const std::string& name, int base,
    cocos2d::Vec2 position,
    const std::string& texture) {
    auto building = new (std::nothrow) TownHall(name, base, position, texture);

    if (building) {
        if (building->initWithFile(texture)) {
            // 标记为自动释放（Cocos2d-x的内存管理机制）
            building->autorelease();

            cocos2d::log("创建大本营: %s (类型: %s, 位置: %f,%f)",
                name.c_str(), position.x, position.y);
            return building;
        }

        // 初始化失败，删除对象
        delete building;
    }

    cocos2d::log("创建大本营失败: %s", name.c_str());
    return nullptr;
}

TownHall* TownHall::GetInstance() {
    InitializeInstance("大本营", 1, { 0,0 }, "buildings/TownHall1.png");
    return instance_;
}

bool TownHall::InitializeInstance(const std::string& name, int base,
    cocos2d::Vec2 position, const std::string& texture) {
    if (instance_ != nullptr) {
        cocos2d::log("TownHall call InitializeInstance() after initialized");
        return false;
    }

    // 创建大本营实例
    instance_ = new TownHall(name, base, position, texture);
    if (instance_) {
        instance_->is_initialized_ = true;
        cocos2d::log("TownHall singleton initialized successfully");
        return true;
    }

    cocos2d::log("TownHall singleton initialization failed");
    return false;
}

void TownHall::DestroyInstance() {
    if (instance_ != nullptr) {
        delete instance_;
        instance_ = nullptr;
        cocos2d::log("大本营单例已销毁");
    }
}

bool TownHall::IsInstanceInitialized() {
    return instance_ != nullptr && instance_->is_initialized_;
}

TownHall* TownHall::GetCurrentInstance() const {
    return instance_;
}

void TownHall::ResetTownHall() {
    if (!IsInstanceInitialized()) {
        cocos2d::log("大本营未初始化，无法重置");
        return;
    }

    // 重置资源
    gold_ = 0;
    elixir_ = 0;

    // 清空管理列表
    gold_storages_.clear();
    elixir_storages_.clear();

    // 重置容量
    max_gold_capacity_ = 0;
    max_elixir_capacity_ = 0;

    // 重置初始化标记
    is_initialized_ = false;

    cocos2d::log("大本营状态已重置");
}



TownHall::TownHall(std::string name, int base, cocos2d::Vec2 position, std::string texture)
    : Building(name, 1, base * 1000, base * 5, base * 60, base * 200, 4, 4,
        position)
    , is_initialized_(false)
    , gold_storage_capacity_(base * 2)
    , elixir_storage_capacity_(base * 2)
    , gold_mine_capacity_(base * 3)
    , elixir_collector_capacity_(base * 3)
    , barrack_capacity_(base)
    , army_capacity_(base * 10)
    , gold_(0)
    , elixir_(0)
    , current_army_count_(0)
    , max_gold_capacity_(0)
    , max_elixir_capacity_(0)
    , wall_capacity_(base * 10)
    , flag_sprite_(nullptr)
    , level_label_(nullptr) {

    // 首先尝试从UserDefault加载资源数据
    cocos2d::UserDefault* userDefault = cocos2d::UserDefault::getInstance();
    int saved_gold = userDefault->getIntegerForKey("player_gold", -1);
    int saved_elixir = userDefault->getIntegerForKey("player_elixir", -1);
    bool use_user_default = (saved_gold != -1 && saved_elixir != -1);

    // 从JSON文件读取玩家数据（备用）
    int json_gold = 0;
    int json_elixir = 0;
    int json_level = base; // 使用base作为默认等级

    // 每次启动时都从Resources目录重新复制JSON文件，确保使用最新配置
    std::string json_file_path = cocos2d::FileUtils::getInstance()->getWritablePath() + "player_save.json";
    std::string source_path = "archived/player_save.json";

    bool json_load_success = false;

    try {
        if (use_user_default) {
            // 如果UserDefault中有保存的值，则使用这些值
            gold_ = saved_gold;
            elixir_ = saved_elixir;
            level_ = userDefault->getIntegerForKey("player_townhall_level", base);

            cocos2d::log("从UserDefault加载资源数据: 金币=%d, 圣水=%d, 等级=%d", gold_, elixir_, level_);

            // 根据等级更新纹理
            std::string new_texture = "buildings/TownHall" + std::to_string(level_) + ".png";
            this->setTexture(new_texture);
        } else {
            // 如果UserDefault中没有数据，则从JSON文件加载
            cocos2d::log("UserDefault中没有资源数据，从JSON文件加载");

            // ✅ 只在第一次没有存档文件时，才从Resources复制一份初始存档
            if (!cocos2d::FileUtils::getInstance()->isFileExist(json_file_path)) {
                if (cocos2d::FileUtils::getInstance()->isFileExist(source_path)) {
                    std::string content = cocos2d::FileUtils::getInstance()->getStringFromFile(source_path);

                    std::ofstream outFile(json_file_path, std::ios::out | std::ios::trunc);
                    if (outFile.is_open()) {
                        outFile << content;
                        outFile.close();
                        cocos2d::log("首次创建存档：从Resources复制到可写路径: %s", json_file_path.c_str());
                    }
                    else {
                        cocos2d::log("无法创建可写路径存档文件: %s", json_file_path.c_str());
                    }
                }
                else {
                    cocos2d::log("Resources目录中找不到源文件: %s", source_path.c_str());
                }
            }
            else {
                cocos2d::log("已存在存档，跳过复制覆盖: %s", json_file_path.c_str());
            }

            // （强烈建议）打印你真正读写的存说明确路径，避免你盯着Resources那份看
            cocos2d::log("SAVE FILE PATH = %s", json_file_path.c_str());


            // 添加额外的强制重新加载选项，可通过构造函数参数控制
            // 如果构造函数传入force_reload_from_resources参数为true，则强制重新复制
            // 这个功能可以在需要重置玩家数据时使用
            // 例如：TownHall("大本营", 1, {0,0}, "buildings/TownHall1.png", true) // 强制重新加载
            // 注意：这需要修改构造函数签名以添加force_reload_from_resources参数

            json_load_success = LoadPlayerDataFromJSON(json_file_path, json_gold, json_elixir, json_level);

            if (json_load_success) {
                // 使用从JSON文件读取的数据
                level_ = json_level;
                gold_ = json_gold;
                elixir_ = json_elixir;

                cocos2d::log("从JSON文件成功初始化大本营数据: 金币=%d, 圣水=%d, 等级=%d",
                    json_gold, json_elixir, level_);

                // 将圣水值分配给储罐（如果有）
                // 注意：这需要在储罐被添加到TownHall之后调用，所以我们需要延迟执行
                // 使用scheduleOnce确保在下一帧执行，此时所有建筑已经加载完成
                auto distributeElixir = [this, json_elixir](float) {
                    if (!elixir_storages_.empty() && json_elixir > 0) {
                        int remaining = json_elixir;
                        for (auto* storage : elixir_storages_) {
                            if (remaining <= 0) break;

                            int capacity = storage->GetCapacity();
                            int current = storage->GetCurrentAmount();
                            int space = capacity - current;

                            if (space > 0) {
                                int add = std::min(remaining, space);
                                storage->AddResource(add);
                                remaining -= add;
                            }
                        }
                        cocos2d::log("已将%d圣水分配到储罐中", json_elixir - remaining);
                    }
                    };

                // 延迟执行，确保所有建筑已经加载完成
                this->scheduleOnce(distributeElixir, 0.1f, "distribute_elixir");

                // 根据等级更新纹理
                std::string new_texture = "buildings/TownHall" + std::to_string(level_) + ".png";
                this->setTexture(new_texture);
            }
            else {
                // JSON读取失败，使用默认值
                cocos2d::log("JSON文件读取失败，使用默认值初始化大本营");
                level_ = base;
                this->setTexture(texture);
            }
        }

        // 如果从UserDefault加载了数据，也需要分配圣水到储罐
        if (use_user_default && !elixir_storages_.empty() && saved_elixir > 0) {
            auto distributeElixir = [this, saved_elixir](float) {
                int remaining = saved_elixir;
                for (auto* storage : elixir_storages_) {
                    if (remaining <= 0) break;

                    int capacity = storage->GetCapacity();
                    int current = storage->GetCurrentAmount();
                    int space = capacity - current;

                    if (space > 0) {
                        int add = std::min(remaining, space);
                        storage->AddResource(add);
                        remaining -= add;
                    }
                }
                cocos2d::log("已将%d圣水从UserDefault分配到储罐中", saved_elixir - remaining);
            };

            // 延迟执行，确保所有建筑已经加载完成
            this->scheduleOnce(distributeElixir, 0.1f, "distribute_elixir_from_userdefault");
        }
    }
    catch (const std::exception& e) {
        // 异常处理
        cocos2d::log("初始化大本营时发生异常: %s，使用默认值", e.what());
        level_ = base;
        this->setTexture(texture);
    }
    catch (...) {
        // 未知异常处理
        cocos2d::log("初始化大本营时发生未知异常，使用默认值");
        level_ = base;
        this->setTexture(texture);
    }

    // 设置位置
    this->setPosition(position);

    // 初始化UI组件
    UpdateLevelLabel();

    // 构造函数执行完成后标记为已初始化
    is_initialized_ = true;

    cocos2d::log("大本营 %s 初始化完成 - 等级: %d, 金币: %d, 圣水: %d",
        name.c_str(), level_, gold_, elixir_);
}

TownHall::~TownHall() {
    // 销毁的是单例实例，重置静态指针
    if (this == instance_) {
        instance_ = nullptr;
    }

    // 清理UI组件
    flag_sprite_ = nullptr;
    level_label_ = nullptr;

    // 清空管理列表
    gold_storages_.clear();
    elixir_storages_.clear();
    walls_.clear();
}

void TownHall::Upgrade() {
    // 调用基类升级
    Building::Upgrade();

    // 提升大本营特有属性
    gold_storage_capacity_ += 1;           // 每级增加1个资源池容量
    elixir_storage_capacity_ += 1;
    gold_mine_capacity_ += 1;              // 每级增加1个金矿容量
    elixir_collector_capacity_ += 1;       // 每级增加1个圣水收集器容量
    barrack_capacity_ += 1;                // 每级增加1个兵营容量
    army_capacity_ += 8;                   // 每级增加5个军队容量
    wall_capacity_ += 5;                   // 每级增加5个城墙容量

    // 更新资源持有上限
    UpdateAllResourceCapacities();

    // 更新等级显示
    UpdateLevelLabel();

    // 播放升级特效
    //PlayUpgradeEffect();

    // 更新纹理（假设纹理命名规则为 "buildings/TownHallX.png"）
    std::string new_texture = "buildings/TownHall" + std::to_string(level_) + ".png";
    this->setTexture(new_texture);

    // 保存更新后的大本营等级到JSON文件
    std::string json_file_path = cocos2d::FileUtils::getInstance()->getWritablePath() + "player_save.json";
    if (!UpdatePlayerDataField(json_file_path, "town_hall_level", level_)) {
        cocos2d::log("警告：保存大本营等级数据到JSON文件失败");
    }


    cocos2d::log("%s 升级到等级 %d，金币池上限: %d，圣水池上限: %d，金矿上限: %d，圣水收集器上限: %d，训练营上限: %d，军队容量: %d",
        name_.c_str(), level_, gold_storage_capacity_, elixir_storage_capacity_,
        gold_mine_capacity_, elixir_collector_capacity_, barrack_capacity_, army_capacity_);
}

// ==================== 金矿管理 ====================
void TownHall::AddGoldMine(SourceBuilding* gold_mine) {
    if (!gold_mine || gold_mines_.size() >= gold_mine_capacity_) {
        cocos2d::log("已达到金矿上限，无法添加更多金矿");
        return;
    }

    auto it = std::find(gold_mines_.begin(), gold_mines_.end(), gold_mine);
    if (it == gold_mines_.end()) {
        gold_mines_.push_back(gold_mine);
        cocos2d::log("添加金矿，当前数量: %zu/%d", gold_mines_.size(), gold_mine_capacity_);
    }
}

void TownHall::RemoveGoldMine(SourceBuilding* gold_mine) {
    auto it = std::find(gold_mines_.begin(), gold_mines_.end(), gold_mine);
    if (it != gold_mines_.end()) {
        gold_mines_.erase(it);
        cocos2d::log("移除金矿，剩余数量: %zu", gold_mines_.size());
    }
}

int TownHall::GetTotalGoldMineCount() const {
    return static_cast<int>(gold_mines_.size());
}

int TownHall::GetTotalGoldProduction() const {
    int total = 0;
    for (const auto* mine : gold_mines_) {
        if (mine && mine->IsActive()) {
            total += mine->GetProductionRate();
        }
    }
    return total;
}

// ==================== 圣水收集器管理 ====================
void TownHall::AddElixirCollector(SourceBuilding* elixir_collector) {
    if (!elixir_collector || elixir_collectors_.size() >= elixir_collector_capacity_) {
        cocos2d::log("已达到圣水收集器上限，无法添加更多圣水收集器");
        return;
    }

    auto it = std::find(elixir_collectors_.begin(), elixir_collectors_.end(), elixir_collector);
    if (it == elixir_collectors_.end()) {
        elixir_collectors_.push_back(elixir_collector);
        cocos2d::log("添加圣水收集器，当前数量: %zu/%d", elixir_collectors_.size(), elixir_collector_capacity_);
    }
}

void TownHall::RemoveElixirCollector(SourceBuilding* elixir_collector) {
    auto it = std::find(elixir_collectors_.begin(), elixir_collectors_.end(), elixir_collector);
    if (it != elixir_collectors_.end()) {
        elixir_collectors_.erase(it);
        cocos2d::log("移除圣水收集器，剩余数量: %zu", elixir_collectors_.size());
    }
}

int TownHall::GetTotalElixirCollectorCount() const {
    return static_cast<int>(elixir_collectors_.size());
}

int TownHall::GetTotalElixirProduction() const {
    int total = 0;
    for (const auto* collector : elixir_collectors_) {
        if (collector && collector->IsActive()) {
            total += collector->GetProductionRate();
        }
    }
    return total;
}

// ==================== 训练营管理 ====================

void TownHall::AddTrainingBuilding(TrainingBuilding* training_building) {
    if (!training_building || barracks_.size() >= barrack_capacity_) {
        cocos2d::log("已达到训练营上限，无法添加更多训练营");
        return;
    }

    auto it = std::find(barracks_.begin(), barracks_.end(), training_building);
    if (it == barracks_.end()) {
        barracks_.push_back(training_building);
        cocos2d::log("添加训练营，当前数量: %zu/%d", barracks_.size(), barrack_capacity_);
    }
}

void TownHall::RemoveTrainingBuilding(TrainingBuilding* training_building) {
    auto it = std::find(barracks_.begin(), barracks_.end(), training_building);
    if (it != barracks_.end()) {
        barracks_.erase(it);
        cocos2d::log("移除训练营，剩余数量: %zu", barracks_.size());
    }
}

int TownHall::GetTotalTrainingBuildingCount() const {
    return static_cast<int>(barracks_.size());
}

// ==================== 军营管理（特指Barracks） ====================

void TownHall::AddBarracks(Barracks* barracks) {
    if (!barracks) return;

    auto it = std::find(all_barracks_.begin(), all_barracks_.end(), barracks);
    if (it == all_barracks_.end()) {
        all_barracks_.push_back(barracks);

        // 更新军队容量（每个军营提供基础容量）
        UpdateArmyCapacityFromBarracks();

        cocos2d::log("添加军营，当前数量: %zu", all_barracks_.size());
    }
}

void TownHall::RemoveBarracks(Barracks* barracks) {
    auto it = std::find(all_barracks_.begin(), all_barracks_.end(), barracks);
    if (it != all_barracks_.end()) {
        all_barracks_.erase(it);

        // 更新军队容量
        UpdateArmyCapacityFromBarracks();

        cocos2d::log("移除军营，剩余数量: %zu", all_barracks_.size());
    }
}

int TownHall::GetTotalBarracksCount() const {
    return static_cast<int>(all_barracks_.size());
}

int TownHall::GetTotalArmyCapacityFromBarracks() const {
    int total_capacity = 0;
    for (const auto* barracks : all_barracks_) {
        if (barracks && barracks->IsActive()) {
            // 假设Barracks有一个GetMaxTroopCapacity()函数
            total_capacity += barracks->GetTrainingCapacity();
        }
    }
    return total_capacity;
}

void TownHall::UpdateArmyCapacityFromBarracks() {
    int total_barracks_capacity = 0;

    // 遍历所有军营，计算总容量
    for (const auto* barracks : all_barracks_) {
        if (barracks && barracks->IsActive()) {
            // 获取军营的训练容量
            // 注意：这里假设Barracks有一个GetTrainingCapacity()方法
            // 如果Barracks类中没有这个方法，需要在Barracks类中添加
            total_barracks_capacity += barracks->GetTrainingCapacity();
        }
    }

    // 计算军队总容量 = 大本营基础容量 + 所有军营容量
    // 大本营基础容量：每级5个单位
    int base_capacity = level_ * 5;
    army_capacity_ = base_capacity + total_barracks_capacity;

    // 如果当前军队人数超过新容量，调整人数
    if (current_army_count_ > army_capacity_) {
        current_army_count_ = army_capacity_;
    }

    cocos2d::log("军队容量更新: 基础%d + 军营%d = %d",
        base_capacity, total_barracks_capacity, army_capacity_);
}

// ==================== 城墙管理 ====================

void TownHall::AddWall(WallBuilding* wall) {
    if (!wall) {
        cocos2d::log("错误: 尝试添加空的城墙指针");
        return;
    }

    // 检查是否已达到上限
    if (IsWallCapacityFull()) {
        cocos2d::log("已达到城墙上限 %d/%d，无法添加更多城墙",
            GetCurrentWallCount(), wall_capacity_);
        return;
    }

    // 检查是否已存在
    auto it = std::find(walls_.begin(), walls_.end(), wall);
    if (it == walls_.end()) {
        walls_.push_back(wall);
        cocos2d::log("添加城墙 %s，当前数量: %d/%d",
            wall->GetName().c_str(),
            GetCurrentWallCount(),
            wall_capacity_);
    }
    else {
        cocos2d::log("城墙 %s 已在管理列表中", wall->GetName().c_str());
    }
}

void TownHall::RemoveWall(WallBuilding* wall) {
    if (!wall) {
        cocos2d::log("错误: 尝试移除空的城墙指针");
        return;
    }

    auto it = std::find(walls_.begin(), walls_.end(), wall);
    if (it != walls_.end()) {
        walls_.erase(it);
        cocos2d::log("移除城墙 %s，剩余数量: %d",
            wall->GetName().c_str(),
            GetCurrentWallCount());
    }
    else {
        cocos2d::log("城墙 %s 不在管理列表中", wall->GetName().c_str());
    }
}

int TownHall::GetCurrentWallCount() const {
    return static_cast<int>(walls_.size());
}

bool TownHall::IsWallCapacityFull() const {
    return GetCurrentWallCount() >= wall_capacity_;
}

bool TownHall::HasDamagedWalls() const {
    for (const auto* wall : walls_) {
        if (wall && wall->IsActive() && wall->GetHealth() < wall->GetMaxHealth()) {
            return true;
        }
    }
    return false;
}

int TownHall::UpgradeAllWalls() {
    int upgraded_count = 0;

    for (auto* wall : walls_) {
        if (!wall || !wall->IsActive()) {
            continue;
        }


        // 计算升级成本（简化处理）
        int upgrade_cost = wall->GetNextBuildCost();

        // 检查资源是否足够（这里只检查金币）
        if (SpendGold(upgrade_cost)) {
            wall->StartUpgrade(wall->GetNextBuildTime());
            upgraded_count++;
            cocos2d::log("开始升级城墙 %s，消耗金币: %d",
                wall->GetName().c_str(), upgrade_cost);
        }
        else {
            cocos2d::log("金币不足，无法升级城墙 %s",
                wall->GetName().c_str());
            break; // 资源不足，停止批量升级
        }

    }

    cocos2d::log("批量升级完成，开始升级 %d 个城墙", upgraded_count);
    return upgraded_count;
}

int TownHall::RepairAllDamagedWalls() {
    int repaired_count = 0;
    int total_cost = 0;

    for (auto* wall : walls_) {
        if (!wall || !wall->IsActive()) {
            continue;
        }

        // 检查是否需要修复
        if (wall->GetHealth() < wall->GetMaxHealth()) {
            // 计算修复成本（简化处理：修复成本为缺失生命值的5%）
            int repair_cost = (wall->GetMaxHealth() - wall->GetHealth()) / 20;

            if (repair_cost <= 0) {
                repair_cost = 1; // 至少1金币
            }

            // 检查金币是否足够
            if (SpendGold(repair_cost)) {
                wall->Repair();
                repaired_count++;
                total_cost += repair_cost;
                cocos2d::log("修复城墙 %s，消耗金币: %d",
                    wall->GetName().c_str(), repair_cost);
            }
            else {
                cocos2d::log("金币不足，停止批量修复");
                break; // 资源不足，停止批量修复
            }
        }
    }

    if (repaired_count > 0) {
        cocos2d::log("批量修复完成，修复 %d 个城墙，总计消耗金币: %d",
            repaired_count, total_cost);
    }

    return repaired_count;
}

void TownHall::GetWallStats(int& active_count, int& damaged_count, int& upgrading_count) const {
    active_count = 0;
    damaged_count = 0;
    upgrading_count = 0;

    for (const auto* wall : walls_) {
        if (!wall) {
            continue;
        }

        if (wall->IsActive()) {
            active_count++;

            if (wall->GetHealth() < wall->GetMaxHealth()) {
                damaged_count++;
            }

            if (wall->IsUpgrading()) {
                upgrading_count++;
            }
        }
    }
}

void TownHall::ClearAllWalls() {
    int wall_count = GetCurrentWallCount();
    walls_.clear();
    cocos2d::log("清空所有 %d 个城墙", wall_count);
}

int TownHall::CountWallsByLevel(int min_level, int max_level) const {
    int count = 0;

    for (const auto* wall : walls_) {
        if (wall && wall->IsActive()) {
            int level = wall->GetLevel();
            if (level >= min_level && level <= max_level) {
                count++;
            }
        }
    }

    return count;
}
// ==================== 军队人数更新 ====================

void TownHall::UpdateArmyCount(int delta) {
    int new_count = current_army_count_ + delta;

    // 限制在0到容量之间
    if (new_count < 0) new_count = 0;
    if (new_count > army_capacity_) new_count = army_capacity_;

    int old_count = current_army_count_;
    current_army_count_ = new_count;

    cocos2d::log("军队人数更新: %d -> %d/%d", old_count, current_army_count_, army_capacity_);

    // 如果军队人数变化，可以触发一些事件
    if (current_army_count_ >= army_capacity_) {
        cocos2d::log("军队已满，无法训练更多士兵");
        // 可以在这里触发UI提示
    }
}

// ==================== 军队容量相关 ====================

int TownHall::GetArmyCapacity() const {
    return army_capacity_;
}

int TownHall::GetCurrentArmyCount() const {
    return current_army_count_;
}

int TownHall::AddGold() {
    int amount = 0;

    for (const auto& collector : gold_mines_) {
        if (collector && collector->IsActive()) {
            amount += collector->CalculateTimeProductionProduct();
        }
    }

    // 获取总金币容量（金币池容量总和）
    int max_capacity = GetTotalGoldCapacity();
    int current_total = gold_;

    if (current_total >= max_capacity) {
        cocos2d::log("金币池已达上限，无法存入更多金币");
        return 0;
    }

    // 计算实际可存入的数量
    int available_space = max_capacity - current_total;
    int actual_add = std::min(amount, available_space);

    // 存入金币池
    int remaining = actual_add;
    for (auto* storage : gold_storages_) {
        if (remaining <= 0) break;

        int space_in_storage = storage->GetCapacity() - storage->GetCurrentAmount();
        if (space_in_storage > 0) {
            int add_to_storage = std::min(remaining, space_in_storage);
            storage->AddResource(add_to_storage);
            remaining -= add_to_storage;
        }
    }

    //更新大本营金币数量
    gold_ += actual_add;
    
    // 使用UserDefault强制保存资源数据
    cocos2d::UserDefault* userDefault = cocos2d::UserDefault::getInstance();
    userDefault->setIntegerForKey("player_gold", gold_);
    userDefault->setIntegerForKey("player_townhall_level", level_);
    userDefault->flush(); // 立即写入磁盘

    // 保存更新后的金币数量到JSON文件
    // 使用可写路径确保文件可以被正确更新
    std::string json_file_path = cocos2d::FileUtils::getInstance()->getWritablePath() + "player_save.json";
    if (!UpdatePlayerDataField(json_file_path, "gold", gold_)) {
        cocos2d::log("警告：保存金币数据到JSON文件失败");
    }

    cocos2d::log("存入金币: %d，剩余可存入: %d", actual_add, remaining);
    return actual_add;
}


bool TownHall::SpendGold(int amount) {
    if (amount <= 0) {
        return false;
    }

    // 获取总金币
    int total_gold = GetTotalGoldFromStorages();
    if (total_gold < amount) {
        cocos2d::log("金币不足，需要: %d，现有: %d", amount, total_gold);
        return false;
    }

    // 从金币池消耗
    int remaining = amount;
    for (auto* storage : gold_storages_) {
        if (remaining <= 0) break;

        int gold_in_storage = storage->GetCurrentAmount();
        if (gold_in_storage > 0) {
            int deduct_from_storage = std::min(remaining, gold_in_storage);
            storage->UseResource(deduct_from_storage);
            remaining -= deduct_from_storage;
        }
    }

    //更新大本营金币数量
    gold_ = GetTotalGoldFromStorages();
    
    // 使用UserDefault强制保存资源数据
    cocos2d::UserDefault* userDefault = cocos2d::UserDefault::getInstance();
    userDefault->setIntegerForKey("player_gold", gold_);
    userDefault->setIntegerForKey("player_townhall_level", level_);
    userDefault->flush(); // 立即写入磁盘

    // 保存更新后的金币数量到JSON文件
    // 使用可写路径确保文件可以被正确更新
    std::string json_file_path = cocos2d::FileUtils::getInstance()->getWritablePath() + "player_save.json";
    if (!UpdatePlayerDataField(json_file_path, "gold", gold_)) {
        cocos2d::log("警告：保存金币数据到JSON文件失败");
    }

    cocos2d::log("消耗金币: %d，剩余: %d", amount, total_gold - amount);
    return true;
}


int TownHall::AddElixir() {
    int amount = 0;

    for (const auto& collector : elixir_collectors_) {
        if (collector && collector->IsActive()) {
            amount += collector->CalculateTimeProductionProduct();
        }
    }

    if (amount <= 0) {
        return 0;
    }

    // 获取总圣水容量（圣水池容量总和）
    int max_capacity = GetTotalElixirCapacity();
    int current_total = elixir_;

    if (current_total >= max_capacity) {
        cocos2d::log("圣水池已达上限，无法存入更多圣水");
        return 0;
    }

    // 计算实际可存入的数量
    int available_space = max_capacity - current_total;
    int actual_add = std::min(amount, available_space);

    // 存入圣水池
    int remaining = actual_add;
    for (auto* storage : elixir_storages_) {
        if (remaining <= 0) break;

        int space_in_storage = storage->GetCapacity() - storage->GetCurrentAmount();
        if (space_in_storage > 0) {
            int add_to_storage = std::min(remaining, space_in_storage);
            storage->AddResource(add_to_storage);
            remaining -= add_to_storage;
        }
    }

    //更新大本营圣水数量
    elixir_ += actual_add;
    
    // 使用UserDefault强制保存资源数据
    cocos2d::UserDefault* userDefault = cocos2d::UserDefault::getInstance();
    userDefault->setIntegerForKey("player_elixir", elixir_);
    userDefault->setIntegerForKey("player_townhall_level", level_);
    userDefault->flush(); // 立即写入磁盘

    // 保存更新后的圣水数量到JSON文件
    // 使用可写路径确保文件可以被正确更新
    std::string json_file_path = cocos2d::FileUtils::getInstance()->getWritablePath() + "player_save.json";
    if (!UpdatePlayerDataField(json_file_path, "elixir", elixir_)) {
        cocos2d::log("警告：保存圣水数据到JSON文件失败");
    }

    cocos2d::log("存入圣水: %d，剩余可存入: %d", actual_add, remaining);
    return actual_add;
}

bool TownHall::SpendElixir(int amount) {
    if (amount <= 0) {
        return false;
    }

    // 获取总圣水
    int total_elixir = GetTotalElixirFromStorages();
    if (total_elixir < amount) {
        cocos2d::log("圣水不足，需要: %d，现有: %d", amount, total_elixir);
        return false;
    }

    // 从圣水池消耗
    int remaining = amount;
    for (auto* storage : elixir_storages_) {
        if (remaining <= 0) break;

        int elixir_in_storage = storage->GetCurrentAmount();
        if (elixir_in_storage > 0) {
            int deduct_from_storage = std::min(remaining, elixir_in_storage);
            storage->UseResource(deduct_from_storage);
            remaining -= deduct_from_storage;
        }
    }

    //更新大本营圣水数量
    elixir_ = GetTotalElixirFromStorages();
    
    // 使用UserDefault强制保存资源数据
    cocos2d::UserDefault* userDefault = cocos2d::UserDefault::getInstance();
    userDefault->setIntegerForKey("player_elixir", elixir_);
    userDefault->setIntegerForKey("player_townhall_level", level_);
    userDefault->flush(); // 立即写入磁盘

    // 保存更新后的圣水数量到JSON文件
    // 使用可写路径确保文件可以被正确更新
    std::string json_file_path = cocos2d::FileUtils::getInstance()->getWritablePath() + "player_save.json";
    if (!UpdatePlayerDataField(json_file_path, "elixir", elixir_)) {
        cocos2d::log("警告：保存圣水数据到JSON文件失败");
    }

    cocos2d::log("消耗圣水: %d，剩余: %d", amount, total_elixir - amount);
    return true;
}

void TownHall::AddGoldStorage(ProductionBuilding* gold_storage) {
    if (!gold_storage) {
        return;
    }

    // 检查是否已达到上限
    if (static_cast<int>(gold_storages_.size()) >= gold_storage_capacity_) {
        cocos2d::log("已达到金币池上限，无法添加更多金币池");
        return;
    }

    // 检查是否已存在
    auto it = std::find(gold_storages_.begin(), gold_storages_.end(), gold_storage);
    if (it == gold_storages_.end()) {
        gold_storages_.push_back(gold_storage);
        UpdateMaxGoldCapacity();
        cocos2d::log("添加金币池，当前数量: %zu", gold_storages_.size());
    }
}

void TownHall::RemoveGoldStorage(ProductionBuilding* gold_storage) {
    if (!gold_storage) {
        return;
    }

    auto it = std::find(gold_storages_.begin(), gold_storages_.end(), gold_storage);
    if (it != gold_storages_.end()) {
        gold_storages_.erase(it);
        UpdateMaxGoldCapacity();
        cocos2d::log("移除金币池，剩余数量: %zu", gold_storages_.size());
    }
}

void TownHall::AddElixirStorage(ProductionBuilding* elixir_storage) {
    if (!elixir_storage) {
        return;
    }

    // 检查是否已达到上限
    if (static_cast<int>(elixir_storages_.size()) >= elixir_storage_capacity_) {
        cocos2d::log("已达到圣水池上限，无法添加更多圣水池");
        return;
    }

    // 检查是否已存在
    auto it = std::find(elixir_storages_.begin(), elixir_storages_.end(), elixir_storage);
    if (it == elixir_storages_.end()) {
        elixir_storages_.push_back(elixir_storage);
        UpdateMaxElixirCapacity();
        cocos2d::log("添加圣水池，当前数量: %zu", elixir_storages_.size());
    }
}

void TownHall::RemoveElixirStorage(ProductionBuilding* elixir_storage) {
    if (!elixir_storage) {
        return;
    }

    auto it = std::find(elixir_storages_.begin(), elixir_storages_.end(), elixir_storage);
    if (it != elixir_storages_.end()) {
        elixir_storages_.erase(it);
        UpdateMaxElixirCapacity();
        cocos2d::log("移除圣水池，剩余数量: %zu", elixir_storages_.size());
    }
}

int TownHall::GetTotalGoldFromStorages() const {
    int total = 0;
    for (const auto* storage : gold_storages_) {
        if (storage && storage->IsActive()) {
            total += storage->GetCurrentAmount();
        }
    }
    return total;
}

int TownHall::GetTotalElixirFromStorages() const {
    int total = 0;
    for (const auto* storage : elixir_storages_) {
        if (storage && storage->IsActive()) {
            total += storage->GetCurrentAmount();
        }
    }
    return total;
}

int TownHall::GetTotalGoldCapacity() const {
    // 计算金币池的总容量
    int total = 0;
    for (const auto* storage : gold_storages_) {
        if (storage && storage->IsActive()) {
            total += storage->GetCapacity();
        }
    }
    return total;
}

int TownHall::GetTotalElixirCapacity() const {
    // 计算圣水池的总容量
    int total = 0;
    for (const auto* storage : elixir_storages_) {
        if (storage && storage->IsActive()) {
            total += storage->GetCapacity();
        }
    }
    return total;
}

void TownHall::UpdateMaxGoldCapacity() {
    // 计算金币池的总容量并更新大本营的金币容量
    max_gold_capacity_ = GetTotalGoldCapacity();

    // 同时更新UI
    if (level_label_) {
        UpdateLevelLabel();
    }

    cocos2d::log("更新金币持有上限: %d", max_gold_capacity_);
}

void TownHall::UpdateMaxElixirCapacity() {
    // 计算圣水池的总容量并更新大本营的圣水容量
    max_elixir_capacity_ = GetTotalElixirCapacity();

    // 同时更新UI
    if (level_label_) {
        UpdateLevelLabel();
    }

    cocos2d::log("更新圣水持有上限: %d", max_elixir_capacity_);
}

void TownHall::UpdateAllResourceCapacities() {
    UpdateMaxGoldCapacity();
    UpdateMaxElixirCapacity();
}

bool TownHall::IsGoldFull() const {
    int total_gold = gold_ + GetTotalGoldFromStorages();
    int total_capacity = GetTotalGoldCapacity();
    return total_gold >= total_capacity;
}

bool TownHall::IsElixirFull() const {
    int total_elixir = elixir_ + GetTotalElixirFromStorages();
    int total_capacity = GetTotalElixirCapacity();
    return total_elixir >= total_capacity;
}

void TownHall::UpdateLevelLabel() {
    // 移除旧的标签
    if (level_label_) {
        level_label_->removeFromParent();
    }

    // 创建新的等级标签
    level_label_ = Label::createWithTTF("Lv." + std::to_string(level_), "fonts/Marker Felt.ttf", 20);
    if (level_label_) {
        level_label_->setTextColor(Color4B::YELLOW);
        level_label_->enableOutline(Color4B::BLACK, 2);
        level_label_->setPosition(Vec2(0, this->getContentSize().height / 2 + 10));
        this->addChild(level_label_, 10);
    }

    // 如果有旗帜精灵，也更新它的位置
    if (flag_sprite_) {
        flag_sprite_->setPosition(Vec2(0, this->getContentSize().height / 2 + 40));
    }
}

void TownHall::PlayUpgradeEffect() {
    // 播放粒子特效
    auto particles = ParticleSystemQuad::create("particles/upgrade.plist");
    if (particles) {
        particles->setPosition(Vec2::ZERO);
        particles->setAutoRemoveOnFinish(true);
        this->addChild(particles);
    }

    // 播放缩放动画
    auto scale_up = ScaleTo::create(0.3f, 1.2f);
    auto scale_down = ScaleTo::create(0.3f, 1.0f);
    auto sequence = Sequence::create(scale_up, scale_down, nullptr);
    this->runAction(sequence);

    // 播放颜色闪烁
    auto tint_gold = TintTo::create(0.2f, 255, 215, 0);  // 金色
    auto tint_purple = TintTo::create(0.2f, 147, 112, 219);  // 紫色
    auto tint_normal = TintTo::create(0.2f, 255, 255, 255);  // 白色
    auto tint_sequence = Sequence::create(tint_gold, tint_purple, tint_normal, nullptr);
    this->runAction(tint_sequence);

    // 播放音效(如果有的话)
    // CocosDenshion::SimpleAudioEngine::getInstance()->playEffect("sounds/upgrade.wav");

    cocos2d::log("%s 升级特效播放完成", name_.c_str());
}

void TownHall::PlayDestroyedEffect() {
    // 播放震动效果
    auto shake_right = MoveBy::create(0.05f, Vec2(10, 0));
    auto shake_left = MoveBy::create(0.05f, Vec2(-20, 0));
    auto shake_back = MoveBy::create(0.05f, Vec2(10, 0));
    auto shake_sequence = Sequence::create(shake_right, shake_left, shake_back, nullptr);
    this->runAction(shake_sequence);

    // 播放渐隐效果
    auto fade_out = FadeOut::create(1.0f);
    this->runAction(fade_out);

    // 播放音效(如果有的话)
    // CocosDenshion::SimpleAudioEngine::getInstance()->playEffect("sounds/destroyed.wav");

    cocos2d::log("%s 被摧毁，游戏结束", name_.c_str());
}

void TownHall::ShowInfo() const {
    // 调用基类显示基本信息
    Building::ShowInfo();

    // 显示大本营特有信息
    cocos2d::log("=== 大本营信息 ===");
    cocos2d::log("金币池数量: %zu/%d", gold_storages_.size(), gold_storage_capacity_);
    cocos2d::log("圣水池数量: %zu/%d", elixir_storages_.size(), elixir_storage_capacity_);
    cocos2d::log("金矿数量: %zu/%d", gold_mines_.size(), gold_mine_capacity_);
    cocos2d::log("圣水收集器数量: %zu/%d", elixir_collectors_.size(), elixir_collector_capacity_);
    cocos2d::log("训练营数量: %zu/%d", barracks_.size(), barrack_capacity_);
    cocos2d::log("军营数量: %zu", all_barracks_.size());
    cocos2d::log("大本营金币: %d/%d", gold_, max_gold_capacity_);
    cocos2d::log("大本营圣水: %d/%d", elixir_, max_elixir_capacity_);
    cocos2d::log("总金币: %d/%d (%.1f%%)",
        gold_ + GetTotalGoldFromStorages(),
        GetTotalGoldCapacity(),
        GetTotalGoldCapacity() > 0 ?
        (gold_ + GetTotalGoldFromStorages()) * 100.0f / GetTotalGoldCapacity() : 0);
    cocos2d::log("总圣水: %d/%d (%.1f%%)",
        elixir_ + GetTotalElixirFromStorages(),
        GetTotalElixirCapacity(),
        GetTotalElixirCapacity() > 0 ?
        (elixir_ + GetTotalElixirFromStorages()) * 100.0f / GetTotalElixirCapacity() : 0);
    cocos2d::log("军队容量: %d/%d (%.1f%%)",
        current_army_count_,
        army_capacity_,
        army_capacity_ > 0 ? current_army_count_ * 100.0f / army_capacity_ : 0);
    cocos2d::log("城墙数量: %d/%d", GetCurrentWallCount(), wall_capacity_);
    cocos2d::log("金币已满: %s", IsGoldFull() ? "是" : "否");
    cocos2d::log("圣水已满: %s", IsElixirFull() ? "是" : "否");

    // 显示生产信息
    if (GetTotalGoldMineCount() > 0) {
        cocos2d::log("金矿总产量: %d/小时", GetTotalGoldProduction());
    }
    if (GetTotalElixirCollectorCount() > 0) {
        cocos2d::log("圣水收集器总产量: %d/小时", GetTotalElixirProduction());
    }
}

std::vector<TownHall::BuildingTemplate> TownHall::GetAllBuildingTemplates() {
    std::vector<TownHall::BuildingTemplate> templates;

    templates.emplace_back(
            "TownHall",
            "buildings/TownHall1.png",
            200,  // 成本
            4,    // 宽度
            4,    // 长度
            []() -> Building* {
                return TownHall::Create("TownHall", 1, { 0, 0 }, "buildings/TownHall1.png");
            }
    );
    // 金矿
    templates.emplace_back(
        "Gold Mine",
        "buildings/goldmine.png",
        150,  // 成本
        3,    // 宽度
        3,    // 长度
        []() -> Building* {
            return SourceBuilding::Create("Gold Mine", 15, { 0, 0 }, "buildings/goldmine.png", "Gold");
        }
    );

    // 圣水收集器
    templates.emplace_back(
        "Elixir Collector",
        "buildings/elixirmine0.png",
        150,
        3,
        3,
        []() -> Building* {
            return SourceBuilding::Create("Elixir Collector", 15, { 0, 0 }, "buildings/elixirmine0.png", "Elixir");
        }
    );

    // 金币储罐
    templates.emplace_back(
        "Gold Storage",
        "buildings/goldpool1.png",
        300,
        3,
        3,
        []() -> Building* {
            return ProductionBuilding::Create("Gold Storage", 15, { 0, 0 }, "buildings/goldpool1.png", "Gold Storage");
        }
    );

    // 圣水储罐
    templates.emplace_back(
        "Elixir Storage",
        "buildings/elixirpool2.png",
        300,
        3,
        3,
        []() -> Building* {
            return ProductionBuilding::Create("Elixir Storage", 15, { 0, 0 }, "buildings/elixirpool2.png", "Elixir Storage");
        }
    );

    // 军营
    templates.emplace_back(
        "Barracks",
        "buildings/barrack.png",
        200,
        3,
        3,
        []() -> Building* {
            return TrainingBuilding::Create("Barrack", 15, { 0, 0 }, "buildings/barrack.png", 50, 2);
        }
    );

    // 训练营
    templates.emplace_back(
        "Training Camp",
        "buildings/trainingcamp.png",
        500,
        4,
        4,
        []() -> Building* {
            // 调用 TrainingBuilding 的 InitializeInstance 函数
            return TrainingBuilding::Create("Training Camp", 15, { 0, 0 },
                "buildings/trainingcamp.png", 10, 20);
        }
    );

    // 城墙
    templates.emplace_back(
        "Wall",
        "buildings/wall1.png",
        100,  // 建造成本
        1,    // 宽度
        1,    // 长度
        []() -> Building* {
            return WallBuilding::Create("Wall", 15, { 0, 0 }, "buildings/wall1.png");
        }
    );

    // 箭塔
    templates.emplace_back(
        "Archer Tower",
        "buildings/archertower.png",
        350,
        2,
        2,
        []() -> Building* {
            return AttackBuilding::Create("Archer Tower", 10, { 0, 0 }, "buildings/archertower.png", 1, 10, 10);
        }
    );

    // 加农炮
    templates.emplace_back(
        "Cannon",
        "buildings/cannon1.png",
        350,
        2,
        2,
        []() -> Building* {
            return AttackBuilding::Create("Cannon", 10, { 0, 0 }, "buildings/cannon1.png", 0.8, 7, 9);
        }
    );

    return templates;
}

std::vector<SoldierTemplate> TownHall::GetSoldierCategory() {
    std::vector<SoldierTemplate> soldiers;

    soldiers.emplace_back(
        SoldierType::kBarbarian,
        "Barbarian",
        "Soldiers/Barbarian/Barbarianwalkright1.png",
        1,    // 人口消耗
        25,   // 训练费用（金币）
        20,   // 训练时间（秒）
        []() -> Soldier* {
            // 使用 Soldier 构造函数创建野蛮人
            return new Soldier(SoldierType::kBarbarian, 25, 100, 1.0f, 0.4f, 1.0f);
        }
    );

    soldiers.emplace_back(
        SoldierType::kArcher,
        "Archer",
        "Soldiers/Archer/Archerattackdown1.png",
        1,
        50,
        25,
        []() -> Soldier* {
            // 使用 Soldier 构造函数创建弓箭手
            return new Soldier(SoldierType::kArcher, 22, 70, 0.8f, 3.5f, 1.0f);
        }
    );

    soldiers.emplace_back(
        SoldierType::kGiant,
        "Giant",
        "Soldiers/Giant/Giantattackdown1.png",
        2,
        500,
        120,
        []() -> Soldier* {
            // 使用 Soldier 构造函数创建巨人
            return new Soldier(SoldierType::kGiant, 30, 200, 0.6f, 1.0f, 2.0f);
        }
    );


    soldiers.emplace_back(
        SoldierType::kBomber,
        "Bomber",
        "Soldiers/Bomber/Bomberwalkdown1.png",
        1,
        1000,
        60,
        []() -> Soldier* {
            // 需要先在 SoldierType 枚举中添加 WallBreaker
            return new Soldier(SoldierType::kBomber, 19, 60, 1.2f, 0.4f, 1.0f);
        }
    );


    return soldiers;
}

std::vector<Soldier*> TownHall::GetAllTrainedSoldiers() const {
    std::vector<Soldier*> soldiers;
    // TODO: 遍历所有军营，收集已训练的士兵
    return soldiers;
}

int TownHall::GetTotalTrainedSoldierCount() const {
    int total = 0;
    for (const auto* barracks : all_barracks_) {
        // 需要 Barracks 类提供获取已训练士兵数量的方法
        // total += barracks->GetTrainedSoldierCount();
    }
    return total;
}

// 在 TownHall.cpp 文件的末尾，最后一个函数之后添加：

// ==================== 士兵模板管理函数实现 ====================

const std::vector<SoldierTemplate>& TownHall::GetSoldierTemplates() {
    return soldier_templates;
}

const SoldierTemplate* TownHall::GetSoldierTemplate(SoldierType type) {
    for (const auto& tmpl : soldier_templates) {
        if (tmpl.type_ == type) {
            return &tmpl;
        }
    }
    return nullptr;
}

const SoldierTemplate* TownHall::GetSoldierTemplate(const std::string& name) {
    for (const auto& tmpl : soldier_templates) {
        if (tmpl.name_ == name) {
            return &tmpl;
        }
    }
    return nullptr;
}

// ==================== 士兵添加函数实现 ====================

bool TownHall::AddTrainedSoldier(Soldier* soldier) {
    if (!soldier) {
        cocos2d::log("错误：尝试添加空的士兵指针");
        return false;
    }

    // 获取士兵模板以确定人口占用
    const SoldierTemplate* tmpl = GetSoldierTemplate(soldier->GetSoldierType());
    if (!tmpl) {
        cocos2d::log("错误：未找到士兵类型 %d 的模板", static_cast<int>(soldier->GetSoldierType()));
        delete soldier;
        return false;
    }

    // 检查军队容量
    if (!CanAddSoldier(tmpl->housing_space_)) {
        cocos2d::log("军队容量不足，无法添加士兵 %s", soldier->GetName().c_str());
        delete soldier;
        return false;
    }

    // 更新军队人数（人口占用）
    UpdateArmyCount(tmpl->housing_space_);

    // 存储士兵指针（如果需要后续使用）
    // 注意：这里根据你的需求决定是否存储士兵对象
    // 如果只需要计数，可以删除士兵对象
    delete soldier; // 先删除，如果后续需要存储，注释掉这行并添加到容器

    cocos2d::log("士兵 %s 已成功添加到军队", tmpl->name_.c_str());
    return true;
}

bool TownHall::CanAddSoldier(int housing_space) const {
    return (current_army_count_ + housing_space) <= army_capacity_;
}