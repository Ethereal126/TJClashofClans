#include "AudioManager/AudioManager.h"
#include "Soldier/Soldier.h"

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
        case AudioID::SFX_Archer:           return "audio/sfx_archer.mp3";
        case AudioID::SFX_Barbarian:        return "audio/sfx_barbarian.mp3";
        case AudioID::SFX_Giant:            return "audio/sfx_giant.mp3";
        case AudioID::SFX_Bomber:           return "audio/sfx_bomber.mp3";
        case AudioID::SFX_Cannon:            return "audio/sfx_cannon.mp3";
        case AudioID::SFX_Building_Destroy: return "audio/sfx_destroy.mp3";
        case AudioID::SFX_Die:              return "audio/sfx_die.mp3";
        case AudioID::SFX_Collect_Resource: return "audio/sfx_collect.mp3";
        case AudioID::SFX_Intro:            return "audio/sfx_intro.mp3";
        case AudioID::SFX_Win:              return "audio/sfx_win.mp3";
        case AudioID::SFX_Lost:             return "audio/sfx_lost.mp3";
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

void AudioManager::playSoldierAttack(SoldierType type) {
    switch(type){
        case SoldierType::kArcher:
            playArcher();
            break;
        case SoldierType::kBarbarian:
            playBarbarian();
            break;
        case SoldierType::kBomber:
            playBomber();
            break;
        case SoldierType::kGiant:
            playGiant();
            break;
        case SoldierType::kSoldierTypes:
            CCLOG("warning : playSoldierAttack() should not choose kSoldierTypes as a type");
            break;
    }
}

void AudioManager::playBuildingAttack(const std::string& type) {
    if(type == "Archer Tower") {
        playArcher();
    }
    if(type == "Cannon"){
        playCannon();
    }
}


