/*
战斗界面的主类。
包含：

控制当前敌人

调用 Soul / Enemy / Bullet

战斗 UI

判断胜利/失败
*/
#pragma once
#include <SFML/Graphics.hpp>
#include <optional>
#include <random>
#include "States/BaseState.h"
#include "Battle/Battle.h"
#include "UI/BattleMenu.h"
#include "Battle/Soul.h"
#include "UI/DialogBox.h"
#include "Battle/BattleActor.h"
#include "Battle/Bullet.h"

class BattleState : public BaseState {
public:
	BattleState(Game& game,
		std::vector<Enemy> enemies,
		std::vector<sf::Vector2f> partyStarts,
		std::optional<sf::Texture> overworldBgTex = std::nullopt,
		sf::Vector2f overworldBgScale = {1.f, 1.f});

	void handleEvent() override;
	void update(float dt) override;
	void draw(sf::RenderWindow& window) override;

private:
	void refreshMenuIfNeeded();
	void handlePhaseTransitions();
	void drawHUD(sf::RenderWindow& window);
	void tryExitBattle();
	void startBattleBoxEnter();
	void startBattleBoxExit();
	void updateBattleBox(float dt);
	void updateBattleBoxTransform();
	void syncSoulToBattleBox();
	void updateBullets(float dt);
	void spawnPatternA();
	void spawnPatternB();
	struct DamageResult { bool shieldTriggered = false; bool damageApplied = false; };
	DamageResult applyDamageRandomHero(int dmg);
	DamageResult applyDamageAllHeroes(int dmg);
	bool hasMantleAlive() const;
	void consumeShield();
	void resetHolyShieldReady();

private:
	Battle m_battle;
	BattleMenu m_menu;
	Soul m_soul;
	DialogueBox m_dialogue;
	sf::RectangleShape m_bulletBox;
	sf::Font m_font;
	BattlePhase m_prevPhase = BattlePhase::Intro;
	bool m_waitingForExit = false;
	bool m_victory = false;

	// 三人战斗入场与待机动画
	std::vector<BattleActorVisual> m_partyVisuals;
	std::vector<sf::Vector2f> m_partyStarts;

	// 背景：战斗动态帧与从 Overworld 捕获的背景的渐隐/渐显
	std::optional<sf::Texture> m_overworldBg;
	std::optional<sf::Sprite> m_overworldBgSprite;
	sf::Vector2f m_overworldBgScale{1.f, 1.f};

	std::vector<sf::Texture> m_battleBgFrames;
	std::optional<sf::Sprite> m_battleBgSprite;
	int m_battleBgIndex = 0;
	float m_battleBgTimer = 0.f;
	float m_battleBgFrameTime = 0.05f;

	// 背景渐变（交叉淡入）：将 Overworld 背景从 100% 淡出到 0%，
	// 同时将战斗背景从 0% 淡入到 100%，时长可调，保证帧率无关。
	float m_bgFadeAlpha = 0.f;      // 0..1，战斗背景的不透明度
	float m_bgFadeTimer = 0.f;      // 渐变累积计时
	float m_bgFadeDuration = 1.28f; // 渐变总时长（秒），加速至 1.25 倍

	// 入场保留：直到三人到位或达到最大时长
	bool m_introHoldActive = true;
	float m_introHoldElapsed = 0.f;
	float m_introHoldMax = 2.4f;

	// Act 描述播报
	std::vector<sf::String> m_pendingActTexts;
	std::size_t m_actTextIndex = 0;
	bool m_playingActTexts = false;
	sf::String m_actCurrentText;
	std::size_t m_actCharIndex = 0;
	float m_actTimer = 0.f;
	float m_actCharDelay = 0.05f;
	float m_actPauseTimer = 0.f;

	// When true, Act-style text is used for exit message (no dialogue box)
	bool m_isExitText = false;
	bool m_debugDraw = false;

	// 回合计数（进入 Selection 阶段时递增）
	int m_turnCount = 0;

	// Battle box animation
	enum class BoxState { Hidden, Entering, Shown, Exiting };
	std::vector<sf::Texture> m_boxFrames;
	std::optional<sf::Sprite> m_boxSprite;
	BoxState m_boxState = BoxState::Hidden;
	int m_boxFrameIndex = 0;
	float m_boxFrameTimer = 0.f;
	float m_boxFrameTime = 0.03f;
	int m_boxEnterEnd = 17; // 0-based indices, 0-17 entering
	int m_boxExitStart = 18; // 18-45 exiting
	int m_boxExitEnd = 45;
	sf::Vector2f m_boxPosition{0.f, 0.f};
	sf::FloatRect m_boxBounds{};

	// 弹幕系统
	struct ActiveBullet {
		Bullet bullet;
		int damage = 0;
		bool hitsAll = false;
	};
	std::vector<ActiveBullet> m_activeBullets;
	float m_bulletSpawnTimer = 0.f;
	enum class BulletPattern { PatternA, PatternB };
	BulletPattern m_currentPattern = BulletPattern::PatternA;
	sf::Texture m_bulletTexture1;
	sf::Texture m_bulletTexture2;
	bool m_bulletTex1Loaded = false;
	bool m_bulletTex2Loaded = false;
	std::mt19937 m_rng;

	// Holy Mantle (per hero)
	std::vector<bool> m_hasHolyMantle;
	std::vector<bool> m_holyShieldReady;
	bool m_sharedShieldReady = false;
	bool m_shieldAnimPlaying = false;
	int m_shieldAnimFrame = 0;
	float m_shieldAnimTimer = 0.f;
	float m_shieldFrameTime = 0.01f;
	sf::Texture m_holyGlowTex;
	bool m_holyGlowLoaded = false;
	std::vector<sf::Texture> m_holyShieldFrames;
};
