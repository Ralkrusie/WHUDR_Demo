#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include <deque>
#include <optional>
#include <string>
#include "Manager/InputManager.h"
#include "Map.h"
#include "Game/Game.h"

// 记录每一帧的状态，用于"毛毛虫"跟随
struct PositionRecord {
    sf::Vector2f position;
    int direction;   // 0:下, 1:左, 2:右, 3:上
    bool isMoving;   // 是否在动（用于播放动画）
    bool isRunning;  // 是否在跑（用于播放快跑动画）
};

// 四个方向各 4 帧的贴图路径（按 Down, Left, Right, Up）
struct SpriteSet {
    std::array<std::array<std::string, 4>, 4> frames;
};

class OverworldCharacter {
public:
    OverworldCharacter(Game& game, const SpriteSet& spriteSet);

    // 队长逻辑：读取输入 -> 移动 -> 碰撞检测
    void updateLeader(float dt, GameMap& map);

    // 队员逻辑：读取历史 -> 设置位置（需要地图用于同步期的碰撞处理）
    void updateFollower(const std::deque<PositionRecord>& history, int followerIndex, float dt, bool leaderMoving, GameMap& map);

    void draw(sf::RenderWindow& window);
    
    // Getters
    sf::Vector2f getPosition() const { return m_sprite ? m_sprite->getPosition() : sf::Vector2f{}; }
    sf::Vector2f getCenter() const; // 获取中心点（用于交互判定）
    sf::FloatRect getBounds() const; // 获取碰撞箱（脚底）
    void collectDrawItem(std::vector<DrawItem>& outItems) const; // 提供绘制排序所需数据
    int getDirection() const { return m_direction; }
    bool isMoving() const { return m_isMoving; }
    PositionRecord getRecord() const; // 获取当前帧状态存入历史

    // 允许外部设置位置
    void setPosition(sf::Vector2f pos) { if (m_sprite) m_sprite->setPosition(pos); }

private:
    Game& m_game; // 引用游戏主程序，获取输入等

    std::optional<sf::Sprite> m_sprite;
    std::array<sf::Texture, 16> m_frames; // 4方向 * 4帧
    std::array<sf::Vector2u, 16> m_frameSizes{};

    // 动画相关
    int m_direction = 0; // 0:Down, 1:Left, 2:Right, 3:Up
    float m_animTimer = 0.f;
    int m_frameIndex = 0;
    const float ANIM_SPEED = 0.15f; // 动画切换速度

    // 移动相关
    float m_walkSpeed = 150.f;
    float m_runSpeed = 250.f;
    bool m_isMoving = false;
    bool m_isRunning = false;

    // 当前帧尺寸
    sf::Vector2f m_frameSize{19.f, 40.f};
    float m_centerOffset = 20.f; // 根据帧高度动态更新

    // 物理：尝试移动并处理碰撞，返回是否发生位移
    bool moveAndSlide(sf::Vector2f velocity, float dt, GameMap& map);
    
    // 动画：更新Sprite的纹理区域
    void updateAnimation(float dt);

    // 将 sprite 切换到指定方向/帧，自动根据贴图尺寸调整原点
    void applyFrame(int direction, int frameIndex);
};