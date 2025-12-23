#include "UI/BattleMenu.h"
#include "Manager/InputManager.h"
#include "Manager/AudioManager.h"
#include "Game/GlobalContext.h"
#include <algorithm>
#include <cctype>

//
// 战斗菜单（BattleMenu）
// ---------------------
// 职责：
// - 指挥每回合的玩家决策：选择角色、行动、子选项与目标
// - 阶段状态机：Hero → Action → Option → Target → Done
// - 游标管理：`m_heroCursor`/`m_actionCursor`/`m_optionCursor`/`m_targetCursor`
// - 结果输出：在确认后构造 `BattleCommand` 返回给战斗状态
// - 绘制 UI：状态栏（头像/HP）、行动图标、子选项列表与敌人信息卡片
// 关键约定：
// - `ActionType` 分为 Fight/Act/Item/Spare/Defend；其中 Fight/Spare 直接选目标，Act/Item 需先选子选项
// - Item 目标为我方成员，其余目标为敌人
// - 头像 variant 根据动作/完成状态选择（见 `actionToHeadVariant`）
// - 面板引入动画使用 `easeOutCubic` 实现上滑效果
//

namespace {
const sf::Color kPanelBg{0, 0, 0, 220};
const sf::Color kPanelOutline{80, 80, 80, 255};
const sf::Color kCyan{40, 140, 200};
const sf::Color kSusie{200, 60, 160};
const sf::Color kRalsei{170, 200, 60};
const sf::Color kOrange{220, 120, 40};
const sf::Color kHPRed{220, 40, 40};
const float kPanelShiftDown = 30.f;

std::string toLowerId(std::string id) {
	for (auto& c : id) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	return id;
}

// 缓动函数：立方缓出（上滑引入时更自然）
float easeOutCubic(float t) {
	t = std::clamp(t, 0.f, 1.f);
	float inv = 1.f - t;
	return 1.f - inv * inv * inv;
}

// 将行动类型映射到头像 variant（用于状态栏显示）
int actionToHeadVariant(ActionType action) {
	switch (action) {
	case ActionType::Fight: return 1;
	case ActionType::Act: return 2;
	case ActionType::Item: return 3;
	case ActionType::Defend: return 4;
	case ActionType::Spare: return 10;
	default: return 0;
	}
}
}

// 构造：加载字体、心形指示器与行动图标；预载头像的多状态贴图
BattleMenu::BattleMenu()
{
	m_actions = { ActionType::Fight, ActionType::Act, ActionType::Item, ActionType::Spare, ActionType::Defend };
	[[maybe_unused]] bool fontOk = m_font.openFromFile("assets/font/Common.ttf");

	[[maybe_unused]] bool heartOk = m_heartTex.loadFromFile("assets/sprite/Heart/spr_heart_0.png");
	m_heartTex.setSmooth(false);
	m_heart.emplace(m_heartTex);
	if (m_heart) m_heart->setScale({1.0f, 1.0f});

	auto loadIcon = [&](int idx, const char* n0, const char* n1) {
		if (idx < 0 || idx >= 5) return;
		if (m_icons[idx].normal.loadFromFile(n0) && m_icons[idx].selected.loadFromFile(n1)) {
			m_icons[idx].normal.setSmooth(false);
			m_icons[idx].selected.setSmooth(false);
			m_icons[idx].sprite.emplace(m_icons[idx].normal);
			m_icons[idx].loaded = true;
		}
	};
	loadIcon(0, "assets/sprite/UI/spr_btfight_0.png", "assets/sprite/UI/spr_btfight_1.png");
	loadIcon(1, "assets/sprite/UI/spr_btact_0.png", "assets/sprite/UI/spr_btact_1.png");
	loadIcon(2, "assets/sprite/UI/spr_btitem_0.png", "assets/sprite/UI/spr_btitem_1.png");
	loadIcon(3, "assets/sprite/UI/spr_btspare_0.png", "assets/sprite/UI/spr_btspare_1.png");
	loadIcon(4, "assets/sprite/UI/spr_btdefend_0.png", "assets/sprite/UI/spr_btdefend_1.png");

	// 头像：加载多状态（0/1/2/3/4/8/10）
	auto loadHeadSet = [&](const std::string& key) {
			std::map<int, sf::Texture> set;
			for (int variant : {0, 1, 2, 3, 4, 8, 10}) {
				std::string path = "assets/sprite/UI/spr_head" + key + "_" + std::to_string(variant) + ".png";
				sf::Texture tex;
				if (tex.loadFromFile(path)) {
					tex.setSmooth(false);
					set.emplace(variant, std::move(tex));
				}
			}
		if (!set.empty()) {
			m_headTextures.emplace(key, std::move(set));
		}
	};
	loadHeadSet("kris");
	loadHeadSet("susie");
	loadHeadSet("ralsei");
}

// 根据行动类型刷新子选项列表（Act 来源于当前角色预设，Item 来源于全局背包）
void BattleMenu::refreshOptionsForAction(ActionType action)
{
	m_options.clear();
	m_pendingAct.reset();
	m_pendingItem.reset();
	m_optionCursor = 0;

	switch (action) {
	case ActionType::Fight:
		m_options.push_back({ sf::String(L"攻击"), sf::String(L"用武器攻击"), std::nullopt, std::nullopt });
		break;
	case ActionType::Act:
		{
			auto heroActs = [&]() {
				std::vector<Option> acts;
				if (!m_partyRef || m_currentHero < 0 || m_currentHero >= static_cast<int>(m_partyRef->size())) {
					acts.push_back({ sf::String(L"查看"), sf::String(L"查看敌方参数"), ActData{ sf::String(L"查看"), sf::String(L"你检查了敌方状态"), 0, 0, 15.f }, std::nullopt });
					return acts;
				}
				auto heroId = toLowerId((*m_partyRef)[m_currentHero].id);
				if (heroId == "kris") {
					acts.push_back({ sf::String(L"查看"), sf::String(L"注意到"), ActData{ sf::String(L"查看"), sf::String(L"你试着使用瞪眼法...\n可惜你不是拉马努金。"), 0, 0, 15.f }, std::nullopt });
					acts.push_back({ sf::String(L"计算"), sf::String(L"尝试计算化简"), ActData{ sf::String(L"计算"), sf::String(L"你尝试着进行计算"), 10, 0, 10.f }, std::nullopt });
					acts.push_back({ sf::String(L"凑微分"), sf::String(L"换元（不包括三角换元）"), ActData{ sf::String(L"凑微分"), sf::String(L"你进行了凑微分变形"), 6, 0, 18.f }, std::nullopt });
				} else if (heroId == "susie") {
					acts.push_back({ sf::String(L"逃跑"), sf::String(L"给你路打油"), ActData{ sf::String(L"逃跑"), sf::String(L"Susie尝试逃跑...\nRalsei制止了她!"), 8, 0, 8.f }, std::nullopt });
					acts.push_back({ sf::String(L"拆分积分"), sf::String(L"将区间拆分后积分"), ActData{ sf::String(L"拆分积分"), sf::String(L"Susie进行了拆分积分"), 14, 0, 6.f }, std::nullopt });
				} else if (heroId == "ralsei") {
					acts.push_back({ sf::String(L"三角代换"), sf::String(L"使用三角代换求解"), ActData{ sf::String(L"三角代换"), sf::String(L"Ralsei使用了三角代换。"), 0, 0, 22.f }, std::nullopt });
					acts.push_back({ sf::String(L"三角恒等变换"), sf::String(L"三角恒等变换"), ActData{ sf::String(L"三角恒等变换"), sf::String(L"Ralsei使用了三角恒等变换。"), 0, 0, 28.f }, std::nullopt });
				} else {
					acts.push_back({ sf::String(L"查看"), sf::String(L"注意到"), ActData{ sf::String(L"查看"), sf::String(L"你试着使用瞪眼法...\n可惜你不是拉马努金。"), 0, 0, 15.f }, std::nullopt });
				}
				return acts;
			}();
			for (auto& opt : heroActs) m_options.push_back(opt);
		}
		break;
	case ActionType::Item:
		if (Global::inventory.empty()) {
			m_options.push_back({ sf::String(L"无物品"), sf::String(L"包里什么也没有"), std::nullopt, std::nullopt });
		} else {
			// 对每种ID，跳过已占用的数量，再展示剩余实例
			std::map<std::string, int> reservedConsumed;
			for (const auto& it : Global::inventory) {
				int reserved = 0;
				auto rcIt = m_reservedItemCounts.find(it.id);
				if (rcIt != m_reservedItemCounts.end()) reserved = std::max(0, rcIt->second);
				int consumed = reservedConsumed[it.id];
				if (consumed < reserved) {
					reservedConsumed[it.id] = consumed + 1;
					continue; // 此实例被占用，跳过
				}
				m_options.push_back({ it.name, it.info, std::nullopt, it.id });
			}
			if (m_options.empty()) {
				m_options.push_back({ sf::String(L"无可用物品"), sf::String(L"已被其他角色占用"), std::nullopt, std::nullopt });
			}
		}
		break;
	case ActionType::Spare:
		m_options.push_back({ sf::String(L"饶恕"), sf::String(L"交卷！"), std::nullopt, std::nullopt });
		break;
	case ActionType::Defend:
		m_options.push_back({ sf::String(L"防御"), sf::String(L"减少伤害"), std::nullopt, std::nullopt });
		break;
	}
}

// 判断该行动是否需要选择目标（Item 目标为我方，其余为敌人）
bool BattleMenu::actionNeedsTarget(ActionType action) const
{
	return action == ActionType::Fight || action == ActionType::Act || action == ActionType::Spare || action == ActionType::Item;
}

// 开始一回合：绑定队伍与敌人引用，重置游标与阶段，并触发面板引入动画
void BattleMenu::beginTurn(const std::vector<HeroRuntime>& party, const std::vector<Enemy>& enemies)
{
	m_partyRef = &party;
	m_enemiesRef = &enemies;
	m_partySize = static_cast<int>(party.size());
	m_enemyCount = static_cast<int>(enemies.size());
	m_currentHero = 0;
	m_heroCursor = 0;
	m_lastCompletedHero = -1;
	m_doneHeroes.assign(m_partySize, false);
	m_committedActions.assign(m_partySize, std::nullopt);
	m_committedItems.assign(m_partySize, std::nullopt);
	m_reservedItemCounts.clear();
	m_completedOrder.clear();
	m_actionCursor = 0;
	m_targetCursor = 0;
	m_optionCursor = 0;
	m_stage = Stage::Hero;
	m_active = m_partySize > 0;
	refreshOptionsForAction(m_actions[m_actionCursor]);
	if (!m_hasShownUI) {
		m_panelReveal = 0.f;
	} else {
		m_panelReveal = 1.f;
	}
}

// 输入处理：分阶段驱动游标与确认，最终产出 BattleCommand
BattleMenu::MenuResult BattleMenu::handleInput(sf::RenderWindow& window)
{
	MenuResult result;
	if (!m_active) return result;

	auto allDone = [&]() {
		return m_partySize > 0 && std::all_of(m_doneHeroes.begin(), m_doneHeroes.end(), [](bool v){ return v; });
	};

	auto nextAvailable = [&](int start, int dir) {
		if (m_partySize == 0) return -1;
		int idx = start;
		for (int i = 0; i < m_partySize; ++i) {
			idx = (idx + dir + m_partySize) % m_partySize;
			if (!m_doneHeroes[idx]) return idx;
		}
		return -1;
	};

	// 阶段 1：选择角色（Hero）
	if (m_stage == Stage::Hero) {
		if (allDone()) { m_stage = Stage::Done; m_active = false; return result; }
		if (m_doneHeroes[m_heroCursor]) {
			int next = nextAvailable(m_heroCursor, +1);
			if (next != -1) m_heroCursor = next;
		}
		if (InputManager::isPressed(Action::Left, window)) {
			int next = nextAvailable(m_heroCursor, -1);
			if (next != -1) m_heroCursor = next;
			AudioManager::getInstance().playSound("button_move");
		}
		if (InputManager::isPressed(Action::Right, window)) {
			int next = nextAvailable(m_heroCursor, +1);
			if (next != -1) m_heroCursor = next;
			AudioManager::getInstance().playSound("button_move");
		}
		if (InputManager::isPressed(Action::Confirm, window)) {
			if (!m_doneHeroes[m_heroCursor]) {
				AudioManager::getInstance().playSound("button_select");
				m_currentHero = m_heroCursor;
				m_stage = Stage::Action;
				m_actionCursor = 0;
				refreshOptionsForAction(m_actions[m_actionCursor]);
			}
		}
		if (InputManager::isPressed(Action::Cancel, window)) {
			if (!m_completedOrder.empty()) {
				int heroToUndo = m_completedOrder.back();
				m_completedOrder.pop_back();
				m_doneHeroes[heroToUndo] = false;
				if (static_cast<int>(m_committedActions.size()) > heroToUndo) {
					// 若撤销的是物品，减少占用计数并清除已提交物品
					if (m_committedActions[heroToUndo].has_value() && *m_committedActions[heroToUndo] == ActionType::Item) {
						if (static_cast<int>(m_committedItems.size()) > heroToUndo && m_committedItems[heroToUndo].has_value()) {
							auto it = m_reservedItemCounts.find(*m_committedItems[heroToUndo]);
							if (it != m_reservedItemCounts.end() && it->second > 0) {
								it->second -= 1;
								if (it->second == 0) m_reservedItemCounts.erase(it);
							}
							m_committedItems[heroToUndo].reset();
						}
					}
					m_committedActions[heroToUndo].reset();
				}
				m_lastCompletedHero = m_completedOrder.empty() ? -1 : m_completedOrder.back();
				m_currentHero = heroToUndo;
				m_heroCursor = heroToUndo;
				m_stage = Stage::Action;
				m_actionCursor = 0;
				refreshOptionsForAction(m_actions[m_actionCursor]);
				m_optionCursor = 0;
				result.undoLast = true;
			}
			AudioManager::getInstance().playSound("button_move");
		}
	// 阶段 2：选择行动（Action）
	} else if (m_stage == Stage::Action) {
		const int actionCount = static_cast<int>(m_actions.size());
		if (InputManager::isPressed(Action::Left, window)) {
			m_actionCursor = (m_actionCursor - 1 + actionCount) % actionCount;
			refreshOptionsForAction(m_actions[m_actionCursor]);
			AudioManager::getInstance().playSound("button_move");
		}
		if (InputManager::isPressed(Action::Right, window)) {
			m_actionCursor = (m_actionCursor + 1) % actionCount;
			refreshOptionsForAction(m_actions[m_actionCursor]);
			AudioManager::getInstance().playSound("button_move");
		}

		if (InputManager::isPressed(Action::Confirm, window)) {
			AudioManager::getInstance().playSound("button_select");
			ActionType chosen = m_actions[m_actionCursor];
			if (chosen == ActionType::Defend) {
				BattleCommand cmd{ m_currentHero, 0, chosen, std::nullopt, std::nullopt };
				result.command = cmd;
				auto finalizeHero = [&](ActionType act){
					m_doneHeroes[m_currentHero] = true;
					m_lastCompletedHero = m_currentHero;
					m_completedOrder.push_back(m_currentHero);
					if (static_cast<int>(m_committedActions.size()) > m_currentHero) m_committedActions[m_currentHero] = act;
					int next = nextAvailable(m_currentHero, +1);
					if (next == -1) { m_active = false; m_stage = Stage::Done; }
					else { m_currentHero = next; m_heroCursor = next; m_stage = Stage::Hero; m_actionCursor = 0; refreshOptionsForAction(m_actions[m_actionCursor]); m_optionCursor = 0; }
				};
				finalizeHero(chosen);
				return result;
			}
			if (chosen == ActionType::Fight || chosen == ActionType::Spare) {
				m_stage = Stage::Target;
				m_targetCursor = 0;
				return result;
			}
			// Act / Item 需要先选子选项
			m_stage = Stage::Option;
			m_optionCursor = 0;
		}
		if (InputManager::isPressed(Action::Cancel, window)) {
			m_stage = Stage::Hero;
			AudioManager::getInstance().playSound("button_move");
		}
	// 阶段 3：选择子选项（Option）
	} else if (m_stage == Stage::Option) {
		if (InputManager::isPressed(Action::Up, window)) {
			if (!m_options.empty()) {
				m_optionCursor = (m_optionCursor - 1 + static_cast<int>(m_options.size())) % static_cast<int>(m_options.size());
				AudioManager::getInstance().playSound("button_move");
			}
		}
		if (InputManager::isPressed(Action::Down, window)) {
			if (!m_options.empty()) {
				m_optionCursor = (m_optionCursor + 1) % static_cast<int>(m_options.size());
				AudioManager::getInstance().playSound("button_move");
			}
		}

		if (InputManager::isPressed(Action::Cancel, window)) {
			m_stage = Stage::Action;
			AudioManager::getInstance().playSound("button_move");
			return result;
		}

		if (InputManager::isPressed(Action::Confirm, window)) {
			AudioManager::getInstance().playSound("button_select");
			ActionType chosen = m_actions[m_actionCursor];
			Option opt = m_options.empty() ? Option{} : m_options[std::clamp(m_optionCursor, 0, static_cast<int>(m_options.size()) - 1)];
			m_pendingAct = opt.act;
			m_pendingItem = opt.itemId;
			// 选择物品后暂存占用计数，避免其他角色重复选择同名物品的过量实例
			if (chosen == ActionType::Item && m_pendingItem.has_value()) {
				m_reservedItemCounts[*m_pendingItem] += 1;
			}
			m_stage = Stage::Target;
			m_targetCursor = 0;
		}
	// 阶段 4：选择目标（Target）
	} else if (m_stage == Stage::Target) {
		ActionType chosen = m_actions[m_actionCursor];
		bool targetingHero = (chosen == ActionType::Item);
		int targetCount = targetingHero ? m_partySize : m_enemyCount;
		if (targetCount <= 0) return result;
		// 支持上下导航（列表为垂直排布），并保留左右导航以兼容原习惯
		if (InputManager::isPressed(Action::Up, window) || InputManager::isPressed(Action::Left, window)) {
			m_targetCursor = (m_targetCursor - 1 + targetCount) % targetCount;
			AudioManager::getInstance().playSound("button_move");
		}
		if (InputManager::isPressed(Action::Down, window) || InputManager::isPressed(Action::Right, window)) {
			m_targetCursor = (m_targetCursor + 1) % targetCount;
			AudioManager::getInstance().playSound("button_move");
		}
		if (InputManager::isPressed(Action::Cancel, window)) {
			if (chosen == ActionType::Fight || chosen == ActionType::Spare) {
				m_stage = Stage::Action;
			} else {
				// 若为物品选择阶段，取消时减少占用计数并清除待提交物品
				if (chosen == ActionType::Item && m_pendingItem.has_value()) {
					auto it = m_reservedItemCounts.find(*m_pendingItem);
					if (it != m_reservedItemCounts.end() && it->second > 0) {
						it->second -= 1;
						if (it->second == 0) m_reservedItemCounts.erase(it);
					}
					m_pendingItem.reset();
				}
				m_stage = Stage::Option;
				refreshOptionsForAction(m_actions[m_actionCursor]);
				m_optionCursor = std::min(m_optionCursor, std::max(0, static_cast<int>(m_options.size()) - 1));
			}
			AudioManager::getInstance().playSound("button_move");
		}
		if (InputManager::isPressed(Action::Confirm, window)) {
			AudioManager::getInstance().playSound("button_select");
			int clampedTarget = std::clamp(m_targetCursor, 0, targetCount - 1);
			BattleCommand cmd{ m_currentHero, clampedTarget, chosen, m_pendingAct, m_pendingItem };
			result.command = cmd;
			m_doneHeroes[m_currentHero] = true;
			m_lastCompletedHero = m_currentHero;
			m_completedOrder.push_back(m_currentHero);
			if (static_cast<int>(m_committedActions.size()) > m_currentHero) {
				m_committedActions[m_currentHero] = chosen;
			}
				// 记录已提交物品，用于撤销时恢复；占用计数保持直到撤销或回合结束
			if (chosen == ActionType::Item && m_pendingItem.has_value()) {
				if (static_cast<int>(m_committedItems.size()) > m_currentHero) {
					m_committedItems[m_currentHero] = m_pendingItem;
				}
			}
			int next = nextAvailable(m_currentHero, +1);
			if (next == -1) {
				m_stage = Stage::Done;
				m_active = false;
			} else {
				m_stage = Stage::Hero;
				m_heroCursor = next;
				m_currentHero = next;
				m_actionCursor = 0;
				refreshOptionsForAction(m_actions[m_actionCursor]);
				m_optionCursor = 0;
			}
			return result;
		}
	}

	return result;
}

// 动效更新：面板引入缓动并记录是否已展示过 UI（避免重复动画）
void BattleMenu::update(float dt)
{
	if (m_panelReveal < 1.f) {
		m_panelReveal = std::min(1.f, m_panelReveal + dt * 1.0f);
		if (m_panelReveal >= 1.f) {
			m_hasShownUI = true;
		}
	}
}

// 绘制状态栏：每个队员的头像、姓名与 HP 条（选中时上移高亮）
void BattleMenu::drawStatus(sf::RenderWindow& window, float yOffset) const
{
	if (!m_partyRef) return;
	const sf::Vector2f viewSize = window.getView().getSize();
	float panelTop = viewSize.y * 0.6f + kPanelShiftDown;
	const float paddingX = 24.f;
	const float gapX = 12.f;
	const int count = std::max(1, m_partySize);
	const float boxW = std::max(140.f, (viewSize.x - 2.f * paddingX - gapX * (count - 1)) / static_cast<float>(count));
	const float boxH = 36.f;
	const float baseY = panelTop + 8.f + yOffset;
	const float liftY = 20.f;

	auto heroColor = [&](const HeroRuntime& h) {
		std::string key = toLowerId(h.id);
		if (key == "susie") return kSusie;
		if (key == "ralsei") return kRalsei;
		return kCyan;
	};

	auto headVariantForHero = [&](int heroIndex) {
		const auto& hero = (*m_partyRef)[heroIndex];
		if (hero.hp <= 0) return 8;
		if (static_cast<int>(m_doneHeroes.size()) > heroIndex && m_doneHeroes[heroIndex]) {
			if (static_cast<int>(m_committedActions.size()) > heroIndex && m_committedActions[heroIndex].has_value()) {
				return actionToHeadVariant(*m_committedActions[heroIndex]);
			}
			return 0;
		}
		bool isActiveHero = false;
		if (m_stage != Stage::Done) {
			if (m_stage == Stage::Hero) isActiveHero = (heroIndex == m_heroCursor);
			else isActiveHero = (heroIndex == m_currentHero);
		}
		if (isActiveHero && (m_stage == Stage::Option || m_stage == Stage::Target)) {
			return actionToHeadVariant(m_actions[m_actionCursor]);
		}
		return 0;
	};

	for (int i = 0; i < m_partySize; ++i) {
		const auto& h = (*m_partyRef)[i];
		bool isCurrent = (m_stage == Stage::Hero ? (i == m_heroCursor) : (i == m_currentHero && m_stage != Stage::Done));
		float x = paddingX + (boxW + gapX) * static_cast<float>(i);
		float y = baseY;
		if (isCurrent && (m_stage == Stage::Action || m_stage == Stage::Option || m_stage == Stage::Target)) {
			// 选中角色时，角色框上移
			y = baseY - liftY;
		}
		sf::Color accent = heroColor(h);

		sf::RectangleShape box({boxW, boxH});
		box.setPosition({x, y});
		box.setFillColor(sf::Color(0, 0, 0, 180));
		box.setOutlineThickness(isCurrent ? 2.f : 1.f);
		box.setOutlineColor(isCurrent ? accent : sf::Color(80, 80, 80));
		window.draw(box);

		std::string key = toLowerId(h.id);
		const sf::Texture* headTex = nullptr;
		auto headIt = m_headTextures.find(key);
		if (headIt != m_headTextures.end() && !headIt->second.empty()) {
			int variant = headVariantForHero(i);
			auto texIt = headIt->second.find(variant);
			if (texIt == headIt->second.end()) texIt = headIt->second.find(0);
			if (texIt == headIt->second.end()) texIt = headIt->second.begin();
			if (texIt != headIt->second.end()) headTex = &texIt->second;
		}
		if (headTex) {
			sf::Sprite head(*headTex);
			head.setScale({1.f, 1.f});
			head.setPosition({x + 6.f, y + 10.f});
			window.draw(head);
		} else {
			sf::CircleShape stub(10.f);
			stub.setFillColor(isCurrent ? accent : sf::Color(120, 120, 120));
			stub.setPosition({x + 10.f, y + 8.f});
			window.draw(stub);
		}

		sf::Text name(m_font, h.name, 18);
		name.setFillColor(sf::Color::White);
		name.setPosition({x + 44.f, y});
		window.draw(name);

		sf::Text hpLabel(m_font, sf::String(L"HP"), 14);
		hpLabel.setFillColor(sf::Color::White);
		hpLabel.setPosition({x + 50.f, y + 18.f});
		window.draw(hpLabel);

		float ratio = (h.maxHP > 0) ? std::clamp(static_cast<float>(h.hp) / static_cast<float>(h.maxHP), 0.f, 1.f) : 0.f;
		float barW = std::max(80.f, boxW - 88.f);
		sf::RectangleShape hpBar({barW, 10.f});
		hpBar.setPosition({x + 76.f, y + 22.f});
		hpBar.setFillColor(sf::Color(30, 30, 30));
		window.draw(hpBar);
		sf::Color hpColor = accent;
		sf::RectangleShape hpFill({barW * ratio, 10.f});
		hpFill.setPosition({x + 76.f, y + 22.f});
		hpFill.setFillColor(hpColor);
		window.draw(hpFill);
		if (ratio < 1.f) {
			float lostW = barW * (1.f - ratio);
			sf::RectangleShape hpLost({lostW, 10.f});
			hpLost.setPosition({x + 76.f + barW * ratio, y + 22.f});
			hpLost.setFillColor(kHPRed);
			window.draw(hpLost);
		}

		sf::Text hpText(m_font, sf::String((std::to_string(h.hp) + std::string("/ ") + std::to_string(h.maxHP)).c_str()), 12);
		hpText.setFillColor(sf::Color::White);
		hpText.setPosition({x + 120.f, y + 2.f});
		window.draw(hpText);
	}
}

// 绘制行动图标：居中排列，当前行动高亮；在 Action/Option/Target 阶段显示
void BattleMenu::drawActions(sf::RenderWindow& window, float yOffset) const
{
	const sf::Vector2f viewSize = window.getView().getSize();
	// 仅当进入 Action/Option/Target 阶段时展示图标，并锚定到当前角色的原始框位置
	if (!(m_stage == Stage::Action || m_stage == Stage::Option || m_stage == Stage::Target)) return;
	int heroIndex = m_currentHero;
	float panelTop = viewSize.y * 0.6f + kPanelShiftDown;
	const float paddingX = 24.f;
	const float gapX = 12.f;
	const int count = std::max(1, m_partySize);
	const float boxW = std::max(140.f, (viewSize.x - 2.f * paddingX - gapX * (count - 1)) / static_cast<float>(count));
	const float boxH = 36.f;
	const float baseY = panelTop + 8.f + yOffset; // 原始位置，不含上移
	float rectX = paddingX + (boxW + gapX) * static_cast<float>(heroIndex);
	float rectY = baseY;
	// 水平居中布置图标
	float spacing = 35.f;
	int n = static_cast<int>(m_actions.size());
	float totalW = spacing * static_cast<float>(n - 1);
	float startX = rectX + (boxW - totalW) * 0.5f;
	float iconBaseY = rectY + (boxH - 32.f) * 0.5f; // 以 32px 高度为参考居中

	for (int i = 0; i < n; ++i) {
		bool selected = ((m_stage == Stage::Action || m_stage == Stage::Option) && i == m_actionCursor);
		const IconPair& icon = m_icons[i];
		float x = startX + spacing * static_cast<float>(i);
		if (icon.loaded && icon.sprite) {
			sf::Sprite sprite = *icon.sprite;
			if (selected) sprite.setTexture(icon.selected, true); else sprite.setTexture(icon.normal, true);
			sprite.setPosition({x, iconBaseY});
			window.draw(sprite);
		} else {
			sf::RectangleShape box({48.f, 32.f});
			box.setPosition({x, iconBaseY});
			box.setFillColor(sf::Color(20, 30, 60, 240));
			box.setOutlineColor(selected ? kOrange : sf::Color(100, 100, 100));
			box.setOutlineThickness(2.f);
			window.draw(box);
		}
	}
}

// 绘制子选项或目标列表：
// - Option 阶段：左侧列表 + 右侧描述；心形指示当前项
// - Target 阶段：Item → 我方列表；否则 → 敌人卡片（含 HP/Mercy 条）
void BattleMenu::drawOptions(sf::RenderWindow& window, float yOffset) const
{
	const sf::Vector2f viewSize = window.getView().getSize();
	float panelTop = viewSize.y * 0.6f + kPanelShiftDown;
	float startY = panelTop + 70.f + yOffset; // 整体上移 30px
	float lineH = 26.f;
	float textX = 60.f;

	bool showingOptions = (m_stage == Stage::Option || m_stage == Stage::Target);

	// 默认显示敌人状态文本
	if (!showingOptions) {
		if (!m_showIdleTip) return;
		sf::Text tip(m_font, m_idleTipText, 18);
		tip.setFillColor(sf::Color(200, 200, 200));
		tip.setPosition({textX, startY});
		window.draw(tip);
		return;
	}

	if (m_stage == Stage::Target) {
		ActionType chosen = m_actions[m_actionCursor];
		if (chosen == ActionType::Item) {
			float y = startY;
			if (!m_partyRef || m_partySize <= 0) {
				sf::Text tip(m_font, sf::String(L"没有可选择的目标"), 18);
				tip.setFillColor(sf::Color(200, 200, 200));
				tip.setPosition({textX, y});
				window.draw(tip);
				return;
			}
			for (int i = 0; i < m_partySize; ++i) {
				float lineY = y + lineH * static_cast<float>(i);
				if (m_heart && i == m_targetCursor) {
					sf::Sprite heart = *m_heart;
					heart.setPosition({textX - 26.f, lineY + 2.f}); // 子选项心形下移 6px
					window.draw(heart);
				}
				sf::Text t(m_font, (*m_partyRef)[i].name, 18);
				t.setFillColor(i == m_targetCursor ? sf::Color(255, 240, 150) : sf::Color::White);
				t.setPosition({textX, lineY});
				window.draw(t);
			}
			return;
		}
		float cardW = 260.f;
		float cardH = 36.f; // 高度为原来约40%
		float gapY = 10.f;
		float areaX = viewSize.x - cardW - 24.f; // 右对齐
		float areaY = startY + 8.f; // 敌人卡片上移 5px
		float barWidth = 80.f;
		float barGap = 14.f;
		float hpX = areaX + 80.f;
		float mercyX = hpX + barWidth + barGap;
		float labelY = areaY - 14.f - 15.f; // HP/Mercy 标签下移 5px
		// 共享的 HP / Mercy 标题
		sf::Text hpLabel(m_font, sf::String(L"HP"), 14);
		hpLabel.setFillColor(sf::Color(220, 220, 220));
		hpLabel.setPosition({hpX, labelY});
		window.draw(hpLabel);
		sf::Text mercyLabel(m_font, sf::String(L"Mercy"), 14);
		mercyLabel.setFillColor(sf::Color(220, 220, 220));
		mercyLabel.setPosition({mercyX, labelY});
		window.draw(mercyLabel);

		for (int i = 0; i < m_enemyCount; ++i) {
			float x = areaX;
			float y = areaY + (cardH + gapY) * static_cast<float>(i) - (i == 0 ? 20.f : 0.f); // 第一张额外上移 20px

			if (m_heart && i == m_targetCursor) {
				sf::Sprite heart = *m_heart;
				heart.setPosition({x - 22.f, y + cardH * 0.5f - 6.f});
				window.draw(heart);
			}

			if (m_enemiesRef && i < static_cast<int>(m_enemiesRef->size())) {
				const auto& e = (*m_enemiesRef)[i];
				sf::Text name(m_font, e.getName(), 18);
				name.setFillColor(sf::Color::White);
				name.setPosition({x + 12.f, y + 6.f});
				window.draw(name);

				float barY = y + 15.f; // HP/Mercy 条整体下移 5px
				float hpRatio = (e.getMaxHP() > 0) ? std::clamp(static_cast<float>(e.getHP()) / static_cast<float>(e.getMaxHP()), 0.f, 1.f) : 0.f;
				float hpLeftW = barWidth * hpRatio;
				float hpRightW = barWidth - hpLeftW;
				if (hpLeftW > 0.f) {
					sf::RectangleShape hpLeft({hpLeftW, 10.f});
					hpLeft.setPosition({hpX, barY});
					hpLeft.setFillColor(sf::Color(70, 190, 90));
					window.draw(hpLeft);
				}
				if (hpRightW > 0.f) {
					sf::RectangleShape hpRight({hpRightW, 10.f});
					hpRight.setPosition({hpX + hpLeftW, barY});
					hpRight.setFillColor(sf::Color(120, 40, 40));
					window.draw(hpRight);
				}

				float mercyRatio = std::clamp(e.getMercy() / 100.f, 0.f, 1.f);
				float mercyLeftW = barWidth * mercyRatio;
				float mercyRightW = barWidth - mercyLeftW;
				if (mercyLeftW > 0.f) {
					sf::RectangleShape mercyLeft({mercyLeftW, 10.f});
					mercyLeft.setPosition({mercyX, barY});
					mercyLeft.setFillColor(sf::Color(240, 200, 40));
					window.draw(mercyLeft);
				}
				if (mercyRightW > 0.f) {
					sf::RectangleShape mercyRight({mercyRightW, 10.f});
					mercyRight.setPosition({mercyX + mercyLeftW, barY});
					mercyRight.setFillColor(sf::Color(160, 100, 40));
					window.draw(mercyRight);
				}
			}
		}
		return;
	}

	if (m_options.empty()) {
		sf::Text tip(m_font, sf::String(L"没有可用选项"), 18);
		tip.setFillColor(sf::Color(180, 180, 180));
		tip.setPosition({textX, startY});
		window.draw(tip);
		return;
	}

	for (std::size_t i = 0; i < m_options.size(); ++i) {
		bool selected = static_cast<int>(i) == m_optionCursor && m_stage == Stage::Option;
		float y = startY + lineH * static_cast<float>(i);
		if (selected && m_heart) {
			sf::Sprite heart = *m_heart;
			heart.setPosition({textX - 26.f, y + 1.f}); // 子选项心形下移 5px
			window.draw(heart);
		}
		sf::Text opt(m_font, m_options[i].label, 18);
		opt.setFillColor(selected ? sf::Color(255, 240, 150) : sf::Color::White);
		opt.setPosition({textX, y});
		window.draw(opt);
	}

	// 描述显示右侧
	int sel = std::clamp(m_optionCursor, 0, static_cast<int>(m_options.size()) - 1);
	sf::Text desc(m_font, m_options[sel].desc, 16);
	desc.setFillColor(sf::Color(200, 200, 200));
	desc.setPosition({textX + 180.f, startY});
	window.draw(desc);
}

// 顶层绘制：面板背景与边框 → 状态栏 → 行动图标 → 子选项/目标区
void BattleMenu::draw(sf::RenderWindow& window) const
{
	const sf::Vector2f viewSize = window.getView().getSize();
	float panelTop = viewSize.y * 0.6f + kPanelShiftDown;
	float yOffset = (1.f - easeOutCubic(m_panelReveal)) * (viewSize.y - panelTop);
	sf::RectangleShape panel({viewSize.x, viewSize.y - panelTop});
	panel.setPosition({0.f, panelTop + yOffset});
	panel.setFillColor(kPanelBg);
	panel.setOutlineThickness(3.f);
	panel.setOutlineColor(kPanelOutline);
	window.draw(panel);

	drawStatus(window, yOffset);
	drawActions(window, yOffset);
	drawOptions(window, yOffset);
}