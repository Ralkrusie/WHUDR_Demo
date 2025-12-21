#pragma once
#include <string>
#include <map>
#include <vector>
#include <SFML/System/String.hpp>




//定义装备结构体
struct Armor
{
    std::string id;
    sf::String name;
    int defense;
};

//定义武器结构体
struct Weapon
{
    std::string id;
    sf::String name;
    int attack;
};

//定义治疗物品结构体
struct HellingItem
{
    std::string id;
    sf::String name;
    int healAmount;
};

//定义小队成员结构体
struct Hero
{
    std::string id;
    sf::String name;
    int maxHP;
    int baseAttack;
    int baseDefense;
    int baseMagic;
    std::string weaponID;
    std::string armorID[2]; //两个装备栏位
};


class Database {
public:
    // 数据库中的静态成员
    static std::map<std::string, Hero> heroes;
    static std::map<std::string, Weapon> weapons;
    static std::map<std::string, Armor> armors;
    static std::map<std::string, HellingItem> hellingItems;

    // 初始化数据库（加载数据）
    static void init();
};

