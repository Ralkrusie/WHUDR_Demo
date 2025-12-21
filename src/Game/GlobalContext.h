/*
游戏全局状态存储中心（类似单例）。
包含：

当前状态（Title / Overworld / Battle）

玩家队伍数据（Hero 结构体）

背景音乐音量

存档位置等
*/

#pragma once
#include <string>
#include <vector>
#include <array>
#include <SFML/System/String.hpp>
#include "Database.h"

struct InventoryItem {
    std::string id;
    sf::String name;
    sf::String info;
};

// 运行时英雄数据（会参与存档）
struct HeroRuntime {
    std::string id;            // 数据库主键
    sf::String name;           // 显示名（允许中文）
    int maxHP;
    int hp;                    // 当前生命值
    int baseAttack;
    int baseDefense;
    int baseMagic;
    std::string weaponID;      // 装备的武器 ID
    std::array<std::string, 2> armorID; // 两个护甲位的 ID
    bool defending = false;    // 当前回合是否处于防御状态
};

//全局变量
struct Global
{
    //基础数据
    static inline std::string playerName = "player";      //玩家名称
    static inline std::string currentMRoomName = "Title"; //当前地图名称    
    static inline int money = 0;                          //钱
    static inline std::vector<InventoryItem> inventory;   //物品（含描述）
    static inline bool hasHolyMantle = false;           // 是否已获得 HolyMantle
    static inline std::vector<HeroRuntime> partyHeroes; // 三人实际数据（会存档）
};

