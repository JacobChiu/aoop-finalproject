#pragma once

#include "core/Pattern.h"

#include <string>
#include <vector>

// =============================================================================
// Project —— 整個鼓機專案（負責人：組員 B 邱禹峰）
//
// 【骨架】由組員 A 先建立，讓 GUI 能編譯執行。簡單的 getter/setter 已經
// 寫好，標 TODO(B) 的部分（播放引擎等）請組員 B 實作或自由改寫。
// 組員 A 的 GUI 只會用到：getActivePattern()、getIsPlaying()、getCurrentStep()。
// =============================================================================
class Project {
public:
    static constexpr int kMinBpm     = 50;
    static constexpr int kMaxBpm     = 200;
    static constexpr int kDefaultBpm = 120;

    Project();

    // ---- 全域設定 -----------------------------------------------------------
    void setBPM(int bpm);   // 夾在 50~200
    int  getBPM() const;

    void togglePlay();
    bool getIsPlaying() const;

    void setSingleMode(bool single);
    bool getIsSingleMode() const;

    // ---- Pattern 管理 -------------------------------------------------------
    Pattern&                    addPattern(const std::string& name);
    bool                        removePattern(int index);   // TODO(B): 至少保留一個？
    int                         getPatternCount() const;
    std::vector<Pattern>&       getPatterns();
    const std::vector<Pattern>& getPatterns() const;

    void           setActivePatternIndex(int index);
    int            getActivePatternIndex() const;
    Pattern&       getActivePattern();
    const Pattern& getActivePattern() const;

    // ---- 播放引擎 -----------------------------------------------------------
    // 每一幀由主迴圈呼叫，deltaTime 單位為秒。
    // TODO(B): 依 BPM 累積時間，每到一個 16 分音符就推進 currentStep 並呼叫
    //          Pattern::triggerStep(currentStep)（單一模式只播 active pattern，
    //          全部模式播所有 isEnabled() 的 pattern）。
    void updateSequencer(float deltaTime);

    // 目前播放到第幾步 (0~15)，停止時為 -1。GUI 用來畫播放頭。
    int getCurrentStep() const;

private:
    std::vector<Pattern> patterns;
    int                  bpm                = kDefaultBpm;
    bool                 isPlaying          = false;
    bool                 isSingleMode       = true;
    int                  activePatternIndex = 0;
    int                  currentStep        = -1;
};
