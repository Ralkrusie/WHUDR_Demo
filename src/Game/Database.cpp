#include "Database.h"


std::map<std::string, Hero> Database::heroes;
std::map<std::string, Weapon> Database::weapons;
std::map<std::string, Armor> Database::armors;
std::map<std::string, HellingItem> Database::hellingItems;


void Database::init() {
    // 初始化武器数据
    weapons["Null"] = {"Null", "No Weapon", 0};
    weapons["wooden_sword"] = {"wooden_sword", "Wooden Sword", 15};
    weapons["axe"] = {"axe", "Battle Axe", 20};
    weapons["scarf"] = {"scarf", "Soft Scarf", 10};

    // 初始化护甲数据
    armors["Null"] = {"Null", "No Armor", 0};
    armors["whu_shirt"] = {"whu_shirt", "WHU Shirt", 2};
    armors["holy_mantle"] = {"holy_mantle", "Holy Mantle", 5};

    // 初始化治疗物品数据
    hellingItems["potion"] = {"potion", "Small Healing Potion", 50};
    
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


