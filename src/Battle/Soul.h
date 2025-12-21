/*
代表 KRIS 的“灵魂”（就是 Undertale 那个红心）。
包含：

移动方式

碰撞判定

战斗区域
*/
#pragma once
#include <SFML/Graphics.hpp>
#include <optional>

class Soul {
public:
	Soul();

	void setBounds(const sf::FloatRect& box);
	void setCenter(const sf::Vector2f& c);
	void resetToCenter();

	// 设置无敌帧时长（秒），期间会闪烁透明度
	void setInvincible(float duration);
	bool isInvincible() const { return m_invincibleTimer > 0.f; }

	void handleInput(sf::RenderWindow& window, float dt);
	void update(float dt);
	void draw(sf::RenderWindow& window) const;

	sf::FloatRect getBounds() const;
	const sf::Vector2f& getPosition() const { return m_position; }
	void setHitboxScale(float s) { m_hitboxScale = std::clamp(s, 0.1f, 1.0f); }
	void setSpawnYOffset(float y) { m_spawnYOffset = y; }

private:
	sf::Texture m_texture;
	sf::Texture m_textureAlt;
	std::optional<sf::Sprite> m_sprite;
	sf::Vector2f m_position{0.f, 0.f};
	float m_speed = 160.f;
	sf::FloatRect m_box{};
	float m_hitboxScale = 0.7f; // collision box relative to sprite size
	float m_spawnYOffset = -12.f; // initial center offset upward
	float m_invincibleTimer = 0.f;
	float m_blinkTimer = 0.f;
	bool m_useAltSprite = false;
};