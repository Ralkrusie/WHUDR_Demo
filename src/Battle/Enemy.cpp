#include "Battle/Enemy.h"
#include <algorithm>
#include <array>
#include <string>

namespace {
int clampPositive(int value, int maxValue) {
	if (value < 0) return 0;
	return std::min(value, maxValue);
}

const char* stageToFile(Enemy::CalcStage s) {
	switch (s) {
	case Enemy::CalcStage::One0: return "assets/sprite/Calculus/1_0.png";
	case Enemy::CalcStage::OneA1: return "assets/sprite/Calculus/1_a_1.png";
	case Enemy::CalcStage::OneA2: return "assets/sprite/Calculus/1_a_2.png";
	case Enemy::CalcStage::OneB1: return "assets/sprite/Calculus/1_b_1.png";
	case Enemy::CalcStage::OneB2: return "assets/sprite/Calculus/1_b_2.png";
	case Enemy::CalcStage::OneB3: return "assets/sprite/Calculus/1_b_3.png";
	}
	return nullptr;
}
}

std::map<Enemy::CalcStage, std::shared_ptr<sf::Texture>> Enemy::s_calcTextures;

Enemy::Enemy(const sf::String& name, int maxHP, int atk, int def)
	: m_name(name), m_maxHP(maxHP), m_hp(maxHP), m_attack(atk), m_defense(def) {}

Enemy Enemy::makeCalculus()
{
	Enemy e(sf::String(L"不定积分"), 120, 0, 0);
	e.m_ignoreDamage = true;
	e.m_isCalc = true;
	e.m_calcStage = CalcStage::One0;
	e.m_mercy = 0.f;
	e.setActs({});
	ensureCalcTextures();
	return e;
}

void Enemy::takeDamage(int amount)
{
	if (m_ignoreDamage) return;
	const int real = std::max(1, amount - m_defense);
	m_hp = clampPositive(m_hp - real, m_maxHP);
	if (m_hp <= 0) {
		m_hp = 0;
		m_mercy = 100.f;
	}
}

void Enemy::addMercy(float amount)
{
	m_mercy = std::clamp(m_mercy + amount, 0.f, 100.f);
}

bool Enemy::trySpare()
{
	if (m_spared) return true;
	if (isDefeated()) { m_spared = true; return true; }
	if (!isSpareable()) return false;
	m_spared = true;
	m_hp = 0;
	return true;
}

void Enemy::heal(int amount)
{
	m_hp = clampPositive(m_hp + amount, m_maxHP);
}

bool Enemy::onAct(const ActData& act)
{
	if (!m_isCalc) return false;
	// 只响应预设的数学步骤，错误步骤不改变状态，也不触发默认伤害/仁慈。
	const sf::String& n = act.name;
	bool progressed = false;
	if (m_calcStage == CalcStage::One0 && n == sf::String(L"三角代换")) {
		setCalcStage(CalcStage::OneA1);
		progressed = true;
	} else if (m_calcStage == CalcStage::One0 && n == sf::String(L"三角恒等变换")) {
		setCalcStage(CalcStage::OneB1);
		progressed = true;
	} else if (m_calcStage == CalcStage::OneA1 && n == sf::String(L"计算")) {
		setCalcStage(CalcStage::OneA2);
		m_mercy = 100.f;
		progressed = true;
	} else if (m_calcStage == CalcStage::OneB1 && n == sf::String(L"凑微分")) {
		setCalcStage(CalcStage::OneB2);
		progressed = true;
	} else if (m_calcStage == CalcStage::OneB2 && n == sf::String(L"计算")) {
		setCalcStage(CalcStage::OneB3);
		m_mercy = 100.f;
		progressed = true;
	}
	return true; // 始终吞掉默认 Act 结算（无伤害，不加额外 Mercy），仅按流程推进
}

void Enemy::ensureCalcTextures()
{
	if (!s_calcTextures.empty()) return;
	for (CalcStage s : {CalcStage::One0, CalcStage::OneA1, CalcStage::OneA2, CalcStage::OneB1, CalcStage::OneB2, CalcStage::OneB3}) {
		const char* file = stageToFile(s);
		if (!file) continue;
		auto tex = std::make_shared<sf::Texture>();
		if (tex->loadFromFile(file)) {
			tex->setSmooth(false);
			s_calcTextures.emplace(s, std::move(tex));
		}
	}
}

const sf::Texture* Enemy::textureForStage(CalcStage stage)
{
	auto it = s_calcTextures.find(stage);
	if (it == s_calcTextures.end()) return nullptr;
	return it->second.get();
}

void Enemy::setCalcStage(CalcStage stage)
{
	m_calcStage = stage;
}

void Enemy::draw(sf::RenderWindow& window, const sf::Font& font, const sf::Vector2f& origin) const
{
	const sf::Texture* tex = (m_isCalc ? textureForStage(m_calcStage) : nullptr);
	if (tex) {
		sf::Sprite sprite(*tex);
		sprite.setScale({0.4f, 0.4f});
		sprite.setPosition({origin.x + 20.f, origin.y});
		window.draw(sprite);
		return;
	}

	sf::RectangleShape body(m_size);
	body.setPosition(origin);
	body.setFillColor(sf::Color(40, 40, 70));
	body.setOutlineColor(sf::Color::White);
	body.setOutlineThickness(3.f);
	window.draw(body);

	sf::Text nameText(font, m_name, 22);
	nameText.setPosition({origin.x + 12.f, origin.y + 8.f});
	// Spareable enemies show name in yellow when mercy is full or they are spared
	if (isSpared() || m_mercy >= 100.f) {
		nameText.setFillColor(sf::Color(255, 240, 100));
	}
	window.draw(nameText);

	const float barWidth = m_size.x - 24.f;
	const float barHeight = 12.f;
	float ratio = (m_maxHP > 0) ? static_cast<float>(m_hp) / static_cast<float>(m_maxHP) : 0.f;
	ratio = std::clamp(ratio, 0.f, 1.f);

	sf::RectangleShape hpBack({barWidth, barHeight});
	hpBack.setPosition({origin.x + 12.f, origin.y + m_size.y - 28.f});
	hpBack.setFillColor(sf::Color(60, 60, 60));
	window.draw(hpBack);

	sf::RectangleShape hpFront({barWidth * ratio, barHeight});
	hpFront.setPosition(hpBack.getPosition());
	hpFront.setFillColor(sf::Color(200, 70, 70));
	window.draw(hpFront);

	float mercyRatio = std::clamp(m_mercy / 100.f, 0.f, 1.f);
	sf::RectangleShape mercyBar({barWidth * mercyRatio, 6.f});
	mercyBar.setPosition({origin.x + 12.f, origin.y + m_size.y - 12.f});
	mercyBar.setFillColor(sf::Color(240, 200, 40));
	window.draw(mercyBar);
}
