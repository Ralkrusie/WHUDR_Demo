/*
战斗中的敌人（高数题）。
包含：

血量

名字（如 “e^x”）

攻击模式列表

行为状态机（Idle, Attack, Hurt）
*/
#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <map>
#include <memory>
#include "Battle/BattleTypes.h"

// 轻量的敌人数据结构，主打 "能跑起来"。
class Enemy {
public:
	enum class CalcStage { One0, OneA1, OneA2, OneB1, OneB2, OneB3 };

	Enemy() = default;
	Enemy(const sf::String& name, int maxHP, int atk, int def);

	// 工厂：构造一只 "高数题" 敌人
	static Enemy makeCalculus();

	bool ignoreDamage() const { return m_ignoreDamage; }
	bool onAct(const ActData& act);
	CalcStage getCalcStage() const { return m_calcStage; }
	bool isSpared() const { return m_spared; }
	bool trySpare();

	const sf::String& getName() const { return m_name; }
	int getHP() const { return m_hp; }
	int getMaxHP() const { return m_maxHP; }
	float getMercy() const { return m_mercy; }
	const std::vector<ActData>& getActs() const { return m_actList; }

	void setActs(std::vector<ActData> acts) { m_actList = std::move(acts); }

	bool isDefeated() const { return m_hp <= 0; }
	bool isSpareable() const { return m_mercy >= 100.f; }

	// 战斗结算接口
	void takeDamage(int amount);
	void addMercy(float amount);
	void heal(int amount);

	// 渲染：目前用占位矩形+文字，后续可替换为贴图
	void draw(sf::RenderWindow& window, const sf::Font& font, const sf::Vector2f& origin) const;

private:
	static void ensureCalcTextures();
	static const sf::Texture* textureForStage(CalcStage stage);
	void setCalcStage(CalcStage stage);

	sf::String m_name;
	int m_maxHP = 0;
	int m_hp = 0;
	int m_attack = 0;
	int m_defense = 0;
	float m_mercy = 0.f; // 0..100
	std::vector<ActData> m_actList;
	sf::Vector2f m_size{160.f, 90.f};
	bool m_ignoreDamage = false;
	bool m_isCalc = false;
	bool m_spared = false;
	CalcStage m_calcStage = CalcStage::One0;
	static std::map<CalcStage, std::shared_ptr<sf::Texture>> s_calcTextures;
};