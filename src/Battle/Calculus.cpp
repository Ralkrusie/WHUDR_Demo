#include "Battle/Calculus.h"

std::vector<Enemy> makeCalculusEncounter()
{
	std::vector<Enemy> enemies;
	enemies.push_back(Enemy::makeCalculus());
	return enemies;
}
