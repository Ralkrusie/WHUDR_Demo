#include "OverworldState.h"
#include <iostream>

OverworldState::OverworldState(Game& game)
    : BaseState(game),
    m_font("assets/font/Common.ttf"),
    m_backgroundTexture("assets/sprite/Room/room_alphysclass.png"),
    m_backgroundSprite(m_backgroundTexture),
    m_backgroundMusic("assets/music/Choral_Chambers.mp3")
{
    // 初始化探索状态的元素

    if (!m_font.openFromFile("assets/font/Common.ttf")) {
        // 处理字体加载失败
        std::cerr << "Failed to load font!" << std::endl;
    }

    if (!m_backgroundTexture.loadFromFile("assets/sprite/Room/room_alphysclass.png")) {
        // 处理背景纹理加载失败
        std::cerr << "Failed to load overworld background texture!" << std::endl;
    }

    if (!m_backgroundMusic.openFromFile("assets/music/Choral_Chambers.mp3")) {
        // 处理音乐加载失败
        std::cerr << "Failed to load overworld background music!" << std::endl;
    }

    m_backgroundSprite.setPosition({0.f, 0.f});
    m_backgroundSprite.setScale({2.0f, 2.0f});

    m_backgroundMusic.setLooping(true);
    m_backgroundMusic.play();
    
}

void OverworldState::handleEvent() {
    // 处理探索状态的输入事件
    // 例如：移动角色、与物体交互等
}

void OverworldState::update(float dt) {
    // 更新探索状态的逻辑
    // 例如：角色位置更新、碰撞检测等
}

void OverworldState::draw(sf::RenderWindow& window) {
    // 绘制探索状态的画面
    window.draw(m_backgroundSprite);
    // 绘制其他游戏元素，如角色、物体等
}

