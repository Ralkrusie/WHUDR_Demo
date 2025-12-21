#include "SaveManager.h"
#include "GlobalContext.h"
#include "Database.h"
#include "Manager/AudioManager.h"
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <json.hpp> // 引入头文件
// 为 sf::String 与 UTF-8 互转
#include <SFML/System/String.hpp>
#include <SFML/System/Utf.hpp>

using json = nlohmann::json; //以此简化代码，不用每次都写很长
namespace fs = std::filesystem;

namespace {
InventoryItem makeInventoryItemFromId(const std::string& id) {
    InventoryItem item;
    item.id = id;
    if (id == "HolyMantle") {
        item.name = sf::String(L"神圣斗篷");
        item.info = sf::String(L"免疫每回合第一次受伤。");
    } else if (id == "luojia_drink") {
        item.name = sf::String(L"珞珈饮品");
        item.info = sf::String(L"恢复80点生命值。");
    } else if (id == "potion") {
        item.name = sf::String(L"小型治疗药水");
        item.info = sf::String(L"恢复50点生命值。");
    } else {
        item.name = sf::String::fromUtf8(id.begin(), id.end());
        item.info = sf::String();
    }
    return item;
}
}

// 获取存档文件路径的辅助函数
std::string SaveManager::getFilePath(int slotId) {
    return "savedata/file_" + std::to_string(slotId) + ".json"; // 改后缀为 .json
}

// ########################################## 保存游戏 ##########################################
void SaveManager::saveGame(int slotId) {
    AudioManager::getInstance().playSound("save");

    if (!fs::exists("savedata")) fs::create_directory("savedata");

    // -----------------------------1. 创建一个空的 json 对象-----------------------------
    json j;

    // -----------------------------2. 像操作 map 一样往里填数据-----------------------------
    j["player_name"] = Global::playerName;
    j["current_room"] = Global::currentMRoomName;
    j["money"] = Global::money;
    // 序列化物品（仅存 id）
    json invArr = json::array();
    for (const auto& it : Global::inventory) {
        invArr.push_back(it.id);
    }
    j["inventory"] = invArr;
    j["has_holy_mantle"] = Global::hasHolyMantle;

    // 序列化三人实际数据（party）
    json partyArr = json::array();
    for (const auto& h : Global::partyHeroes) {
        std::string nameUtf8;
        nameUtf8.reserve(h.name.getSize());
        sf::Utf<32>::toUtf8(h.name.begin(), h.name.end(), std::back_inserter(nameUtf8));
        json one = json::object();
        one["id"] = h.id;
        one["name"] = nameUtf8;
        one["max_hp"] = h.maxHP;
        one["hp"] = h.hp;
        one["atk"] = h.baseAttack;
        one["def"] = h.baseDefense;
        one["mag"] = h.baseMagic;
        one["weapon_id"] = h.weaponID;
        one["armor_id"] = { h.armorID[0], h.armorID[1] };
        partyArr.push_back(std::move(one));
    }
    j["party"] = std::move(partyArr);

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
    AudioManager::getInstance().playSound("save");

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
        Global::inventory.clear();

        if (j.contains("inventory") && j["inventory"].is_array()) {
            const auto& invJ = j["inventory"];
            for (const auto& el : invJ) {
                if (!el.is_string()) continue;
                std::string id = el.get<std::string>();
                if (id.empty()) continue;
                Global::inventory.push_back(makeInventoryItemFromId(id));
            }
        }

        Global::hasHolyMantle = j.value("has_holy_mantle", false);

        // 载入三人实际数据（若存在）
        Global::partyHeroes.clear();
        if (j.contains("party") && j["party"].is_array()) {
            for (const auto& el : j["party"]) {
                if (!el.is_object()) continue;
                HeroRuntime rt;
                rt.id = el.value("id", std::string(""));
                {
                    std::string nameUtf8 = el.value("name", rt.id);
                    rt.name = sf::String::fromUtf8(nameUtf8.begin(), nameUtf8.end());
                }
                rt.maxHP = el.value("max_hp", 1);
                rt.hp = el.value("hp", rt.maxHP);
                rt.baseAttack = el.value("atk", 0);
                rt.baseDefense = el.value("def", 0);
                rt.baseMagic = el.value("mag", 0);
                rt.weaponID = el.value("weapon_id", std::string("Null"));
                if (el.contains("armor_id") && el["armor_id"].is_array() && el["armor_id"].size() >= 2) {
                    rt.armorID[0] = el["armor_id"][0].get<std::string>();
                    rt.armorID[1] = el["armor_id"][1].get<std::string>();
                } else {
                    rt.armorID[0] = "Null";
                    rt.armorID[1] = "Null";
                }
                if (!rt.id.empty()) Global::partyHeroes.push_back(std::move(rt));
            }
        }

        
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
            info.roomName = j.value("current_room", "Start");
            file.close();
        } catch (...) {
            info.exists = false;
        }
    }
    return info;
}

// 音效由 Game.cpp 在 AudioManager 中统一预加载与维护