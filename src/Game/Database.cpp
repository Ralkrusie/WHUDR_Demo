#include "Database.h"


std::map<std::string, Hero> Database::heroes;
std::map<std::string, Weapon> Database::weapons;
std::map<std::string, Armor> Database::armors;
std::map<std::string, HellingItem> Database::hellingItems;


void Database::init() {
    // 初始化武器数据
    weapons["Null"] = {"Null", L"No Weapon", 0}; // 保持 Null 项不翻译
    weapons["wooden_sword"] = {"wooden_sword", L"木剑", 15};
    weapons["axe"] = {"axe", L"战斧", 20};
    weapons["scarf"] = {"scarf", L"柔软围巾", 10};

    // 初始化护甲数据
    armors["Null"] = {"Null", L"No Armor", 0}; // 保持 Null 项不翻译
    armors["whu_shirt"] = {"whu_shirt", L"武汉大学校服", 2};
    armors["holy_mantle"] = {"holy_mantle", L"神圣斗篷", 5};

    // 初始化治疗物品数据
    hellingItems["potion"] = {"potion", L"小型治疗药水", 50};
    hellingItems["luojia_drink"] = {"luojia_drink", L"珞珈饮品", 80};
    
    // 初始化英雄数据
    heroes["kris"] = { 
        "kris", "Kris", 
        100,  // HP
        10,  // Atk
        2,   // Def
        0,   // Magic
        "wooden_sword", 
        {
            "whu_shirt", 
            "Null"
        }
    };
    heroes["susie"] = { 
        "susie", "Susie", 
        120,  // HP
        20,   // Atk
        5,    // Def
        0,    // Magic
        "axe", 
        {
            "whu_shirt", 
            "Null"
        }
    };
    heroes["ralsei"] = { 
        "ralsei", "Ralsei", 
        80,   // HP
        5,    // Atk
        3,    // Def
        15,   // Magic
        "scarf", 
        {
            "whu_shirt", 
            "Null"
        }
    };
}


