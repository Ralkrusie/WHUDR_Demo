#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <optional>

class DialogueBox {
public:
    DialogueBox();

    // 启动一段新的对话
    // text: 要说的话
    // faceTexture: 头像纹理指针 (如果是 nullptr 则不显示头像)
    // voiceKey:   可选的音效键，交由 AudioManager 播放（std::nullopt 表示无语音）
    // 返回值：true 表示成功开始对话；false 表示资源缺失或输入无效，未启动
    bool start(const sf::String& text, const sf::Texture* faceTexture = nullptr, std::optional<std::string> voiceKey = std::nullopt);

    // 含选项的对话：在文本打完后显示选项列表，可用方向键选择，确认提交
    bool startWithChoices(const sf::String& text,
                          const std::vector<sf::String>& options,
                          const sf::Texture* faceTexture = nullptr,
                          std::optional<std::string> voiceKey = std::nullopt);

    // 每帧调用
    void update(float dt);
    
    // 渲染
    void draw(sf::RenderWindow& window);

    // 玩家按下了确定键 (Z/Enter)
    // 返回值: true 表示这段话这就结束了(翻页)，false 表示刚刚跳过打字机直接显示全了
    bool onConfirm();

    // 选项模式：导航与确认
    bool isChoiceActive() const { return m_active && m_isChoiceMode && !isTyping(); }
    void moveSelection(int delta);
    // 返回值：若确认成功，返回所选索引；否则返回 std::nullopt
    std::optional<int> onConfirmChoice();

    // 调整选项布局（起始中心位置与横向间隔）；默认值在构造函数中设定
    void setOptionLayout(const sf::Vector2f& basePos, float hGap) {
        m_optionBasePos = basePos;
        m_optionHGap = hGap;
    }

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
    int m_sfxInterval = 1;       // 每隔几个字符播放一次音效（>=1），默认每1个字播一次

    // --- 图形元素 ---
    sf::RectangleShape m_boxBg;  // 黑框背景
    sf::RectangleShape m_boxBorder; // 白框边框
    std::optional<sf::Sprite> m_faceSprite;    // 头像
    bool m_hasFace = false;

    // --- 音效 ---
    std::optional<std::string> m_voiceKey; // 说话音效键（交给 AudioManager 播放）

    // --- 选项模式 ---
    bool m_isChoiceMode = false;
    std::vector<sf::Text> m_optionTexts; // 渲染用
    int m_selectIndex = 0;
    sf::Vector2f m_optionBasePos{320.f, 380.f}; // 选项居中基准点
    float m_optionLineGap = 44.f;               // 纵向行距（若需多行）
    float m_optionHGap = 180.f;                 // 横向间隔（左右摆放）

    // 选项指示器（红心），参考 TitleState
    std::optional<sf::Sprite> m_selectorSprite;
    sf::Texture m_selectorTexture;
};