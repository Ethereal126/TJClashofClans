#pragma once
#ifndef __AUDIO_MANAGER_H__
#define __AUDIO_MANAGER_H__

#include "cocos2d.h"
#include "audio/include/AudioEngine.h"
#include <string>
#include <map>
#include <vector>

// 音频类型枚举
enum class AudioType {
    Music,       // 背景音乐（循环播放，同时只有一个）
    Effect,      // 音效（短音频，可同时播放多个）
};

// 音频ID枚举（预定义的音频资源）
enum class AudioID {
    // ===== 背景音乐 =====
    BGM_MainMenu,     // 主菜单音乐
    BGM_Village,      // 村庄音乐
    BGM_Battle,       // 战斗音乐
    BGM_Victory,      // 胜利音乐
    BGM_Defeat,       // 失败音乐

    // ===== UI音效 =====
    SFX_ButtonClick,      // 按钮点击

    // ===== 建筑音效 =====
    SFX_BuildingPlace,    // 放置建筑
    SFX_BuildingDestroy,  // 摧毁建筑


    // ===== 军队音效 =====
    SFX_ArcherAttack,     // 弓箭手攻击
    SFX_BarbarianAttack,  // 野蛮人攻击
    SFX_BomberAttack,     // 炸弹人攻击
    SFX_GiantAttack,      // 巨人攻击

    // ===== 战斗音效 =====
    SFX_Explosion,        // 爆炸
    SFX_ArrowShoot,       // 射箭
    SFX_Spell,            // 法术释放
    SFX_TowerAttack,      // 防御塔攻击


    // ===== 资源采集音效 =====
    SFX_GoldMine,         // 金矿生产
    SFX_ElixirCollector,  // 圣水收集器
};

// 音频资源信息结构体
struct AudioInfo {
    AudioID id;                   // 音频ID
    std::string filePath;         // 文件路径
    AudioType type;               // 音频类型
    float defaultVolume;          // 默认音量（0.0 - 1.0）
    bool preload;                 // 是否预加载

    AudioInfo() : id(AudioID::BGM_MainMenu), type(AudioType::Effect),
        defaultVolume(1.0f), preload(false) {
    }
};

// 音频管理器：单例模式，负责管理所有音频的播放、暂停、停止和音量控制
class AudioManager {
public:
    // ========== 单例管理 ==========
    static AudioManager* getInstance();
    static void destroyInstance();

    // ========== 初始化 ==========
    // 初始化音频管理器
    // 调用时机：AppDelegate::applicationDidFinishLaunching()
    bool init();

    // 加载音频配置文件（JSON格式，定义所有音频资源）
    // 调用时机：init()之后
    bool loadAudioConfig(const std::string& configPath);

    // ========== 预加载与卸载 ==========
    // 预加载单个音频资源（避免播放时卡顿）
    // 调用时机：场景加载时预加载该场景需要的音效
    void preloadAudio(AudioID audioID);

    // 预加载所有音频（游戏启动时）
    // 调用时机：加载界面时
    void preloadAllAudio();

    // 释放单个音频资源（节省内存）
    void unloadAudio(AudioID audioID);

    // 释放所有音频
    void unloadAllAudio();

    // ========== 背景音乐控制 ==========
    // 播放背景音乐（自动循环，同时只能播放一个）
    // fadeIn：是否淡入（平滑过渡）
    // 调用时机：进入场景时
    // 区别：playMusic会停止之前的音乐，只保留一个背景音乐
    void playMusic(AudioID musicID, bool fadeIn = false, float fadeTime = 1.0f);

    // 停止背景音乐
    // fadeOut：是否淡出（平滑结束）
    // 调用时机：退出游戏、切换场景前
    void stopMusic(bool fadeOut = false, float fadeTime = 1.0f);

    // 暂停背景音乐（保留进度）
    // 调用时机：游戏暂停时
    // 区别于stopMusic：pauseMusic可以从暂停位置恢复
    void pauseMusic();

    // 恢复背景音乐（从暂停位置继续）
    // 调用时机：取消暂停时
    void resumeMusic();

    // ========== 音效控制 ==========
    // 播放音效（可同时播放多个，返回音效实例ID）
    // loop：是否循环（一般音效不循环，只有环境音可能循环）
    // 调用时机：触发事件时（点击按钮、建筑放置、攻击等）
    // 返回值：音效实例ID，用于后续控制该音效（如提前停止）
    int playSoundEffect(AudioID effectID, bool loop = false, float volume = 1.0f);

    // 停止指定音效实例
    // 调用时机：需要中断某个正在播放的音效
    void stopSoundEffect(int effectInstanceID);

    // 停止所有音效（不影响背景音乐）
    // 调用时机：场景切换、游戏暂停
    void stopAllSoundEffects();

    // ========== 音量控制 ==========
    // 设置音乐音量（0.0 - 1.0）
    // 调用时机：设置界面中调整音量滑块
    void setMusicVolume(float volume);

    // 设置音效音量
    void setSoundEffectVolume(float volume);

    // 设置主音量（影响所有音频）
    void setMasterVolume(float volume);

    // 获取当前音量
    float getMusicVolume() const;
    float getSoundEffectVolume() const;
    float getMasterVolume() const;

    // ========== 静音控制 ==========
    // 静音/取消静音（分类控制）
    // 调用时机：设置界面中的静音开关
    void setMusicMuted(bool muted);
    void setSoundEffectMuted(bool muted);

    // 获取静音状态
    bool isMusicMuted() const;
    bool isSoundEffectMuted() const;

    // ========== 状态查询 ==========
    // 检查背景音乐是否正在播放
    bool isMusicPlaying() const;

    // 检查指定音效实例是否正在播放
    bool isSoundEffectPlaying(int effectInstanceID) const;

    // ========== 全局控制（用于游戏暂停） ==========
    // 暂停所有音频（音乐+音效+语音）
    // 调用时机：按下暂停键、应用进入后台
    void pauseAll();

    // 恢复所有音频
    // 调用时机：取消暂停、应用回到前台
    void resumeAll();

    // 停止所有音频
    // 调用时机：退出游戏
    void stopAll();

protected:
    AudioManager();
    virtual ~AudioManager();

    // 根据AudioID获取音频信息
    const AudioInfo* getAudioInfo(AudioID audioID) const;

    // 应用音量设置（考虑主音量、分类音量、静音状态）
    float getEffectiveVolume(AudioType type, float volume = 1.0f) const;

    // 淡入淡出效果
    void fadeInMusic(int musicInstanceID, float fadeTime);
    void fadeOutMusic(int musicInstanceID, float fadeTime, const std::function<void()>& callback);

private:
    static AudioManager* _instance;

    // 音频资源映射（AudioID -> AudioInfo）
    std::map<AudioID, AudioInfo> _audioMap;

    // 当前播放的背景音乐实例ID
    int _currentMusicID;
    AudioID _currentMusicEnum;

    // 正在播放的音效实例ID列表
    std::vector<int> _activeSoundEffects;

    // 音量设置
    float _musicVolume;          // 音乐音量
    float _soundEffectVolume;    // 音效音量

    // 静音状态
    bool _musicMuted;
    bool _soundEffectMuted;

    // 暂停状态标记
    bool _musicPaused;
};

#endif // __AUDIO_MANAGER_H__