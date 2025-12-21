#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <optional>
#include <deque>

// 简易帧动画器 + 线性平移 + 残影
class BattleActorVisual {
public:
    BattleActorVisual() = default;

    // 提供帧路径列表以加载动画（会关闭平滑以保留像素风格）
    static std::vector<sf::Texture> loadTextures(const std::vector<std::string>& paths);

    void setIntroFrames(std::vector<sf::Texture> frames, float frameTime);
    void setIdleFrames(std::vector<sf::Texture> frames, float frameTime);

    void setStartPosition(const sf::Vector2f& p);
    void setTargetPosition(const sf::Vector2f& p);
    void setMoveSpeed(float pixelsPerSecond);
    void setScale(const sf::Vector2f& s) { m_scale = s; if (m_sprite) m_sprite->setScale(m_scale); }

    // 开始入场：使用 intro 序列播放并开始平移
    void startIntro();
    // 切换到待机循环
    void startIdleLoop();

    void update(float dt);
    void draw(sf::RenderWindow& window) const;

    bool isAtTarget() const { return m_atTarget; }

private:
    void advanceFrame(float dt);
    void updateMovement(float dt);
    void pushAfterImage();
    void applyTexture(const sf::Texture& tex);

private:
    std::optional<sf::Sprite> m_sprite;
    std::vector<sf::Texture> m_intro;
    std::vector<sf::Texture> m_idle;
    float m_introFrameTime = 0.06f;
    float m_idleFrameTime = 0.12f;
    float m_timer = 0.f;
    int m_frameIndex = 0;
    bool m_playIntro = true;
    bool m_loop = false;

    sf::Vector2f m_pos{0.f, 0.f};
    sf::Vector2f m_target{0.f, 0.f};
    sf::Vector2f m_vel{0.f, 0.f};
    float m_speed = 180.f; // 像素/秒
    bool m_atTarget = false;
    sf::Vector2f m_scale{2.f, 2.f};

    struct AfterImage { sf::Vector2f pos; float alpha; };
    mutable std::deque<AfterImage> m_trails; // draw 调用会消耗 alpha
    int m_trailMax = 6;

    // 入场动画重写：使用缓动（ease-out）与轻微缩放弹跳
    bool m_easedIntro = true;
    float m_introElapsed = 0.f;
    float m_introDuration = 0.f; // 依据距离与速度计算：dist / speed
    sf::Vector2f m_introStart{0.f, 0.f};
};
