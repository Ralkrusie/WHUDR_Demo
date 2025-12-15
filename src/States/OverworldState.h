/*
探索状态（地图游玩）。
包含：

地图切换

撞墙

进入战斗或进入教室等
*/

#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "States/BaseState.h"

class OverworldState : public BaseState {
private:
    // 私有成员变量（如果有的话）
    sf::Font m_font;
    sf::Texture m_backgroundTexture;
    sf::Sprite m_backgroundSprite;
    sf::Music m_backgroundMusic;
    
public:
    OverworldState(Game& game);
    virtual void handleEvent() override;
    virtual void update(float dt) override;
    virtual void draw(sf::RenderWindow& window) override;
};