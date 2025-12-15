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
#include "Database.h"

//全局变量
struct Global
{
    //基础数据
    static inline std::string playerName = "player";      //玩家名称
    static inline std::string currentMRoomName = "Title"; //当前地图名称    
    static inline int money = 0;                          //钱
    static inline std::vector<std::string> inventory;   //物品
};

