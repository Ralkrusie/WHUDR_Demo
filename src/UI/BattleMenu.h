#pragma once
#include <SFML/Graphics.hpp>
#include <optional>
#include <vector>
#include <algorithm>
#include <map>
#include <map>
#include "Battle/Battle.h"
#include "Battle/Enemy.h"

// 负责菜单输入与光标绘制（Deltarune 风格）
class BattleMenu {
public:
	BattleMenu();

	void beginTurn(const std::vector<HeroRuntime>& party, const std::vector<Enemy>& enemies);
	struct MenuResult {
		std::optional<BattleCommand> command;
		bool undoLast = false;
	};
	MenuResult handleInput(sf::RenderWindow& window);
	void update(float dt);
	void draw(sf::RenderWindow& window) const;
	void setShowIdleTip(bool show) { m_showIdleTip = show; }
	void setIdleTip(const sf::String& tip) { m_idleTipText = tip; }

	bool isAwaitingCommand() const { return m_active && !m_doneHeroes.empty() && std::count(m_doneHeroes.begin(), m_doneHeroes.end(), false) > 0; }
	int currentHeroIndex() const { return m_currentHero; }

private:
	enum class Stage { Hero, Action, Option, Target, Done };

	struct Option {
		sf::String label;
		sf::String desc;
		std::optional<ActData> act;
		std::optional<std::string> itemId;
	};

	struct IconPair {
		sf::Texture normal;
		sf::Texture selected;
		std::optional<sf::Sprite> sprite;
		bool loaded = false;
	};

	void refreshOptionsForAction(ActionType action);
	bool actionNeedsTarget(ActionType action) const;
	void drawStatus(sf::RenderWindow& window, float yOffset) const;
	void drawActions(sf::RenderWindow& window, float yOffset) const;
	void drawOptions(sf::RenderWindow& window, float yOffset) const;

private:
	Stage m_stage = Stage::Done;
	bool m_active = false; // 当前回合是否仍在收集输入
	int m_currentHero = 0; // 正在操作的角色索引
	int m_heroCursor = 0; // Hero 阶段的游标
	int m_lastCompletedHero = -1; // 最近提交过行动的角色（用于撤销）
	std::vector<bool> m_doneHeroes; // 每个角色是否已提交行动
	int m_actionCursor = 0; // Action 阶段游标
	int m_optionCursor = 0; // Option 阶段游标
	int m_targetCursor = 0; // Target 阶段游标
	int m_partySize = 0;
	int m_enemyCount = 0;

	const std::vector<HeroRuntime>* m_partyRef = nullptr;

	sf::Font m_font;
	sf::Texture m_heartTex;
	std::optional<sf::Sprite> m_heart;
	std::vector<ActionType> m_actions; // 固定顺序的顶层行动列表
	std::vector<Option> m_options; // 当前 Action 下的子选项（Act/Item 等）
	std::optional<ActData> m_pendingAct; // 在 Option 阶段缓存的 Act 数据
	std::optional<std::string> m_pendingItem; // 在 Option 阶段缓存的物品 ID
	std::vector<std::optional<ActionType>> m_committedActions; // 已提交行动的记录，供头像 variant 及撤销使用
	std::vector<std::optional<std::string>> m_committedItems; // 每个角色本回合已提交的物品ID
	std::map<std::string, int> m_reservedItemCounts; // 本回合暂存占用的物品计数（支持同名多件）
	std::vector<int> m_completedOrder; // 已完成角色的顺序栈（用于多次撤销）
	float m_panelReveal = 0.f; // 0 -> hidden below, 1 -> fully shown
	bool m_hasShownUI = false; // 是否已播放过面板上滑动画

	IconPair m_icons[5]; // Fight, Act, Item, Spare, Defend
	std::map<std::string, std::map<int, sf::Texture>> m_headTextures;
	const std::vector<Enemy>* m_enemiesRef = nullptr;
	bool m_showIdleTip = true;
	sf::String m_idleTipText = sf::String(L"高数题毫无仁慈。");
};
