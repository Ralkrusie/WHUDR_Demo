#include "UI/DialogBox.h"
#include "Manager/AudioManager.h"
#include <iostream>

namespace {
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

// 构造函数：初始化外观
DialogueBox::DialogueBox():
    m_renderText(m_font){
    // 1. 加载字体 (必须是支持中文的字体！例如 simhei.ttf 或像素字体)
    // 注意：实际项目中建议通过 ResourceManager 获取，这里为了演示直接加载
    if (!m_font.openFromFile("assets/font/Common.ttf")) {
        // 如果加载失败，回退到默认或者报错
        std::cerr << "Failed to load font!" << std::endl;
        // 字体缺失时禁用对话，避免后续绘制异常
        m_active = false;
    }
    m_renderText.setCharacterSize(24); // 字体大小
    m_renderText.setLineSpacing(1.55f); // 行距稍大，便于阅读
    m_renderText.setFillColor(sf::Color::White);

    // 2. 设置对话框背景 (屏幕底部)
    // 窗口大小是 640*480，我们在底部留出一点空间
    m_boxBg.setSize({580.f, 180.f});
    m_boxBg.setPosition({30.f, 280.f});
    m_boxBg.setFillColor(sf::Color::Black);

    // 3. 设置边框
    m_boxBorder.setSize({580.f, 180.f});
    m_boxBorder.setPosition({30.f, 280.f});
    m_boxBorder.setFillColor(sf::Color::Transparent);
    m_boxBorder.setOutlineColor(sf::Color::White);
    m_boxBorder.setOutlineThickness(4.f);

    // 加载红心指示器（用于选项选择提示）
    if (m_selectorTexture.loadFromFile("assets/sprite/Heart/spr_heart_0.png")) {
        m_selectorTexture.setSmooth(false);
        m_selectorSprite.emplace(m_selectorTexture);
        m_selectorSprite->setColor(sf::Color::Red);
        m_selectorSprite->setScale({1.3f, 1.3f});
    }
}

// moved to bool DialogueBox::start

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
                m_timer = -0.3f; // 让计时器变成负数，这样要过很久才会加回正数，从而产生停顿
            }
            // 增加一个字
            m_charIndex++;
            
            // SFML 的 substring 方法：从 0 开始，截取 m_charIndex 个长度
            m_renderText.setString(m_targetText.substring(0, m_charIndex));

            // --- 关键：播放音效 ---
            // 节流：仅在非空白字符且按间隔播音（如每2字一次）
            if (m_voiceKey.has_value()) {
                bool isWhitespace = (nextChar == L' ' || nextChar == L'\n' || nextChar == L'\t');
                if (!isWhitespace && m_sfxInterval >= 1 && (m_charIndex % m_sfxInterval == 0)) {
                    // float randomPitch = 0.9f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 0.2f));
                    // AudioManager::getInstance().playSound(m_voiceKey.value(), randomPitch);
                    AudioManager::getInstance().playSound(m_voiceKey.value());
                }
            }
        }
    }
}


void DialogueBox::draw(sf::RenderWindow& window) {
    if (!m_active) return;

    window.draw(m_boxBg);     // 画黑底
    window.draw(m_boxBorder); // 画白框

    if (m_hasFace && m_faceSprite.has_value()) {
        window.draw(*m_faceSprite); // 解引用来绘制头像
    }

    window.draw(m_renderText); // 画文字 

    // 文本打完后绘制选项（选项模式）
    if (m_isChoiceMode && !isTyping() && !m_optionTexts.empty()) {
        for (std::size_t i = 0; i < m_optionTexts.size(); ++i) {
            // 高亮当前选中项
            if (static_cast<int>(i) == m_selectIndex) {
                m_optionTexts[i].setFillColor(sf::Color::Yellow);
            } else {
                m_optionTexts[i].setFillColor(sf::Color::White);
            }
            window.draw(m_optionTexts[i]);
        }
        // 红心指示器：放在当前选项左侧居中
        if (m_selectorSprite.has_value()) {
            const sf::Text& sel = m_optionTexts[m_selectIndex];
            sf::FloatRect b = sel.getGlobalBounds();
            sf::FloatRect hb = m_selectorSprite->getGlobalBounds();
            sf::Vector2f heartPos{ b.position.x - hb.size.x - 10.f, b.position.y + (b.size.y - hb.size.y) * 0.5f };
            m_selectorSprite->setPosition(heartPos);
            window.draw(*m_selectorSprite);
        }
    }
}

bool DialogueBox::start(const sf::String& text, const sf::Texture* faceTexture, std::optional<std::string> voiceKey) {
    // 字体未就绪则直接返回，避免后续渲染崩溃
    if (m_font.getInfo().family.empty()) {
        std::cerr << "DialogueBox start skipped: font not loaded." << std::endl;
        m_active = false;
        return false;
    }

    if (text.isEmpty()) {
        std::cerr << "DialogueBox start skipped: empty text." << std::endl;
        m_active = false;
        return false;
    }

    m_active = true;
    m_targetText = wrapTextToWidth(text, 520.f, m_font, m_renderText.getCharacterSize());
    m_charIndex = 0;
    m_timer = 0.f;
    m_renderText.setString(""); // 清空当前显示
    m_voiceKey = std::move(voiceKey);

    // 设置头像
    if (faceTexture) {
        m_hasFace = true;
        // 核心修改：构造一个新的 Sprite 放入 optional
        m_faceSprite.emplace(*faceTexture); 
        // 使用 -> 访问成员
        m_faceSprite->setScale({1.8f, 1.8f});  // 放大头像
        m_faceSprite->setPosition({35.f, 310.f});
        m_renderText.setPosition({160.f, 300.f}); // 文字起始位置：有头像时，文字靠右
    } else {
        m_hasFace = false;
        m_faceSprite.reset(); // 清空，表示现在没头像
        // 文字起始位置：无头像时，文字靠左
        m_renderText.setPosition({70.f, 300.f});
    }

    return true;
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

    // 字已打完，再按确定：若为选项模式，不在此关闭，由 onConfirmChoice 处理
    if (!m_isChoiceMode) {
        m_active = false;
    }
    return true;
}

bool DialogueBox::startWithChoices(const sf::String& text,
                                   const std::vector<sf::String>& options,
                                   const sf::Texture* faceTexture,
                                   std::optional<std::string> voiceKey)
{
    if (options.empty()) {
        std::cerr << "DialogueBox startWithChoices skipped: empty options." << std::endl;
        return false;
    }
    bool ok = start(text, faceTexture, std::move(voiceKey));
    if (!ok) return false;
    m_isChoiceMode = true;
    m_selectIndex = 0;
    m_optionTexts.clear();
    m_optionTexts.reserve(options.size());
    // 横向居中排布：起点根据数量自适应，左右放置
    const float startX = m_optionBasePos.x - (static_cast<float>(options.size() - 1) * m_optionHGap * 0.5f);
    for (std::size_t i = 0; i < options.size(); ++i) {
        sf::Text t(m_font, options[i], 26);
        t.setFillColor(sf::Color::White);
        sf::Vector2f pos = { startX + static_cast<float>(i) * m_optionHGap, m_optionBasePos.y + 20.f };
        sf::FloatRect bounds = t.getLocalBounds();
        t.setOrigin({bounds.size.x / 2.f, bounds.size.y / 2.f});
        t.setPosition(pos);
        m_optionTexts.push_back(std::move(t));
    }
    return true;
}

void DialogueBox::moveSelection(int delta)
{
    if (!isChoiceActive() || m_optionTexts.empty()) return;
    int n = static_cast<int>(m_optionTexts.size());
    m_selectIndex = (m_selectIndex + delta + n) % n;
    AudioManager::getInstance().playSound("button_move");
}

std::optional<int> DialogueBox::onConfirmChoice()
{
    if (!isChoiceActive()) return std::nullopt;
    int chosen = m_selectIndex;
    m_active = false;
    m_isChoiceMode = false;
    AudioManager::getInstance().playSound("button_select");
    return chosen;
}