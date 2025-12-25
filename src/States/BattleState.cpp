//===============================================
// BattleState
// 战斗状态的核心逻辑与渲染实现。
//
// 职责概述：
// - 管理战斗阶段（选择指令、执行指令、弹幕阶段、胜利/失败）
// - 负责底部 UI 菜单、Act 描述的打字机效果、对话框的交互
// - 绘制并驱动“战斗箱”（弹幕盒）的入场/退出动画与心形（Soul）移动边界
// - 维护弹幕生成、更新、碰撞以及“圣斗篷”（Holy Mantle）护盾的触发与表现
// - 处理战斗背景与从 Overworld 捕获的背景的渐隐/渐显效果
// - 加载与更新队伍角色的入场与待机动画（Kris/Susie/Ralsei）
// - 播放相关音效与循环 BGM
//
// 设计要点：
// - 所有入场/退出/渐隐等视觉效果按固定计时器驱动，保证帧率无关性
// - 弹幕与心形碰撞采用矩形相交的简化检测，配合无敌时间（i-frame）避免多次伤害
// - 圣斗篷为“每回合一次”的共享护盾：当队伍中有佩戴且存活的角色时，本回合第一次命中由护盾吸收
// - Act 文本采用底部 UI 的打字机呈现，播放完毕后再执行队列中的行动
// - 战斗箱的显示位置与心形初始位置可微调，以适配具体素材与视觉布局
//===============================================
#include "States/BattleState.h"
#include "States/OverworldState.h"
#include "States/TitleState.h"
#include "Battle/Enemy.h"
#include "Manager/InputManager.h"
#include "Manager/AudioManager.h"
#include <memory>
#include <optional>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>

// 构造函数：
// - 初始化战斗对象、背景素材、战斗箱帧、角色入场动画
// - 预加载音效与 BGM，设置弹幕/护盾相关资源
// - 调整 UI 与视觉位置/缩放，确保与窗口尺寸匹配
BattleState::BattleState(Game& game,
	std::vector<Enemy> enemies,
	std::vector<sf::Vector2f> partyStarts,
	std::optional<sf::Texture> overworldBgTex,
	sf::Vector2f overworldBgScale)
	: BaseState(game), m_battle(std::move(enemies)), m_partyStarts(std::move(partyStarts)), m_overworldBg(std::move(overworldBgTex)), m_overworldBgScale(overworldBgScale), m_rng(std::random_device{}())
{
	// 字体加载（用于敌人信息与底部文字）
	[[maybe_unused]] bool fontOk = m_font.openFromFile("assets/font/Common.ttf");

	// 弹幕碰撞盒（逻辑边界）基础设置
	m_bulletBox.setSize({360.f, 150.f});
	m_bulletBox.setPosition({140.f, 250.f});
	m_bulletBox.setFillColor(sf::Color(0, 0, 0, 0));
	m_bulletBox.setOutlineColor(sf::Color::White);
	m_bulletBox.setOutlineThickness(3.f);
	m_soul.setBounds(m_bulletBox.getGlobalBounds());
	m_prevPhase = m_battle.getPhase();

	// 载入战斗箱入场/退出序列帧，用于显示“盒子”动画
	for (int i = 1; i <= 46; ++i) {
		char path[128];
		std::snprintf(path, sizeof(path), "assets/sprite/Battle Box Sequence/BBS_%04d.png", i);
		sf::Texture tex;
		if (tex.loadFromFile(path)) {
			tex.setSmooth(false);
			m_boxFrames.push_back(std::move(tex));
		}
	}
	if (!m_boxFrames.empty()) {
		m_boxSprite.emplace(m_boxFrames[0]);
		sf::Vector2u sz = m_boxFrames[0].getSize();
		m_boxSprite->setOrigin({static_cast<float>(sz.x) * 0.5f, static_cast<float>(sz.y) * 0.5f});
		m_boxSprite->setScale({0.5f, 0.5f});
		sf::Vector2f viewSize = m_game.getWindow().getView().getSize();
		m_boxPosition = { viewSize.x * 0.5f - 25.f, viewSize.y * 0.50f + 15.f - 100.f };
		updateBattleBoxTransform();
	}

	// 捕获 overworld 背景用于渐隐
	if (m_overworldBg) {
		m_overworldBgSprite.emplace(*m_overworldBg);
		m_overworldBgSprite->setScale(m_overworldBgScale);
	}

	// 预加载战斗背景帧
	char path[64];
	m_battleBgFrames.reserve(100);
	for (int i = 1; i <= 100; ++i) {
		std::snprintf(path, sizeof(path), "assets/sprite/Frames/b%04d.png", i);
		sf::Texture tex;
		if (tex.loadFromFile(path)) {
			tex.setSmooth(false);
			m_battleBgFrames.push_back(std::move(tex));
		}
	}
	if (!m_battleBgFrames.empty()) {
		auto& win = m_game.getWindow();
		m_battleBgSprite.emplace(m_battleBgFrames[0]);
		auto texSize = m_battleBgFrames[0].getSize();
		auto winSize = win.getSize();
	}

	// 音频：战斗入场音效 & 循环 BGM
	auto& audio = AudioManager::getInstance();
	audio.loadSound("battle_intro", "assets/sound/snd_intro_battle.wav");
	audio.playSound("battle_intro");
	audio.playMusic("assets/music/rudebuster_boss.ogg", true);
	audio.loadSound("hurt", "assets/sound/snd_hurt.wav");
	audio.loadSound("holyshield", "assets/sound/snd_holyshield.ogg");

	// 弹幕贴图
	m_bulletTex1Loaded = m_bulletTexture1.loadFromFile("assets/sprite/Bullet/spr_clubsball_a.png");
	m_bulletTex2Loaded = m_bulletTexture2.loadFromFile("assets/sprite/Bullet/spr_diamondbullet.png");
	if (m_bulletTex1Loaded) m_bulletTexture1.setSmooth(false);
	if (m_bulletTex2Loaded) m_bulletTexture2.setSmooth(false);
	// 圣斗篷贴图与破碎动画帧
	m_holyGlowLoaded = m_holyGlowTex.loadFromFile("assets/sprite/Heart/holymantle_glow.png");
	if (m_holyGlowLoaded) m_holyGlowTex.setSmooth(false);
	m_holyShieldFrames.reserve(32);
	for (int i = 0; i <= 20; ++i) {
		char p[96];
		std::snprintf(p, sizeof(p), "assets/sprite/Holyshield/spr_holyshield_break_%d.png", i);
		sf::Texture tex;
		if (tex.loadFromFile(p)) {
			tex.setSmooth(false);
			m_holyShieldFrames.push_back(std::move(tex));
		}
	}

	// 初始化队伍圣斗篷状态
	const auto& partyInit = m_battle.getParty();
	m_hasHolyMantle.resize(partyInit.size(), false);
	m_holyShieldReady.resize(partyInit.size(), false);
	m_sharedShieldReady = false;
	bool anyMantle = false;
	for (std::size_t i = 0; i < partyInit.size(); ++i) {
		bool hasMantle = false;
		for (const auto& armor : partyInit[i].armorID) {
			if (armor == "holy_mantle" || armor == "HolyMantle") { hasMantle = true; break; }
		}
		m_hasHolyMantle[i] = hasMantle;
		m_holyShieldReady[i] = hasMantle;
		anyMantle = anyMantle || hasMantle;
	}
	m_sharedShieldReady = anyMantle;


	// 准备队伍可视化（左侧垂直排列）
	m_partyVisuals.resize(3);
	const std::array<sf::Vector2f, 3> targets = { sf::Vector2f{95.f, 100.f}, sf::Vector2f{50.f, 185.f}, sf::Vector2f{90.f, 265.f} };
	const float desiredIntroTime = 1.f; // 目标：入场约 1 秒完成
	const float minIntroSpeed = 40.f;    // 较低的最小速度，避免瞬移
	for (int i = 0; i < 3; ++i) {
		sf::Vector2f start = (i < static_cast<int>(m_partyStarts.size())) ? m_partyStarts[i] : sf::Vector2f{-80.f, 120.f + 60.f * i};
		sf::Vector2f toTarget{ targets[i].x - start.x, targets[i].y - start.y };
		float dist = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y);
		float speed = (dist > 1e-3f) ? std::max(minIntroSpeed, dist / desiredIntroTime) : minIntroSpeed;
		m_partyVisuals[i].setStartPosition(start);
		m_partyVisuals[i].setTargetPosition(targets[i]);
		m_partyVisuals[i].setMoveSpeed(speed);
		m_partyVisuals[i].setScale({2.0f, 2.0f});
	}

	// 加载各自的 intro / idle 帧（缺失则使用 idle 代替）
	auto krisIntro = BattleActorVisual::loadTextures({
		"assets/sprite/Kris/spr_krisb_intro/spr_krisb_intro_0.png",
		"assets/sprite/Kris/spr_krisb_intro/spr_krisb_intro_1.png",
		"assets/sprite/Kris/spr_krisb_intro/spr_krisb_intro_2.png",
		"assets/sprite/Kris/spr_krisb_intro/spr_krisb_intro_3.png",
		"assets/sprite/Kris/spr_krisb_intro/spr_krisb_intro_4.png",
		"assets/sprite/Kris/spr_krisb_intro/spr_krisb_intro_5.png",
		"assets/sprite/Kris/spr_krisb_intro/spr_krisb_intro_6.png",
		"assets/sprite/Kris/spr_krisb_intro/spr_krisb_intro_7.png",
		"assets/sprite/Kris/spr_krisb_intro/spr_krisb_intro_8.png",
		"assets/sprite/Kris/spr_krisb_intro/spr_krisb_intro_9.png",
		"assets/sprite/Kris/spr_krisb_intro/spr_krisb_intro_10.png",
		"assets/sprite/Kris/spr_krisb_intro/spr_krisb_intro_11.png"
	});
	auto krisIdle = BattleActorVisual::loadTextures({
		"assets/sprite/Kris/spr_krisb_idle/spr_krisb_idle_0.png",
		"assets/sprite/Kris/spr_krisb_idle/spr_krisb_idle_1.png",
		"assets/sprite/Kris/spr_krisb_idle/spr_krisb_idle_2.png",
		"assets/sprite/Kris/spr_krisb_idle/spr_krisb_idle_3.png",
		"assets/sprite/Kris/spr_krisb_idle/spr_krisb_idle_4.png",
		"assets/sprite/Kris/spr_krisb_idle/spr_krisb_idle_5.png"
	});
	m_partyVisuals[0].setIntroFrames(std::move(krisIntro), 0.128f);
	m_partyVisuals[0].setIdleFrames(std::move(krisIdle), 0.12f);

	auto susieIdle = BattleActorVisual::loadTextures({
		"assets/sprite/Susie/spr_susie_idle/spr_susie_idle_0.png",
		"assets/sprite/Susie/spr_susie_idle/spr_susie_idle_1.png",
		"assets/sprite/Susie/spr_susie_idle/spr_susie_idle_2.png",
		"assets/sprite/Susie/spr_susie_idle/spr_susie_idle_3.png"
	});
	
	auto susieIntro = BattleActorVisual::loadTextures({
		"assets/sprite/Susie/spr_susie_attack/spr_susie_attack_0.png",
		"assets/sprite/Susie/spr_susie_attack/spr_susie_attack_1.png",
		"assets/sprite/Susie/spr_susie_attack/spr_susie_attack_2.png",
		"assets/sprite/Susie/spr_susie_attack/spr_susie_attack_3.png"
	});
	m_partyVisuals[1].setIntroFrames(std::move(susieIntro), 0.128f);
	m_partyVisuals[1].setIdleFrames(susieIdle, 0.08f);
	auto susieIdle2 = BattleActorVisual::loadTextures({
		"assets/sprite/Susie/spr_susie_idle/spr_susie_idle_0.png",
		"assets/sprite/Susie/spr_susie_idle/spr_susie_idle_1.png",
		"assets/sprite/Susie/spr_susie_idle/spr_susie_idle_2.png",
		"assets/sprite/Susie/spr_susie_idle/spr_susie_idle_3.png"
	});
	m_partyVisuals[1].setIdleFrames(std::move(susieIdle2), 0.14f);

	auto ralseiIntro = BattleActorVisual::loadTextures({
		"assets/sprite/Ralsei/spr_ralsei_battleintro/spr_ralsei_battleintro_0.png",
		"assets/sprite/Ralsei/spr_ralsei_battleintro/spr_ralsei_battleintro_1.png",
		"assets/sprite/Ralsei/spr_ralsei_battleintro/spr_ralsei_battleintro_2.png",
		"assets/sprite/Ralsei/spr_ralsei_battleintro/spr_ralsei_battleintro_3.png",
		"assets/sprite/Ralsei/spr_ralsei_battleintro/spr_ralsei_battleintro_4.png",
		"assets/sprite/Ralsei/spr_ralsei_battleintro/spr_ralsei_battleintro_5.png",
		"assets/sprite/Ralsei/spr_ralsei_battleintro/spr_ralsei_battleintro_6.png",
		"assets/sprite/Ralsei/spr_ralsei_battleintro/spr_ralsei_battleintro_7.png",
		"assets/sprite/Ralsei/spr_ralsei_battleintro/spr_ralsei_battleintro_8.png",
		"assets/sprite/Ralsei/spr_ralsei_battleintro/spr_ralsei_battleintro_9.png",
		"assets/sprite/Ralsei/spr_ralsei_battleintro/spr_ralsei_battleintro_10.png"
	});
	auto ralseiIdle = BattleActorVisual::loadTextures({
		"assets/sprite/Ralsei/spr_ralsei_idle/spr_ralsei_idle_0.png",
		"assets/sprite/Ralsei/spr_ralsei_idle/spr_ralsei_idle_1.png",
		"assets/sprite/Ralsei/spr_ralsei_idle/spr_ralsei_idle_2.png",
		"assets/sprite/Ralsei/spr_ralsei_idle/spr_ralsei_idle_3.png",
		"assets/sprite/Ralsei/spr_ralsei_idle/spr_ralsei_idle_4.png"
	});
	m_partyVisuals[2].setIntroFrames(std::move(ralseiIntro), 0.128f);
	m_partyVisuals[2].setIdleFrames(std::move(ralseiIdle), 0.12f);

	for (auto& v : m_partyVisuals) v.startIntro();
}

// 输入事件处理：
// - 优先处理胜利/失败后的退出提示与对话确认
// - 选择阶段处理菜单输入（撤销/排队指令/进入行动阶段）
// - 弹幕阶段的心形移动输入在 update() 中按 dt 处理，以更平滑
void BattleState::handleEvent()
{
	sf::RenderWindow& win = m_game.getWindow();

	if (m_waitingForExit) {
		if (m_dialogue.isActive()) {
			if (InputManager::isPressed(Action::Confirm, win)) {
				if (m_dialogue.onConfirm()) {
					tryExitBattle();
				}
			}
		} else if (InputManager::isPressed(Action::Confirm, win)) {
			tryExitBattle();
		}
		return;
	}

	if (m_dialogue.isActive()) {
		if (InputManager::isPressed(Action::Confirm, win)) {
			m_dialogue.onConfirm();
		}
		return;
	}

	// Debug toggle
	if (InputManager::isPressed(Action::Debug, win)) {
		m_debugDraw = !m_debugDraw;
	}

	if (m_battle.getPhase() == BattlePhase::Selection) {
		if (!m_menu.isAwaitingCommand()) {
			refreshMenuIfNeeded();
		}
		BattleMenu::MenuResult res = m_menu.handleInput(win);
		if (res.undoLast) {
			m_battle.undoLastCommand();
		}
		if (res.command) {
			m_battle.queueCommand(*res.command);
			if (!m_menu.isAwaitingCommand()) {
				m_battle.startActionPhase();
			}
		}
	} else if (m_battle.getPhase() == BattlePhase::BulletHell) {
		// Input handled during update for smoother dt-based movement
	}
}

// 帧更新：
// - 驱动背景动画与渐变、入场保留、Act 文本打字机、菜单与战斗逻辑
// - 弹幕阶段更新心形与弹幕、护盾动画；并持续更新队伍入场/待机循环
// - 根据阶段变化触发相应的进入/退出流程（盒子动画、弹幕重置等）
void BattleState::update(float dt)
{
	sf::RenderWindow& win = m_game.getWindow();
	// 入场保留：直到三人到位或达到最大时长
	if (m_introHoldActive) {
		m_introHoldElapsed += dt;
		bool allAtTarget = true;
		for (const auto& v : m_partyVisuals) {
			if (!v.isAtTarget()) { allAtTarget = false; break; }
		}
		if (allAtTarget || m_introHoldElapsed >= m_introHoldMax) {
			m_introHoldActive = false;
		}
	}
	// 背景帧动画（伴随渐变淡入）
	if (!m_battleBgFrames.empty()) {
		m_battleBgTimer += dt;
		while (m_battleBgTimer >= m_battleBgFrameTime && !m_battleBgFrames.empty()) {
			m_battleBgTimer -= m_battleBgFrameTime;
			m_battleBgIndex = (m_battleBgIndex + 1) % static_cast<int>(m_battleBgFrames.size());
			if (m_battleBgSprite) m_battleBgSprite->setTexture(m_battleBgFrames[m_battleBgIndex], true);
		}
	}
	// 背景渐变：按 dt 累加淡入值（0 → 1）
	if (m_bgFadeAlpha < 1.f) {
		m_bgFadeTimer += dt;
		m_bgFadeAlpha = std::min(1.f, m_bgFadeTimer / std::max(0.0001f, m_bgFadeDuration));
	}

	// 对话打字机更新（胜利/退出等）
	m_dialogue.update(dt);

	// Act 描述播报期间暂停战斗计时，等文本播报完再执行指令
	if (!m_playingActTexts) {
		m_battle.update(dt);
	}
	m_menu.update(dt);
	if (m_battle.getPhase() == BattlePhase::BulletHell) {
		m_soul.handleInput(win, dt);
		m_soul.update(dt);
		updateBullets(dt);
	} else if (m_boxState == BoxState::Shown) {
		// 非弹幕阶段时，心形仅响应边界输入
		m_soul.handleInput(win, dt);
		m_soul.update(dt);
	}
	// 战斗角色可视（入场 + 待机）始终更新，保证待机循环
	for (auto& v : m_partyVisuals) v.update(dt);
	updateBattleBox(dt);

	// Act 描述播报推进（底部 UI 区域）
	if (m_playingActTexts) {
		if (m_actCharIndex < m_actCurrentText.getSize()) {
			m_actTimer += dt;
			while (m_actTimer >= m_actCharDelay && m_actCharIndex < m_actCurrentText.getSize()) {
				m_actTimer -= m_actCharDelay;
				m_actCharIndex++;
			}
		} else {
			m_actPauseTimer -= dt;
			if (m_actPauseTimer <= 0.f) {
				if (m_actTextIndex < m_pendingActTexts.size()) {
					m_actCurrentText = m_pendingActTexts[m_actTextIndex];
					m_actTextIndex++;
					m_actCharIndex = 0;
					m_actTimer = 0.f;
					m_actPauseTimer = 0.4f;
				} else {
					m_playingActTexts = false;
					m_pendingActTexts.clear();
					m_actTextIndex = 0;
					if (!m_isExitText) {
						m_battle.executeQueuedCommands();
						if (m_battle.getPhase() != BattlePhase::Victory && m_battle.getPhase() != BattlePhase::GameOver) {
							startBattleBoxEnter();
						}
					} else {
						m_isExitText = false; // 标记已处理退出文本
					}
				}
			}
		}
	}
	// 菜单提示更新（非弹幕阶段且无 Act 文本时显示）
	m_menu.setShowIdleTip(!m_playingActTexts && !m_waitingForExit && m_battle.getPhase() != BattlePhase::BulletHell && !m_introHoldActive);
	handlePhaseTransitions();
	m_prevPhase = m_battle.getPhase();
}

// 渲染：
// - 先绘制战斗背景（不进行渐隐/渐显）
// - 显示战斗箱（或回退矩形），并在需要时绘制碰撞调试框
// - 弹幕阶段绘制弹幕；盒子完全显示时绘制心形与护盾效果
// - 绘制队伍角色入场/待机、敌人信息、底部菜单与 Act 文本、对话框
void BattleState::draw(sf::RenderWindow& window)
{
	// 背景：交叉淡入（Overworld → Battle）
	window.clear(sf::Color(12, 12, 24));
	// 先绘制 Overworld 背景（随渐变淡出）
	if (m_overworldBgSprite) {
		sf::Sprite ow = *m_overworldBgSprite;
		sf::Color c = ow.getColor();
		c.a = static_cast<std::uint8_t>(255.f * std::clamp(1.f - m_bgFadeAlpha, 0.f, 1.f));
		ow.setColor(c);
		window.draw(ow);
	}
	// 再绘制战斗背景（随渐变淡入）
	if (!m_battleBgFrames.empty() && m_battleBgSprite) {
		sf::Sprite bg = *m_battleBgSprite;
		sf::Color c = bg.getColor();
		c.a = static_cast<std::uint8_t>(255.f * std::clamp(m_bgFadeAlpha, 0.f, 1.f));
		bg.setColor(c);
		window.draw(bg);
	}
	// 弹幕盒不再显示，仅保留逻辑边界（入场保留期内不绘制 UI）
	if (m_battle.getPhase() != BattlePhase::Intro && !m_introHoldActive) {
		if (m_boxSprite && m_boxState != BoxState::Hidden) {
			sf::Sprite box = *m_boxSprite;
			window.draw(box);
		} else if (m_boxState != BoxState::Hidden) {
			// Fallback draw if frames failed to load
			sf::Vector2f fbSize = m_bulletBox.getSize();
			if (!m_boxFrames.empty()) {
				sf::Vector2u sz = m_boxFrames[0].getSize();
				fbSize = { static_cast<float>(sz.x) * 0.5f, static_cast<float>(sz.y) * 0.5f };
			}
			sf::RectangleShape fallback(fbSize);
			fallback.setOrigin(fallback.getSize() * 0.5f);
			fallback.setPosition(m_boxPosition);
			fallback.setFillColor(sf::Color(0, 0, 0, 160));
			fallback.setOutlineColor(sf::Color::White);
			fallback.setOutlineThickness(3.f);
			window.draw(fallback);
		}
		if (m_boxState != BoxState::Hidden) {
			// Debug box
			if (m_debugDraw) {
				sf::RectangleShape boxDbg;
				boxDbg.setPosition({ m_boxBounds.position.x, m_boxBounds.position.y });
				boxDbg.setSize(m_boxBounds.size);
				boxDbg.setFillColor(sf::Color(0, 0, 0, 0));
				boxDbg.setOutlineColor(sf::Color(0, 120, 255, 200));
				boxDbg.setOutlineThickness(2.f);
				window.draw(boxDbg);
			}

			// Draw bullets when弹幕阶段
			if (m_battle.getPhase() == BattlePhase::BulletHell) {
				for (const auto& b : m_activeBullets) {
					b.bullet.draw(window);
					if (m_debugDraw) {
						// Debug draw bullet collision bounds
						sf::FloatRect r = b.bullet.getBounds();
						sf::RectangleShape rect;
						rect.setPosition({ r.position.x, r.position.y });
						rect.setSize(r.size);
						rect.setFillColor(sf::Color(255, 0, 255, 30));
						rect.setOutlineColor(sf::Color(255, 0, 255, 180));
						rect.setOutlineThickness(2.f);
						window.draw(rect);
					}
				}
			}

			// Draw soul when 盒子完全显示
			if (m_boxState == BoxState::Shown) {
				if (m_debugDraw) {
					sf::FloatRect soulRect = m_soul.getBounds();
					sf::RectangleShape soulDbg;
					soulDbg.setPosition({ soulRect.position.x, soulRect.position.y });
					soulDbg.setSize(soulRect.size);
					soulDbg.setFillColor(sf::Color(255, 0, 0, 30));
					soulDbg.setOutlineColor(sf::Color(255, 80, 80, 200));
					soulDbg.setOutlineThickness(2.f);
					window.draw(soulDbg);
				}

				// 圣斗篷光晕 & 护盾动画
				bool anyShieldReady = m_sharedShieldReady && std::any_of(m_holyShieldReady.begin(), m_holyShieldReady.end(), [](bool v) { return v; });
				if (anyShieldReady && m_holyGlowLoaded) {
					sf::Sprite glow(m_holyGlowTex);
					glow.setOrigin(glow.getLocalBounds().size * 0.5f);
					glow.setPosition(m_soul.getPosition());
					glow.setColor(sf::Color(255, 255, 255, 128));
					window.draw(glow);
				}

				m_soul.draw(window);

				// 护盾破碎动画
				if (m_shieldAnimPlaying && m_shieldAnimFrame < static_cast<int>(m_holyShieldFrames.size())) {
					sf::Sprite sh(m_holyShieldFrames[static_cast<std::size_t>(m_shieldAnimFrame)]);
					sh.setOrigin(sh.getLocalBounds().size * 0.5f);
					sh.setPosition(m_soul.getPosition());
					sh.setColor(sf::Color(255, 255, 255, 200)); // 80% 不透明度
					window.draw(sh);
				}
			}
		}
	}

	// 三人入场/待机绘制（移至最上层，见下方）

	// 敌人展示（贴图置于右侧 UI 上方）
	const auto& enemies = m_battle.getEnemies();
	sf::Vector2f viewSize = window.getView().getSize();
	float panelTop = viewSize.y * 0.6f + 30.f; // 与底部 UI 对齐的基线
	float baseX = viewSize.x - 260.f;
	float baseY = panelTop - 220.f;
	float gapY = 140.f;
	for (std::size_t i = 0; i < enemies.size(); ++i) {
		sf::Vector2f pos{ baseX, baseY + gapY * static_cast<float>(i) };
		enemies[i].draw(window, m_font, pos);
	}

	if (m_battle.getPhase() != BattlePhase::Intro && !m_introHoldActive) {
		m_menu.draw(window);
	}

	// 底部 UI 区域播放 Act 描述（占用“高数题毫无仁慈”位置）
	if (m_playingActTexts && !m_actCurrentText.isEmpty()) {
		sf::Text t(m_font, m_actCurrentText.substring(0, m_actCharIndex), 20);
		t.setFillColor(sf::Color::White);
		sf::Vector2f viewSize2 = window.getView().getSize();
		float panelTop = viewSize2.y * 0.6f + 30.f;
		t.setPosition({60.f, panelTop + 70.f});
		window.draw(t);
	}

	if (m_dialogue.isActive()) {
		m_dialogue.draw(window);
	}

	// 角色动画置于最上层绘制，确保不被背景/敌人/弹幕覆盖
	for (const auto& v : m_partyVisuals) v.draw(window);
}

// 刷新菜单并将心形重置到盒子中心（新回合开始时）
void BattleState::refreshMenuIfNeeded()
{
	m_menu.beginTurn(m_battle.getParty(), m_battle.getEnemies());
	m_soul.resetToCenter();
}

// 阶段切换处理：
// - Selection：重置护盾、轮换闲置提示、准备菜单
// - ActionExecute：收集 Act 文本并以打字机播放，播放完毕后执行行动并决定是否进入战斗箱
// - BulletHell：设置心形生成高度、同步盒子边界、选择弹幕模式、清空并开始生成弹幕
// - TurnEnd：清理弹幕并播放战斗箱退出
// - Victory/GameOver：进入等待退出状态并使用打字机显示提示
void BattleState::handlePhaseTransitions()
{
	BattlePhase phase = m_battle.getPhase();
	if (phase == BattlePhase::Selection && m_prevPhase != BattlePhase::Selection) {
		resetHolyShieldReady();
		// 新回合开始：记录回合数并根据三回合轮换设置闲置提示
		++m_turnCount;
		int index = (m_turnCount - 1) % 3; // 每回合轮换三条提示
		switch (index) {
		case 0:
			m_menu.setIdleTip(sf::String(L"高数题毫无仁慈。"));
			break;
		case 1:
			m_menu.setIdleTip(sf::String(L"Susie说高数不会就是不会。"));
			break;
		case 2:
			m_menu.setIdleTip(sf::String(L"Ralsei提醒你不定积分不要忘记常数C."));
			break;
		}
		refreshMenuIfNeeded();
	}
	if (phase == BattlePhase::ActionExecute && m_prevPhase == BattlePhase::Selection) {
		m_pendingActTexts.clear();
		m_actTextIndex = 0;
		m_actCurrentText.clear();
		const auto& cmds = m_battle.getCommandQueue();
		for (const auto& c : cmds) {
			if (c.actData.has_value()) {
				m_pendingActTexts.push_back(c.actData->description);
			}
		}
		if (!m_pendingActTexts.empty()) {
			m_playingActTexts = true;
			m_actCurrentText = m_pendingActTexts[0];
			m_actTextIndex = 1;
			m_actCharIndex = 0;
			m_actTimer = 0.f;
			m_actPauseTimer = 0.4f;
		} else {
			//
			m_battle.executeQueuedCommands();
			if (m_battle.getPhase() != BattlePhase::Victory && m_battle.getPhase() != BattlePhase::GameOver) {
				startBattleBoxEnter();
			}
		}
	}
	if (phase == BattlePhase::BulletHell && m_prevPhase == BattlePhase::ActionExecute) {
		m_battle.setBulletExtraWait(5.f);
		m_soul.setSpawnYOffset(-10.f);
		syncSoulToBattleBox();
		// 每个回合轮换弹幕模式
		m_currentPattern = ((m_turnCount % 2) == 1) ? BulletPattern::PatternA : BulletPattern::PatternB;
		m_activeBullets.clear();
		m_bulletSpawnTimer = 0.f;
		// 确保战斗箱已显示
		if (m_boxState == BoxState::Hidden) {
			startBattleBoxEnter();
		}
	}
	if (phase == BattlePhase::TurnEnd && m_prevPhase == BattlePhase::BulletHell) {
		m_activeBullets.clear();
		m_bulletSpawnTimer = 0.f;
		startBattleBoxExit();
	}

	if ((phase == BattlePhase::Victory || phase == BattlePhase::GameOver) && !m_waitingForExit) {
		m_waitingForExit = true;
		m_victory = (phase == BattlePhase::Victory);
		sf::String msg = m_victory ? sf::String(L"胜利！按确认返回教室。") : sf::String(L"你倒下了……按确认回到标题界面。");
		// Act 文本播放完毕后显示退出提示
		m_isExitText = true;
		m_pendingActTexts.clear();
		m_actCurrentText = msg;
		m_playingActTexts = true;
		m_actTextIndex = 0;
		m_actCharIndex = 0;
		m_actTimer = 0.f;
		m_actPauseTimer = 0.4f;
	}
}

// 旧版 HUD 绘制（敌人与日志）：当前逻辑保留、渲染已禁用（主 draw 已覆盖）
void BattleState::drawHUD(sf::RenderWindow& window)
{
	// 敌人绘制与日志
	const auto& enemies = m_battle.getEnemies();
	float startX = 180.f;
	for (std::size_t i = 0; i < enemies.size(); ++i) {
		sf::Vector2f pos{ startX + static_cast<float>(i) * 200.f, 80.f };
		enemies[i].draw(window, m_font, pos);
	}

	const auto& logs = m_battle.getLog();
	float logY = 210.f;
	for (const auto& line : logs) {
		sf::Text t(m_font, line, 20);
		t.setPosition({140.f, logY});
		t.setFillColor(sf::Color(230, 230, 230));
		window.draw(t);
		logY += 24.f;
	}
}

// 退出战斗：
// - 胜利返回 Overworld，失败返回 Title
// - 停止音乐，切换状态
void BattleState::tryExitBattle()
{
	AudioManager::getInstance().stopMusic();
	if (m_victory) {
		m_game.changeState(std::make_unique<OverworldState>(m_game));
	} else {
		m_game.changeState(std::make_unique<TitleState>(m_game));
	}
}

// 战斗箱入场动画：重置帧索引与计时器并进入 Entering 状态
void BattleState::startBattleBoxEnter()
{
	if (!m_boxSprite || m_boxFrames.empty()) return;
	m_boxState = BoxState::Entering;
	m_boxFrameIndex = 0;
	m_boxFrameTimer = 0.f;
	updateBattleBoxTransform();
}

// 战斗箱退出动画：从指定起始帧进入 Exiting 状态，播放至结束隐藏
void BattleState::startBattleBoxExit()
{
	if (!m_boxSprite || m_boxFrames.size() <= static_cast<std::size_t>(m_boxExitStart)) return;
	m_boxState = BoxState::Exiting;
	m_boxFrameIndex = m_boxExitStart;
	m_boxFrameTimer = 0.f;
	updateBattleBoxTransform();
}

// 更新战斗箱显示帧与位置（显示用偏移与缩放）
void BattleState::updateBattleBoxTransform()
{
	if (!m_boxSprite || m_boxFrames.empty()) return;
	m_boxFrameIndex = std::clamp(m_boxFrameIndex, 0, static_cast<int>(m_boxFrames.size()) - 1);
	m_boxSprite->setTexture(m_boxFrames[static_cast<std::size_t>(m_boxFrameIndex)], true);
	// Display-only offset: right 25px, down 100px
	m_boxSprite->setPosition({ m_boxPosition.x + 25.f, m_boxPosition.y + 100.f });
}

// 将心形的移动边界与战斗箱对齐：
// - 使用固定大小的矩形作为碰撞/移动范围
// - 在显示位置上施加少量偏移，便于视觉对齐素材
void BattleState::syncSoulToBattleBox()
{
	if (!m_boxSprite || m_boxState == BoxState::Hidden) return;
	sf::Vector2f size{ 145.f, 145.f };
	m_boxBounds = sf::FloatRect({ m_boxPosition.x - size.x * 0.5f + 28.f, m_boxPosition.y - size.y * 0.5f + 32.f }, size);
	m_soul.setBounds(m_boxBounds);
}

// 战斗箱动画更新：
// - Entering：逐帧到达显示状态，并在首次显示时设置心形初始中心与边界
// - Shown：保持在入场末帧
// - Exiting：逐帧退出直至隐藏
void BattleState::updateBattleBox(float dt)
{
	if (!m_boxSprite || m_boxFrames.empty()) return;
	if (m_boxState == BoxState::Hidden) return;
	m_boxFrameTimer += dt;
	bool dirty = false;
	while (m_boxFrameTimer >= m_boxFrameTime) {
		m_boxFrameTimer -= m_boxFrameTime;
		dirty = true;
		switch (m_boxState) {
		case BoxState::Entering:
			if (m_boxFrameIndex < m_boxEnterEnd) {
				++m_boxFrameIndex;
				if (m_boxFrameIndex >= m_boxEnterEnd) {
					m_boxFrameIndex = m_boxEnterEnd;
					m_boxState = BoxState::Shown;
					m_soul.setCenter({320.f, 180.f});
					syncSoulToBattleBox();
				}
			}
			break;
		case BoxState::Shown:
			m_boxFrameIndex = std::min(m_boxFrameIndex, m_boxEnterEnd);
			break;
		case BoxState::Exiting:
			if (m_boxFrameIndex < m_boxExitEnd) {
				++m_boxFrameIndex;
			} else {
				m_boxState = BoxState::Hidden;
				dirty = false;
				break;
			}
			break;
		case BoxState::Hidden:
			break;
		}
		if (m_boxState == BoxState::Hidden) break;
	}
	if (dirty && m_boxState != BoxState::Hidden) {
		updateBattleBoxTransform();
	}
}

// 弹幕更新：
// - 按当前模式周期生成弹幕，并更新其运动与碰撞
// - 命中时根据共享护盾与防御判定伤害，播放对应音效
// - 命中后设置心形无敌时间，防止短时间内多次伤害
// - 同步护盾破碎动画的逐帧推进
void BattleState::updateBullets(float dt)
{
	if (m_battle.getPhase() != BattlePhase::BulletHell) return;
	float spawnInterval = (m_currentPattern == BulletPattern::PatternA) ? 0.5f : 0.3f;
	m_bulletSpawnTimer += dt;
	while (m_bulletSpawnTimer >= spawnInterval) {
		m_bulletSpawnTimer -= spawnInterval;
		if (m_currentPattern == BulletPattern::PatternA) spawnPatternA();
		else spawnPatternB();
	}

	sf::Vector2f viewSize = m_game.getWindow().getView().getSize();
	sf::FloatRect viewBounds({0.f, 0.f}, viewSize);

	auto soulBounds = m_soul.getBounds();
		auto intersects = [](const sf::FloatRect& a, const sf::FloatRect& b) {
		float ax1 = a.position.x;
		float ay1 = a.position.y;
		float ax2 = ax1 + a.size.x;
		float ay2 = ay1 + a.size.y;
		float bx1 = b.position.x;
		float by1 = b.position.y;
		float bx2 = bx1 + b.size.x;
		float by2 = by1 + b.size.y;
		return !(ax2 < bx1 || ax1 > bx2 || ay2 < by1 || ay1 > by2);
	};
	m_activeBullets.erase(std::remove_if(m_activeBullets.begin(), m_activeBullets.end(), [&](ActiveBullet& b) {
		b.bullet.update(dt);
		bool hitSoul = intersects(soulBounds, b.bullet.getBounds());
		if (hitSoul) {
			if (!m_soul.isInvincible()) {
				DamageResult dmgRes = b.hitsAll ? applyDamageAllHeroes(b.damage) : applyDamageRandomHero(b.damage);
				bool shieldTriggered = dmgRes.shieldTriggered;
				bool tookDamage = dmgRes.damageApplied;
				if (shieldTriggered) {
					AudioManager::getInstance().playSound("holyshield");
					m_shieldAnimPlaying = !m_holyShieldFrames.empty();
					m_shieldAnimFrame = 0;
					m_shieldAnimTimer = 0.f;
				}
				if (tookDamage) {
					AudioManager::getInstance().playSound("hurt");
				}
				m_soul.setInvincible(0.5f);
			}
			return true;
		}
		if (b.bullet.isOffscreen(viewBounds)) return true;
		return false;
	}), m_activeBullets.end());

	if (m_shieldAnimPlaying && !m_holyShieldFrames.empty()) {
		m_shieldAnimTimer += dt;
		while (m_shieldAnimTimer >= m_shieldFrameTime) {
			m_shieldAnimTimer -= m_shieldFrameTime;
			m_shieldAnimFrame++;
			if (m_shieldAnimFrame >= static_cast<int>(m_holyShieldFrames.size())) {
				m_shieldAnimPlaying = false;
				break;
			}
		}
	}
}

// 弹幕模式 A：
// - 在心形周围随机角度生成，朝心形方向匀速推进
// - 伤害值偏高，碰撞框略作偏移用于贴合素材
void BattleState::spawnPatternA()
{
	if (!m_bulletTex1Loaded) return;
	const sf::Vector2f heart = m_soul.getPosition();
	std::uniform_real_distribution<float> angleDist(0.f, 6.2831853f);
	float ang = angleDist(m_rng);
	sf::Vector2f dir{ std::cos(ang), std::sin(ang) };
	sf::Vector2f spawnPos{ heart.x + dir.x * 150.f, heart.y + dir.y * 150.f };
	sf::Vector2f toHeart{ heart.x - spawnPos.x, heart.y - spawnPos.y };
	float len = std::sqrt(toHeart.x * toHeart.x + toHeart.y * toHeart.y);
	if (len < 1e-3f) len = 1.f;
	toHeart.x /= len; toHeart.y /= len;
	float speed = 192.f; // 20% slower
	ActiveBullet ab{ Bullet(m_bulletTexture1, spawnPos, { toHeart.x * speed, toHeart.y * speed }), 15, false };
	ab.bullet.setHitboxOffset({ -2.f, -5.f });
	m_activeBullets.push_back(std::move(ab));
}

// 弹幕模式 B：
// - 在盒子上边界随机 X 生成，竖直下落
// - 作为群体伤害（hitsAll），更容易触发共享护盾
void BattleState::spawnPatternB()
{
	if (!m_bulletTex2Loaded) return;
	float xMin = m_boxBounds.position.x;
	float xMax = m_boxBounds.position.x + m_boxBounds.size.x;
	std::uniform_real_distribution<float> xDist(xMin, xMax);
	float spawnX = xDist(m_rng);
	float spawnY = m_boxBounds.position.y - 8.f;
	float speed = 260.f;
	ActiveBullet ab{ Bullet(m_bulletTexture2, { spawnX, spawnY }, { 0.f, speed }), 10, true };
	ab.bullet.setRotation(90.f);
	m_activeBullets.push_back(std::move(ab));
}

// 对单个随机存活角色结算伤害：
// - 若共享护盾可用且队伍中有圣斗篷存活，则直接吸收一次命中
// - 伤害 = max(0, dmg - 基础防御)；防御中时减半（向上取整）
BattleState::DamageResult BattleState::applyDamageRandomHero(int dmg)
{
	DamageResult res{};
	auto& party = m_battle.partyMutable();
	if (party.empty()) return res;
	if (m_sharedShieldReady && hasMantleAlive()) {
		consumeShield();
		res.shieldTriggered = true;
		return res;
	}
	std::vector<int> alive;
	alive.reserve(party.size());
	for (int i = 0; i < static_cast<int>(party.size()); ++i) {
		if (party[i].hp > 0) alive.push_back(i);
	}
	if (alive.empty()) return res;
	std::uniform_int_distribution<int> idxDist(0, static_cast<int>(alive.size()) - 1);
	int chosen = alive[idxDist(m_rng)];
	int realDmg = std::max(0, dmg - party[chosen].baseDefense);
	if (party[chosen].defending) {
		realDmg = (realDmg + 1) / 2; // 50% 减伤，向上取整
	}
	if (realDmg > 0) {
		int before = party[chosen].hp;
		party[chosen].hp = std::max(0, party[chosen].hp - realDmg);
		res.damageApplied = (party[chosen].hp < before);
	}
	return res;
}

// 对所有存活角色结算群体伤害：
// - 若共享护盾可用且队伍中有圣斗篷存活，则全体伤害被护盾完全吸收一次
// - 个体伤害同样受基础防御与“防御中”状态影响
BattleState::DamageResult BattleState::applyDamageAllHeroes(int dmg)
{
	DamageResult res{};
	auto& party = m_battle.partyMutable();
	if (m_sharedShieldReady && hasMantleAlive()) {
		consumeShield();
		res.shieldTriggered = true;
		return res; 
	}
	for (std::size_t i = 0; i < party.size(); ++i) {
		auto& h = party[i];
		if (h.hp <= 0) continue;
		int realDmg = std::max(0, dmg - h.baseDefense);
		if (h.defending) {
			realDmg = (realDmg + 1) / 2;
		}
		if (realDmg > 0) {
			int before = h.hp;
			h.hp = std::max(0, h.hp - realDmg);
			res.damageApplied = res.damageApplied || (h.hp < before);
		}
	}
	return res;
}

// 判定队伍中是否存在“携带圣斗篷且存活”的角色
bool BattleState::hasMantleAlive() const
{
	const auto& party = m_battle.getParty();
	std::size_t n = std::min(party.size(), m_hasHolyMantle.size());
	for (std::size_t i = 0; i < n; ++i) {
		if (m_hasHolyMantle[i] && party[i].hp > 0) return true;
	}
	return false;
}

// 消耗共享护盾：
// - 关闭当回合的共享护盾，并清空每个角色的护盾就绪状态
void BattleState::consumeShield()
{
	m_sharedShieldReady = false;
	std::fill(m_holyShieldReady.begin(), m_holyShieldReady.end(), false);
}

// 在新回合开始时重置护盾就绪：
// - 对拥有圣斗篷且存活的角色设置就绪
// - 同步共享护盾的总体可用性
// - 清零护盾破碎动画的播放状态
void BattleState::resetHolyShieldReady()
{
	// 根据当前队伍状态重置护盾就绪
	auto& party = m_battle.partyMutable();
	std::size_t n = party.size();
	m_holyShieldReady.resize(n, false);
	m_sharedShieldReady = false;
	for (std::size_t i = 0; i < n; ++i) {
		bool hasMantle = (i < m_hasHolyMantle.size()) ? m_hasHolyMantle[i] : false;
		bool ready = hasMantle && party[i].hp > 0;
		m_holyShieldReady[i] = ready;
		m_sharedShieldReady = m_sharedShieldReady || ready;
	}
	m_shieldAnimPlaying = false;
	m_shieldAnimFrame = 0;
	m_shieldAnimTimer = 0.f;
}
