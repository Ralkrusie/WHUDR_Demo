#include "Overworld/AlphysClass.h"

void buildAlphysClass(GameMap& map)
{
	map.clear();

	// 背景示意路径，按需替换实际资源
	map.setBackground("assets/sprite/Room/room_alphysclass.png", {2.25f, 2.25f}, {-40.f, -30.f});

	// 边界墙（基于 640x480 逻辑坐标，再放大由背景缩放决定）
	const float thickness = 4.f;
	map.addWall(sf::FloatRect({0.f, 120.f}, {640.f, thickness}));               // 顶部
	map.addWall(sf::FloatRect({0.f, 465.f - thickness}, {640.f, thickness})); // 底部
	map.addWall(sf::FloatRect({40.f, 0.f}, {thickness, 480.f}));               // 左侧
	map.addWall(sf::FloatRect({605.f - thickness, 0.f}, {thickness, 480.f})); // 右侧

	// 测试对话交互区（示例位置，可按需调整）
	map.addInteractable(Interactable{
		sf::FloatRect({210.f, 140.f}, {48.f, 24.f}),
		"talk_test_alphys"
	});

	// 讲台贴图与碰撞（贴图约 160x50，放大与背景一致 2.25 倍；碰撞仅作用于下半部分）
	const sf::Vector2f deskPos{185.f, 110.f};
	const sf::Vector2f deskScale{2.25f, 2.25f};
	const sf::Vector2f deskSizeBase{90.f, 30.f};
	const sf::Vector2f deskSize{deskSizeBase.x * deskScale.x, deskSizeBase.y * deskScale.y};
	map.addAnimatedProp({ "assets/sprite/Room/spr_alphysdesk.png" }, deskPos, deskScale, 0.2f);
	// 碰撞箱放在讲台下半部，便于角色靠近讲台前端
	map.addWall(sf::FloatRect({ deskPos.x + 5 , deskPos.y + deskSize.y }, { deskSize.x, deskSize.y * 0.5f }));

	// 右上角传送到 SecretRoom（示意坐标，可按需调整）

	map.addWarp(WarpTrigger{
		sf::FloatRect({490.f, 30.f}, {80.f, 100.f}),
		"SecretRoom",
		// 进入 SecretRoom 时：落在 SecretRoom 传送区上方一点
		// SecretRoom 的返回传送位于 {270,400} 大小 {100,40}，中心 x=320，顶 y=400
		sf::Vector2f{320.f, 376.f} // 顶部上方 24 像素，避免立即再触发传送
	});
}
