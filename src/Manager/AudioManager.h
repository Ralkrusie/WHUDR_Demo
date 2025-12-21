/*
管理所有 BGM 和音效。
包含：

播放 / 停止

切换场景淡入淡出

缓存音效
*/
#pragma once
#include <SFML/Audio.hpp>
#include <map>
#include <string>
#include <list>
#include <memory>
#include <iostream>

class AudioManager {
public:
    // --- 单例模式访问 ---
    static AudioManager& getInstance() {
        static AudioManager instance;
        return instance;
    }

    // 禁止拷贝和赋值
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    // --- 资源加载 ---
    // 预加载音效 (建议在游戏初始化或场景加载时调用)
    // key: "text_kris", path: "assets/audio/snd_text.wav"
    void loadSound(const std::string& key, const std::string& path);

    // --- 播放控制 ---
    
    // 播放背景音乐 (流式播放，不占内存)
    // loop: 是否循环 (BGM通常为 true)
    void playMusic(const std::string& path, bool loop = true);
    
    // 停止音乐
    void stopMusic();

    // 播放音效 (最常用)
    // key: 之前 loadSound 用的 key
    // pitch: 音调 (1.0 正常, >1 尖细, <1 低沉) - 用于对话音效
    // volume: 音量 (0-100)
    void playSound(const std::string& key, float pitch = 1.0f, float volume = 100.f);

    // --- 维护 ---
    // 必须在 Game loop 中每帧调用！用于清理播放完的音效对象
    void update();

    // --- 设置 ---
    void setMusicVolume(float volume);
    void setSoundVolume(float volume);

private:
    AudioManager() = default;

    // 缓存 SoundBuffer (重资产，加载到内存)
    std::map<std::string, sf::SoundBuffer> m_soundBuffers;

    // 当前正在播放的音效列表
    // 为什么要用 list？因为 sf::Sound 播放时必须存活。
    // 我们把 sound 对象存在这里，播放完再从这里删掉。
    std::list<sf::Sound> m_activeSounds;

    // 背景音乐 (SFML 3 中 sf::Music 不可拷贝，直接持有一个实例)
    sf::Music m_music;

    // 全局音量设置
    float m_musicVolume = 50.f;
    float m_soundVolume = 100.f;
};