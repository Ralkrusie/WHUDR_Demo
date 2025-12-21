#include "BaseState.h"
#include "Game/Game.h"
#include "Manager/InputManager.h"
#include "Manager/AudioManager.h"
#include "OverworldState.h"
#include "Game/GlobalContext.h"
#include "Game/SaveManager.h"
#include "States/BattleState.h"
#include "Battle/Calculus.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>

//
// 探索状态（OverworldState）模块说明
// ------------------------------------
// 负责游戏在“探索/行走”场景下的主循环，包括：
// - 资源加载：字体、背景贴图、背景音乐与角色行走贴图集
// - 输入分发：退出、调试开关、菜单/背包、对话锁定下的选择与推进
// - 地图交互：探测“可交互物体”（存档点、道具、战斗触发等）并驱动 UI
// - 队伍编队：队长与跟随者的历史轨迹与跟随行为
// - 传送与渐变：房间切换的淡入淡出状态机
// - 渲染管理：背景、地图项、角色、对话框、背包 UI、调试覆盖与渐变遮罩
// 关键约定：
// - 角色行走贴图按上下左右四方向、每方向四帧组织为 SpriteSet
// - 交互对象通过 Map::checkInteraction 以“脚前方小矩形传感框”检出
// - 渐变状态机分 None/Out/In；Out 完成后实际切房，随后 In 淡入
// - 背包 UI 打开时锁定输入，不更新角色移动，关闭后恢复
//

namespace {
// 贴图路径：为 Kris 角色构造四个方向的 4 帧行走序列
// 返回的 SpriteSet.frames 为 4 × 4 字符串数组，依次为 Down/Left/Right/Up
SpriteSet makeKrisSprites() {
    SpriteSet set{};
    set.frames = {
        std::array<std::string, 4>{
            "assets/sprite/Kris/spr_krisd_dark/spr_krisd_dark_0.png",
            "assets/sprite/Kris/spr_krisd_dark/spr_krisd_dark_1.png",
            "assets/sprite/Kris/spr_krisd_dark/spr_krisd_dark_2.png",
            "assets/sprite/Kris/spr_krisd_dark/spr_krisd_dark_3.png"},
        std::array<std::string, 4>{
            "assets/sprite/Kris/spr_krisl_dark/spr_krisl_dark_0.png",
            "assets/sprite/Kris/spr_krisl_dark/spr_krisl_dark_1.png",
            "assets/sprite/Kris/spr_krisl_dark/spr_krisl_dark_2.png",
            "assets/sprite/Kris/spr_krisl_dark/spr_krisl_dark_3.png"},
        std::array<std::string, 4>{
            "assets/sprite/Kris/spr_krisr_dark/spr_krisr_dark_0.png",
            "assets/sprite/Kris/spr_krisr_dark/spr_krisr_dark_1.png",
            "assets/sprite/Kris/spr_krisr_dark/spr_krisr_dark_2.png",
            "assets/sprite/Kris/spr_krisr_dark/spr_krisr_dark_3.png"},
        std::array<std::string, 4>{
            "assets/sprite/Kris/spr_krisu_dark/spr_krisu_dark_0.png",
            "assets/sprite/Kris/spr_krisu_dark/spr_krisu_dark_1.png",
            "assets/sprite/Kris/spr_krisu_dark/spr_krisu_dark_2.png",
            "assets/sprite/Kris/spr_krisu_dark/spr_krisu_dark_3.png"}
    };
    return set;
}

// 贴图路径：为 Ralsei 角色构造四方向行走帧序列（每方向 4 帧）
// 命名约定：assets/sprite/Ralsei/spr_ralsei_walk_<dir>/spr_ralsei_walk_<dir>_<frame>.png
SpriteSet makeRalseiSprites() {
    SpriteSet set{};
    set.frames = {
        std::array<std::string, 4>{ "assets/sprite/Ralsei/spr_ralsei_walk_down/spr_ralsei_walk_down_0.png", "assets/sprite/Ralsei/spr_ralsei_walk_down/spr_ralsei_walk_down_1.png", "assets/sprite/Ralsei/spr_ralsei_walk_down/spr_ralsei_walk_down_2.png", "assets/sprite/Ralsei/spr_ralsei_walk_down/spr_ralsei_walk_down_3.png" },
        std::array<std::string, 4>{ "assets/sprite/Ralsei/spr_ralsei_walk_left/spr_ralsei_walk_left_0.png", "assets/sprite/Ralsei/spr_ralsei_walk_left/spr_ralsei_walk_left_1.png", "assets/sprite/Ralsei/spr_ralsei_walk_left/spr_ralsei_walk_left_2.png", "assets/sprite/Ralsei/spr_ralsei_walk_left/spr_ralsei_walk_left_3.png" },
        std::array<std::string, 4>{ "assets/sprite/Ralsei/spr_ralsei_walk_right/spr_ralsei_walk_right_0.png", "assets/sprite/Ralsei/spr_ralsei_walk_right/spr_ralsei_walk_right_1.png", "assets/sprite/Ralsei/spr_ralsei_walk_right/spr_ralsei_walk_right_2.png", "assets/sprite/Ralsei/spr_ralsei_walk_right/spr_ralsei_walk_right_3.png" },
        std::array<std::string, 4>{ "assets/sprite/Ralsei/spr_ralsei_walk_up/spr_ralsei_walk_up_0.png", "assets/sprite/Ralsei/spr_ralsei_walk_up/spr_ralsei_walk_up_1.png", "assets/sprite/Ralsei/spr_ralsei_walk_up/spr_ralsei_walk_up_2.png", "assets/sprite/Ralsei/spr_ralsei_walk_up/spr_ralsei_walk_up_3.png" }
    };
    return set;
}

// 贴图路径：为 Susie 角色构造四方向行走帧序列（每方向 4 帧）
// 资源名后缀 “_dw” 与美术导出约定一致
SpriteSet makeSusieSprites() {
    SpriteSet set{};
    set.frames = {
        std::array<std::string, 4>{ "assets/sprite/Susie/spr_susie_walk_down_dw/spr_susie_walk_down_dw_0.png", "assets/sprite/Susie/spr_susie_walk_down_dw/spr_susie_walk_down_dw_1.png", "assets/sprite/Susie/spr_susie_walk_down_dw/spr_susie_walk_down_dw_2.png", "assets/sprite/Susie/spr_susie_walk_down_dw/spr_susie_walk_down_dw_3.png" },
        std::array<std::string, 4>{ "assets/sprite/Susie/spr_susie_walk_left_dw/spr_susie_walk_left_dw_0.png", "assets/sprite/Susie/spr_susie_walk_left_dw/spr_susie_walk_left_dw_1.png", "assets/sprite/Susie/spr_susie_walk_left_dw/spr_susie_walk_left_dw_2.png", "assets/sprite/Susie/spr_susie_walk_left_dw/spr_susie_walk_left_dw_3.png" },
        std::array<std::string, 4>{ "assets/sprite/Susie/spr_susie_walk_right_dw/spr_susie_walk_right_dw_0.png", "assets/sprite/Susie/spr_susie_walk_right_dw/spr_susie_walk_right_dw_1.png", "assets/sprite/Susie/spr_susie_walk_right_dw/spr_susie_walk_right_dw_2.png", "assets/sprite/Susie/spr_susie_walk_right_dw/spr_susie_walk_right_dw_3.png" },
        std::array<std::string, 4>{ "assets/sprite/Susie/spr_susie_walk_up_dw/spr_susie_walk_up_dw_0.png", "assets/sprite/Susie/spr_susie_walk_up_dw/spr_susie_walk_up_dw_1.png", "assets/sprite/Susie/spr_susie_walk_up_dw/spr_susie_walk_up_dw_2.png", "assets/sprite/Susie/spr_susie_walk_up_dw/spr_susie_walk_up_dw_3.png" }
    };
    return set;
}

// 文本换行（按像素宽度）
// 参数：
// - input：要渲染的 sf::String（支持多语言，含中文）
// - maxWidth：最大行宽（像素），超过则在词/字符处换行
// - font/charSize：测量用字体与字号
// 方法：
// - 使用 sf::Text 实例动态设置 trial 文本并读取其 LocalBounds 宽度
// - 若超过 maxWidth 且当前行非空，则输出当前行并插入换行，重置行缓冲
// - 保留显式换行符 '\n' 的处理；最终追加最后一行
sf::String wrapTextToWidth(const sf::String& input, float maxWidth, const sf::Font& font, unsigned int charSize) {
    sf::Text measure(font, "", charSize);
    sf::String currentLine;
    sf::String output;

    for (std::size_t i = 0; i < input.getSize(); ++i) {
        char32_t c = input[i];
        if (c == '\n') {
            output += currentLine;
            output += '\n';
            currentLine.clear();
            continue;
        }
        sf::String trial = currentLine + c;
        measure.setString(trial);
        if (measure.getLocalBounds().size.x > maxWidth && !currentLine.isEmpty()) {
            output += currentLine;
            output += '\n';
            currentLine.clear();
            currentLine += c;
        } else {
            currentLine += c;
        }
    }
    output += currentLine;
    return output;
}
}

// 构造探索状态：加载资源、初始化队伍与地图，并启动背景音乐
OverworldState::OverworldState(Game& game)
    : BaseState(game),
      m_font("assets/font/Common.ttf"),
      //m_backgroundTexture("assets/sprite/Room/room_alphysclass.png"),
      m_backgroundSprite(m_backgroundTexture),
      m_backgroundMusic("assets/music/Choral_Chambers.mp3"),
    m_kris(game, makeKrisSprites()),
    m_ralsei(game, makeRalseiSprites()),
    m_susie(game, makeSusieSprites())
{
    // 初始化探索状态的元素（字体/背景/音乐）

    if (!m_font.openFromFile("assets/font/Common.ttf")) {
        // 处理字体加载失败
        std::cerr << "Failed to load font!" << std::endl;
    }

    if (!m_backgroundTexture.loadFromFile("assets/sprite/Room/room_alphysclass.png")) {
        // 处理背景纹理加载失败
        std::cerr << "Failed to load overworld background texture!" << std::endl;
    }

    if (!m_backgroundMusic.openFromFile("assets/music/Choral_Chambers.mp3")) {
        // 处理音乐加载失败
        std::cerr << "Failed to load overworld background music!" << std::endl;
    }

    m_backgroundSprite.setPosition({0.f, 0.f});
    m_backgroundSprite.setScale({2.0f, 2.0f});

    m_backgroundMusic.setLooping(true);
    m_backgroundMusic.setVolume(30.f); // 设置适当的音量
    m_backgroundMusic.play();

    // 初始化队伍顺序：默认 Kris 为队长，Susie、Ralsei 跟随
    m_party = { &m_kris, &m_susie, &m_ralsei };
    m_leaderIndex = 0; // Kris 为队长

    // 初始开启调试矩形，可按 D 切换
    m_map.setDebugDraw(m_debugDrawEnabled);

    // 物品栏心形指示器（与标题状态一致资源）
    m_inventoryHeartTex.emplace();
    if (m_inventoryHeartTex->loadFromFile("assets/sprite/Heart/spr_heart_0.png")) {
        m_inventoryHeartTex->setSmooth(false);
        m_inventoryHeart.emplace(*m_inventoryHeartTex);
        m_inventoryHeart->setColor(sf::Color::Red);
        m_inventoryHeart->setScale({1.3f, 1.3f});
    } else {
        m_inventoryHeartTex.reset();
        m_inventoryHeart.reset();
    }

    // 加载地图（房间构建器）并设置初始位置：
    // 若从“继续游戏”进入，依据 Global::currentMRoomName 决定出生点；否则默认教室
    std::string desiredRoom = Global::currentMRoomName.empty() ? std::string("AlphysClass") : Global::currentMRoomName;
    // 兜底：如果不是已知房间，则回落到 AlphysClass（防止默认值为 "Title" 导致地图为空）
    if (desiredRoom != "AlphysClass" && desiredRoom != "SecretRoom") {
        desiredRoom = "AlphysClass";
    }
    sf::Vector2f spawn = {350.f, 300.f};
    if (desiredRoom == "SecretRoom") {
        // 秘密房：存档点旁固定位置出生
        spawn = {500.f, 320.f};
    }
    loadRoom(desiredRoom, spawn);
}

// 顶层输入事件处理：退出键、渐变锁、背包、对话锁、调试、菜单与交互
void OverworldState::handleEvent() {
// 1. 全局退出检查
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
        m_game.getWindow().close();
    }

    // 渐变中不处理输入
    if (m_fadePhase != FadePhase::None) {
        return;
    }

    // 物品栏优先处理（暂停移动）：打开时仅处理背包输入
    if (m_inventoryOpen) {
        handleInventoryInput();
        return;
    }

    // 2. 如果输入被锁定（比如正在对话），把输入权交给 UI
    if (m_isInputLocked) {
        if (m_dialogueBox.isActive()) {
            // 选项模式下的导航
            if (m_dialogueBox.isChoiceActive()) {
                if (InputManager::isPressed(Action::Left, m_game.getWindow())) {
                    m_dialogueBox.moveSelection(-1);
                }
                if (InputManager::isPressed(Action::Right, m_game.getWindow())) {
                    m_dialogueBox.moveSelection(1);
                }
                if (InputManager::isPressed(Action::Confirm, m_game.getWindow())) {
                    if (auto chosen = m_dialogueBox.onConfirmChoice()) {
                        // 处理选择结果
                        if (m_pendingAction == PendingAction::SavePrompt) {
                            if (*chosen == 0) { // 0: 存档
                                Global::currentMRoomName = "SecretRoom";
                                SaveManager::saveGame(0);
                            }
                        }
                        m_pendingAction = PendingAction::None;
                        m_isInputLocked = false;
                    }
                }
            } else {
                // 普通文本对话：按确认推进/关闭；对话结束后可能触发待处理动作
                if (InputManager::isPressed(Action::Confirm, m_game.getWindow())) {
                    if (m_dialogueBox.onConfirm()) {
                        // 对话结束后执行待处理动作（如拾取道具）
                        if (m_pendingAction == PendingAction::CollectHolyMantle) {
                            auto& inv = Global::inventory;
                            auto it = std::find_if(inv.begin(), inv.end(), [](const InventoryItem& item) {
                                return item.id == "HolyMantle";
                            });
                            if (it == inv.end()) {
                                inv.push_back(InventoryItem{ "HolyMantle", L"神圣斗篷", L"免疫每回合第一次受伤。" });
                            }
                            Global::hasHolyMantle = true;
                            // 重新加载当前地图以移除道具贴图与交互，保持当前位置
                            const sf::Vector2f curPos = getLeader().getPosition();
                            loadRoom(m_currentRoom, curPos);
                        } else if (m_pendingAction == PendingAction::StartBattleCalculus) {
                            // 进入战斗：收集三人当前在 Overworld 的位置作为入场起点
                            std::vector<sf::Vector2f> starts;
                            starts.reserve(m_party.size());
                            for (auto* ch : m_party) {
                                // 使用角色中心点，保证与 Battle 中底边中心原点一致
                                starts.push_back(ch->getCenter());
                            }
                            m_pendingAction = PendingAction::None;
                            m_isInputLocked = false;
                            m_game.changeState(std::make_unique<BattleState>(m_game, makeCalculusEncounter(), std::move(starts), m_backgroundTexture, m_backgroundSprite.getScale()));
                            return;
                        }
                        m_pendingAction = PendingAction::None;
                        m_isInputLocked = false; // 解锁玩家控制
                    }
                }
            }
        }
        return; // 既然被锁定了，就不要执行下面的移动/交互逻辑
    }

    // 调试开关：按 D 切换地图碰撞/交互的可视化矩形
    if (InputManager::isPressed(Action::Debug, m_game.getWindow())) {
        m_debugDrawEnabled = !m_debugDrawEnabled;
        m_map.setDebugDraw(m_debugDrawEnabled);
    }

    // 3. 打开物品栏 (按 C 或 Ctrl)
    if (InputManager::isPressed(Action::Menu, m_game.getWindow())) {
        openInventory();
        return;
    }

    // 4. 交互检测 (按 Z 或 Enter)：在脚前探测框内检索可交互对象
    if (InputManager::isPressed(Action::Confirm, m_game.getWindow())) {
        checkInteraction();
    }
}

// 辅助函数：交互判定逻辑（把具体处理拆出来，保持 handleEvent 简洁）
void OverworldState::checkInteraction() {
    // A. 获取玩家中心点（当前队长）
    OverworldCharacter& leader = getLeader();
    sf::Vector2f playerPos = leader.getCenter();
    int dir = leader.getDirection();

    // B. 创建探测框 (Sensor)
    // 探测距离 reach：脚前 40 像素；探测范围 size：20×20 像素
    float reach = 40.f;
    float size = 20.f;
    sf::Vector2f sensorPos = playerPos;

    // 根据朝向偏移探测框
    switch (dir) {
        case 0: sensorPos.y += reach; break; // Down
        case 1: sensorPos.x -= reach; break; // Left
        case 2: sensorPos.x += reach; break; // Right
        case 3: sensorPos.y -= reach; break; // Up
    }

    // 居中 Sensor
    sensorPos.x -= size / 2.f;
    sensorPos.y -= size / 2.f;
    
    sf::FloatRect sensorRect(sensorPos, {size, size});

    // C. 询问地图：这个框里有可交互对象吗？返回 Interactable*，否则 nullptr
    // checkInteraction 返回一个 Interactable 指针，没东西返回 nullptr
    auto* obj = m_map.checkInteraction(sensorRect);
    
    if (obj != nullptr) {
        std::cout << "Interacted with ID: " << obj->textID << std::endl;
        
        // 特殊：存档点 -> 选项对话
        if (obj->textID == "save_secret") {
            const sf::String prompt = L"一抹熟悉的光芒，你不由自主的伸出了手。\n需要存档吗？";
            std::vector<sf::String> options = { L"存档", L"不存" };
            // 使用通用文字语音效果（assets/sound/snd_text.wav，对应 key "text"）
            bool started = m_dialogueBox.startWithChoices(prompt, options, nullptr, std::optional<std::string>{"text"});
            if (started) {
                m_pendingAction = PendingAction::SavePrompt;
                m_isInputLocked = true;
            }
            return;
        }

        // 特殊：Secret Room 中心道具（HolyMantle），一次性拾取（获取后移除）
        if (obj->textID == "holy_mantle") {
            // 若已获得则不再处理（地图构建已避免生成，这里再兜底）
            const sf::String prompt = L"你获得了神圣斗篷！";
            if (Global::hasHolyMantle) return;
            bool started = m_dialogueBox.start(prompt, nullptr, std::optional<std::string>{"text"});
            if (started) {
                m_pendingAction = PendingAction::CollectHolyMantle;
                m_isInputLocked = true;
            }
            return;
        }

        // 教室里的高数题战斗触发：展示提示后切入战斗状态
        if (obj->textID == "talk_test_alphys") {
            const sf::String prompt = L"只是一些高数卷子...";
            bool started = m_dialogueBox.start(prompt, nullptr, std::optional<std::string>{"text"});
            if (started) {
                m_pendingAction = PendingAction::StartBattleCalculus;
                m_isInputLocked = true;
            }
            return;
        }

        // 普通交互：直接把 textID 当展示文本（资源即文本内容或键）
        bool started = m_dialogueBox.start(obj->textID, nullptr, std::nullopt);
        if (started) {
            m_isInputLocked = true;
        }
    }
}

// 主更新循环：地图动画、渐变优先、背包暂停、队伍跟随与传送
void OverworldState::update(float dt) {
    // 地图内动画（例如存档点）始终更新
    m_map.update(dt);

    // 渐变处理优先
    if (m_fadePhase != FadePhase::None) {
        updateFade(dt);
        return;
    }

    // 物品栏开启时暂停角色更新
    if (m_inventoryOpen) {
        return;
    }

    if (m_isInputLocked) {
        // ... 对话框逻辑 ...
        m_dialogueBox.update(dt);
        return;
    }




    // 更新探索状态的逻辑（队长移动与跟随者历史）
    OverworldCharacter& leader = getLeader();

    leader.updateLeader(dt, m_map);
    bool leaderMoving = leader.isMoving();
    PositionRecord rec = leader.getRecord();

    if (leaderMoving) {
        m_leaderHistory.push_front(rec);
        if (m_leaderHistory.size() > m_maxHistory) {
            m_leaderHistory.pop_back();
        }
    } else {
        m_leaderHistory.clear();
        m_leaderHistory.push_front(rec);
    }

    // 传送检查：使用脚底碰撞箱命中 WarpTrigger 后发起淡出
    if (WarpTrigger* warp = m_map.checkWarp(leader.getBounds())) {
        startFadeToRoom(warp->targetMap, warp->targetPos);
        return;
    }

    int followerSlot = 0;
    for (std::size_t i = 0; i < m_party.size(); ++i) {
        if (static_cast<int>(i) == m_leaderIndex) continue;
        m_party[i]->updateFollower(m_leaderHistory, followerSlot, dt, leaderMoving, m_map);
        ++followerSlot;
    }
}

// 绘制流程：背景 → 地图项+角色（按 y 排序） → 对话框 → 背包 UI → 调试 → 渐变遮罩
void OverworldState::draw(sf::RenderWindow& window) {
    // 1) 背景
    m_map.drawBackground(window);

    // 2) 收集需要按 y 排序的绘制项（道具 + 角色）
    std::vector<DrawItem> items;
    items.reserve(m_party.size() + 8);
    m_map.gatherDrawItems(items);
    for (auto* ch : m_party) {
        ch->collectDrawItem(items);
    }

    std::stable_sort(items.begin(), items.end(), [](const DrawItem& a, const DrawItem& b) {
        return a.yKey < b.yKey;
    });
    for (const auto& it : items) {
        if (it.drawable) {
            window.draw(*it.drawable);
        }
    }

    // 3) 对话框（始终在最上层）
    if (m_dialogueBox.isActive()) {
        m_dialogueBox.draw(window);
    }

    // 物品栏 UI（居顶显示）
    if (m_inventoryOpen) {
        drawInventory(window);
    }

    // 4) 调试覆盖（若开启）
    m_map.drawDebugOverlays(window);

    // 5) 渐变遮罩
    drawFade(window);

}

// 获取当前队长引用（便于统一访问）
OverworldCharacter& OverworldState::getLeader() {
    return *m_party[m_leaderIndex];
}

// 构建并加载房间：同步全局房间名、重置队伍位置与历史
void OverworldState::loadRoom(const std::string& roomName, const sf::Vector2f& playerPos)
{
    m_currentRoom = roomName;
    // 同步全局当前房间名，便于存档
    Global::currentMRoomName = roomName;

    if (roomName == "AlphysClass") {
        buildAlphysClass(m_map);
    } else if (roomName == "SecretRoom") {
        buildSecretRoom(m_map);
    } else {
        m_map.clear();
    }

    // 重置队伍位置与历史
    m_kris.setPosition(playerPos);
    if (m_party.size() >= 2) m_party[1]->setPosition(playerPos + sf::Vector2f{-16.f, 12.f});
    if (m_party.size() >= 3) m_party[2]->setPosition(playerPos + sf::Vector2f{16.f, 12.f});

    m_leaderHistory.clear();
    m_leaderHistory.push_front(getLeader().getRecord());
}

// 开始一次房间切换的渐变：先淡出（Out），Out 完成后执行实际换房，再淡入（In）
void OverworldState::startFadeToRoom(const std::string& roomName, const sf::Vector2f& spawn)
{
    m_hasPendingWarp = true;
    m_pendingRoom = roomName;
    m_pendingSpawn = spawn;
    m_fadePhase = FadePhase::Out;
    m_fadeTimer = 0.f;
    m_fadeAlpha = 0.f;
    m_isInputLocked = true;
}

// 渐变更新：根据阶段（Out/In）推进 alpha 与计时器
void OverworldState::updateFade(float dt)
{
    if (m_fadePhase == FadePhase::None) return;
    m_fadeTimer += dt;
    float t = std::min(m_fadeTimer / m_fadeDuration, 1.f);

    if (m_fadePhase == FadePhase::Out) {
        m_fadeAlpha = t;
        if (t >= 1.f) {
            if (m_hasPendingWarp) {
                loadRoom(m_pendingRoom, m_pendingSpawn);
                m_hasPendingWarp = false;
            }
            m_fadePhase = FadePhase::In;
            m_fadeTimer = 0.f;
            m_fadeAlpha = 1.f;
        }
    } else if (m_fadePhase == FadePhase::In) {
        m_fadeAlpha = 1.f - t;
        if (t >= 1.f) {
            m_fadeAlpha = 0.f;
            m_fadePhase = FadePhase::None;
            m_fadeTimer = 0.f;
            m_isInputLocked = false;
        }
    }
}

// 绘制黑色渐变遮罩（覆盖在最上层），alpha 由状态机控制
void OverworldState::drawFade(sf::RenderWindow& window)
{
    if (m_fadePhase == FadePhase::None && m_fadeAlpha <= 0.f) return;
    sf::RectangleShape mask({640.f, 480.f});
    mask.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(255.f * std::clamp(m_fadeAlpha, 0.f, 1.f))));
    mask.setPosition({0.f, 0.f});
    window.draw(mask);
}

// 打开背包 UI：锁定输入，规范化游标位置
void OverworldState::openInventory()
{
    m_inventoryOpen = true;
    m_selectingAction = false;
    m_actionCursor = 0;
    int count = static_cast<int>(Global::inventory.size());
    if (count == 0) {
        m_itemCursor = 0;
    } else if (m_itemCursor >= count) {
        m_itemCursor = std::max(0, count - 1);
    }
    // 暂停移动
    m_isInputLocked = true;
}

// 关闭背包 UI：解除输入锁，复位游标
void OverworldState::closeInventory()
{
    m_inventoryOpen = false;
    m_selectingAction = false;
    m_actionCursor = 0;
    m_isInputLocked = false;
}

// 背包输入：上下移动条目、左右选择动作（使用/丢弃），确认与取消
void OverworldState::handleInventoryInput()
{
    sf::RenderWindow& win = m_game.getWindow();
    const int count = static_cast<int>(Global::inventory.size());

    // 关闭
    if (InputManager::isPressed(Action::Cancel, win) || InputManager::isPressed(Action::Menu, win)) {
        closeInventory();
        return;
    }

    if (!m_selectingAction) {
        if (count == 0) {
            // 空背包：确认直接关闭
            if (InputManager::isPressed(Action::Confirm, win)) {
                AudioManager::getInstance().playSound("button_select");
                closeInventory();
            }
            return;
        }

        if (InputManager::isPressed(Action::Up, win)) {
            AudioManager::getInstance().playSound("button_move");
            m_itemCursor = (m_itemCursor - 1 + count) % count;
        }
        if (InputManager::isPressed(Action::Down, win)) {
            AudioManager::getInstance().playSound("button_move");
            m_itemCursor = (m_itemCursor + 1) % count;
        }

        if (InputManager::isPressed(Action::Confirm, win)) {
            AudioManager::getInstance().playSound("button_select");
            m_selectingAction = true;
            m_actionCursor = 0;
        }
    } else {
        // 选择 使用/丢弃 动作（左右切换，确认执行，取消返回）
        if (InputManager::isPressed(Action::Left, win)) {
            AudioManager::getInstance().playSound("button_move");
            m_actionCursor = 0;
        }
        if (InputManager::isPressed(Action::Right, win)) {
            AudioManager::getInstance().playSound("button_move");
            m_actionCursor = 1;
        }
        if (InputManager::isPressed(Action::Cancel, win)) {
            m_selectingAction = false;
            m_actionCursor = 0;
            return;
        }
        if (InputManager::isPressed(Action::Confirm, win)) {
            AudioManager::getInstance().playSound("button_select");
            if (count == 0 || m_itemCursor >= count) {
                m_selectingAction = false;
                return;
            }
            auto& inv = Global::inventory;
            if (m_actionCursor == 0) {
                // 使用：示例为“神圣斗篷”装备到 Kris 的第二护甲槽
                const std::string itemId = inv[m_itemCursor].id;
                bool consumed = false;
                if (itemId == "HolyMantle") {
                    // 为 Kris 装备二号防具为 holy_mantle
                    auto itHero = std::find_if(Global::partyHeroes.begin(), Global::partyHeroes.end(), [](const HeroRuntime& h) {
                        return h.id == "kris";
                    });
                    if (itHero != Global::partyHeroes.end()) {
                        itHero->armorID[1] = "holy_mantle";
                        Global::hasHolyMantle = true;
                        consumed = true;
                    }
                }
                if (consumed) {
                    inv.erase(inv.begin() + m_itemCursor);
                    int newCount = static_cast<int>(inv.size());
                    if (newCount == 0) {
                        m_itemCursor = 0;
                    } else if (m_itemCursor >= newCount) {
                        m_itemCursor = newCount - 1;
                    }
                }
                m_selectingAction = false;
                m_actionCursor = 0;
            } else {
                // 丢弃：移除物品；若为“神圣斗篷”，同步清除全局标记
                const std::string itemId = inv[m_itemCursor].id;
                inv.erase(inv.begin() + m_itemCursor);
                if (itemId == "HolyMantle") {
                    Global::hasHolyMantle = false;
                }
                int newCount = static_cast<int>(inv.size());
                if (newCount == 0) {
                    m_itemCursor = 0;
                    m_selectingAction = false;
                    m_actionCursor = 0;
                } else {
                    if (m_itemCursor >= newCount) m_itemCursor = newCount - 1;
                    m_selectingAction = false;
                    m_actionCursor = 0;
                }
            }
        }
    }
}

// 背包绘制：整体布局、标题与描述区、列表与光标心形、底部动作区
void OverworldState::drawInventory(sf::RenderWindow& window)
{
    const sf::Vector2f boxSize{520.f, 360.f};
    const sf::Vector2f boxPos{60.f, 60.f};

    sf::RectangleShape bg(boxSize);
    bg.setPosition(boxPos);
    bg.setFillColor(sf::Color(0, 0, 0, 220));
    bg.setOutlineColor(sf::Color::White);
    bg.setOutlineThickness(3.f);
    window.draw(bg);

    auto makeText = [&](const sf::String& str, unsigned int size) {
        sf::Text t(m_font, str, size);
        t.setFillColor(sf::Color::White);
        return t;
    };

    // 标题
    sf::Text title = makeText(sf::String(L"物品"), 28);
    title.setPosition({boxPos.x + boxSize.x * 0.5f - title.getGlobalBounds().size.x * 0.5f, boxPos.y + 12.f});
    window.draw(title);

    // 描述区：显示物品说明或名称，超宽按像素换行
    sf::String descStr = L"没有物品。";
    if (!Global::inventory.empty() && m_itemCursor < static_cast<int>(Global::inventory.size())) {
        const auto& item = Global::inventory[m_itemCursor];
        if (!item.info.isEmpty()) {
            descStr = item.info;
        } else {
            descStr = item.name;
        }
    }
    sf::String descWrapped = wrapTextToWidth(descStr, boxSize.x - 40.f, m_font, 18);
    sf::Text desc = makeText(descWrapped, 18);
    desc.setPosition({boxPos.x + 20.f, boxPos.y + 58.f});
    window.draw(desc);

    // 列表：逐行渲染；选中行高亮并在左侧绘制心形指示器
    float listStartY = boxPos.y + 110.f;
    const float lineH = 32.f;
    for (std::size_t i = 0; i < Global::inventory.size(); ++i) {
        const auto& item = Global::inventory[i];
        sf::Text row = makeText(item.name, 22);
        float rowY = listStartY + lineH * static_cast<float>(i);
        row.setPosition({boxPos.x + 50.f, rowY});
        if (static_cast<int>(i) == m_itemCursor && !m_selectingAction) {
            row.setFillColor(sf::Color::Yellow);
            if (m_inventoryHeart.has_value()) {
                auto bounds = row.getGlobalBounds();
                m_inventoryHeart->setPosition({boxPos.x + 24.f, rowY + bounds.size.y * 0.5f - 5.f});
                window.draw(*m_inventoryHeart);
            }
        }
        window.draw(row);
    }

    // 底部操作：两项横向排列（使用/丢弃），选中项高亮并绘制心形
    const std::array<sf::String, 2> actions = { L"使用", L"丢弃" };
    float actionY = boxPos.y + boxSize.y - 50.f;
    for (int i = 0; i < 2; ++i) {
        sf::Text act = makeText(actions[i], 22);
        float actX = boxPos.x + 120.f + 140.f * static_cast<float>(i);
        act.setPosition({actX, actionY});
        if (m_selectingAction && m_actionCursor == i) {
            act.setFillColor(sf::Color::Yellow);
            if (m_inventoryHeart.has_value()) {
                auto bounds = act.getGlobalBounds();
                m_inventoryHeart->setPosition({actX - 26.f, actionY + bounds.size.y * 0.5f});
                window.draw(*m_inventoryHeart);
            }
        } else {
            act.setFillColor(sf::Color(220, 220, 220));
        }
        window.draw(act);
    }
}

