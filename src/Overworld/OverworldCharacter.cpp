#include "OverworldCharacter.h"
#include "Manager/InputManager.h" // 引用你的输入管理器
#include <algorithm>
#include <cmath> // for std::abs

//
// 探索角色（OverworldCharacter）
// ------------------------------
// 职责：
// - 队长（玩家）输入驱动的移动、朝向与动画
// - 队员（跟随者）基于队长历史记录的延迟跟随与追赶机制
// - 移动碰撞：与地图墙体/交互碰撞箱做检测，简单分离轴移动（先 X 后 Y）
// - 绘制与深度排序键收集（底边 y）
// 关键约定：
// - 角色原点在贴图底边中心；脚底碰撞箱为底部较小矩形，随缩放而变化
// - 动画序列为 4 方向 × 4 帧，索引映射为 Down/Left/Right/Up × frame
// - 跟随延迟按队员序号线性增加，拉长时启用追赶倍率上限
//

namespace {
constexpr float kMinFootW = 8.f;
constexpr float kMinFootH = 6.f;
// 碰撞箱尺寸（相对缩放后贴图），集中在下半身并比整图小
constexpr float kColliderWidthRatio = 0.45f;
constexpr float kColliderHeightRatio = 0.18f;
// 跟随延迟（以帧为单位），值越大成员越远
constexpr int kFollowDelayPerMemberFrames = 10;
// 贴近目标时停止移动的阈值，避免抖动
constexpr float kArriveEpsilon = 0.5f;
// 追赶加速触发距离与倍率
constexpr float kCatchUpDistance = 24.f;    // 超过该距离开始轻微加速
constexpr float kCatchUpMultiplier = 1.15f; // 追赶时的速度倍率
// 直线拉长修正：动态追赶上限倍率与松弛阈值
constexpr float kCatchUpMaxMultiplier = 1.6f;
constexpr float kStretchRatioThreshold = 1.1f; // 距离超过期望 10% 时开启增强追赶
constexpr float kMoveEpsilonSq = 0.001f;       // 判定“未移动”的平方距离阈值
}

OverworldCharacter::OverworldCharacter(Game& game, const SpriteSet& spriteSet)
    : m_game(game) {
    // 预加载 4 方向 × 4 帧的贴图；按传入 SpriteSet 的资源路径加载
    // Down: frames[0], Left: frames[1], Right: frames[2], Up: frames[3]
    size_t idx = 0;
    for (int dir = 0; dir < 4; ++dir) {
        for (int f = 0; f < 4; ++f, ++idx) {
            const std::string& path = spriteSet.frames[dir][f];
            if (!m_frames[idx].loadFromFile(path)) {
                // 如果加载失败，可记录日志；先保留空纹理
            }
            m_frameSizes[idx] = m_frames[idx].getSize();
            if (m_frameSizes[idx].x == 0u || m_frameSizes[idx].y == 0u) {
                m_frameSizes[idx] = {19u, 40u}; // 兜底尺寸
            }
        }
    }

    m_sprite.emplace(m_frames[0]);
    m_frameSize = sf::Vector2f(static_cast<float>(m_frameSizes[0].x), static_cast<float>(m_frameSizes[0].y));
    m_centerOffset = m_frameSize.y * 0.5f;
    m_sprite->setOrigin({m_frameSize.x * 0.5f, m_frameSize.y});
    // 放大角色贴图为原来的两倍
    m_sprite->setScale({2.f, 2.f});
}

// ==========================================
// 队长逻辑 (Kris)
// ==========================================
void OverworldCharacter::updateLeader(float dt, GameMap& map) {
    // 读取输入 → 确定朝向 → 归一化 → 应用速度 → 移动碰撞 → 更新动画
    sf::Vector2f velocity(0.f, 0.f);
    m_isMoving = false;
    m_isRunning = InputManager::isHeld(Action::Cancel,m_game.getWindow()); // 按住 X 键奔跑

    // 1. 读取输入
    if (InputManager::isHeld(Action::Up, m_game.getWindow()))    velocity.y -= 1.f;
    if (InputManager::isHeld(Action::Down, m_game.getWindow()))  velocity.y += 1.f;
    if (InputManager::isHeld(Action::Left, m_game.getWindow()))  velocity.x -= 1.f;
    if (InputManager::isHeld(Action::Right, m_game.getWindow())) velocity.x += 1.f;

    // 2. 确定朝向 (优先保留最后按下的方向，这里简化处理)
    if (velocity.y > 0) m_direction = 0; // 下
    else if (velocity.y < 0) m_direction = 3; // 上
    
    // 左右覆盖上下 (Deltarune 风格倾向)
    if (velocity.x < 0) m_direction = 1; // 左
    else if (velocity.x > 0) m_direction = 2; // 右

    // 3. 归一化向量（避免斜着走变快）并按步速移动
    if (velocity.x != 0 || velocity.y != 0) {
        m_isMoving = true;
        // 简单的归一化逻辑
        float length = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
        velocity /= length;

        // 应用速度
        float speed = m_isRunning ? m_runSpeed : m_walkSpeed;
        velocity *= speed;
        
        // 4. 执行移动 + 碰撞检测
        bool moved = moveAndSlide(velocity, dt, map);
        if (!moved) {
            m_isMoving = false; // 被墙挡住则视为未移动，避免驱动队员
        }
    }

    // 5. 更新动画
    updateAnimation(dt);
}

// ==========================================
// 碰撞与移动 (核心算法: 分离轴移动)
// ==========================================
bool OverworldCharacter::moveAndSlide(sf::Vector2f velocity, float dt, GameMap& map) {
    // 简单分离轴：先 X 后 Y，分别尝试；若某轴上碰撞则回退该轴位移
    sf::Vector2f offset = velocity * dt;
    if (offset.x == 0.f && offset.y == 0.f) return false;

    const sf::Vector2f startPos = getPosition();

    // 尝试 X
    m_sprite->move({offset.x, 0.f});
    if (map.checkCollision(getBounds())) {
        m_sprite->move({-offset.x, 0.f});
    }

    // 尝试 Y
    m_sprite->move({0.f, offset.y});
    if (map.checkCollision(getBounds())) {
        m_sprite->move({0.f, -offset.y});
    }

    const sf::Vector2f endPos = getPosition();
    const float dx = endPos.x - startPos.x;
    const float dy = endPos.y - startPos.y;
    return (dx * dx + dy * dy) > kMoveEpsilonSq;
}

// ==========================================
// 队员跟随逻辑 (Susie / Ralsei)
// ==========================================
void OverworldCharacter::updateFollower(const std::deque<PositionRecord>& history, int followerIndex, float dt, bool leaderMoving, GameMap& map) {
    // 根据队长历史记录延迟追随；历史不足时直接追赶最近记录点
    // 如果队长停下，队员立即停下
    if (!leaderMoving) {
        m_isMoving = false;
        m_isRunning = false;
        updateAnimation(dt);
        return;
    }

    // 延迟帧数：每多一个队员，延迟线性增加
    int delay = kFollowDelayPerMemberFrames * (followerIndex + 1);

    if (history.size() <= static_cast<size_t>(delay)) {
        // 历史不足：立即追赶到最新的记录点（不再同步移动）
        const auto& targetNow = history.back();

        // 朝向用历史方向，避免抽搐
        m_direction = targetNow.direction;
        m_isRunning = targetNow.isRunning;

        sf::Vector2f cur = getPosition();
        sf::Vector2f diff = { targetNow.position.x - cur.x, targetNow.position.y - cur.y };
        float dx = diff.x;
        float dy = diff.y;
        float dist = std::sqrt(dx * dx + dy * dy);
        // 估算理想间距：最新点到目标点的距离作为基准，防止直线奔跑时拉长
        float desiredSpacing = std::max(8.f, std::sqrt(std::pow(history.front().position.x - targetNow.position.x, 2.f) +
                                                      std::pow(history.front().position.y - targetNow.position.y, 2.f)));
        float distToLeader = std::sqrt(std::pow(history.front().position.x - cur.x, 2.f) +
                                       std::pow(history.front().position.y - cur.y, 2.f));

        if (dist > kArriveEpsilon) {
            float speed = m_isRunning ? m_runSpeed : m_walkSpeed;
            if (dist > kCatchUpDistance) speed *= kCatchUpMultiplier;
            // 若整体距离被拉长（相对理想间距），按比例增强追赶但有上限
            float stretchRatio = (desiredSpacing > 0.01f) ? (distToLeader / desiredSpacing) : 1.f;
            if (stretchRatio > kStretchRatioThreshold) {
                float extra = 1.f + (stretchRatio - 1.f) * 0.5f; // 拉长越大，倍率越高
                speed *= std::min(kCatchUpMaxMultiplier, extra);
            }

            float step = speed * dt;
            bool moved = false;
            if (dist <= step) {
                if (getPosition() != targetNow.position) {
                    m_sprite->setPosition(targetNow.position);
                    moved = true;
                }
            } else {
                sf::Vector2f v(dx / dist * speed, dy / dist * speed);
                moved = moveAndSlide(v, dt, map);
            }
            m_isMoving = moved;
        } else {
            m_isMoving = false;
        }

        updateAnimation(dt);
        return;
    }

    // 获取对应的“过去时刻”的状态
    const auto& target = history[delay];

    // 使用历史方向，避免基于微小位移造成的朝向抽搐
    m_direction = target.direction;
    m_isRunning = target.isRunning;

    sf::Vector2f cur = getPosition();
    sf::Vector2f diff = { target.position.x - cur.x, target.position.y - cur.y };
    float dx = diff.x;
    float dy = diff.y;
    float dist = std::sqrt(dx * dx + dy * dy);
    // 估算理想间距：最新点到目标点的距离作为基准，防止直线奔跑时整体拉长
    float desiredSpacing = std::max(8.f, std::sqrt(std::pow(history.front().position.x - target.position.x, 2.f) +
                                                  std::pow(history.front().position.y - target.position.y, 2.f)));
    float distToLeader = std::sqrt(std::pow(history.front().position.x - cur.x, 2.f) +
                                   std::pow(history.front().position.y - cur.y, 2.f));

    if (dist > kArriveEpsilon) {

        float speed = m_isRunning ? m_runSpeed : m_walkSpeed;
        // 轻微追赶：距离过大时小幅提速，减少掉队感
        if (dist > kCatchUpDistance) {
            speed *= kCatchUpMultiplier;
        }
        // 若整体距离被拉长（相对理想间距），按比例增强追赶但有上限
        float stretchRatio = (desiredSpacing > 0.01f) ? (distToLeader / desiredSpacing) : 1.f;
        if (stretchRatio > kStretchRatioThreshold) {
            float extra = 1.f + (stretchRatio - 1.f) * 0.5f;
            speed *= std::min(kCatchUpMaxMultiplier, extra);
        }

        // 当前帧最大可移动距离（到达则直接贴合历史点）
        float step = speed * dt;
        bool moved = false;
        if (dist <= step) {
            // 足以到达目标：直接贴合到历史点，避免“绕过”
            if (getPosition() != target.position) {
                m_sprite->setPosition(target.position);
                moved = true;
            }
        } else {
            // 归一化方向并按速度移动，使用与队长一致的碰撞处理
            sf::Vector2f v = { dx / dist * speed, dy / dist * speed };
            moved = moveAndSlide(v, dt, map);
        }

        m_isMoving = moved;
    } else {
        m_isMoving = false;
    }

    // 播放动画
    updateAnimation(dt);
}

// ==========================================
// 动画与辅助函数
// ==========================================
void OverworldCharacter::updateAnimation(float dt) {
    // 移动时按步速推进帧；静止时回到第 0 帧
    if (m_isMoving) {
        m_animTimer += dt;
        float speedMod = m_isRunning ? 1.5f : 1.0f;
        if (m_animTimer >= ANIM_SPEED / speedMod) {
            m_animTimer = 0.f;
            m_frameIndex = (m_frameIndex + 1) % 4;
        }
    } else {
        m_frameIndex = 0;
        m_animTimer = 0.f;
    }

    applyFrame(m_direction, m_frameIndex);
}

sf::FloatRect OverworldCharacter::getBounds() const {
    // 返回脚底的小矩形用于碰撞（缩放后尺寸，集中下半身）
    sf::Vector2f pos = getPosition(); // 脚底中心
    sf::Vector2f scale(1.f, 1.f);
    if (m_sprite.has_value()) {
        scale = m_sprite->getScale();
    }

    float worldW = m_frameSize.x * scale.x;
    float worldH = m_frameSize.y * scale.y;

    float footW = std::max(kMinFootW * scale.x, worldW * kColliderWidthRatio);
    float footH = std::max(kMinFootH * scale.y, worldH * kColliderHeightRatio);

    // 碰撞箱位于脚下方区域：底边在 pos.y，向上延伸 footH
    return sf::FloatRect({pos.x - footW * 0.5f, pos.y - footH}, {footW, footH});
}

void OverworldCharacter::collectDrawItem(std::vector<DrawItem>& outItems) const {
    if (!m_sprite.has_value()) return;
    auto bounds = m_sprite->getGlobalBounds();
    float yKey = bounds.position.y + bounds.size.y; // 以底部 y 作为排序键（越低越后画）
    outItems.push_back(DrawItem{ const_cast<sf::Sprite*>(&(*m_sprite)), yKey });
}

sf::Vector2f OverworldCharacter::getCenter() const {
    sf::Vector2f pos = getPosition();
    return sf::Vector2f(pos.x, pos.y - m_centerOffset);
}

PositionRecord OverworldCharacter::getRecord() const {
    return { getPosition(), m_direction, m_isMoving, m_isRunning };
}

void OverworldCharacter::draw(sf::RenderWindow& window) {
    if (m_sprite.has_value()) {
        window.draw(*m_sprite);
    }
}

void OverworldCharacter::applyFrame(int direction, int frameIndex) {
    // 根据方向与帧索引设置纹理，并重新计算尺寸与原点
    if (!m_sprite.has_value()) return;
    int dir = std::clamp(direction, 0, 3);
    int frame = std::clamp(frameIndex, 0, 3);
    int idx = dir * 4 + frame;
    m_sprite->setTexture(m_frames[idx], true);
    m_frameSize = sf::Vector2f(static_cast<float>(m_frameSizes[idx].x), static_cast<float>(m_frameSizes[idx].y));
    m_centerOffset = m_frameSize.y * 0.5f;
    m_sprite->setOrigin({m_frameSize.x * 0.5f, m_frameSize.y});
}