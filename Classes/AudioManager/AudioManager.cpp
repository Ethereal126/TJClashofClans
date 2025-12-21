#include "AudioManager/AudioManager.h"

USING_NS_CC;

AudioManager* AudioManager::_instance = nullptr;

AudioManager* AudioManager::getInstance() {
    if (!_instance) _instance = new AudioManager();
    return _instance;
}

AudioManager::AudioManager() 
    : _currentMusicID(AudioEngine::INVALID_AUDIO_ID)
    , _musicVolume(0.5f)
    , _sfxVolume(0.8f) {}

std::string AudioManager::getPath(AudioID id) const {
    switch (id) {
        case AudioID::BGM_Village:          return "audio/bgm_village.mp3";
        case AudioID::BGM_Battle:           return "audio/bgm_battle.mp3";
        case AudioID::SFX_Attack_Melee:     return "audio/sfx_melee.mp3";
        case AudioID::SFX_Attack_Ranged:    return "audio/sfx_ranged.mp3";
        case AudioID::SFX_Explosion_Bomber: return "audio/sfx_bomber.mp3";
        case AudioID::SFX_Building_Destroy: return "audio/sfx_destroy.mp3";
        case AudioID::SFX_Collect_Resource: return "audio/sfx_collect.mp3";
        default: return "";
    }
}

void AudioManager::playMusic(bool isBattle) {
    stopMusic();
    std::string path = getPath(isBattle ? AudioID::BGM_Battle : AudioID::BGM_Village);
    if (!path.empty()) {
        _currentMusicID = AudioEngine::play2d(path, true, _musicVolume);
    }
}

void AudioManager::stopMusic() {
    if (_currentMusicID != AudioEngine::INVALID_AUDIO_ID) {
        AudioEngine::stop(_currentMusicID);
        _currentMusicID = AudioEngine::INVALID_AUDIO_ID;
    }
}

void AudioManager::playEffect(AudioID id) {
    std::string path = getPath(id);
    if (!path.empty()) {
        AudioEngine::play2d(path, false, _sfxVolume);
    }
}

void AudioManager::setMusicVolume(float volume) {
    _musicVolume = std::clamp(volume, 0.0f, 1.0f);
    if (_currentMusicID != AudioEngine::INVALID_AUDIO_ID) {
        AudioEngine::setVolume(_currentMusicID, _musicVolume);
    }
}

void AudioManager::setSoundEffectVolume(float volume) {
    _sfxVolume = std::clamp(volume, 0.0f, 1.0f);
}


