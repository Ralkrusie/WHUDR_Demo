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
#include <deque>
#include <vector>
#include "States/BaseState.h"
#include "Overworld/Map.h"
#include "UI/DialogBox.h"
#include "Overworld/OverworldCharacter.h"
#include "Overworld/AlphysClass.h"
#include "Overworld/SecretRoom.h"

class OverworldState : public BaseState {
private:
    // 私有成员变量（如果有的话）
    sf::Font m_font;
    sf::Texture m_backgroundTexture;
    sf::Sprite m_backgroundSprite;
    sf::Music m_backgroundMusic;
    GameMap m_map; // 地图数据与绘制
    bool m_isInputLocked = false; // 输入锁定（对话时锁定）
    DialogueBox m_dialogueBox; // 对话框组件

    OverworldCharacter m_kris;
    OverworldCharacter m_ralsei;
    OverworldCharacter m_susie;

    std::vector<OverworldCharacter*> m_party; // 动态队伍，索引 0..n-1
    int m_leaderIndex = 0;                    // 当前队长在 m_party 中的索引
    std::deque<PositionRecord> m_leaderHistory; // 队长移动记录
    const std::size_t m_maxHistory = 300;       // 限制历史长度
    std::string m_currentRoom;
    bool m_debugDrawEnabled = true;              // 调试矩形开关

    // 物品栏 UI 状态
    bool m_inventoryOpen = false;
    int m_itemCursor = 0;
    int m_actionCursor = 0; // 0: 使用, 1: 丢弃
    bool m_selectingAction = false;
    std::optional<sf::Texture> m_inventoryHeartTex;
    std::optional<sf::Sprite> m_inventoryHeart;

    // 房间切换渐变
    enum class FadePhase { None, Out, In };
    FadePhase m_fadePhase = FadePhase::None;
    float m_fadeTimer = 0.f;
    float m_fadeDuration = 0.18f;
    float m_fadeAlpha = 0.f; // 0..1
    bool m_hasPendingWarp = false;
    std::string m_pendingRoom;
    sf::Vector2f m_pendingSpawn{0.f, 0.f};
    // 对话完成后的待处理动作（例如存档确认、拾取道具）
    enum class PendingAction { None, SavePrompt, CollectHolyMantle, StartBattleCalculus };
    PendingAction m_pendingAction = PendingAction::None;
    
public:
    OverworldState(Game& game);
    virtual void handleEvent() override;
    virtual void update(float dt) override;
    virtual void draw(sf::RenderWindow& window) override;
    void checkInteraction(); // 检查交互
    void loadRoom(const std::string& roomName, const sf::Vector2f& playerPos);
    void openInventory();
    void closeInventory();
    void handleInventoryInput();
    void drawInventory(sf::RenderWindow& window);
    void startFadeToRoom(const std::string& roomName, const sf::Vector2f& spawn);
    void updateFade(float dt);
    void drawFade(sf::RenderWindow& window);

private:
    OverworldCharacter& getLeader();
};