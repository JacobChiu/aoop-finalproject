#pragma once

#include "core/Step.h"

#include <array>
#include <memory>
#include <string>

// 只做前置宣告：SFML 的音訊物件完全封裝在 InstrumentTrack.cpp 裡，
// 使用這個標頭的人（Pattern、GUI、Exporter）不需要 include SFML。
namespace sf {
class SoundBuffer;
class Sound;
}  // namespace sf

// =============================================================================
// InstrumentTrack —— 單一樂器音軌（負責人：組員 A 徐偉瀞）
//
// 封裝：軌道名稱、.wav 路徑、音量 (0~100)、16 個 Step，以及 SFML 的
// SoundBuffer / Sound。外部只要呼叫 playStep(i) 就會依照目前音量發聲。
//
// 注意：
//  * 這個類別「只能移動、不能複製」(move-only)，因為 sf::Sound 綁定在
//    SoundBuffer 的記憶體位址上。放進 std::vector 沒問題（會用 move）。
//  * 所有帶 index 的函式，index 超出 0~15 會丟 std::out_of_range，
//    方便在開發時立刻抓到播放引擎算錯步數的 bug。
//  * 字串（名稱、路徑）一律是 UTF-8，中文路徑也可以載入。
// =============================================================================
class InstrumentTrack {
public:
    static constexpr int   kStepCount     = 16;
    static constexpr float kMinVolume     = 0.f;
    static constexpr float kMaxVolume     = 100.f;
    static constexpr float kDefaultVolume = 80.f;

    explicit InstrumentTrack(std::string name = "Track");
    ~InstrumentTrack();

    InstrumentTrack(InstrumentTrack&&) noexcept;
    InstrumentTrack& operator=(InstrumentTrack&&) noexcept;
    InstrumentTrack(const InstrumentTrack&)            = delete;
    InstrumentTrack& operator=(const InstrumentTrack&) = delete;

    // ---- 名稱 ---------------------------------------------------------------
    const std::string& getTrackName() const;
    void               setTrackName(std::string name);

    // ---- 音色 ---------------------------------------------------------------
    // 載入音檔（.wav；SFML 也支援 .ogg/.flac）。
    // 成功回傳 true 並取代舊音色；失敗回傳 false，舊音色與路徑保持不變。
    bool               loadWavFile(const std::string& utf8Path);
    bool               hasSample() const;
    const std::string& getWavFilePath() const;   // 尚未載入時為空字串
    float              getSampleDuration() const; // 秒；尚未載入時為 0

    // ---- 音量 (0~100) -------------------------------------------------------
    void  setVolume(float vol);   // 超出範圍會被夾住
    float getVolume() const;

    // ---- 步進 ---------------------------------------------------------------
    void        toggleStep(int index);
    void        setStepActive(int index, bool state);
    bool        isStepActive(int index) const;
    Step&       getStep(int index);
    const Step& getStep(int index) const;
    int         getActiveStepCount() const;
    void        clearSteps();

    // ---- 發聲 ---------------------------------------------------------------
    // 若第 index 步為 ON 且已載入音色，就以「音量 × 力度」播放一次。
    // 回傳是否真的有發聲（給播放引擎 / GUI 做視覺回饋用）。
    bool playStep(int index);
    // 不看步進狀態，直接試聽音色（GUI 的「試聽」按鈕用）。
    void preview();
    void stop();

private:
    static void checkIndex(int index);
    void        trigger(int velocity);

    std::string                       trackName;
    std::string                       wavFilePath;
    float                             volume = kDefaultVolume;
    std::array<Step, kStepCount>      steps{};
    std::unique_ptr<sf::SoundBuffer>  soundBuffer;
    std::unique_ptr<sf::Sound>        sound;
};
