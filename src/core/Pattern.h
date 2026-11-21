#pragma once

#include "core/InstrumentTrack.h"

#include <string>
#include <vector>

// =============================================================================
// Pattern —— 一個節奏樣式，管理多條 InstrumentTrack
//
// 分工：
//   * 組員 A：音軌增減 (addTrack / removeTrack) 與發聲 (triggerStep)
//   * 組員 B：名稱、總覽模式開關 (enabledInAllMode)，以及之後要加的功能
//
// 注意：addTrack 回傳的參考在下一次 addTrack / removeTrack 後可能失效
// （vector 重新配置），請不要長期保存，需要時用 getTrack(index) 重新取得。
// =============================================================================
class Pattern {
public:
    static constexpr int kMaxTracks = 32;

    explicit Pattern(std::string name = "Pattern");

    // InstrumentTrack 是 move-only，所以 Pattern 也是 move-only
    Pattern(Pattern&&) noexcept            = default;
    Pattern& operator=(Pattern&&) noexcept = default;
    Pattern(const Pattern&)                = delete;
    Pattern& operator=(const Pattern&)     = delete;

    // ---- 名稱 / 總覽模式開關（組員 B）--------------------------------------
    const std::string& getName() const;
    void               setName(std::string name);
    void               setEnabled(bool enabled);
    bool               isEnabled() const;

    // ---- 音軌管理（組員 A）--------------------------------------------------
    bool             canAddTrack() const;   // 未達 kMaxTracks
    // 新增空白音軌；已達上限時丟 std::length_error
    InstrumentTrack& addTrack(const std::string& name);
    // 新增音軌並載入音色；音色載入失敗時音軌仍會建立（hasSample() == false）
    InstrumentTrack& addTrack(const std::string& name, const std::string& wavPath);
    // 刪除第 index 條音軌；index 無效時回傳 false
    bool             removeTrack(int index);

    int                                 getTrackCount() const;
    InstrumentTrack&                    getTrack(int index);        // 無效 index 丟 std::out_of_range
    const InstrumentTrack&              getTrack(int index) const;
    std::vector<InstrumentTrack>&       getTracks();
    const std::vector<InstrumentTrack>& getTracks() const;

    // ---- 發聲（組員 A；給組員 B 的播放引擎呼叫）----------------------------
    // 讓所有音軌播放第 stepIndex 步，回傳實際發聲的音軌數
    int  triggerStep(int stepIndex);
    void stopAll();
    void clearAllSteps();

private:
    std::string                  name;
    bool                         enabledInAllMode = true;
    std::vector<InstrumentTrack> tracks;
};
