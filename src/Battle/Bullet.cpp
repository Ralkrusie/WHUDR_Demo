// Bullet implementation
#include "Battle/Bullet.h"

Bullet::Bullet(const sf::Texture& tex, const sf::Vector2f& pos, const sf::Vector2f& vel)
    : m_sprite(tex), m_position(pos), m_velocity(vel)
{
	m_sprite.setTexture(tex, true);
	m_sprite.setOrigin(sf::Vector2f{ static_cast<float>(tex.getSize().x) * 0.5f, static_cast<float>(tex.getSize().y) * 0.5f });
	m_sprite.setPosition(m_position);
}

void Bullet::update(float dt)
{
	m_position += m_velocity * dt;
	m_sprite.setPosition(m_position);
}

void Bullet::draw(sf::RenderWindow& window) const
{
	window.draw(m_sprite);
}

sf::FloatRect Bullet::getBounds() const
{
	float halfX = m_hitboxSize.x * 0.5f;
	float halfY = m_hitboxSize.y * 0.5f;
	return sf::FloatRect({ m_position.x - halfX + m_hitboxOffset.x, m_position.y - halfY + m_hitboxOffset.y }, m_hitboxSize);
}

void Bullet::setRotation(float deg)
{
	m_sprite.setRotation(sf::degrees(deg));
}

bool Bullet::isOffscreen(const sf::FloatRect& viewBounds) const
{
	sf::FloatRect b = getBounds();
	float ax1 = b.position.x;
	float ay1 = b.position.y;
	float ax2 = ax1 + b.size.x;
	float ay2 = ay1 + b.size.y;
	float bx1 = viewBounds.position.x;
	float by1 = viewBounds.position.y;
	float bx2 = bx1 + viewBounds.size.x;
	float by2 = by1 + viewBounds.size.y;
	return (ax2 < bx1) || (ax1 > bx2) || (ay2 < by1) || (ay1 > by2);
}
