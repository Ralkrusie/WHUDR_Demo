#pragma once
#include <SFML/Graphics.hpp>
#include <optional>
#include <string>

// 战斗阶段枚举
enum class BattlePhase {
    Intro,          // 战斗开始动画（敌人滑入，队伍就位）
    Selection,      // 玩家选择指令 (Kris -> Susie -> Ralsei)
    ActionExecute,  // 执行玩家指令 (攻击动画、回血、播放对话框文字)
    BulletHell,     // 躲避弹幕阶段 (核心玩法)
    TurnEnd,        // 回合结算 (Buff扣除，回合数+1)
    Victory,        // 胜利结算
    GameOver        // 失败
};

// 动作类型(注意：Act和Action是不同的概念)
enum class ActionType {
    Fight,      // 普通攻击
    Act,        // 行动(简化dr的行动和魔法)
    Item,       // 物品
    Spare,      // 饶恕
    Defend      // 防御
};

// 一个具体的技能/行动定义
struct ActData {
    sf::String name;       // 技能名 (e.g. "Rude Buster", "Check")
    sf::String description;// 描述
    int damage = 0;         // 伤害值 (0表示非攻击)
    int heal = 0;           // 治疗值
    float mercyAdd = 0.f;   // 增加多少饶恕度
};

// 玩家选完指令后生成的“命令包”
struct BattleCommand {
    int actorIndex;     // 谁在行动 (0:Kris, 1:Susie...)
    int targetIndex;    // 目标是谁
    ActionType type;    // 干什么
    std::optional<ActData> actData; // 如果是 Act，则包含具体数据
    std::optional<std::string> itemID; // 如果是 Item，则包含物品ID
};
