#pragma once
#include <SFML/Graphics.hpp>

// 前向声明：告诉编译器 Game 类是存在的，但具体细节之后再说
// 避免 "循环引用" (Game 引用 State, State 引用 Game)
class Game; 

class BaseState {
protected:
    Game& m_game; // 持有游戏主程序的引用，方便切换场景

public:
    // 构造函数：必须传入 Game 的引用
    BaseState(Game& game) : m_game(game) {}
    
    virtual ~BaseState() = default;

    // 三大核心虚函数
    //virtual void handleEvent(const sf::Event& event) = 0; // 处理输入事件
    virtual void handleEvent() = 0;       // 处理输入
    virtual void update(float dt) = 0;    // 更新逻辑 (dt = delta time)
    virtual void draw(sf::RenderWindow& window) = 0; // 绘制画面
};