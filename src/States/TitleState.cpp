#include "States/TitleState.h"
#include "Game/Game.h"
#include "States/OverworldState.h"
#include <memory>
#include <iostream>

static bool statefinished = false;

TitleState::TitleState(Game& game)
    : BaseState(game),
    m_font("assets/font/Common.ttf"), // 假设有一个字体文件
    m_titleText(m_font),
    m_backgroundTexture("assets/sprite/logo.png"),
    m_backgroundSprite(m_backgroundTexture),
    m_backgroundMusic("assets/music/whu.wav") 
{
    // 初始化标题界面元素
    m_titleText.setString("WHUDR Title Screen");
    m_titleText.setCharacterSize(48);

    //m_dialogueBox = DialogueBox();
    m_dialogueBox.start("Welcome to WHUDR!\nPress Z or Enter to start game!", nullptr, nullptr);

    //
    m_backgroundSprite.setPosition({50.f, 100.f});
    m_backgroundSprite.setScale({0.2f, 0.2f});
    m_backgroundSprite.setColor(sf::Color(0, 65, 172, 200)); // green with some transparency

    // 播放背景音乐
    m_backgroundMusic.setLooping(true);
    m_backgroundMusic.play();
}

void TitleState::handleEvent() {
    //test ispressed
    if (InputManager::isPressed(Action::Cancel, m_game.getWindow())) {
        std::cout << "Cancel button pressed in TitleState!" << std::endl;
    }
    if (m_dialogueBox.isActive()) {
        if (InputManager::isPressed(Action::Confirm, m_game.getWindow())) {
            bool finished = m_dialogueBox.onConfirm();
            if (finished) {
                // 对话结束，切换状态或进行其他操作
                std::cout << "Dialogue finished in TitleState!" << std::endl;
                statefinished = true;
            }
        }
    }

    if (statefinished) {
        //切换到下一个状态:OverworldState
        if (InputManager::isPressed(Action::Confirm, m_game.getWindow())) {
            m_game.changeState(std::make_unique<OverworldState>(m_game));
        }
    }
}

void TitleState::update(float dt) {
    // 更新标题界面的逻辑
    m_dialogueBox.update(dt);
}

void TitleState::draw(sf::RenderWindow& window) {
    // 绘制标题界面
    window.draw(m_backgroundSprite);
    window.draw(m_titleText);

    m_dialogueBox.draw(window);
}