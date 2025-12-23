#include "Battle/BattleActor.h"
#include <cmath>

std::vector<sf::Texture> BattleActorVisual::loadTextures(const std::vector<std::string>& paths)
{
    std::vector<sf::Texture> out;
    out.reserve(paths.size());
    for (const auto& p : paths) {
        sf::Texture tex;
        if (tex.loadFromFile(p)) {
            tex.setSmooth(false);
            out.push_back(std::move(tex));
        }
        // 加载失败则跳过该帧，继续其他帧
    }
    return out;
}

void BattleActorVisual::setIntroFrames(std::vector<sf::Texture> frames, float frameTime)
{
    m_intro = std::move(frames);
    m_introFrameTime = frameTime;
}

void BattleActorVisual::setIdleFrames(std::vector<sf::Texture> frames, float frameTime)
{
    m_idle = std::move(frames);
    m_idleFrameTime = frameTime;
}

// 将当前 sprite 原点设为贴图底边中心，并在重设原点后恢复位置到 m_pos
void BattleActorVisual::applyTexture(const sf::Texture& tex)
{
    if (!m_sprite) m_sprite.emplace(tex);
    else m_sprite->setTexture(tex, true);

    const auto size = tex.getSize();
    float ox = static_cast<float>(size.x) * 0.5f;
    float oy = static_cast<float>(size.y);
    m_sprite->setOrigin({ ox, oy });
    m_sprite->setScale(m_scale);
    m_sprite->setPosition(m_pos);
}

void BattleActorVisual::setStartPosition(const sf::Vector2f& p)
{
    m_pos = p;
}

void BattleActorVisual::setTargetPosition(const sf::Vector2f& p)
{
    m_target = p;
}

void BattleActorVisual::setMoveSpeed(float pixelsPerSecond)
{
    m_speed = pixelsPerSecond;
}

void BattleActorVisual::startIntro()
{
    m_playIntro = true;
    m_loop = false;
    m_timer = 0.f;
    m_frameIndex = 0;
    if (!m_intro.empty()) {
        applyTexture(m_intro[0]);
    }
    // 重写入场：记录起点并按距离/速度计算时长，用缓动推进
    m_introStart = m_pos;
    sf::Vector2f to{ m_target.x - m_pos.x, m_target.y - m_pos.y };
    float dist = std::sqrt(to.x*to.x + to.y*to.y);
    if (dist > 1e-5f) {
        // 统一入场时长约 1.44 秒（= 1.8 / 1.25），稍微加快
        m_introDuration = 1.44f;
        m_introElapsed = 0.f;
        m_atTarget = false;
        // 线性速度仍保留（用于回退逻辑），主路径改用缓动
        sf::Vector2f dir{ to.x / dist, to.y / dist };
        m_vel = { dir.x * m_speed, dir.y * m_speed };
    } else {
        // 起点与目标几乎重合：直接视为到位
        m_pos = m_target;
        if (m_sprite) m_sprite->setPosition(m_pos);
        m_vel = {0.f, 0.f};
        m_atTarget = true;
    }
}

void BattleActorVisual::startIdleLoop()
{
    m_playIntro = false;
    m_loop = true;
    m_timer = 0.f;
    m_frameIndex = 0;
    if (!m_idle.empty()) {
        applyTexture(m_idle[0]);
    }
    // 入场结束后清理残影
    m_trails.clear();
}

void BattleActorVisual::advanceFrame(float dt)
{
    m_timer += dt;
    float ft = m_playIntro ? m_introFrameTime : m_idleFrameTime;
    const auto& frames = m_playIntro ? m_intro : m_idle;
    if (frames.empty()) return;
    while (m_timer >= ft) {
        m_timer -= ft;
        m_frameIndex++;
        if (m_playIntro) {
            if (m_frameIndex >= static_cast<int>(frames.size())) {
                // 停在最后一帧，直到移动完成后再切到 idle
                m_frameIndex = static_cast<int>(frames.size()) - 1;
            }
        } else {
            m_frameIndex %= static_cast<int>(frames.size());
        }
        if (m_sprite) applyTexture(frames[m_frameIndex]);
    }
}

void BattleActorVisual::updateMovement(float dt)
{
    if (m_atTarget) return;

    if (m_playIntro && m_easedIntro && m_introDuration > 0.f) {
        // 缓动：ease-in-out-cubic（前后更慢，中间更快）
        m_introElapsed = std::min(m_introDuration, m_introElapsed + dt);
        float t = std::clamp(m_introElapsed / m_introDuration, 0.f, 1.f);
        float p = (t < 0.5f) ? (4.f * t * t * t) : (1.f - std::pow(-2.f * t + 2.f, 3.f) / 2.f);
        sf::Vector2f delta{ m_target.x - m_introStart.x, m_target.y - m_introStart.y };
        m_pos = { m_introStart.x + delta.x * p, m_introStart.y + delta.y * p };

        // 轻微缩放弹跳：减弱幅度，整体更柔和
        float bounce = 1.f + 0.04f * std::sin(3.1415926f * std::min(1.f, t * 1.2f));
        if (m_sprite) {
            m_sprite->setPosition(m_pos);
            m_sprite->setScale({ m_scale.x * bounce, m_scale.y * bounce });
        }

        if (t >= 1.f - 1e-6f) {
            m_pos = m_target;
            m_atTarget = true;
            m_vel = {0.f, 0.f};
            m_trails.clear();
            if (m_sprite) m_sprite->setScale(m_scale);
            // 移动完成：此时切换到待机循环
            startIdleLoop();
        } else {
            pushAfterImage();
        }
    } else {
        // 回退到线性移动
        sf::Vector2f step{ m_vel.x * dt, m_vel.y * dt };
        sf::Vector2f next{ m_pos.x + step.x, m_pos.y + step.y };
        sf::Vector2f toTarget{ m_target.x - m_pos.x, m_target.y - m_pos.y };
        float dist2 = toTarget.x*toTarget.x + toTarget.y*toTarget.y;
        float step2 = step.x*step.x + step.y*step.y;
        if (step2 > 0.f && dist2 <= step2) {
            m_pos = m_target;
            m_atTarget = true;
            m_vel = {0.f, 0.f};
            m_trails.clear();
        } else {
            m_pos = next;
            pushAfterImage();
        }
        if (m_sprite) m_sprite->setPosition(m_pos);
    }
}

void BattleActorVisual::pushAfterImage()
{
    if (m_trails.size() >= static_cast<std::size_t>(m_trailMax)) m_trails.pop_front();
    m_trails.push_back(AfterImage{ m_pos, 100.f });
}

void BattleActorVisual::update(float dt)
{
    advanceFrame(dt);
    updateMovement(dt);
    // 衰减残影透明度
    for (auto& t : m_trails) {
        t.alpha = std::max(0.f, t.alpha - 360.f * dt);
    }
    while (!m_trails.empty() && m_trails.front().alpha <= 0.f) {
        m_trails.pop_front();
    }
}

void BattleActorVisual::draw(sf::RenderWindow& window) const
{
    if (!m_sprite) return;
    // 先画残影（简单做法：用当前帧贴图在历史位置绘制不同透明度）
    sf::Sprite ghost = *m_sprite;
    for (const auto& t : m_trails) {
        ghost.setPosition(t.pos);
        sf::Color c = ghost.getColor();
        c.a = static_cast<std::uint8_t>(std::clamp(t.alpha, 0.f, 255.f));
        ghost.setColor(c);
        window.draw(ghost);
    }
    // 再画本体（不透明）
    sf::Sprite cur = *m_sprite;
    sf::Color c = cur.getColor();
    c.a = 255;
    cur.setColor(c);
    window.draw(cur);
}
