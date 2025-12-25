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

    // 士兵音效
    SFX_Archer,           // 弓箭手
    SFX_Barbarian,        // 野蛮人
    SFX_Giant,            // 巨人
    SFX_Bomber,           // 炸弹兵

    // 战斗音效
    SFX_Cannon,            // 加农炮
    SFX_Building_Destroy, // 建筑被摧毁
    SFX_Die,              // 死亡音效

    // 系统音效
    SFX_Collect_Resource, // 资源采集
    SFX_Intro,            // 开场
    SFX_Win,              // 胜利
    SFX_Lost,             // 失败
};

class AudioManager {
public:
    static AudioManager* getInstance();
    
    // ========== 核心业务触发接口 ==========
    
    // 播放背景音乐
    void playMusic(bool isBattle);
    
    // 触发各类音效
    void playArcher()         { playEffect(AudioID::SFX_Archer); }
    void playBarbarian()      { playEffect(AudioID::SFX_Barbarian); }
    void playGiant()          { playEffect(AudioID::SFX_Giant); }
    void playBomber()         { playEffect(AudioID::SFX_Bomber); }
    void playCannon()          { playEffect(AudioID::SFX_Cannon); }
    void playBuildingDestroy(){ playEffect(AudioID::SFX_Building_Destroy); }
    void playDie()            { playEffect(AudioID::SFX_Die); }
    void playResourceCollect(){ playEffect(AudioID::SFX_Collect_Resource); }
    void playIntro()          { playEffect(AudioID::SFX_Intro); }
    void playWin()            { playEffect(AudioID::SFX_Win); }
    void playLost()           { playEffect(AudioID::SFX_Lost); }

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