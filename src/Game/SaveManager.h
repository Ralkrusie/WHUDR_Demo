/*
管理存档系统。
包含：

保存队伍数据

读取游戏进度
*/

#pragma once
#include <string>
#include <vector>

// 定义一个存档数据的结构体 (仅用于UI显示预览，比如显示在存档菜单上)
struct SavePreview {
    bool exists = false;
    std::string name;
    std::string roomName;
};

class SaveManager {
public:

    // 保存游戏 (传入存档槽位，比如 0, 1, 2)
    static void saveGame(int slotId);

    // 加载游戏
    static bool loadGame(int slotId);

    // 获取存档预览信息 (用于在标题画面显示 "File 1: Kris - LV1")
    static SavePreview getSavePreview(int slotId);

private:
    // 获取文件路径的辅助函数
    static std::string getFilePath(int slotId);
};