/*
管理键盘输入。
包含：

当前帧按下/释放

菜单、移动、战斗输入统一处理
*/
#pragma once
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

enum class Action {
    Up,
    Down,
    Left,
    Right,
    Confirm, // Z 或 Enter
    Cancel,  // X 或 Shift
    Menu     // C 或 Ctrl
};

class InputManager {
public:
    // 检测是否【按住】（用于移动）
    static bool isHeld(Action action, sf::RenderWindow& window);

    // 检测是否【单次按下】（用于菜单选择，防止一按跳好几格）
    static bool isPressed(Action action, sf::RenderWindow& window);
};