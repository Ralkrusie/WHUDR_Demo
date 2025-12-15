#include "Game/Game.h"
#include "States/TitleState.h"

Game::Game():
    m_window(sf::VideoMode({640, 480}), "WHUDR Game Window")
{
    m_window.setFramerateLimit(30);
    // 初始状态为标题界面
    pushState(std::make_unique<TitleState>(*this));
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
            }
            m_states.top()->update(dt.asSeconds());
            
            m_window.clear();
            m_states.top()->draw(m_window);
            m_window.display();
        }
    }
}

// ... pushState 等函数的实现 ...
void Game::pushState(std::unique_ptr<BaseState> state) {
    m_states.push(std::move(state));
}

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
sf::RenderWindow& Game::getWindow() {
    return m_window;
}

Game::~Game() {
    // 自动清理栈中的状态
    while (!m_states.empty()) {
        m_states.pop();
    }
}
