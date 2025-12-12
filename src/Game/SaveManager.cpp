#include "SaveManager.h"
#include "GlobalContext.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <json.hpp> // 引入头文件

using json = nlohmann::json; //以此简化代码，不用每次都写很长
namespace fs = std::filesystem;

// 获取存档文件路径的辅助函数
std::string SaveManager::getFilePath(int slotId) {
    return "savedata/file_" + std::to_string(slotId) + ".json"; // 改后缀为 .json
}

// ########################################## 保存游戏 ##########################################
void SaveManager::saveGame(int slotId) {
    if (!fs::exists("savedata")) fs::create_directory("savedata");

    // -----------------------------1. 创建一个空的 json 对象-----------------------------
    json j;

    // -----------------------------2. 像操作 map 一样往里填数据-----------------------------
    j["player_name"] = Global::playerName;
    j["current_room"] = Global::currentMRoomName;
    j["money"] = Global::money;
    j["inventory"] = Global::inventory;

    // -----------------------------3. 写入文件-----------------------------
    std::ofstream file(getFilePath(slotId));
    if (file.is_open()) {
        // dump(4) 的意思是缩进 4 个空格，这样生成的 JSON 很好看，方便人眼阅读
        file << j.dump(4); 
        file.close();
        std::cout << "Saved JSON to slot " << slotId << std::endl;
    }
}

//########################################### 加载游戏 ##########################################
bool SaveManager::loadGame(int slotId) {
    std::ifstream file(getFilePath(slotId));
    if (!file.is_open()) return false;

    try {
        // ------------------------------1. 解析文件到 json 对象------------------------------
        json j;
        file >> j; // 一行代码搞定读取

        // ------------------------------2. 将数据填回 GlobalContext------------------------------
        // 使用 .value("key", default_value) 可以防止 key 不存在时崩溃，非常安全！
        // 基础数据
        Global::playerName = j.value("player_name", "player");
        Global::currentMRoomName = j.value("current_room", "Start");
        Global::money = j.value("money", 0);
        Global::inventory = j.value("inventory", std::vector<std::string>{});

        
        file.close();
        std::cout << "Loaded JSON from slot " << slotId << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "JSON Parse Error: " << e.what() << std::endl;
        return false;
    }
}

// 获取预览信息
SavePreview SaveManager::getSavePreview(int slotId) {
    SavePreview info;
    std::ifstream file(getFilePath(slotId));
    if (file.is_open()) {
        try {
            json j;
            file >> j;
            info.exists = true;
            info.name = j.value("player_name", "player");
            info.roomName = j.value("current_room_name", "Start");
            file.close();
        } catch (...) {
            info.exists = false;
        }
    }
    return info;
}