#include "Battle/Soul.h"
#include "Manager/InputManager.h"
#include <algorithm>
#include <cmath>

Soul::Soul()
{
	bool ok0 = m_texture.loadFromFile("assets/sprite/Heart/spr_heart_0.png");
	bool ok1 = m_textureAlt.loadFromFile("assets/sprite/Heart/spr_heart_1.png");
	if (ok0) m_texture.setSmooth(false);
	if (ok1) m_textureAlt.setSmooth(false);
	if (ok0) {
		m_sprite.emplace(m_texture);
		m_sprite->setColor(sf::Color::Red);
		m_sprite->setScale({1.3f, 1.3f});
	}
}

void Soul::setBounds(const sf::FloatRect& box)
{
	m_box = box;
}

void Soul::setCenter(const sf::Vector2f& c)
{
	if (!m_sprite) return;
	m_position = c;
	float halfW = m_sprite->getGlobalBounds().size.x * 0.5f;
	float halfH = m_sprite->getGlobalBounds().size.y * 0.5f;
	m_sprite->setPosition({ m_position.x - halfW, m_position.y - halfH });
}

void Soul::resetToCenter()
{
	if (m_box.size.x <= 0.f || m_box.size.y <= 0.f || !m_sprite) return;
	// center of the battle box plus upward spawn offset
	sf::Vector2f center{ m_box.position.x + m_box.size.x * 0.5f, m_box.position.y + m_box.size.y * 0.5f };
	m_position = { center.x, center.y + m_spawnYOffset };
	// place sprite centered at m_position
	float halfW = m_sprite->getGlobalBounds().size.x * 0.5f;
	float halfH = m_sprite->getGlobalBounds().size.y * 0.5f;
	m_sprite->setPosition({ m_position.x - halfW, m_position.y - halfH });
}

void Soul::setInvincible(float duration)
{
	m_invincibleTimer = std::max(0.f, duration);
	m_blinkTimer = 0.f;
	m_useAltSprite = true; // 受伤立即切到替换贴图
	if (m_sprite) {
		if (m_textureAlt.getSize().x > 0) m_sprite->setTexture(m_textureAlt, true);
		else m_sprite->setTexture(m_texture, true);
		m_sprite->setColor(sf::Color::Red);
	}
}

void Soul::handleInput(sf::RenderWindow& window, float dt)
{
	if (!m_sprite) return;
	sf::Vector2f dir{0.f, 0.f};
	if (InputManager::isHeld(Action::Left, window)) dir.x -= 1.f;
	if (InputManager::isHeld(Action::Right, window)) dir.x += 1.f;
	if (InputManager::isHeld(Action::Up, window)) dir.y -= 1.f;
	if (InputManager::isHeld(Action::Down, window)) dir.y += 1.f;

	float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
	if (len > 0.001f) {
		dir.x /= len; dir.y /= len;
	}
	m_position += dir * (m_speed * dt); // 以 dt 平滑移动，独立于帧率

	// Clamp center within battle box using scaled hitbox
	float halfW_sprite = m_sprite->getGlobalBounds().size.x * 0.5f;
	float halfH_sprite = m_sprite->getGlobalBounds().size.y * 0.5f;
	float halfW_hit = halfW_sprite * m_hitboxScale;
	float halfH_hit = halfH_sprite * m_hitboxScale;
	// Shrink effective hitbox by 2px side length (1px per half)
	halfW_hit = std::max(0.f, halfW_hit - 1.f);
	halfH_hit = std::max(0.f, halfH_hit - 1.f);
	m_position.x = std::clamp(m_position.x, m_box.position.x + halfW_hit, m_box.position.x + m_box.size.x - halfW_hit);
	m_position.y = std::clamp(m_position.y, m_box.position.y + halfH_hit, m_box.position.y + m_box.size.y - halfH_hit);
	// place sprite centered at m_position
	m_sprite->setPosition({ m_position.x - halfW_sprite, m_position.y - halfH_sprite });
}

void Soul::update(float dt)
{
	if (m_invincibleTimer > 0.f) {
		m_invincibleTimer = std::max(0.f, m_invincibleTimer - dt);
		m_blinkTimer += dt;
		if (m_blinkTimer >= 0.2f) {
			m_blinkTimer -= 0.2f;
			m_useAltSprite = !m_useAltSprite;
			if (m_sprite) {
				if (m_useAltSprite && m_textureAlt.getSize().x > 0) m_sprite->setTexture(m_textureAlt, true);
				else m_sprite->setTexture(m_texture, true);
				m_sprite->setColor(sf::Color::Red);
			}
		}
	} else {
		// 恢复完全不透明
		if (m_sprite) {
			m_sprite->setTexture(m_texture, true);
			m_sprite->setColor(sf::Color::Red);
		}
		m_blinkTimer = 0.f;
		m_useAltSprite = false;
	}
}

void Soul::draw(sf::RenderWindow& window) const
{
	if (m_sprite) window.draw(*m_sprite);
}

sf::FloatRect Soul::getBounds() const
{
	if (!m_sprite) return sf::FloatRect{};
	// Collision rect centered at m_position, scaled down
	sf::FloatRect spriteRect = m_sprite->getGlobalBounds();
	sf::Vector2f center{ m_position.x, m_position.y };
	sf::Vector2f size{ spriteRect.size.x * m_hitboxScale, spriteRect.size.y * m_hitboxScale };
	// Apply -2px to side length, clamp to minimum of 1px
	size.x = std::max(1.f, size.x - 2.f);
	size.y = std::max(1.f, size.y - 2.f);
	return sf::FloatRect({ center.x - size.x * 0.5f, center.y - size.y * 0.5f }, size);
}
