#include "Battle/Battle.h"
#include "Battle/Enemy.h"
#include <algorithm>
#include <sstream>

Battle::Battle(std::vector<Enemy> enemies)
	: m_enemies(std::move(enemies))
{
	m_party = &Global::partyHeroes;
	m_phase = BattlePhase::Intro;
	m_phaseTimer = 0.f;
}

void Battle::queueCommand(const BattleCommand& cmd)
{
	m_commandQueue.push_back(cmd);
}

bool Battle::undoLastCommand()
{
	if (m_commandQueue.empty()) return false;
	m_commandQueue.pop_back();
	return true;
}

void Battle::startSelection()
{
	m_phase = BattlePhase::Selection;
	m_phaseTimer = 0.f;
	m_log.clear();
	if (m_party) {
		for (auto& h : *m_party) {
			h.defending = false;
		}
	}
}

void Battle::startActionPhase()
{
	m_phase = BattlePhase::ActionExecute;
	m_phaseTimer = 0.f;
}

void Battle::startBulletHell()

{
	m_phase = BattlePhase::BulletHell;
	m_phaseTimer = 0.f;
}

void Battle::startTurnEnd()
{
	m_phase = BattlePhase::TurnEnd;
	m_phaseTimer = 0.f;
	m_commandQueue.clear();
}

void Battle::executeQueuedCommands()
{
	processQueuedCommands();
	// If victory achieved through actions (e.g., Spare), end before bullet hell
	if (isVictory()) {
		m_phase = BattlePhase::Victory;
		m_phaseTimer = 0.f;
	}
}

void Battle::update(float dt)
{
	m_phaseTimer += dt;

	switch (m_phase) {
	case BattlePhase::Intro:
		if (m_phaseTimer > 1.1f) {
			startSelection();
		}
		break;
	case BattlePhase::ActionExecute:
		if (m_phaseTimer > m_actionDuration) {
			startBulletHell();
		}
		break;
	case BattlePhase::BulletHell:
		if (m_phaseTimer > m_bulletDuration + m_bulletExtraWait) {
			startTurnEnd();
		}
		break;
	case BattlePhase::TurnEnd:
		if (isVictory()) {
			m_phase = BattlePhase::Victory;
		} else if (isGameOver()) {
			m_phase = BattlePhase::GameOver;
		} else {
			m_bulletExtraWait = 0.f;
			startSelection();
		}
		break;
	case BattlePhase::Victory:
	case BattlePhase::GameOver:
	case BattlePhase::Selection:
	default:
		break;
	}
}

bool Battle::isVictory() const
{
	return std::all_of(m_enemies.begin(), m_enemies.end(), [](const Enemy& e) {
		return e.isDefeated() || e.isSpared();
	});
}

bool Battle::isGameOver() const
{
	if (!m_party) return false;
	return std::all_of(m_party->begin(), m_party->end(), [](const HeroRuntime& h) {
		return h.hp <= 0;
	});
}

void Battle::processQueuedCommands()
{
	for (const auto& cmd : m_commandQueue) {
		applyCommand(cmd);
	}
}

void Battle::applyCommand(const BattleCommand& cmd)
{
	if (!m_party || m_party->empty()) return;
	if (cmd.actorIndex < 0 || cmd.actorIndex >= static_cast<int>(m_party->size())) return;

	HeroRuntime& actor = (*m_party)[cmd.actorIndex];
	Enemy* target = nullptr;
	if (cmd.targetIndex >= 0 && cmd.targetIndex < static_cast<int>(m_enemies.size())) {
		target = &m_enemies[cmd.targetIndex];
	}

	switch (cmd.type) {
	case ActionType::Fight: {
		if (!target) break;
		if (target->ignoreDamage()) {
			pushLog(sf::String(actor.name + sf::String(L" 的攻击未对敌人造成影响。")));
			break;
		}
		int dmg = std::max(1, actor.baseAttack + 6);
		target->takeDamage(dmg);
		std::ostringstream oss;
		oss << actor.name.toAnsiString() << " 造成 " << dmg << " 伤害。";
		pushLog(sf::String(oss.str().c_str()));
		break; }
	case ActionType::Act: {
		if (!target) break;
		bool handled = false;
		if (cmd.actData.has_value()) {
			const ActData& act = *cmd.actData;
			handled = target->onAct(act);
			if (!handled) {
				if (act.damage > 0) {
					target->takeDamage(act.damage);
				}
				if (act.mercyAdd > 0.f) {
					target->addMercy(act.mercyAdd);
				}
				if (act.heal > 0) {
					actor.hp = std::min(actor.maxHP, actor.hp + act.heal);
				}
			}
			pushLog(sf::String(actor.name + sf::String(L" 使用行动：") + act.name));
		}
		break; }
	case ActionType::Item: {
		int heal = 50;
		if (cmd.itemID.has_value() && cmd.itemID == std::string("luojia_drink")) {
			heal = 80;
		}
		int targetHeroIdx = std::clamp(cmd.targetIndex, 0, static_cast<int>(m_party->size()) - 1);
		HeroRuntime& targetHero = (*m_party)[targetHeroIdx];
		targetHero.hp = std::min(targetHero.maxHP, targetHero.hp + heal);
		pushLog(actor.name + sf::String(L" 使用道具，回复 ") + targetHero.name + sf::String(L" ") + sf::String(std::to_string(heal).c_str()) + sf::String(L" HP"));
		// 简单移除背包里的第一瓶匹配物品
		if (cmd.itemID.has_value()) {
			auto& bag = Global::inventory;
			auto it = std::find_if(bag.begin(), bag.end(), [&](const InventoryItem& item) {
				return item.id == *cmd.itemID;
			});
			if (it != bag.end()) bag.erase(it);
		}
		break; }
	case ActionType::Spare: {
		if (target) {
			bool spared = target->trySpare();
			if (spared) {
				pushLog(actor.name + sf::String(L" 饶恕了敌人。"));
			} else {
				pushLog(actor.name + sf::String(L" 尝试饶恕，但敌人还没有放过你的意思。"));
			}
		}
		break; }
	case ActionType::Defend: {
		actor.defending = true;
		pushLog(sf::String(actor.name + sf::String(L" 防御，减少即将到来的伤害。")));
		break; }
	}
}

void Battle::pushLog(const sf::String& line)
{
	m_log.push_back(line);
	if (m_log.size() > 4) {
		m_log.erase(m_log.begin());
	}
}
