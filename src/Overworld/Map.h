/*
地图数据结构。
包含：

地图

传送点

物体列表（NPC, Interactable）
*/

#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <optional>

// 旋转矩形：以 position 为左上角，绕该点按角度旋转
struct RotRect {
    sf::Vector2f position;
    sf::Vector2f size;
    float angleDeg = 0.f;
};

struct WarpTrigger {
    sf::FloatRect area;      // 踩到哪里触发
    std::string targetMap;   // 去哪个地图
    sf::Vector2f targetPos;  // 去地图的哪个坐标
};

struct Interactable {
    sf::FloatRect area;      // 交互范围
    std::string textID;      // 对话ID
    // 也可以加回调函数
    float areaAngleDeg = 0.f;              // 交互范围的旋转角度（度）
    std::optional<RotRect> collider;       // 可选：用于阻挡移动的碰撞箱（可旋转）
};

// 深度排序绘制项：yKey 越大越后画（通常取贴图底部的 y）
struct DrawItem {
    sf::Drawable* drawable = nullptr;
    float yKey = 0.f;
};

class GameMap {
public:
    GameMap() = default;

    // 工具：清空并按房间数据构建
    void clear();
    void setBackground(const std::string& path, const sf::Vector2f& scale = {1.f, 1.f}, const sf::Vector2f& position = {0.f, 0.f});
    // 轴对齐墙（无旋转）
    void addWall(const sf::FloatRect& wall);
    // 旋转墙：以 position 为左上角，绕该点旋转 angleDeg
    void addWall(const RotRect& wall);
    void addInteractable(const Interactable& interactable);
    void addWarp(const WarpTrigger& warp);
    void setDebugDraw(bool enabled) { m_debugDraw = enabled; }
    // 兼容旧接口（若仍需按名称加载可扩展为调用外部房间构建器）
    void load(const std::string& mapName);

    // 绘制背景（不含道具/调试）
    void drawBackground(sf::RenderWindow& window);

    // 收集需要按 y 排序的绘制项（道具等）
    void gatherDrawItems(std::vector<DrawItem>& outItems);

    // 绘制调试辅助（墙/交互/传送/碰撞框）
    void drawDebugOverlays(sf::RenderWindow& window);
    
    // 动画道具：在地图上循环播放的简单精灵（如存档点）
    int addAnimatedProp(const std::vector<std::string>& framePaths,
                        const sf::Vector2f& position,
                        const sf::Vector2f& scale = {1.f, 1.f},
                        float frameTime = 0.12f);
    
    // 每帧更新（驱动动画等）
    void update(float dt);
    
    // 核心：检查某个人会不会撞墙
    // 返回 true 表示撞了
    bool checkCollision(const sf::FloatRect& bounds);
    // 计算最小平移向量（MTV）将 AABB 推出障碍，返回是否相交
    bool resolveCollision(const sf::FloatRect& bounds, sf::Vector2f& outMTV) const;

    // 核心：检查交互
    // sensor: 玩家面前的一小块区域
    Interactable* checkInteraction(const sf::FloatRect& sensor);
    
    // 检查是否踩到传送门
    WarpTrigger* checkWarp(const sf::FloatRect& bounds);

    // 旧接口：仅供兼容，不再用于深度排序；保留便于未来调用
    void draw(sf::RenderWindow& window);

private:
    std::optional<sf::Sprite> m_bgSprite;
    std::optional<sf::Texture> m_bgTexture;
    
    std::vector<RotRect> m_walls;             // 所有的墙（支持旋转）
    std::vector<Interactable> m_interactables;
    std::vector<WarpTrigger> m_warps;
    bool m_debugDraw = true; // 调试绘制可视化

    struct AnimatedProp {
        std::vector<sf::Texture> frames;
        std::optional<sf::Sprite> sprite;
        sf::Vector2f position{0.f, 0.f};
        sf::Vector2f scale{1.f, 1.f};
        float frameTime = 0.12f; // 每帧时长
        float timeAcc = 0.f;
        int frameIndex = 0;
    };
    std::vector<AnimatedProp> m_props;
};

