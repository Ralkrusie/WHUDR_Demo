#include "Overworld/Map.h"
#include <iostream>
#include <cmath>

namespace {
inline float dot(const sf::Vector2f& a, const sf::Vector2f& b) { return a.x * b.x + a.y * b.y; }
inline sf::Vector2f rotate(const sf::Vector2f& v, float rad) {
    float c = std::cos(rad), s = std::sin(rad);
    return { v.x * c - v.y * s, v.x * s + v.y * c };
}

// 获取旋转矩形四个顶点（按 position 左上角为原点旋转）
std::array<sf::Vector2f, 4> getVertices(const RotRect& rr) {
    float rad = rr.angleDeg * 3.14159265358979323846f / 180.f;
    sf::Vector2f p = rr.position;
    sf::Vector2f w = { rr.size.x, 0.f };
    sf::Vector2f h = { 0.f, rr.size.y };
    std::array<sf::Vector2f, 4> v{
        p,
        p + rotate(w, rad),
        p + rotate(sf::Vector2f{ rr.size.x, rr.size.y }, rad),
        p + rotate(h, rad)
    };
    return v;
}

std::array<sf::Vector2f, 4> getVertices(const sf::FloatRect& r) {
    sf::Vector2f p = r.position;
    sf::Vector2f s = r.size;
    return { p, sf::Vector2f{ p.x + s.x, p.y }, sf::Vector2f{ p.x + s.x, p.y + s.y }, sf::Vector2f{ p.x, p.y + s.y } };
}

sf::Vector2f centerOf(const std::array<sf::Vector2f, 4>& verts) {
    sf::Vector2f c{0.f, 0.f};
    for (auto& v : verts) { c.x += v.x; c.y += v.y; }
    c.x *= 0.25f; c.y *= 0.25f;
    return c;
}

void projectOnAxis(const std::array<sf::Vector2f, 4>& verts, const sf::Vector2f& axis, float& outMin, float& outMax) {
    sf::Vector2f n = axis;
    float len = std::sqrt(n.x * n.x + n.y * n.y);
    if (len <= 1e-6f) { outMin = 0.f; outMax = 0.f; return; }
    n.x /= len; n.y /= len;
    float mn = dot(verts[0], n), mx = mn;
    for (int i = 1; i < 4; ++i) {
        float p = dot(verts[i], n);
        if (p < mn) mn = p; if (p > mx) mx = p;
    }
    outMin = mn; outMax = mx;
}

bool overlapsOnAxis(const std::array<sf::Vector2f, 4>& a, const std::array<sf::Vector2f, 4>& b, const sf::Vector2f& axis) {
    float aMin, aMax, bMin, bMax;
    projectOnAxis(a, axis, aMin, aMax);
    projectOnAxis(b, axis, bMin, bMax);
    return !(aMax < bMin || bMax < aMin);
}

// 旋转矩形 vs 轴对齐矩形相交（含四边轴与 X/Y 轴）
bool intersectsRotAABB(const RotRect& rr, const sf::FloatRect& aabb) {
    auto rv = getVertices(rr);
    auto av = getVertices(aabb);
    // 轴：旋转矩形的两条边方向，以及 AABB 的 X/Y 轴
    sf::Vector2f e0 = rv[1] - rv[0];
    sf::Vector2f e1 = rv[3] - rv[0];
    if (!overlapsOnAxis(rv, av, e0)) return false;
    if (!overlapsOnAxis(rv, av, e1)) return false;
    if (!overlapsOnAxis(rv, av, {1.f, 0.f})) return false;
    if (!overlapsOnAxis(rv, av, {0.f, 1.f})) return false;
    return true;
}

// 计算最小平移向量（MTV），用于将 AABB 推出 RotRect；若不相交返回 false
bool mtvRotAABB(const RotRect& rr, const sf::FloatRect& aabb, sf::Vector2f& outMTV) {
    auto rv = getVertices(rr);
    auto av = getVertices(aabb);
    sf::Vector2f cA = centerOf(rv);
    sf::Vector2f cB = centerOf(av);

    std::array<sf::Vector2f, 4> axes{
        rv[1] - rv[0],
        rv[3] - rv[0],
        sf::Vector2f{1.f, 0.f},
        sf::Vector2f{0.f, 1.f}
    };

    float minOverlap = std::numeric_limits<float>::max();
    sf::Vector2f bestAxis{0.f, 0.f};

    for (auto axis : axes) {
        float aMin, aMax, bMin, bMax;
        projectOnAxis(rv, axis, aMin, aMax);
        projectOnAxis(av, axis, bMin, bMax);
        float overlap = std::min(aMax, bMax) - std::max(aMin, bMin);
        if (overlap <= 0.f) return false; // 分离，不相交
        // 记录最小重叠
        if (overlap < minOverlap) {
            minOverlap = overlap;
            // 判定推离方向：从 A 到 B 的向量在该轴上的符号
            float sign = (dot(cB - cA, axis) < 0.f) ? -1.f : 1.f;
            float len = std::sqrt(axis.x * axis.x + axis.y * axis.y);
            if (len > 1e-6f) axis = { axis.x / len, axis.y / len };
            bestAxis = { axis.x * overlap * sign, axis.y * overlap * sign };
        }
    }

    outMTV = bestAxis;
    return true;
}
}

void GameMap::clear()
{
    m_bgSprite.reset();
    m_bgTexture.reset();
    m_walls.clear();
    m_interactables.clear();
    m_warps.clear();
    m_props.clear(); // 清除上次房间的动画道具，防止重复叠加
}

void GameMap::setBackground(const std::string& path, const sf::Vector2f& scale, const sf::Vector2f& position)
{
    m_bgTexture.emplace();
    if (!m_bgTexture->loadFromFile(path)) {
        std::cerr << "GameMap::setBackground failed: " << path << std::endl;
    }
    m_bgTexture->setSmooth(false);

    m_bgSprite.emplace(*m_bgTexture);
    m_bgSprite->setPosition(position);
    m_bgSprite->setScale(scale);
}

void GameMap::drawBackground(sf::RenderWindow& window)
{
    if (m_bgSprite.has_value()) {
        window.draw(*m_bgSprite);
    }
}

void GameMap::addWall(const sf::FloatRect& wall)
{
    m_walls.push_back(RotRect{ wall.position, wall.size, 0.f });
}

void GameMap::addWall(const RotRect& wall)
{
    m_walls.push_back(wall);
}

void GameMap::addInteractable(const Interactable& interactable)
{
    m_interactables.push_back(interactable);
}

void GameMap::addWarp(const WarpTrigger& warp)
{
    m_warps.push_back(warp);
}

// 兼容旧接口：目前仅调用 clear + setBackground，具体内容应由房间文件构建
void GameMap::load(const std::string& mapName)
{
    clear();
    const std::string path = "assets/sprite/Room/" + mapName + ".png";
    setBackground(path, {1.f, 1.f});
}

int GameMap::addAnimatedProp(const std::vector<std::string>& framePaths,
                             const sf::Vector2f& position,
                             const sf::Vector2f& scale,
                             float frameTime)
{
    AnimatedProp prop;
    prop.position = position;
    prop.scale = scale;
    prop.frameTime = frameTime;
    prop.frames.resize(framePaths.size());
    for (std::size_t i = 0; i < framePaths.size(); ++i) {
        if (!prop.frames[i].loadFromFile(framePaths[i])) {
            std::cerr << "GameMap::addAnimatedProp failed: " << framePaths[i] << std::endl;
        }
        prop.frames[i].setSmooth(false);
    }
    if (!prop.frames.empty()) {
        prop.sprite.emplace(prop.frames[0]);
        prop.sprite->setPosition(prop.position);
        prop.sprite->setScale(prop.scale);
    }
    m_props.push_back(std::move(prop));
    return static_cast<int>(m_props.size() - 1);
}

void GameMap::gatherDrawItems(std::vector<DrawItem>& outItems)
{
    for (auto& p : m_props) {
        if (!p.sprite.has_value()) continue;
        auto bounds = p.sprite->getGlobalBounds();
        float yKey = bounds.position.y + bounds.size.y; // 使用底部 y 做深度
        outItems.push_back(DrawItem{ &(*p.sprite), yKey });
    }
}

void GameMap::update(float dt)
{
    for (auto& p : m_props) {
        if (p.frames.empty()) continue;
        p.timeAcc += dt;
        while (p.timeAcc >= p.frameTime) {
            p.timeAcc -= p.frameTime;
            p.frameIndex = (p.frameIndex + 1) % static_cast<int>(p.frames.size());
            if (p.sprite.has_value()) {
                p.sprite->setTexture(p.frames[p.frameIndex]);
                // 保持位置与缩放
                p.sprite->setPosition(p.position);
                p.sprite->setScale(p.scale);
            }
        }
    }
}

bool GameMap::checkCollision(const sf::FloatRect& bounds)
{
    for (const auto& wall : m_walls) {
        if (intersectsRotAABB(wall, bounds)) return true;
    }
    // 交互物体的碰撞箱（若设置）也参与阻挡
    for (const auto& it : m_interactables) {
        if (it.collider.has_value()) {
            if (intersectsRotAABB(*it.collider, bounds)) return true;
        }
    }
    return false;
}

bool GameMap::resolveCollision(const sf::FloatRect& bounds, sf::Vector2f& outMTV) const
{
    bool collided = false;
    float bestLen = std::numeric_limits<float>::max();
    sf::Vector2f best{0.f, 0.f};

    auto consider = [&](const RotRect& rr) {
        sf::Vector2f mtv;
        if (mtvRotAABB(rr, bounds, mtv)) {
            float len = std::sqrt(mtv.x * mtv.x + mtv.y * mtv.y);
            if (len < bestLen) {
                bestLen = len;
                best = mtv;
                collided = true;
            }
        }
    };

    for (const auto& wall : m_walls) consider(wall);
    for (const auto& it : m_interactables) {
        if (it.collider.has_value()) {
            consider(*it.collider);
        }
    }

    if (collided) outMTV = best;
    return collided;
}

Interactable* GameMap::checkInteraction(const sf::FloatRect& sensor)
{
    for (auto& it : m_interactables) {
        // 交互范围支持旋转
        RotRect areaRR{ it.area.position, it.area.size, it.areaAngleDeg };
        if (intersectsRotAABB(areaRR, sensor)) {
            return &it;
        }
    }
    return nullptr;
}

WarpTrigger* GameMap::checkWarp(const sf::FloatRect& bounds)
{
    for (auto& wp : m_warps) {
        if (wp.area.findIntersection(bounds)) {
            return &wp;
        }
    }
    return nullptr;
}

void GameMap::draw(sf::RenderWindow& window)
{
    drawBackground(window);

    // 默认绘制：先背景、再道具（未排序），主要保留兼容性；深度排序请使用 gatherDrawItems + 外部排序
    for (auto& p : m_props) {
        if (p.sprite.has_value()) {
            window.draw(*p.sprite);
        }
    }

    if (m_debugDraw) {
        drawDebugOverlays(window);
    }
}

void GameMap::drawDebugOverlays(sf::RenderWindow& window)
{
    if (!m_debugDraw) return;
    // 可视化：墙体（绿色半透明）、交互（蓝色半透明）、交互碰撞（红色半透明）、传送（黄色半透明）
    sf::RectangleShape rect;
    rect.setFillColor(sf::Color(0, 255, 0, 60));
    rect.setOutlineColor(sf::Color(0, 180, 0));
    rect.setOutlineThickness(1.f);
    for (const auto& w : m_walls) {
        rect.setPosition(w.position);
        rect.setSize(w.size);
        rect.setRotation(sf::degrees(w.angleDeg));
        window.draw(rect);
    }

    // Interactables area - blue
    for (const auto& it : m_interactables) {
        rect.setFillColor(sf::Color(0, 0, 255, 60));
        rect.setOutlineColor(sf::Color(0, 0, 180));
        rect.setPosition(it.area.position);
        rect.setSize(it.area.size);
        rect.setRotation(sf::degrees(it.areaAngleDeg));
        window.draw(rect);
        if (it.collider.has_value()) {
            rect.setFillColor(sf::Color(255, 0, 0, 60));
            rect.setOutlineColor(sf::Color(180, 0, 0));
            rect.setPosition(it.collider->position);
            rect.setSize(it.collider->size);
            rect.setRotation(sf::degrees(it.collider->angleDeg));
            window.draw(rect);
        }
    }

    // Warps - yellow
    rect.setFillColor(sf::Color(255, 255, 0, 60));
    rect.setOutlineColor(sf::Color(180, 180, 0));
    for (const auto& wp : m_warps) {
        rect.setPosition(wp.area.position);
        rect.setSize(wp.area.size);
        window.draw(rect);
    }
}
