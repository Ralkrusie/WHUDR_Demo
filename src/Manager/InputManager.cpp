#include "InputManager.h"
#include <array>

//
// 输入管理（InputManager）
// ----------------------
// 职责：
// - 将逻辑动作 `Action` 映射到实际按键（支持多按键）
// - 提供两类查询：
//   * `isHeld()`：按住状态（用于持续移动等）
//   * `isPressed()`：单次按下（边沿触发，用于菜单导航/确认等）
// - 处理窗口焦点丢失与按键重复：
//   * 失焦清空上一帧状态，避免重新激活时误触发
//   * `isPressed()` 禁用重复键；`isHeld()` 启用重复键
// 约定：
// - `Action` 的枚举值用于索引状态数组，范围以 `Action::Debug` 为最大值
//

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

    // 记录上一帧各 Action 的按下状态（用于边沿检测 isPressed）
    std::array<bool, static_cast<std::size_t>(Action::Debug) + 1> s_prevPressed{};
}

// 检测是否【按住】（用于移动等持续行为）
// 启用重复键：按住时连续为 true
bool InputManager::isHeld(Action action, sf::RenderWindow& window) {
    window.setKeyRepeatEnabled(true);
    return isKeyDown(action);
}

// 检测是否【单次按下】（边沿触发：本帧为 down 且上帧为 up）
// 禁用重复键：避免一帧多次触发；失焦时清空历史，防止误触发
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