/*
标题界面（选择开始游戏、退出、载入存档）。
*/
#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "States/BaseState.h"
#include "Manager/InputManager.h"
#include "UI/DialogBox.h"

class TitleState : public BaseState {
private:
    // 私有成员变量（如果有的话）
    sf::Font m_font;
    sf::Text m_titleText;
    sf::Texture m_backgroundTexture;
    sf::Sprite m_backgroundSprite;
    sf::Music m_backgroundMusic;

    DialogueBox m_dialogueBox; // 对话框组件
    // sf::Texture m_faceTexture; // 头像纹理
    // sf::SoundBuffer m_voiceBuffer; // 语音音效缓冲

public:
    TitleState(Game& game);
    virtual void handleEvent() override;
    virtual void update(float dt) override;
    virtual void draw(sf::RenderWindow& window) override;
};