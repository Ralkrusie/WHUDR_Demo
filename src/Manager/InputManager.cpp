#include "InputManager.h"
#include <array>

namespace {
    // 将 Action 映射到数组下标
    std::size_t idx(Action action) {
        return static_cast<std::size_t>(action);
    }

    // 判断某个 Action 当前是否按下（支持多按键映射）
    bool isKeyDown(Action action) {
        switch (action) {
            case Action::Up:       return sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up);
            case Action::Down:     return sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down);
            case Action::Left:     return sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left);
            case Action::Right:    return sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right);
            case Action::Confirm:  return sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Z)     || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Enter);
            case Action::Cancel:   return sf::Keyboard::isKeyPressed(sf::Keyboard::Key::X)     || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift);
            case Action::Menu:     return sf::Keyboard::isKeyPressed(sf::Keyboard::Key::C)     || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl);
            case Action::Debug:    return sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);
            default: return false;
        }
    }

    // 记录上一帧各 Action 的按下状态，用于边沿检测
    std::array<bool, static_cast<std::size_t>(Action::Debug) + 1> s_prevPressed{};
}

// 检测是否【按住】（用于移动）
bool InputManager::isHeld(Action action, sf::RenderWindow& window) {
    window.setKeyRepeatEnabled(true);
    return isKeyDown(action);
}

// 检测是否【单次按下】（用于菜单选择，防止一按跳好几格）
bool InputManager::isPressed(Action action, sf::RenderWindow& window) {
    window.setKeyRepeatEnabled(false);

    // 失焦时清空状态，避免重新激活后误触发
    if (!window.hasFocus()) {
        s_prevPressed.fill(false);
        return false;
    }

    const bool now = isKeyDown(action);
    const std::size_t i = idx(action);
    const bool pressed = now && !s_prevPressed[i];
    s_prevPressed[i] = now;
    return pressed;
}