#include "Overworld/SecretRoom.h"
#include "Game/GlobalContext.h"

void buildSecretRoom(GameMap& map)
{
	map.clear();

	// 背景示意路径，按需替换实际资源
	map.setBackground("assets/sprite/Room/bg_unusedclass_empty.png", {2.5f, 2.5f}, {-80.f, -30.f});
	// 边界墙
	const float thickness = 4.f;
	map.addWall(sf::FloatRect({0.f, 230.f}, {640.f, thickness}));
	map.addWall(sf::FloatRect({0.f, 420.f - thickness}, {640.f, thickness}));
	map.addWall(sf::FloatRect({20.f, 0.f}, {thickness, 480.f}));
	map.addWall(sf::FloatRect({620.f - thickness, 0.f}, {thickness, 480.f}));
	map.addWall(RotRect({150.f, 200.f}, {thickness, 240.f}, 45.f)); // 中间斜墙
	// 可选：简单交互测试区（中心拾取道具），未拾取时才生成
	if (!Global::hasHolyMantle) {
		map.addInteractable(Interactable{
			sf::FloatRect({320.f - 24.f, 240.f - 24.f}, {48.f, 48.f}),
			"holy_mantle",
			0.f,
			RotRect({320.f - 16.f, 240.f - 16.f}, {32.f, 32.f}, 0.f)
		});
		// 显示道具贴图（单帧作为动画道具）
		map.addAnimatedProp({ "assets/sprite/holymantle.png" }, {320.f - 34.f, 240.f - 34.f}, {2.2f, 2.2f}, 0.2f);
	}

	// 存档点（可交互物）：靠近房间中下部区域
	// 说明：与其交互会保存到槽位0，并将当前房间名设为 SecretRoom
	map.addInteractable(Interactable{
		// 交互触发区域（可略大一些，便于触发）
		sf::FloatRect({480.f, 340.f}, {56.f, 56.f}),
		"save_secret", // 特殊ID，OverworldState::checkInteraction 中识别并执行保存
		0.f,
		// 可选的物理碰撞箱（这里给一个小方块作为存档石）
		RotRect({492.f, 352.f}, {32.f, 32.f}, 0.f)
	});

	// 存档点动画（6帧循环）
	{
		std::vector<std::string> frames = {
			"assets/sprite/Save/spr_savepoint_0.png",
			"assets/sprite/Save/spr_savepoint_1.png",
			"assets/sprite/Save/spr_savepoint_2.png",
			"assets/sprite/Save/spr_savepoint_3.png",
			"assets/sprite/Save/spr_savepoint_4.png",
			"assets/sprite/Save/spr_savepoint_5.png",
		};
		// 放在碰撞小方块的同一位置，缩放与房间背景近似
		map.addAnimatedProp(frames, {482.f, 342.f}, {2.5f, 2.5f}, 0.12f);
	}

	// 传送回 AlphysClass（底部中间示意）
	map.addWarp(WarpTrigger{
		sf::FloatRect({270.f,400.f}, {100.f, 40.f}),
		"AlphysClass",
		// 进入 AlphysClass 时：落在 AlphysClass 传送区下方一点
		// AlphysClass 的传送位于 {500,30} 大小 {80,100}，中心 x=540，底 y=130
		sf::Vector2f{540.f, 154.f} // 底部下方 24 像素，避免立即再触发传送
	});
}
