#include "AudioManager.h"

//
// 音频管理（AudioManager）
// ----------------------
// 职责：
// - 资源加载：将音效文件读取到 `sf::SoundBuffer` 并以键值缓存
// - 播放音乐：通过 `sf::Music` 控制 BGM 的打开、循环与音量
// - 播放音效：为每次播放创建独立的 `sf::Sound`，以确保缓冲与实例生命周期正确
// - 生命周期清理：在 `update()` 中清理已停止的 Sound，避免容器无限增长
// - 全局音量：区分音乐音量与音效音量，支持设置接口
// 约定与提示：
// - 在播放音效前需 `loadSound(key, path)`，否则会提示未加载
// - `playSound` 的 `volume` 会与全局 `m_soundVolume` 混合（百分比）
// - 如需切歌判定，可在类中记录当前 BGM 路径并做条件检查
//

void AudioManager::loadSound(const std::string& key, const std::string& path) {
    // 如果已经加载过，就不再加载
    if (m_soundBuffers.contains(key)) return;

    sf::SoundBuffer buffer;
    if (buffer.loadFromFile(path)) {
        // 使用 C++ map 的 emplace 放入 buffer
        m_soundBuffers[key] = std::move(buffer);
        // std::cout << "Loaded SFX: " << key << std::endl;
    } else {
        std::cerr << "Failed to load SFX: " << path << std::endl;
    }
}

void AudioManager::playMusic(const std::string& path, bool loop) {
    // 如果需要播放的已经在播放了，就不打断
    // (这里没有存 path，简单判定的话如果状态是 Playing 就不重开，或者你可以加个成员变量记录当前BGM路径)
    
    if (m_music.openFromFile(path)) {
        m_music.setLooping(loop);
        m_music.setVolume(m_musicVolume);
        m_music.play();
    } else {
        std::cerr << "Failed to open music: " << path << std::endl;
    }
}

void AudioManager::stopMusic() {
    m_music.stop();
}

void AudioManager::playSound(const std::string& key, float pitch, float volume) {
    // 1. 查找 Buffer（事先通过 loadSound 加载）
    auto it = m_soundBuffers.find(key);
    if (it == m_soundBuffers.end()) {
        std::cerr << "Sound not found: " << key << " (Did you load it?)" << std::endl;
        return;
    }

    // 2. 创建一个新的 Sound 对象并放入 list 尾部
    // 关键：sf::Sound 必须绑定 Buffer 且保持存活；列表持有以防局部对象销毁
    m_activeSounds.emplace_back(it->second); // 使用 Buffer 初始化 Sound

    // 3. 获取刚才创建的 Sound 的引用
    sf::Sound& sound = m_activeSounds.back();

    // 4. 设置属性（音高与音量，其中音量结合全局音效音量）
    sound.setPitch(pitch);
    // 综合考虑传入的 volume 和全局 soundVolume
    sound.setVolume(volume * (m_soundVolume / 100.f)); 

    // 5. 播放
    sound.play();
}

void AudioManager::update() {
    // 遍历活动音效列表，移除已经停止的实例
    // 使用 C++20 的 erase_if 简化清理逻辑
    std::erase_if(m_activeSounds, [](const sf::Sound& s) {
        return s.getStatus() == sf::Sound::Status::Stopped;
    });
}

void AudioManager::setMusicVolume(float volume) {
    m_musicVolume = volume;
    m_music.setVolume(volume);
}

void AudioManager::setSoundVolume(float volume) {
    m_soundVolume = volume;
}
