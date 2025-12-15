#include "UI/DialogBox.h"
#include <iostream>

// 构造函数：初始化外观
DialogueBox::DialogueBox():
    m_renderText(m_font){
    // 1. 加载字体 (必须是支持中文的字体！例如 simhei.ttf 或像素字体)
    // 注意：实际项目中建议通过 ResourceManager 获取，这里为了演示直接加载
    if (!m_font.openFromFile("assets/font/Common.ttf")) {
        // 如果加载失败，回退到默认或者报错
        std::cerr << "Failed to load font!" << std::endl;
    }
    m_renderText.setCharacterSize(24); // 字体大小
    m_renderText.setFillColor(sf::Color::White);

    // 2. 设置对话框背景 (屏幕底部)
    // 窗口大小是 640*480，我们在底部留出一点空间
    m_boxBg.setSize({540.f, 180.f});
    m_boxBg.setPosition({50.f, 280.f});
    m_boxBg.setFillColor(sf::Color::Black);

    // 3. 设置边框
    m_boxBorder.setSize({540.f, 180.f});
    m_boxBorder.setPosition({50.f, 280.f});
    m_boxBorder.setFillColor(sf::Color::Transparent);
    m_boxBorder.setOutlineColor(sf::Color::White);
    m_boxBorder.setOutlineThickness(2.f);
}

void DialogueBox::start(const sf::String& text, const sf::Texture* faceTexture, const sf::SoundBuffer* voiceBuffer) {
    m_active = true;
    m_targetText = text;
    m_charIndex = 0;
    m_timer = 0.f;
    m_renderText.setString(""); // 清空当前显示

    // 设置头像
    if (faceTexture) {
        m_hasFace = true;
        // 核心修改：构造一个新的 Sprite 放入 optional
        m_faceSprite.emplace(*faceTexture); 
        
        // 使用 -> 访问成员
        m_faceSprite->setPosition({60.f, 300.f});
        m_renderText.setPosition({150.f, 300.f}); // 文字起始位置：有头像时，文字靠右
    } else {
        m_hasFace = false;
        m_faceSprite.reset(); // 清空，表示现在没头像
        // 文字起始位置：无头像时，文字靠左
        m_renderText.setPosition({70.f, 300.f});
    }

    // 设置声音
    if (voiceBuffer) {
        // 核心修改：构造 Sound
        m_voice.emplace(*voiceBuffer);
    } else {
        m_voice.reset();
    }
}

void DialogueBox::update(float dt) {
    if (!m_active) return;

    // 如果字还没打完
    if (m_charIndex < m_targetText.getSize()) {
        m_timer += dt;

        // 到了显示下一个字的时间
        if (m_timer >= m_charDelay) {
            m_timer = 0.f; // 重置计时器
            // 获取即将显示的字符，避免 0 时访问越界
            std::uint32_t nextChar = m_targetText[m_charIndex];

            // 如果是标点符号，增加额外的延迟
            if (nextChar == L'，' || nextChar == L',' || nextChar == L'。' || nextChar == L'.') {
                m_timer = -0.5f; // 让计时器变成负数，这样要过很久才会加回正数，从而产生停顿
            }
            // 增加一个字
            m_charIndex++;
            
            // SFML 的 substring 方法：从 0 开始，截取 m_charIndex 个长度
            m_renderText.setString(m_targetText.substring(0, m_charIndex));

            // --- 关键：播放音效 ---
            // 为了不让声音太吵，可以每隔 1-2 个字才播一次，或者非空格才播
            // 这里的逻辑是：只要有声音资源，就播放
                if (m_voice.has_value()) {
                float randomPitch = 0.9f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 0.2f));
                m_voice->setPitch(randomPitch); // 使用 ->
                m_voice->stop(); 
                m_voice->play();
            }
        }
    }
}

void DialogueBox::draw(sf::RenderWindow& window) {
    if (!m_active) return;

    window.draw(m_boxBg);     // 画黑底
    window.draw(m_boxBorder); // 画白框

    if (m_hasFace && m_faceSprite.has_value()) {
        window.draw(*m_faceSprite); // 解引用来绘制
    }

    window.draw(m_renderText); // 画文字
}

bool DialogueBox::onConfirm() {
    if (!m_active) return false;

    const bool wasTyping = m_charIndex < m_targetText.getSize();

    // 无论如何先同步到全文，避免逻辑分支遗漏
    m_charIndex = m_targetText.getSize();
    m_renderText.setString(m_targetText);
    m_timer = 0.f;

    // 如果原先在打字，说明这次只是跳过打字效果
    if (wasTyping) {
        return false;
    }

    // 字已打完，再按确定就是关闭/翻页
    m_active = false;
    return true;
}