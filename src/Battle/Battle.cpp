//
// 战斗系统核心实现（Battle.cpp）
//
// 功能概览：
// - 管理战斗阶段：开场(Intro) -> 选择(Selection) -> 行动(ActionExecute) -> 弹幕(BulletHell) -> 回合结束(TurnEnd) -> 胜利/失败
// - 维护我方与敌方状态，处理玩家在选择阶段生成的指令队列并在行动阶段应用
// - 提供胜利/失败判定与简易战斗日志（仅保留最近 4 条）
//
#include "Battle/Battle.h"
#include "Battle/Enemy.h"
#include <algorithm>
#include <sstream>

// 构造：初始化敌人列表、我方队伍指针与战斗初始阶段
Battle::Battle(std::vector<Enemy> enemies)
	: m_enemies(std::move(enemies))
{
	m_party = &Global::partyHeroes;
	m_phase = BattlePhase::Intro;
	m_phaseTimer = 0.f;
}

// 在“选择阶段”入队一条玩家指令
void Battle::queueCommand(const BattleCommand& cmd)
{
	m_commandQueue.push_back(cmd);
}

// 撤销最近一条已选择的指令（若队列为空则返回 false）
bool Battle::undoLastCommand()
{
	if (m_commandQueue.empty()) return false;
	m_commandQueue.pop_back();
	return true;
}

// 进入“选择阶段”：重置计时与日志，并清除我方防御标记
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

// 进入“行动执行阶段”：按队列逐条应用指令
void Battle::startActionPhase()
{
	m_phase = BattlePhase::ActionExecute;
	m_phaseTimer = 0.f;
}

// 进入“弹幕阶段”：敌方攻势/我方躲避
void Battle::startBulletHell()

{
	m_phase = BattlePhase::BulletHell;
	m_phaseTimer = 0.f;
}

// 进入“回合结束阶段”：清空本回合已执行的指令队列
void Battle::startTurnEnd()
{
	m_phase = BattlePhase::TurnEnd;
	m_phaseTimer = 0.f;
	m_commandQueue.clear();
}

// 执行本回合已选择的全部指令
// 若因行动（如饶恕）直接达成胜利，则在弹幕前结束战斗
void Battle::executeQueuedCommands()
{
	processQueuedCommands();
	// If victory achieved through actions (e.g., Spare), end before bullet hell
	if (isVictory()) {
		m_phase = BattlePhase::Victory;
		m_phaseTimer = 0.f;
	}
}

// 主更新：驱动阶段状态机，根据计时与条件进行流转
void Battle::update(float dt)
{
	m_phaseTimer += dt;

	switch (m_phase) {
	case BattlePhase::Intro:
		// 开场动画播放完成 -> 进入选择阶段
		if (m_phaseTimer > 1.1f) {
			startSelection();
		}
		break;
	case BattlePhase::ActionExecute:
		// 行动持续时间结束 -> 进入弹幕阶段
		if (m_phaseTimer > m_actionDuration) {
			startBulletHell();
		}
		break;
	case BattlePhase::BulletHell:
		// 弹幕+额外等待结束 -> 进入回合结束阶段
		if (m_phaseTimer > m_bulletDuration + m_bulletExtraWait) {
			startTurnEnd();
		}
		break;
	case BattlePhase::TurnEnd:
		// 结算：胜利/失败/否则开启新一轮选择
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

// 胜利判定：所有敌人均为“被击败”或“已被饶恕”
bool Battle::isVictory() const
{
	return std::all_of(m_enemies.begin(), m_enemies.end(), [](const Enemy& e) {
		return e.isDefeated() || e.isSpared();
	});
}

// 失败判定：存在我方队伍且所有成员 HP <= 0
bool Battle::isGameOver() const
{
	if (!m_party) return false;
	return std::all_of(m_party->begin(), m_party->end(), [](const HeroRuntime& h) {
		return h.hp <= 0;
	});
}

// 处理本回合的所有已选择指令（逐条应用）
void Battle::processQueuedCommands()
{
	for (const auto& cmd : m_commandQueue) {
		applyCommand(cmd);
	}
}

// 应用单条指令到战斗状态：依据类型进行伤害、治疗、仁慈值、标记等结算
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
		// 普通攻击：若目标可受伤，则计算伤害并记录日志
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
		// 行动：优先让敌人自定义处理；否则按通用字段结算（伤害/仁慈/治疗）
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
		// 物品：根据 ID 决定治疗量，应用到目标我方角色，并尝试从背包移除一件对应物品
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
		// 饶恕：若敌人满足条件，则设置为“被饶恕”并记录日志
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
		// 防御：标记角色为防御状态，弹幕阶段降低伤害
		actor.defending = true;
		pushLog(sf::String(actor.name + sf::String(L" 防御，减少即将到来的伤害。")));
		break; }
	}
}

// 记录一条战斗日志：仅保留最近 4 条，便于 UI 展示
void Battle::pushLog(const sf::String& line)
{
	m_log.push_back(line);
	if (m_log.size() > 4) {
		m_log.erase(m_log.begin());
	}
}
