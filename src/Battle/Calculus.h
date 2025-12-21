/*
高数敌人的技能逻辑，例如：

求导攻击

泰勒攻击

不定积分攻击

生成不同模式的弹幕?
*/
#pragma once
#include <vector>
#include "Battle/Enemy.h"

// 生成 "高数题" 遭遇配置（目前只有一只）
std::vector<Enemy> makeCalculusEncounter();