#pragma once
#ifndef __AUDIO_MANAGER_H__
#define __AUDIO_MANAGER_H__

#include "cocos2d.h"
#include "audio/include/AudioEngine.h"
#include <string>

enum class AudioID {
    // 背景音乐
    BGM_Village,          // 村庄
    BGM_Battle,           // 战斗

    // 核心音效
    SFX_Attack_Melee,     // 近战攻击
    SFX_Attack_Ranged,    // 远程攻击
    SFX_Explosion_Bomber, // 炸弹兵爆炸
    SFX_Building_Destroy, // 建筑被摧毁
    SFX_Collect_Resource, // 资源采集
};

class AudioManager {
public:
    static AudioManager* getInstance();
    
    // ========== 核心业务触发接口 ==========
    
    // 播放背景音乐
    void playMusic(bool isBattle);
    
    // 触发各类音效
    void playMeleeAttack()    { playEffect(AudioID::SFX_Attack_Melee); }
    void playRangedAttack()   { playEffect(AudioID::SFX_Attack_Ranged); }
    void playBomberExplosion(){ playEffect(AudioID::SFX_Explosion_Bomber); }
    void playBuildingDestroy(){ playEffect(AudioID::SFX_Building_Destroy); }
    void playResourceCollect(){ playEffect(AudioID::SFX_Collect_Resource); }

    // ========== UI 兼容接口 (必须保留) ==========
    void setMusicVolume(float volume);
    void setSoundEffectVolume(float volume);
    float getMusicVolume() const { return _musicVolume; }
    float getSoundEffectVolume() const { return _sfxVolume; }

    // ========== 全局控制 ==========
    void stopMusic();
    void stopAll() { cocos2d::AudioEngine::stopAll(); }

private:
    AudioManager();
    void playEffect(AudioID id);
    std::string getPath(AudioID id) const;

    static AudioManager* _instance;
    int _currentMusicID;
    float _musicVolume;
    float _sfxVolume;
};

#endif __AUDIO_MANAGER_H__