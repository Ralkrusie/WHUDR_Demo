#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <stack>
#include <memory> // 用于 std::unique_ptr
#include "States/BaseState.h"

class Game {
private:
    sf::RenderWindow m_window;
    sf::View m_view; // 固定 640x480 的游戏视图（用于 Letterboxing）
    // 使用栈来管理状态：栈顶就是当前显示的画面
    std::stack<std::unique_ptr<BaseState>> m_states;

    // 根据窗口大小更新视口以实现 4:3 信箱黑边
    void updateViewViewport(unsigned int winW, unsigned int winH);

public:
    Game();  // 构造函数初始化窗口
    ~Game();

    void run();  // 主循环

    // 状态管理函数
    void pushState(std::unique_ptr<BaseState> state);
    void popState();
    void changeState(std::unique_ptr<BaseState> state);  // 替换当前状态

    sf::RenderWindow& getWindow();  // 让 State 能获取窗口来画图
    sf::Texture ralseiFaceTexture; // 预加载的 Ralsei 头像纹理
    sf::Texture susieFaceTexture;  // 预加载的 Susie 头像纹理
};