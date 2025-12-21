#pragma once
#include <SFML/Graphics.hpp>
#include <queue>
#include <vector>
#include <string>
#include "Game/GlobalContext.h"
#include "Battle/BattleTypes.h"
#include "Battle/Enemy.h"

// 战斗核心逻辑：管理阶段、指令队列与结算
class Battle {
public:
    explicit Battle(std::vector<Enemy> enemies);

    // 输入阶段
    void queueCommand(const BattleCommand& cmd);
    bool undoLastCommand();
    void startSelection();

    // 阶段推进
    void startActionPhase();
    void startBulletHell();
    void startTurnEnd();
    void executeQueuedCommands();
    void update(float dt);
    void setBulletExtraWait(float wait) { m_bulletExtraWait = wait; }

    // 查询
    BattlePhase getPhase() const { return m_phase; }
    const std::vector<Enemy>& getEnemies() const { return m_enemies; }
    const std::vector<BattleCommand>& getCommandQueue() const { return m_commandQueue; }
    std::vector<Enemy>& enemiesMutable() { return m_enemies; }
    const std::vector<HeroRuntime>& getParty() const { return *m_party; }
    std::vector<HeroRuntime>& partyMutable() { return *m_party; }
    const std::vector<sf::String>& getLog() const { return m_log; }
    bool isVictory() const;
    bool isGameOver() const;

private:
    void processQueuedCommands();
    void applyCommand(const BattleCommand& cmd);
    void pushLog(const sf::String& line);

private:
    BattlePhase m_phase = BattlePhase::Intro;
    float m_phaseTimer = 0.f;
    float m_actionDuration = 1.6f;
    float m_bulletDuration = 4.f;
    float m_bulletExtraWait = 0.f;
    std::vector<Enemy> m_enemies;
    std::vector<HeroRuntime>* m_party = nullptr; // 指向 Global::partyHeroes
    std::vector<BattleCommand> m_commandQueue;
    std::vector<sf::String> m_log;
};