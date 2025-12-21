#include "AudioManager.h"

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
    // 1. 查找 Buffer
    auto it = m_soundBuffers.find(key);
    if (it == m_soundBuffers.end()) {
        std::cerr << "Sound not found: " << key << " (Did you load it?)" << std::endl;
        return;
    }

    // 2. 创建一个新的 Sound 对象并放入 list 尾部
    // 这一步非常关键：因为 sf::Sound 必须绑定 Buffer 且保持存活才能发出声音
    m_activeSounds.emplace_back(it->second); // 使用 Buffer 初始化 Sound

    // 3. 获取刚才创建的 Sound 的引用
    sf::Sound& sound = m_activeSounds.back();

    // 4. 设置属性
    sound.setPitch(pitch);
    // 综合考虑传入的 volume 和全局 soundVolume
    sound.setVolume(volume * (m_soundVolume / 100.f)); 

    // 5. 播放
    sound.play();
}

void AudioManager::update() {
    // 遍历活动音效列表，移除已经停止的
    // 使用 erase_if (C++20) 或者 remove_if
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
