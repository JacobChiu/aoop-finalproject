#pragma once

// =============================================================================
// Step —— 一個步進格（負責人：組員 A 徐偉瀞）
//
// 只記錄「這一格要不要發聲」以及「發聲力度」，不碰任何音訊物件。
// velocity 採 MIDI 慣例 0~127，方便日後擴充 MIDI 匯出。
// =============================================================================
class Step {
public:
    static constexpr int kMinVelocity     = 0;
    static constexpr int kMaxVelocity     = 127;
    static constexpr int kDefaultVelocity = 127;

    Step() = default;
    explicit Step(bool active, int velocity = kDefaultVelocity);

    void toggle();
    void setActive(bool state);
    bool getIsActive() const;

    int  getVelocity() const;
    void setVelocity(int velocity);   // 超出範圍會被夾到 0~127

private:
    bool isActive = false;
    int  velocity = kDefaultVelocity;
};
