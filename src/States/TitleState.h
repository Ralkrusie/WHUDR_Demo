// 标题界面（选择开始游戏、退出、载入存档）
#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include "States/BaseState.h"
#include "Manager/InputManager.h"
#include "UI/DialogBox.h"
#include "Game/SaveManager.h"

class TitleState : public BaseState {
private:
    sf::Font m_font;
    sf::Text m_titleText;
    sf::Texture m_backgroundTexture;
    sf::Sprite m_backgroundSprite;
    sf::Music m_backgroundMusic;

    DialogueBox m_dialogueBox; // 对话框组件
    // 菜单项
    std::vector<sf::Text> m_menuOptions;
    int m_selectIndex = 0; // 当前选中的是第几个 (0, 1, 2)
    //sf::Sprite ralseiFaceSprite; // Ralsei 头像


    
    // 我们可以把选项名字写死在这里
    const std::vector<sf::String> STR_OPTIONS = {
        L"新的游戏",
        L"继续游戏",
        L"退出游戏"
    };
    
    // 选中的时候显示的红心（Deltarune 风格）
    sf::Texture m_soulTexture;
    sf::Sprite m_soulSprite;

    // 辅助函数：更新文字颜色
    void updateTextColors();

public:
    TitleState(Game& game);
    virtual void handleEvent() override;
    virtual void update(float dt) override;
    virtual void draw(sf::RenderWindow& window) override;
};