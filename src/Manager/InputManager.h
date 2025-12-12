/*
管理键盘输入。
包含：

当前帧按下/释放

菜单、移动、战斗输入统一处理
*/
#pragma once
#include <SFML/Window.hpp>

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
    static bool isHeld(Action action) {
        switch (action) {
            case Action::Up:    return sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W);
            case Action::Down:  return sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S);
            case Action::Left:  return sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A);
            case Action::Right: return sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);
            case Action::Confirm: return sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Z) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Enter);
            case Action::Cancel:  return sf::Keyboard::isKeyPressed(sf::Keyboard::Key::X) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift);
            default: return false;
        }
    }

    // 检测是否【单次按下】（用于菜单选择，防止一按跳好几格）
    // 这通常需要配合 event loop 使用，这里简化演示
    static bool isJustPressed(sf::Keyboard::Key key, Action action) {
        // 这个通常在 pollEvent 的循环里调用
        if (action == Action::Confirm && (key == sf::Keyboard::Key::Z || key == sf::Keyboard::Key::Enter)) return true;
        // ... 其他映射
        return false;
    }
};