#include "States/TitleState.h"
#include "Game/Game.h"
#include "States/OverworldState.h"
#include "Manager/AudioManager.h"
#include "Game/GlobalContext.h"
#include <memory>
#include <iostream>


TitleState::TitleState(Game& game)
    : BaseState(game),
    m_font("assets/font/Common.ttf"), // 假设有一个字体文件
    m_titleText(m_font),
    m_backgroundTexture("assets/sprite/logo.png"),
    m_backgroundSprite(m_backgroundTexture),
    m_backgroundMusic("assets/music/whu.wav"),
    m_soulTexture("assets/sprite/Heart/spr_heart_0.png"),
    m_soulSprite(m_soulTexture)
    //ralseiFaceSprite(game.ralseiFaceTexture)  
{
    // 初始化标题界面元素
    m_titleText.setString("WHUDR Title Screen");
    m_titleText.setCharacterSize(24);

    //设置背景精灵属性
    m_backgroundSprite.setPosition({50.f, 100.f});
    m_backgroundSprite.setScale({0.2f, 0.2f});
    m_backgroundSprite.setColor(sf::Color(0, 65, 172, 200)); // green with some transparency
    // 像素风格：关闭纹理平滑
    m_backgroundTexture.setSmooth(false);

    //初始化菜单文字
    // SFML 3 注意：std::vector 需要用 emplace_back 直接构造 Text 对象
    for (size_t i = 0; i < STR_OPTIONS.size(); ++i) {
        // 参数：字体, 内容, 字号
        m_menuOptions.emplace_back(m_font, STR_OPTIONS[i], 30);
        
        // 设置位置 (居中简单算法)
        sf::FloatRect bounds = m_menuOptions.back().getLocalBounds();
        m_menuOptions.back().setOrigin({bounds.size.x / 2.f, bounds.size.y / 2.f});
        m_menuOptions.back().setPosition({300.f,300.f + i * 50.f}); 
    }
    
    // 3. 设置红心属性
    m_soulSprite.setColor(sf::Color::Red);
    m_soulSprite.setScale({1.5f, 1.5f}); // 如果图太大就缩小点
    m_soulTexture.setSmooth(false);

    // 初始化颜色
    updateTextColors();

    // 播放背景音乐
    m_backgroundMusic.setLooping(true);
    m_backgroundMusic.setVolume(200.f); // 设置适当的音量
    m_backgroundMusic.play();

    // 使用 Game.cpp 已预加载的对话音效键 "text"
    m_dialogueBox.start(L"欢迎来到WHUDR!\n按 Z 或 Enter 开始游戏。", &game.ralseiFaceTexture, std::make_optional<std::string>("textralsei"));

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
            }
        }
    } else {
        // 处理菜单输入
        if (InputManager::isPressed(Action::Up, m_game.getWindow())) {
            AudioManager::getInstance().playSound("button_move");
            m_selectIndex = (m_selectIndex - 1 + m_menuOptions.size()) % m_menuOptions.size();
            updateTextColors();
        }
        if (InputManager::isPressed(Action::Down, m_game.getWindow())) {
            AudioManager::getInstance().playSound("button_move");
            m_selectIndex = (m_selectIndex + 1) % m_menuOptions.size();
            updateTextColors();
        }
        if (InputManager::isPressed(Action::Confirm, m_game.getWindow())) {
            AudioManager::getInstance().playSound("button_select");
            // 根据选择的菜单项执行操作
            if (STR_OPTIONS[m_selectIndex] == L"新的游戏") {
                // 开始新游戏
                // 进入新游戏时明确设定房间为 AlphysClass，避免默认 "Title" 造成 Overworld 地图为空
                Global::currentMRoomName = "AlphysClass";
                m_game.changeState(std::make_unique<OverworldState>(m_game));
            } else if (STR_OPTIONS[m_selectIndex] == L"继续游戏") {
                // 继续游戏（加载存档）
                SaveManager::loadGame(0);
                // 这里可以添加加载存档的逻辑
                m_game.changeState(std::make_unique<OverworldState>(m_game));
            } else if (STR_OPTIONS[m_selectIndex] == L"退出游戏") {
                // 退出游戏
                m_game.getWindow().close();
            }
        }
    }

    // if (statefinished) {
    //     //切换到下一个状态:OverworldState
    //     if (InputManager::isPressed(Action::Confirm, m_game.getWindow())) {
    //         m_game.changeState(std::make_unique<OverworldState>(m_game));
    //     }
    // }
}

void TitleState::update(float dt) {
    // 更新标题界面的逻辑
    m_dialogueBox.update(dt);
    // 清理播放完的音效，避免积累
    AudioManager::getInstance().update();
}

void TitleState::draw(sf::RenderWindow& window) {
    // 绘制标题界面
    window.draw(m_backgroundSprite);
    window.draw(m_titleText);

    // 先画菜单层
    for (const auto& option : m_menuOptions) {
        window.draw(option);
    }
    sf::Vector2f textPos = m_menuOptions[m_selectIndex].getPosition();
    m_soulSprite.setPosition({textPos.x - 100.f, textPos.y - 8.f});
    window.draw(m_soulSprite);

    // 最后绘制对话框，保证在最上层
    m_dialogueBox.draw(window);
}

void TitleState::updateTextColors() {
for (size_t i = 0; i < m_menuOptions.size(); ++i) {
        if (i == m_selectIndex) {
            m_menuOptions[i].setFillColor(sf::Color::Yellow); // 选中的是黄色
        } else {
            m_menuOptions[i].setFillColor(sf::Color::White);  // 没选中是白色
        }
    }
}