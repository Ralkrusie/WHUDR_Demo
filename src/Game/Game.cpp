#include "Game/Game.h"
#include "States/TitleState.h"
#include "Manager/AudioManager.h"
#include "Game/Database.h"
#include "Game/GlobalContext.h"

Game::Game()
    : m_window(sf::VideoMode({640, 480}), "WHUDR Game Window"),
    ralseiFaceTexture("assets/sprite/Ralsei/spr_face_r_nohat/spr_face_r_nohat_0.png"), 
    susieFaceTexture("assets/sprite/Susie/spr_face_susie_alt/spr_face_susie_alt_2.png"){
    m_window.setFramerateLimit(30);

    // 固定逻辑视口为 640x480，并初始化居中视图
    m_view.setSize({640.f, 480.f});
    m_view.setCenter({320.f, 240.f});
    const auto size = m_window.getSize();
    updateViewViewport(size.x, size.y);
    m_window.setView(m_view);
    // 初始化数据库（武器/护甲/英雄基础数据）
    Database::init();

    // 初始化全局三人实际数据（若尚未由读档覆盖）
    if (Global::partyHeroes.empty()) {
        auto makeHero = [](const Hero& h) {
            HeroRuntime rt;
            rt.id = h.id;
            rt.name = h.name;
            rt.maxHP = h.maxHP;
            rt.hp = h.maxHP; // 开局满血
            rt.baseAttack = h.baseAttack;
            rt.baseDefense = h.baseDefense;
            rt.baseMagic = h.baseMagic;
            rt.weaponID = h.weaponID;
            rt.armorID[0] = h.armorID[0];
            rt.armorID[1] = h.armorID[1];
            return rt;
        };
        // 读取数据库中的三人模板
        if (auto it = Database::heroes.find("kris"); it != Database::heroes.end()) {
            Global::partyHeroes.push_back(makeHero(it->second));
        }
        if (auto it = Database::heroes.find("susie"); it != Database::heroes.end()) {
            Global::partyHeroes.push_back(makeHero(it->second));
        }
        if (auto it = Database::heroes.find("ralsei"); it != Database::heroes.end()) {
            Global::partyHeroes.push_back(makeHero(it->second));
        }
    }

    // 初始给予两瓶珞珈饮品（仅在新游戏时生效：inventory 为空）
    if (Global::inventory.empty()) {
        Global::inventory.push_back(InventoryItem{ "luojia_drink", sf::String(L"珞珈饮品"), sf::String(L"恢复80点生命值。") });
        Global::inventory.push_back(InventoryItem{ "luojia_drink", sf::String(L"珞珈饮品"), sf::String(L"恢复80点生命值。") });
    }

    // 初始状态为标题界面
    pushState(std::make_unique<TitleState>(*this));
    // 音效初始化
    auto& audio = AudioManager::getInstance();
    // 预加载常用音效 (Key, Path)
    audio.loadSound("button_move", "assets/sound/snd_button_move.wav");
    audio.loadSound("button_select", "assets/sound/snd_button_select.wav");
    audio.loadSound("text", "assets/sound/snd_text.wav");
    audio.loadSound("textsusie", "assets/sound/snd_txtsus.wav");
    audio.loadSound("textralsei", "assets/sound/snd_txtral.wav");
    audio.loadSound("save", "assets/sound/snd_save.wav");
}

void Game::run() {
    sf::Clock clock;
    while (m_window.isOpen()) {
        sf::Time dt = clock.restart();
        
        if (!m_states.empty()) {
            // 永远只操作栈顶的那个状态
            // m_states.top()->handleEvent();
            while (const std::optional<sf::Event> event = m_window.pollEvent()) {
                // 这里可以把事件传给当前状态处理输入
                m_states.top()->handleEvent();
                
                if (event->is<sf::Event::Closed>()) {
                    m_window.close();
                }
                // 窗口尺寸变化：更新 viewport 以维持 4:3 并添加黑边
                if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                    updateViewViewport(resized->size.x, resized->size.y);
                    m_window.setView(m_view);
                }
            }
            m_states.top()->update(dt.asSeconds());
            
            // 先清屏为黑色，露出上下/左右黑边
            m_window.clear(sf::Color::Black);
            // 确保使用 letterbox 视图进行绘制
            m_window.setView(m_view);
            m_states.top()->draw(m_window);
            m_window.display();
        }
    }

    // 清理播放完的音效
    AudioManager::getInstance().update();
}

// 计算 letterbox 视口：把 640x480 的内容画进窗口居中的 4:3 区域
void Game::updateViewViewport(unsigned int winW, unsigned int winH) {
    if (winW == 0u || winH == 0u) return;

    const float targetRatio = 4.f / 3.f;   // 640 / 480
    const float windowRatio = static_cast<float>(winW) / static_cast<float>(winH);

    sf::FloatRect viewport;
    if (windowRatio > targetRatio) {
        // 窗口更宽：左右黑边
        const float width = targetRatio / windowRatio; // 0..1 范围
        const float left = (1.f - width) * 0.5f;
        viewport = sf::FloatRect({left, 0.f}, {width, 1.f});
    } else if (windowRatio < targetRatio) {
        // 窗口更高：上下黑边
        const float height = windowRatio / targetRatio; // 0..1 范围
        const float top = (1.f - height) * 0.5f;
        viewport = sf::FloatRect({0.f, top}, {1.f, height});
    } else {
        // 刚好 4:3，铺满
        viewport = sf::FloatRect({0.f, 0.f}, {1.f, 1.f});
    }

    m_view.setViewport(viewport);
}

// ... pushState 等函数的实现 ...
void Game::pushState(std::unique_ptr<BaseState> state) { m_states.push(std::move(state)); }

void Game::popState() {
    if (!m_states.empty()) {
        m_states.pop();
    }
}

void Game::changeState(std::unique_ptr<BaseState> state) {
    if (!m_states.empty()) {
        m_states.pop();
    }
    m_states.push(std::move(state));
}

sf::RenderWindow& Game::getWindow() { return m_window; }

Game::~Game() {
    // 自动清理栈中的状态
    while (!m_states.empty()) {
        m_states.pop();
    }
}
