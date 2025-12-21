#pragma once
#include <SFML/Graphics.hpp>

// 轻量级弹幕实体：负责自身位置推进与绘制，碰撞/伤害由上层驱动。
class Bullet {
public:
	Bullet(const sf::Texture& tex, const sf::Vector2f& pos, const sf::Vector2f& vel);

	void update(float dt);
	void draw(sf::RenderWindow& window) const;

	sf::FloatRect getBounds() const;
	const sf::Vector2f& getPosition() const { return m_position; }
	void setVelocity(const sf::Vector2f& v) { m_velocity = v; }
	void setRotation(float deg);
	void setHitboxOffset(const sf::Vector2f& o) { m_hitboxOffset = o; }
	bool isOffscreen(const sf::FloatRect& viewBounds) const;

private:
	sf::Sprite m_sprite;
	sf::Vector2f m_position{0.f, 0.f};
	sf::Vector2f m_velocity{0.f, 0.f};
	sf::Vector2f m_hitboxSize{10.f, 10.f};
	sf::Vector2f m_hitboxOffset{0.f, 0.f};
};