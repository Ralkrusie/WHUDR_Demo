#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <string>
#include <vector>
#include <optional>

class DialogueBox {
public:
    DialogueBox();

    // 启动一段新的对话
    // text: 要说的话
    // faceTexture: 头像纹理指针 (如果是 nullptr 则不显示头像)
    // voiceBuffer: 声音素材指针
    void start(const sf::String& text, const sf::Texture* faceTexture = nullptr, const sf::SoundBuffer* voiceBuffer = nullptr);

    // 每帧调用
    void update(float dt);
    
    // 渲染
    void draw(sf::RenderWindow& window);

    // 玩家按下了确定键 (Z/Enter)
    // 返回值: true 表示这段话这就结束了(翻页)，false 表示刚刚跳过打字机直接显示全了
    bool onConfirm();

    // 是否正在打字
    bool isTyping() const { return m_charIndex < m_targetText.getSize(); }
    // 对话框是否激活
    bool isActive() const { return m_active; }

private:
    bool m_active = false;

    // --- 文本相关 ---
    sf::Font m_font;
    sf::Text m_renderText;       // 用于显示的文本对象
    sf::String m_targetText;     // 完整的目标文本 (使用 sf::String 支持中文)
    std::size_t m_charIndex = 0; // 当前显示到第几个字了
    
    // --- 计时器 ---
    float m_timer = 0.f;
    float m_charDelay = 0.05f;   // 每个字的间隔 (秒)，越小越快

    // --- 图形元素 ---
    sf::RectangleShape m_boxBg;  // 黑框背景
    sf::RectangleShape m_boxBorder; // 白框边框
    std::optional<sf::Sprite> m_faceSprite;    // 头像
    bool m_hasFace = false;

    // --- 音效 ---
    std::optional<sf::Sound> m_voice;           // 声音播放器
};